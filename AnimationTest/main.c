#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "window.h"
#include "threading.h"
#include "sockets.h"
#include "session.h"
#include "colorpicker.h"
#include "client.h"
#include "server.h"

// Totally not lazey
#define ICON_PLAY L"\x25B6"
#define ICON_STOP L"\x25A0"

enum EPaintMode
{
	// They mean the same thing for now
	// For future:
	//	UpdateFrame when client receives user drawings, UpdateCanvas for client-side changes like zoom & drag
	PaintMode_UpdateFrame = 0,
	PaintMode_UpdateCanvas = 0
};

DrawTool tool_brush = { 2, 0x000000 }, tool_eraser = { 16, 0xFFFFFF };
DrawTool* tool_active = &tool_brush;

ThreadHandle thread_play = 0;
MutexHandle mtx_play;

MenuItem* mProperties;
WndItem* int_frame, * int_fps, * int_brush, * btn_add, * btn_rem, * btn_clear, * btn_play, * bBrush, * bEraser;
ColorPicker* picker_brush, * picker_bkg;
int lLastX, lLastY, mLastX, mLastY, canvasX = 0, canvasY = 35;
char bPlaying = 0;

WndHandle wnd_prop, wnd_main;
WndItem* int_width, * int_height, * btn_new, * btn_cancel;

void ResetProject();
int PlayThread(void* UserData);

void SetTool(DrawTool* Tool)
{
	tool_active = Tool;
	if (tool_active)
	{
		Window_Item_SetValuei(int_brush, tool_active->Width);
		ColorPicker_SetColor(picker_brush, tool_active->Color);
	}
}

int OnMenu(MenuItem* Item)
{
	if (Item == mProperties)
		Window_Show(wnd_prop, 1);
	return WndCallback_None;
}

void OnColorPicked(ColorPicker* Picker)
{
	if (tool_active)
		tool_active->Color = Picker->color;
}

int OnMouse(WndHandle Wnd, int X, int Y, int MouseBtn, int Down)
{
	switch (MouseBtn)
	{
	case MouseBtn_Left:
		Mutex_Lock(mtx_play);
		if (!bPlaying)
			Session_LockFrames();
		FrameItem* active = Session_ActiveFrame();
		if (active && Down && tool_active)
		{
			Vec2 point = { X - canvasX, Y - canvasY };
			if (Down > 1)
				my_netint.addToStroke(user_local, &point);
			else
				my_netint.beginStroke(user_local, &point, tool_active, active->data);
			lLastX = X, lLastY = Y;
		}
		else if (!Down)
		{
			Session_LockUsers();
			if (user_local->bDrawing)
				my_netint.endStroke(user_local);
			Session_UnlockUsers();
		}
		if (!bPlaying)
			Session_UnlockFrames();
		Mutex_Unlock(mtx_play);
		break;
	case MoustBtn_Middle:
		if (Down > 1)
		{
			canvasX += X - mLastX, canvasY += Y - mLastY;
			Window_Redraw(Wnd, 0);
		}
		if (Down)
			mLastX = X, mLastY = Y;
		break;
	}
	return WndCallback_None;
}

int OnKeyboard(WndHandle Wnd, char Key, char bDown)
{
	// Main window
	switch (Key)
	{
	case Key_Comma: // <
	case Key_Period: // >
		if (bDown)
		{
			Mutex_Lock(mtx_play);
			if (!bPlaying)
			{
				Session_LockFrames();

				int idx = Session_ActiveFrameIndex() + (Key == Key_Comma ? -1 : 1);
				my_netint.setFrame(idx);

				Session_UnlockFrames();
			}
			Mutex_Unlock(mtx_play);
		}
		break;
	case Key_Space:
		Mutex_Lock(mtx_play);
		if (!bPlaying && bDown)
		{
			Session_LockFrames();
			int idx = Session_ActiveFrameIndex() + 1;
			my_netint.insertFrame(idx);
			my_netint.setFrame(idx);
			Session_UnlockFrames();
		}
		Mutex_Unlock(mtx_play);
		break;
	case 'E':
	case 'B':
	{
		if (bDown)
		{
			WndItem* item = Key == 'B' ? bBrush : bEraser, * other = Key != 'B' ? bBrush : bEraser;
			Window_Item_SetValuei(item, 1);
			Window_Item_SetValuei(other, 0);
			SetTool(Key == 'B' ? &tool_brush : &tool_eraser);
		}
		break;
	}
	case 'Z':
	case 'Y':
		if (bDown)
		{
			Mutex_Lock(mtx_play);
			if (!bPlaying)
			{
				Session_LockFrames();
				UserStroke* stroke = 0;
				if (Key == 'Z')
				{
					my_netint.undoStroke();
					//if (my_netint.undoStroke())
						//stroke = (UserStroke*)user_local->undone->tail->data;
				}
				else
				{
					my_netint.redoStroke();
					//if (my_netint.redoStroke())
						//stroke = (UserStroke*)user_local->strokes->tail->data;
				}

				if (stroke)
				{
					// Present the acted-on frame to the user
					FrameItem* active = Session_ActiveFrame();
					if (!active || active->data != stroke->framedat)
						my_netint.setFrame(Session_FrameData_GetIndex(stroke->framedat));
				}

				Session_UnlockFrames();
			}
			Mutex_Unlock(mtx_play);
		}
		break;
	case Key_LBracket:
	case Key_RBracket:
		Session_LockUsers();
		if (user_local->bDrawing && tool_active)
		{
			tool_active->Width += Key == Key_LBracket ? -1 : 1;
			SetTool(tool_active); // Lazy way to make things update
		}
		Session_UnlockUsers();
		break;
	}
	return WndCallback_None;
}

