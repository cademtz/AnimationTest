#include "../window.h"
#include <Windows.h>
#include <stdio.h>

HINSTANCE hInst = 0;

struct _OSDataImpl
{
	void* szTitle;
	void* aClass;
	HWND hWnd;
	MenuItem mainMenu;
};

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LPSTR UnicodeToUTF8(const wchar_t* Unicode);
MenuItem* Menu_FindId(WndHandle Wnd, void* Id);

OSData* _Window_Create_Impl(const WindowCreationArgs* Args)
{
	OSData* win32 = malloc(sizeof(*win32));
	ZeroMemory(win32, sizeof(*win32));

	 win32->szTitle = Args->sztitle;
#ifndef UNICODE
	win32->szTitle = UnicodeToUTF8(Args->sztitle);
#endif

	hInst = GetModuleHandle(0);

	WNDCLASS cl;
	if (GetClassInfo(hInst, win32->szTitle, &cl))
		win32->aClass = win32->szTitle; // Class already exists
	else
	{
		ZeroMemory(&cl, sizeof(cl));
		cl.hInstance = hInst;
		cl.hbrBackground = WHITE_BRUSH;
		cl.lpszClassName = win32->szTitle;
		cl.hCursor = LoadCursor(0, IDC_ARROW);
		cl.lpfnWndProc = &WndProc;
		win32->aClass = RegisterClass(&cl);
	}

	win32->hWnd = CreateWindow(
		win32->aClass, win32->szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		Args->width, Args->height, 0, 0, hInst, 0);

	if (Args->visible)
		ShowWindow(win32->hWnd, SW_SHOW);

	return win32;
}

int _Window_Run(WndHandle Wnd)
{
	MSG msg;
	if (GetMessage(&msg, Wnd->_data->hWnd, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		return 1;
	}
	return 0;
}

void Window_Draw_Dot(WndHandle Wnd, int X, int Y)
{
	HDC dc = GetDC(Wnd->_data->hWnd);

	static HPEN pen = 0;
	if (!pen)
		pen = CreatePen(PS_SOLID, 5, RGB(0, 0, 0));

	HGDIOBJ oldpen = SelectObject(dc, pen);

	MoveToEx(dc, X, Y, 0);
	LineTo(dc, X, Y);

	SelectObject(dc, oldpen);
}

int _Window_Menu_Add_Impl(WndHandle Wnd, MenuItem* Item)
{
	if (Item->id)
		return 0;

	if (!GetMenu(Wnd->_data->hWnd))
	{
		Wnd->_data->mainMenu.id = CreateMenu();
		SetMenu(Wnd->_data->hWnd, Wnd->_data->mainMenu.id);
	}

	HMENU menu = CreateMenu();

#ifdef UNICODE
	AppendMenu(Wnd->_data->mainMenu.id, 0, menu, Item->szname);
#else
	LPSTR text = UnicodeToUTF8(Item->szname);
	AppendMenu(args->mainMenu.id, 0, menu, text);
	free(text);
#endif

	DrawMenuBar(Wnd->_data->hWnd);

	Item->id = menu;
	Item->parent = &Wnd->_data->mainMenu;
	return menu != 0;
}

int _Window_Menu_Add_Child_Impl(WndHandle Wnd, MenuItem* Parent, MenuItem* Item)
{
	HMENU hParent = Parent->id;
	if (!hParent || Item->id)
		return 0;

	if (Parent->parent)
	{
		HMENU hAncestor = Parent->parent->id;
		DWORD state = GetMenuState(hAncestor, Parent->id, MF_BYCOMMAND);
		if (state == -1)
			return 0;

		if (!(state & MF_POPUP))
		{
#ifdef UNICODE
			ModifyMenu(hAncestor, hParent, MF_BYCOMMAND | state | MF_POPUP, hParent, Parent->szname);
#else
			LPSTR text = UnicodeToUTF8(Parent->szname);
			ModifyMenu(hAncestor, hParent, MF_BYCOMMAND | state | MF_POPUP, hParent, text);
			free(text);
#endif
		}
	}

	HMENU menu = CreateMenu();

#ifdef UNICODE
	AppendMenu(hParent, MF_STRING, menu, Item->szname);
#else
	LPSTR text = UnicodeToUTF8(Item->szname);
	AppendMenu(hParent, MF_STRING, menu, text);
	free(text);
#endif

	DrawMenuBar(Wnd->_data->hWnd);

	Item->id = menu;
	return menu != 0;
}

void HandleMouse(WndHandle Wnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int x = LOWORD(lParam), y = HIWORD(lParam);
	static int btn = MouseBtn_None;
	static int down = 0;

	if (uMsg != WM_MOUSEMOVE || down)
		down++;

	switch (uMsg)
	{
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
		down = 0;
		break;
	}

	switch (uMsg)
	{
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
		btn = MouseBtn_Left;
		break;
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
		btn = MouseBtn_Right;
		break;
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
		btn = MoustBtn_Middle;
		break;
	}

	if (Wnd->_args.onmouse)
		Wnd->_args.onmouse(Wnd, x, y, btn, down);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	WndHandle wnd = 0;
	for (WndHandle next = 0; next = _Window_list_Next(next);)
	{
		if (next->_data->hWnd == hWnd)
		{
			wnd = next;
			break;
		}
	}

	if (uMsg == WM_CLOSE)
	{
		DestroyWindow(hWnd);
		PostQuitMessage(EXIT_SUCCESS);
		return 1;
	}

	if (wnd)
	{
		switch (uMsg)
		{
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		case WM_MOUSEMOVE:
			HandleMouse(wnd, uMsg, wParam, lParam);
			break;
		case WM_COMMAND:
		{
			MenuItem* found = Menu_FindId(wnd, (DWORD)wParam);
			if (found && found->callback)
				found->callback();
			break;
		}
		}
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LPSTR UnicodeToUTF8(const wchar_t* Unicode)
{
	size_t len = wcslen(Unicode) + 1;
	LPSTR text = malloc(len);
	WideCharToMultiByte(CP_UTF8, WC_COMPOSITECHECK, Unicode, -1, text, len, 0, 0);
	return text;
}

MenuItem* Menu_FindId_Recurse(WndHandle Wnd, MenuItem* Item, void* Id)
{
	if ((DWORD)Item->id == Id)
		return Item;

	for (MenuItem* next = 0; next = _Window_Menu_Child_Next(Item, next);)
	{
		MenuItem* found = Menu_FindId_Recurse(Wnd, next, Id);
		if (found)
			return found;
	}
	
	return 0;
}

MenuItem* Menu_FindId(WndHandle Wnd, void* Id)
{
	for (MenuItem* next = 0; next = _Window_Menu_Next(Wnd, next);)
	{
		MenuItem* found = Menu_FindId_Recurse(Wnd, next, Id);
		if (found)
			return found;
	}
	
	return 0;
}
