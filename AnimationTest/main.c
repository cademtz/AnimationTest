#include <stdio.h>
#include <string.h>
#include "window.h"
#include "project.h"

MenuItem* menu_prop;
WndItem* int_frame, * btn_add, * btn_rem, * btn_clear, * btn_play, * lbl_frame, * lbl_add;
int lLastX, lLastY, mLastX, mLastY, canvasX = 0, canvasY = 35;


WndHandle wnd_properties, wnd_main;
WndItem* int_width, * int_height, * btn_new, * btn_cancel;

void ResetProject();

inline void ChangedFrameCount() {
	Window_IntBox_SetRange(int_frame, 0, my_project._frames->count - 1);
}

void SetFrame(int Index)
{
	Project_ChangeFrame(Index);
	Window_Item_SetValuei(int_frame, Index);
	Window_Redraw(wnd_main, 0);
}

int OnMenu(MenuItem* Item)
{
	if (Item == menu_prop)
		Window_Show(wnd_properties, 1);
	return WndCallback_None;
}

int OnMouse(WndHandle Wnd, int X, int Y, int MouseBtn, int Down)
{
	switch (MouseBtn)
	{
	case MouseBtn_Left:
		if (my_project.frame_active && Down > 1)
		{
			int x1 = lLastX, y1 = lLastY, x2 = X, y2 = Y;
			x1 -= canvasX, x2 -= canvasX, y1 -= canvasY, y2 -= canvasY;
			Bitmap_Draw_Line(my_project.frame_active->data->bmp, x1, y1, x2, y2, 0x000000);
			Window_Redraw(Wnd, 0);
		}
		if (Down)
			lLastX = X, lLastY = Y;
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
			int idx = Window_Item_GetValuei(int_frame) + (Key == Key_Comma ? -1 : 1);
			if (idx < 0)
				idx = my_project._frames->count - 1;
			else if (idx >= my_project._frames->count)
				idx = 0;
			SetFrame(idx);
		}
	}
	return WndCallback_None;
}

int OnWndMsg(WndHandle Wnd, int WndMsg)
{
	if (Wnd == wnd_properties)
	{
		if (WndMsg == WndMsg_Closing)
		{
			Window_Show(Wnd, 0);
			//Project_Init(600, 800, 0xAAAAAA);
			//ResetProject();
			return WndCallback_Skip;
		}
	}
	else // Main window
	{
		switch (WndMsg)
		{
		case WndMsg_Draw:
			if (my_project.frame_active)
				Window_Draw_Bitmap(Wnd, my_project.frame_active->data->bmp, canvasX, canvasY);
			break;
		case WndMsg_Closing:
			return WndCallback_Close;
		}
	}
	return WndCallback_None;
}

int OnItemMsg(WndItem* Item, int ItemMsg, ItemMsgData* Data)
{
	switch (ItemMsg)
	{
	case ItemMsg_ValueChanged:
		if (Item == int_frame)
		{
			Project_ChangeFrame(Data->newval.i);
			Window_Redraw(int_frame->wnd, 0);
		}
		break;
	case ItemMsg_Clicked:
		if (Item == btn_add)
		{
			int idx = Window_Item_GetValuei(int_frame) + 1;
			Project_InsertFrame(idx);
			//Project_ChangeFrame(idx);
			ChangedFrameCount();
			SetFrame(idx);
			//Window_Item_SetValuei(int_frame, idx);
			//Window_Redraw(Item->wnd, 0);
		}
		else if (Item == btn_rem && my_project._frames->count > 1)
		{
			int idx = Window_Item_GetValuei(int_frame);
			FrameItem* frame = FrameList_Remove_At(idx);
			if (frame)
			{
				char dup = 0;
				for (FrameItem* next = 0; next = FrameList_Next(next);)
					if (next->data == frame->data)
						dup = 1;

				if (!dup) // No other frame is using the same data
					FrameData_Destroy(frame->data);
				Frame_Destroy(frame);
				ChangedFrameCount();
				Project_ChangeFrame(idx >= my_project._frames->count - 1 ? --idx : idx);
				Window_Redraw(Item->wnd, 0);
			}
		}
	}

	return WndCallback_None;
}

int main()
{
	printf("Hello from C\n");

	Project_Init(400, 300, 0xFFFFFF);

	WindowCreationArgs args = { 0 };
	args.width = my_project.width, args.height = my_project.height + 35;
	args.sztitle = L"ouchy owwy ouch";
	args.visible = 1;
	args.on_mouse = &OnMouse;
	args.on_keyboard = &OnKeyboard;
	args.on_wndmsg = &OnWndMsg;
	args.on_menu = &OnMenu;
	args.on_itemmsg = &OnItemMsg;

	wnd_main = Window_Create(&args);
	MenuItem* mProj = Window_Menu_Add(wnd_main, L"&Project", 0);
	menu_prop = Window_Menu_Add_Child(wnd_main, mProj, L"&Properties...", &OnMenu);

	MenuItem* mSesh = Window_Menu_Add(wnd_main, L"&Session", 0);
	Window_Menu_Add_Child(wnd_main, mSesh, L"&Join", 0);
	Window_Menu_Add_Child(wnd_main, mSesh, L"&Host", 0);

	int nextx = 5;
	btn_add = Window_Item_Add(wnd_main, ItemType_Button, 5, 5, 40, 23, L"Add");

	nextx += btn_add->width + 5;
	btn_rem = Window_Item_Add(wnd_main, ItemType_Button, nextx, btn_add->y, 70, btn_add->height, L"Remove");

	nextx += btn_rem->width + 5;
	int_frame = Window_Item_Add(wnd_main, ItemType_IntBox, nextx, btn_add->y, 100, btn_add->height, L"An intbox...");

	ResetProject();

	memset(&args, 0, sizeof(args));
	args.width = 200, args.height = 250;
	args.sztitle = L"Properties";
	args.visible = 0;
	args.on_wndmsg = &OnWndMsg;
	
	wnd_properties = Window_Create(&args);

	return Window_RunAll();
}

void ResetProject()
{
	FrameList_Reset();
	Window_IntBox_SetRange(int_frame, 0, my_project._frames->count - 1);
	Window_Item_SetValuei(int_frame, 0);
}