int OnWndMsg(WndHandle Wnd, int WndMsg)
{
	if (Wnd == wnd_prop)
	{
		if (WndMsg == WndMsg_Closing)
		{
			Window_Show(Wnd, 0);
			return WndCallback_Skip;
		}
	}
	else // Main window
	{
		switch (WndMsg)
		{
		case WndMsg_Draw:
		{
			Mutex_Lock(mtx_play);
			char playing = bPlaying;
			if (!bPlaying)
				Session_LockFrames();

			FrameItem* frame = Session_ActiveFrame();
			if (frame)
				Window_Draw_Bitmap(Wnd, frame->data->saved, canvasX, canvasY);

			if (!bPlaying)
				Session_UnlockFrames();
			Mutex_Unlock(mtx_play);
			break;
		}
		case WndMsg_Closing:
			return WndCallback_Close;
		}
	}
	return WndCallback_None;
}

char bItemMsg = 0;
MutexHandle mtx_items;

int OnItemMsg(WndItem* Item, int ItemMsg, ItemMsgData* Data)
{
	Mutex_Lock(mtx_items);
	bItemMsg = 1;
	Mutex_Unlock(mtx_items);
	switch (ItemMsg)
	{
	case ItemMsg_ValueChanged:
		if (Item == int_frame)
		{
			Mutex_Lock(mtx_play);
			if (!bPlaying)
				Session_LockFrames();
			my_netint.setFrame(Data->newval.i);
			if (!bPlaying)
				Session_UnlockFrames();
			Window_Redraw(int_frame->wnd, 0);
			Mutex_Unlock(mtx_play);
		}
		else if (Item == int_fps)
		{
			Mutex_Lock(mtx_play);
			if (!bPlaying)
				Session_LockFrames();

			my_netint.setFPS(Data->newval.i);

			if (!bPlaying)
				Session_UnlockFrames();
			Mutex_Unlock(mtx_play);
		}
		else if (Item == int_brush)
			if (tool_active)
				tool_active->Width = Data->newval.i;
		break;
	case ItemMsg_Clicked:

		// Main window

		if (Item == btn_add)
		{
			Session_LockFrames();
			int idx = Session_ActiveFrameIndex() + 1;
			my_netint.insertFrame(idx);
			my_netint.setFrame(idx);
			Session_UnlockFrames();
		}
		else if (Item == btn_rem)
		{
			Session_LockFrames();
			my_netint.removeFrame(Session_ActiveFrameIndex());
			Session_UnlockFrames();
		}
		else if (Item == btn_play)
		{
			Mutex_Lock(mtx_play);
			bPlaying = !bPlaying;
			Window_Item_SetText(btn_play, bPlaying ? ICON_STOP : ICON_PLAY);
			Mutex_Unlock(mtx_play);
			if (!bPlaying && thread_play)
			{
				Thread_Wait(thread_play, 0); // Make sure mutexes are released
				Thread_Destroy(thread_play);
				thread_play = 0;
			}
			else
			{
				thread_play = Thread_Create(&PlayThread, 0);
				Thread_Resume(thread_play);
			}
		}
		else if (Item == bBrush || Item == bEraser)
		{
			int checked = Window_Item_GetValuei(Item);
			WndItem* other = Item == bBrush ? bEraser : bBrush;
			if (checked)
			{
				SetTool(Item == bBrush ? &tool_brush : &tool_eraser);
				if (Window_Item_GetValuei(other))
					Window_Item_SetValuei(other, 0);
			}
			else
				SetTool(0);
		}

		// Properties window

		else if (Item == btn_new)
		{
			char bConfirm = OpenDialog(DialogType_YesNo,
				L"Are you sure you want to create a new project?\n"
				L"Any existing data will be wiped.");
			if (bConfirm)
			{
				int w = Window_Item_GetValuei(int_width), h = Window_Item_GetValuei(int_height);
				ResetProject();
				Window_Show(wnd_prop, 0);
			}
		}
		else if (Item == btn_cancel) 
			Window_Show(wnd_prop, 0);
	}
	Mutex_Lock(mtx_items);
	bItemMsg = 0;
	Mutex_Unlock(mtx_items);
	return WndCallback_None;
}

