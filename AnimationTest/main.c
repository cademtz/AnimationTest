#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "window.h"
#include "project.h"

typedef struct _DrawTool
{
	int Width;
	unsigned int Color; // RGB
} DrawTool;

DrawTool tool_brush = { 2, 0x000000 }, tool_eraser = { 16, 0xFFFFFF };
DrawTool* tool = &tool_brush;

MenuItem* mProperties, * mEraser, * mBrush;
WndItem* int_frame, * int_brush, * btn_add, * btn_rem, * btn_clear, * btn_play, * bBrush, * bEraser;
int lLastX, lLastY, mLastX, mLastY, canvasX = 0, canvasY = 35;

WndHandle wnd_prop, wnd_main;
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

void AddFrame()
{
	int idx = Window_Item_GetValuei(int_frame) + 1;
	Project_InsertFrame(idx);
	ChangedFrameCount();
	SetFrame(idx);
}

void SetTool(DrawTool* Tool)
{
	tool = Tool;
	if (tool)
		Window_Item_SetValuei(int_brush, tool->Width);
}

int OnMenu(MenuItem* Item)
{
	if (Item == mProperties)
		Window_Show(wnd_prop, 1);
	return WndCallback_None;
}

int OnMouse(WndHandle Wnd, int X, int Y, int MouseBtn, int Down)
{
	switch (MouseBtn)
	{
	case MouseBtn_Left:
		if (my_project.frame_active && Down > 1 && tool)
		{
			int x1 = lLastX, y1 = lLastY, x2 = X, y2 = Y;
			x1 -= canvasX, x2 -= canvasX, y1 -= canvasY, y2 -= canvasY;
			Bitmap_Draw_Line(my_project.frame_active->data->bmp, x1, y1, x2, y2, tool->Width, tool->Color);
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
		break;
	case Key_Space:
		if (bDown)
			AddFrame();
		break;
	case 'E':
	case 'B':
	{
		WndItem* item = Key == 'B' ? bBrush : bEraser, * other = Key != 'B' ? bBrush : bEraser;
		Window_Item_SetValuei(item, 1);
		Window_Item_SetValuei(other, 0);
		SetTool(Key == 'B' ? &tool_brush : &tool_eraser);
	}
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
		else if (Item == int_brush)
			if (tool)
				tool->Width = Data->newval.i;
		break;
	case ItemMsg_Clicked:

		// Main window

		if (Item == btn_add)
			AddFrame();
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
				Project_Init(w, h, 0xFFFFFF);
				ResetProject();
				Window_Show(wnd_prop, 0);
			}
		}
		else if (Item == btn_cancel) 
			Window_Show(wnd_prop, 0);
	}

	return WndCallback_None;
}

int main()
{
	printf("Hello from C\n");

	Project_Init(600, 400, 0xFFFFFF);

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
	mProperties = Window_Menu_Add_Child(wnd_main, mProj, L"&Properties...", &OnMenu);

	//mBrush = Window_Menu_Add(wnd_main, L"Brush");
	//mEraser = Window_Menu_Add(wnd_main, L"Eraser");

	MenuItem* mSesh = Window_Menu_Add(wnd_main, L"&Session", 0);
	Window_Menu_Add_Child(wnd_main, mSesh, L"&Join", 0);
	Window_Menu_Add_Child(wnd_main, mSesh, L"&Host", 0);

	int nextx = 5;
	btn_add = Window_Item_Add(wnd_main, ItemType_Button, 5, 5, 40, 23, L"Add");
	nextx += btn_add->width + 5;
	btn_rem = Window_Item_Add(wnd_main, ItemType_Button, nextx, btn_add->y, 70, btn_add->height, L"Remove");
	nextx += btn_rem->width + 5;
	int_frame = Window_Item_Add(wnd_main, ItemType_IntBox, nextx, btn_add->y, 100, btn_add->height, L"An intbox...");
	nextx += int_frame->width + 5;
	bBrush = Window_Item_Add(wnd_main, ItemType_CheckBox, nextx, btn_add->y, 50, btn_add->height, L"Brush");
	nextx += bBrush->width + 5;
	bEraser = Window_Item_Add(wnd_main, ItemType_CheckBox, nextx, btn_add->y, 50, btn_add->height, L"Eraser");
	nextx += bEraser->width + 5;
	WndItem* tmp = Window_Item_Add(wnd_main, ItemType_Label, nextx, btn_add->y, 30, btn_add->height + 5, L"Brush Size");
	nextx += tmp->width + 5;
	int_brush = Window_Item_Add(wnd_main, ItemType_IntBox, nextx, btn_add->y, 50, btn_add->height, L"int_width");

	Window_Item_SetValuei(bBrush, 1);
	Window_IntBox_SetRange(int_brush, 1, 1000);
	SetTool(&tool_brush);
	//Window_Item_SetValuei(int_brush, tool_brush.Width);

	ResetProject();

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

	btn_new = Window_Item_Add(wnd_prop, ItemType_Button, 5, 100, 80, 30, L"New project");
	btn_cancel = Window_Item_Add(wnd_prop, ItemType_Button, 90, 100, 80, 30, L"Cancel");

	Window_IntBox_SetRange(int_width, 1, SHRT_MAX - 1);
	Window_IntBox_SetRange(int_height, 1, SHRT_MAX - 1);
	Window_Item_SetValuei(int_width, 1);
	Window_Item_SetValuei(int_height, 1);

	return Window_RunAll();
}

void ResetProject()
{
	FrameList_Reset();
	Window_IntBox_SetRange(int_frame, 0, my_project._frames->count - 1);
	Window_Item_SetValuei(int_frame, 0);
	tool_eraser.Color = my_project.bkgcol;
	Window_Redraw(wnd_main, 0);
}
