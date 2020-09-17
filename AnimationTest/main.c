#include <stdio.h>
#include <string.h>
#include "window.h"

BitmapHandle bmp = 0;
MenuItem* menu_prop;
WndItem* int_framecount, * btn_addframe;
WndHandle wnd_properties;
int lastX, lastY;

int OnMenu(MenuItem* Item)
{
	if (Item == menu_prop)
		Window_Show(wnd_properties, 1);
	return WndCallback_None;
}

int OnMouse(WndHandle Wnd, int X, int Y, int MouseBtn, int Down)
{
	if (MouseBtn == MouseBtn_Left && Down)
	{
		if (Down > 1)
		{
			Bitmap_Draw_Line(bmp, lastX, lastY, X, Y, 0x000000);
			Window_Redraw(Wnd);
		}
		lastX = X, lastY = Y;
	}
	return WndCallback_None;
}

int OnMsg(WndHandle Wnd, int WndMsg)
{
	if (Wnd == wnd_properties)
	{
		if (WndMsg == WndMsg_Closing)
		{
			Window_Show(Wnd, 0);
			return WndCallback_Skip;
		}
	}
	else
	{
		switch (WndMsg)
		{
		case WndMsg_Draw:
			Window_Draw_Bitmap(Wnd, bmp, 0, 0);
			break;
		case WndMsg_Closing:
			return WndCallback_Close;
		}
	}
	return WndCallback_None;
}

int main()
{
	printf("Hello from C\n");

	WindowCreationArgs args = { 0 };
	args.width = 300, args.height = args.width + 35;
	args.sztitle = L"ouchy owwy ouch";
	args.visible = 1;
	args.on_mouse = &OnMouse;
	args.on_wndmsg = &OnMsg;
	args.on_menu = &OnMenu;

	WndHandle wnd = Window_Create(&args);
	MenuItem* mProj = Window_Menu_Add(wnd, L"&Project", 0);
	menu_prop = Window_Menu_Add_Child(wnd, mProj, L"&Properties...", &OnMenu);

	MenuItem* mSesh = Window_Menu_Add(wnd, L"&Session", 0);
	Window_Menu_Add_Child(wnd, mSesh, L"&Join", 0);
	Window_Menu_Add_Child(wnd, mSesh, L"&Host", 0);

	bmp = Bitmap_Create(args.width, args.width);
	Bitmap_Draw_Rect(bmp, 0, 0, args.width, args.height, 0xFFFFFF);

	int nextx = 5;
	btn_addframe = Window_Item_Add(wnd, ItemType_Button, 5, args.width + 5, 100, 23, L"Mai Kewl Btn");
	nextx += btn_addframe->width + 5;

	WndItem* item_frames = Window_Item_Add(wnd, ItemType_IntBox, nextx, btn_addframe->y, 100, btn_addframe->height, L"An intbox...");
	Window_IntBox_SetRange(item_frames, 0, 0);
	Window_Item_SetValuei(item_frames, 0);

	memset(&args, 0, sizeof(args));
	args.width = 200, args.height = 250;
	args.sztitle = L"Properties";
	args.visible = 0;
	args.on_wndmsg = &OnMsg;
	
	wnd_properties = Window_Create(&args);

	return Window_RunAll();
}