int PlayThread(void* UserData)
{
	Session_LockFrames();
	while (1) // xddddddd
	{
		Mutex_Lock(mtx_play);
		char stop = !bPlaying;
		Mutex_Unlock(mtx_play);

		if (stop)
			break;

		// Directly setting frame to avoid usless network spam
		Session_SetFrame(Session_ActiveFrameIndex() + 1);
		Thread_Current_Sleep(1000 / my_sesh.fps);
	}
	Session_UnlockFrames();
	return 0;
}

void OnSeshMsg(int Msg, UID Object)
{
	Mutex_Lock(mtx_items);
	char item = bItemMsg;
	Mutex_Unlock(mtx_items);
	switch (Msg)
	{
	case SessionMsg_UserStrokeAdd:
	case SessionMsg_UserStrokeRedo:
	case SessionMsg_UserStrokeUndo:
		Window_Redraw(wnd_main, 0);
		break;
	case SessionMsg_ChangedFPS:
		if (!item)
			Window_Item_SetValuei(int_fps, my_sesh.fps);
		break;
	case SessionMsg_ChangedFrame:
		Window_Item_SetValuei(int_frame, Session_ActiveFrameIndex());
		Window_Redraw(wnd_main, 0);
		break;
	case SessionMsg_ChangedFramelist:
		Window_IntBox_SetRange(int_frame, 0, Session_FrameCount() - 1);
		break;
	case SessionMsg_UserJoin:
	{
		NetUser* user = Session_GetUser(Object);
		printf("User \"%S\" has joined\n", user->szName);
		break;
	}
	case SessionMsg_UserLeave:
	{
		NetUser * user = Session_GetUser(Object);
		printf("User \"%S\" has left\n", user->szName);
		break;
	}
	}
}

