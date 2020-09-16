#include <stdio.h>
#include "window.h"

int OnMouse(WndHandle Wnd, int X, int Y, int MouseBtn, int Down)
{
	if (MouseBtn == MouseBtn_Left && Down)
		Window_Draw_Dot(Wnd, X, Y);
	return 0;
}

int OnProperties()
{
	printf("User opened properties\n");
	return 0;
}

int main()
{
	printf("Hello from C\n");

	WindowCreationArgs args;
	args.width = args.height = 300;
	args.sztitle = L"ouchy owwy ouch";
	args.onmouse = &OnMouse;
	args.visible = 1;

	WndHandle wnd = Window_Create(&args);
	MenuItem* mProj = Window_Menu_Add(wnd, L"&Project", 0);
	Window_Menu_Add_Child(wnd, mProj, L"&Properties...", &OnProperties);

	MenuItem* mSesh = Window_Menu_Add(wnd, L"&Session", 0);
	Window_Menu_Add_Child(wnd, mSesh, L"&Join", 0);
	Window_Menu_Add_Child(wnd, mSesh, L"&Host", 0);

	return Window_RunAll();
}