int main()
{
	printf("Hello from C\n");

	mtx_play = Mutex_Create();
	mtx_items = Mutex_Create();

	WindowCreationArgs args = { 0 };
	args.width = 600, args.height = 400 + 35;
	args.x = args.y = -1;
	args.sztitle = L"ouchy owwy ouch";
	args.visible = 1;
	args.on_mouse = &OnMouse;
	args.on_keyboard = &OnKeyboard;
	args.on_wndmsg = &OnWndMsg;
	args.on_menu = &OnMenu;
	args.on_itemmsg = &OnItemMsg;

	wnd_main = Window_Create(&args);
	picker_brush = ColorPicker_Create(wnd_main, L"Brush color");
	picker_brush->on_changed = &OnColorPicked;
	Window_SetPos(picker_brush->wnd, 0, 35);

	MenuItem* mProj = Window_Menu_Add(wnd_main, L"&Project");
	mProperties = Window_Menu_Add_Child(wnd_main, mProj, L"&Properties...");

	MenuItem* mSesh = Window_Menu_Add(wnd_main, L"&Session");
	Window_Menu_Add_Child(wnd_main, mSesh, L"&Join");
	Window_Menu_Add_Child(wnd_main, mSesh, L"&Host");

	int nextx = 5;
	btn_add = Window_Item_Add(wnd_main, ItemType_Button, 5, 5, 40, 23, L"Add");
	nextx += btn_add->width + 5;
	btn_rem = Window_Item_Add(wnd_main, ItemType_Button, nextx, btn_add->y, 70, btn_add->height, L"Remove");
	nextx += btn_rem->width + 5;
	btn_play = Window_Item_Add(wnd_main, ItemType_Button, nextx, btn_add->y, 40, btn_add->height, ICON_PLAY);
	nextx += btn_play->width + 5;
	WndItem* tmp = Window_Item_Add(wnd_main, ItemType_Label, nextx, btn_add->y, 30, btn_add->height, L"Frame:");
	nextx += tmp->width + 5;
	int_frame = Window_Item_Add(wnd_main, ItemType_IntBox, nextx, btn_add->y, 100, btn_add->height, L"int_frame");
	nextx += int_frame->width + 5;
	tmp = Window_Item_Add(wnd_main, ItemType_Label, nextx, btn_add->y, 20, btn_add->height, L"FPS: ");
	nextx += tmp->width + 5;
	int_fps = Window_Item_Add(wnd_main, ItemType_IntBox, nextx, btn_add->y, 40, btn_add->height, L"int_fps ");
	nextx += int_fps->width + 5;
	bBrush = Window_Item_Add(wnd_main, ItemType_CheckBox, nextx, btn_add->y, 50, btn_add->height, L"Brush");
	nextx += bBrush->width + 5;
	bEraser = Window_Item_Add(wnd_main, ItemType_CheckBox, nextx, btn_add->y, 50, btn_add->height, L"Eraser");
	nextx += bEraser->width + 5;
	tmp = Window_Item_Add(wnd_main, ItemType_Label, nextx, btn_add->y, 30, btn_add->height + 5, L"Brush Size");
	nextx += tmp->width + 5;
	int_brush = Window_Item_Add(wnd_main, ItemType_IntBox, nextx, btn_add->y, 50, btn_add->height, L"int_width");

	Window_IntBox_SetRange(int_fps, 1, 100);

	Window_Item_SetValuei(bBrush, 1);
	Window_IntBox_SetRange(int_brush, 1, 1000);
	SetTool(&tool_brush);

	memset(&args, 0, sizeof(args));
	args.width = 175, args.height = 140;
	args.sztitle = L"Properties";
	args.visible = 0;
	args.on_wndmsg = &OnWndMsg;
	args.on_itemmsg = &OnItemMsg;
	
	wnd_prop = Window_Create(&args);
	Window_Item_Add(wnd_prop, ItemType_Label, 10, 10, 50, 23, L"Width");
	int_width = Window_Item_Add(wnd_prop, ItemType_IntBox, 60, 10, 110, 23, L"");
	Window_Item_Add(wnd_prop, ItemType_Label, 10, 40, 50, 23, L"Height");
	int_height = Window_Item_Add(wnd_prop, ItemType_IntBox, 60, 40, 110, 23, L"");

	picker_bkg = ColorPicker_Create(wnd_prop, L"Bkg color");
	int nexty = int_height->y + int_height->height;
	Window_SetPos(picker_bkg->wnd, 5, nexty);

	int wndW = 0, wndH = 0;
	Window_GetSize(picker_bkg->wnd, &wndW, &wndH);
	nexty += wndH;

	btn_new = Window_Item_Add(wnd_prop, ItemType_Button, 5, nexty, 80, 30, L"New project");
	btn_cancel = Window_Item_Add(wnd_prop, ItemType_Button, 90, nexty , 80, 30, L"Cancel");
	nexty += btn_cancel->height + 5;

	Window_SetSize(wnd_prop, args.width, nexty);

	Window_IntBox_SetRange(int_width, 1, SHRT_MAX - 1);
	Window_IntBox_SetRange(int_height, 1, SHRT_MAX - 1);
	Window_Item_SetValuei(int_width, 600);
	Window_Item_SetValuei(int_height, 400);

	my_sesh.on_seshmsg = &OnSeshMsg;

	ResetProject();

	return Window_RunAll();
}

char bServer = 0;

int SetupThread(void* UserData) {
	if (bServer)
		Server_StartAndRun("42042");
	else
		Client_StartAndRun(L"poggers lucas", "127.0.0.1", "42042");
}

void ResetProject()
{
	int width = Window_Item_GetValuei(int_width), height = Window_Item_GetValuei(int_height);

	Client_StartAndRun_Local();
	Session_Init(picker_bkg->color, width, height, 24);

	bServer = OpenDialog(DialogType_YesNo, L"Start as server?");

	Thread_Resume(Thread_Create(&SetupThread, 0));
	Thread_Current_Sleep(1000); // Testing


	if (bServer)
	my_netint.join(L"reynante_gamer512");

	tool_eraser.Color = my_sesh.bkgcol;
	Window_Redraw(wnd_main, 0);
}
