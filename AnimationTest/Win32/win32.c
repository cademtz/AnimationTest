#include "../window.h"
#include <Windows.h>
#include <Uxtheme.h>
#include <CommCtrl.h>
#include <stdio.h>

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "UXTHEME.lib")

HINSTANCE hInst = 0;
HFONT font_best; // best font :)

struct _OSDataImpl
{
	void* szTitle;
	void* aClass;
	HWND hWnd;
	MenuItem mainMenu;
};

struct _OSBitmapImpl
{
	HBITMAP hBmp;
	DWORD* bits;
};

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LPTSTR NativeString(const UniChar* Unicode);
void NativeCleanup(LPTSTR Text);
MenuItem* Menu_FindId(WndHandle Wnd, void* Id);
WndItem* Item_FindId(WndHandle Wnd, void* Id);

OSData* _Window_Create_Impl(const WindowCreationArgs* Args)
{
	static char init = 1;
	if (init)
	{
		InitCommonControls();
		font_best = CreateFont(14, 0, 0, 0, FW_DONTCARE, FALSE, FALSE,
			0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
			FIXED_PITCH | FF_MODERN, TEXT("Segoe UI"));
		init = 0;
	}

	OSData* win32 = malloc(sizeof(*win32));
	ZeroMemory(win32, sizeof(*win32));

	 win32->szTitle = Args->sztitle;
#ifndef UNICODE
	win32->szTitle = NativeString(Args->sztitle);
#endif

	hInst = GetModuleHandle(0);

	WNDCLASS cl;
	if (GetClassInfo(hInst, win32->szTitle, &cl))
		win32->aClass = win32->szTitle; // Class already exists
	else
	{
		ZeroMemory(&cl, sizeof(cl));
		cl.hInstance = hInst;
		cl.hbrBackground = (HBRUSH)COLOR_WINDOW;
		cl.lpszClassName = win32->szTitle;
		cl.hCursor = LoadCursor(0, IDC_ARROW);
		cl.lpfnWndProc = &WndProc;
		win32->aClass = RegisterClass(&cl);
	}

	RECT rect = { 0 };
	rect.right = Args->width;
	rect.bottom = Args->height;
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

	win32->hWnd = CreateWindow(
		win32->aClass, win32->szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		rect.right - rect.left, rect.bottom - rect.top, 0, 0, hInst, 0);

	if (Args->visible)
		ShowWindow(win32->hWnd, SW_SHOW);

	return win32;
}

int Window_RunAll()
{
	MSG msg;
	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

int Window_Show(WndHandle Wnd, char bShow) {
	return ShowWindow(Wnd->_data->hWnd, bShow ? SW_SHOW : SW_HIDE);
}

void Window_Redraw(WndHandle Wnd) {
	BOOL bruh = InvalidateRect(Wnd->_data->hWnd, 0, 0);
}

void Window_Draw_Bitmap(WndHandle Wnd, BitmapHandle Bmp, int X, int Y)
{
	HDC hdc = GetDC(Wnd->_data->hWnd);

	BITMAP bm;
	HDC hdcMem = CreateCompatibleDC(hdc);
	HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, Bmp->_data->hBmp);

	GetObject(Bmp->_data->hBmp, sizeof(bm), &bm);
	BitBlt(hdc, X, Y, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);
	SelectObject(hdcMem, hbmOld);
	DeleteDC(hdcMem);

	ReleaseDC(Wnd->_data->hWnd, hdc);
}

void Window_Item_SetValuei(WndItem* Item, int Value)
{
	HWND hWnd = (HWND)Item->_id;
	switch (Item->type)
	{
	case ItemType_IntBox:
		SendMessage(hWnd, UDM_SETPOS32, 0, Value);
		break;
	}
}

void Window_Item_SetValuef(WndItem* Item, float Value)
{
	HWND hWnd = (HWND)Item->_id;
	switch (Item->type)
	{
	case ItemType_IntBox:
		return Window_Item_SetValuei(Item, Value);
	}
}

int Window_Item_GetValuei(WndItem* Item)
{
	HWND hWnd = (HWND)Item->_id;
	BOOL b;
	switch (Item->type)
	{
	case ItemType_IntBox:
		return (int)SendMessage(hWnd, UDM_GETPOS32, 0, &b);
	}
	return 0;
}

float Window_Item_GetValuef(WndItem* Item)
{
	HWND hWnd = (HWND)Item->_id;
	switch (Item->type)
	{
	case ItemType_IntBox:
		return Window_Item_GetValuei(Item);
	}
	return 0.0f;
}

int _Window_Menu_Add_Impl(WndHandle Wnd, MenuItem* Item)
{
	if (Item->_id)
		return 0;

	if (!GetMenu(Wnd->_data->hWnd))
	{
		// Adjust window size to maintain original client size
		DWORD style = GetWindowLong(Wnd->_data->hWnd, GWL_STYLE);

		RECT client, rect;
		GetWindowRect(Wnd->_data->hWnd, &rect);
		GetClientRect(Wnd->_data->hWnd, &client);
		AdjustWindowRect(&client, style, TRUE);

		MoveWindow(Wnd->_data->hWnd, rect.left, rect.top, client.right - client.left, client.bottom - client.top, TRUE);

		Wnd->_data->mainMenu._id = CreateMenu();
		SetMenu(Wnd->_data->hWnd, Wnd->_data->mainMenu._id);
	}

	HMENU menu = CreateMenu();

#ifdef UNICODE
	AppendMenu(Wnd->_data->mainMenu._id, 0, menu, Item->sztext);
#else
	LPTSTR text = NativeString(Item->sztext);
	AppendMenu(args->mainMenu._id, 0, menu, text);
	free(text);
#endif

	DrawMenuBar(Wnd->_data->hWnd);

	Item->_id = menu;
	Item->_parent = &Wnd->_data->mainMenu;
	return menu != 0;
}

int _Window_Menu_Add_Child_Impl(WndHandle Wnd, MenuItem* Parent, MenuItem* Item)
{
	HMENU hParent = Parent->_id;
	if (!hParent || Item->_id)
		return 0;

	if (Parent->_parent)
	{
		HMENU hAncestor = Parent->_parent->_id;
		DWORD state = GetMenuState(hAncestor, Parent->_id, MF_BYCOMMAND);
		if (state == -1)
			return 0;

		if (!(state & MF_POPUP))
		{
#ifdef UNICODE
			ModifyMenu(hAncestor, hParent, MF_BYCOMMAND | state | MF_POPUP, hParent, Parent->sztext);
#else
			LPTSTR text = NativeString(Parent->sztext);
			ModifyMenu(hAncestor, hParent, MF_BYCOMMAND | state | MF_POPUP, hParent, text);
			free(text);
#endif
		}
	}

	HMENU menu = CreateMenu();

#ifdef UNICODE
	AppendMenu(hParent, MF_STRING, menu, Item->sztext);
#else
	LPTSTR text = NativeString(Item->sztext);
	AppendMenu(hParent, MF_STRING, menu, text);
	free(text);
#endif

	DrawMenuBar(Wnd->_data->hWnd);

	Item->_id = menu;
	return menu != 0;
}

int _Window_Item_Add_Impl(WndItem* Item)
{
	LPTSTR sztype = 0;
	DWORD style = WS_CHILD | WS_VISIBLE;

	HWND hParent = Item->wnd->_data->hWnd;

	switch (Item->type)
	{
	case ItemType_Button:
		sztype = WC_BUTTON; break;
	case ItemType_Label:
		sztype = WC_STATIC; break;
	case ItemType_IntBox:
		sztype = UPDOWN_CLASS;
		style |= UDS_SETBUDDYINT | UDS_AUTOBUDDY | UDS_ALIGNLEFT | UDS_ARROWKEYS | UDS_WRAP;
		HWND edit = CreateWindow(WC_EDIT, TEXT("UDSBuddy"), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
			Item->x, Item->y, Item->width, Item->height, hParent, 0, hInst, 0);
		SendMessage(edit, WM_SETFONT, font_best, MAKELPARAM(TRUE, 0));
		break;
	}

	if (!sztype)
		return -1;

	LPTSTR name = NativeString(Item->sztext);

	HWND hWnd = CreateWindow(sztype, name, style, Item->x, Item->y, Item->width, Item->height, hParent, 0, hInst, 0);
	NativeCleanup(name);

	SendMessage(hWnd, WM_SETFONT, font_best, MAKELPARAM(TRUE, 0));

	Item->_id = hWnd;
	return 0;
}

char _Window_Item_UpdateText_Impl(WndItem* Item)
{
	LPTSTR text = NativeString(Item->sztext);
	SendMessage(Item->_id, WM_SETTEXT, 0, text);
	NativeCleanup(text);
	return 0;
}

char _Window_IntBox_GetSetRange_Impl(WndItem* IntBox, int* opt_getMinMax, int* opt_setMinMax)
{
	if (IntBox->type != ItemType_IntBox)
		return 0;

	if (opt_getMinMax)
		SendMessage(IntBox->_id, UDM_GETRANGE32, &opt_getMinMax[0], &opt_getMinMax[1]);
	if (opt_setMinMax)
		SendMessage(IntBox->_id, UDM_SETRANGE32, opt_setMinMax[0], opt_setMinMax[1]);

    return 1;
}

int _Bitmap_Create_Impl(BitmapData* Data)
{
	BITMAPINFO info = { 0 };
	info.bmiHeader.biSize = sizeof(info.bmiHeader); // ??? The number of bytes required by the structure.
	info.bmiHeader.biWidth = Data->width;
	info.bmiHeader.biHeight = Data->height;
	info.bmiHeader.biPlanes = 1; // The number of planes for the target device. This value must be set to 1.
	info.bmiHeader.biBitCount = 32;
	info.bmiHeader.biCompression = BI_RGB;
	info.bmiHeader.biSizeImage = 0; // The size, in bytes, of the image. This may be set to zero for BI_RGB bitmaps.
	info.bmiHeader.biYPelsPerMeter = 1;
	info.bmiHeader.biXPelsPerMeter = 1;
	info.bmiHeader.biClrUsed = 0; // If this value is zero, the bitmap uses the maximum number of colors 

	HDC dc = CreateCompatibleDC(0);

	void* fart = 0;
	HBITMAP hBmp = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &fart, 0, 0);

	DeleteDC(dc);

	Data->_data = malloc(sizeof(*Data->_data));
	Data->_data->hBmp = hBmp, Data->_data->bits = fart;
	return hBmp != 0;
}

int _Bitmap_Destroy_Impl(OSBitmap* osData)
{
	DeleteObject(osData->hBmp);
	free(osData);
	return 0;
}

void Bitmap_Draw_Line(BitmapHandle Bmp, int X1, int Y1, int X2, int Y2, unsigned int Color)
{
	HDC dcBmp = CreateCompatibleDC(0);
	SelectObject(dcBmp, Bmp->_data->hBmp);

	SetDCPenColor(dcBmp, Color);
	SelectObject(dcBmp, GetStockObject(DC_PEN));

	MoveToEx(dcBmp, X1, Y1, 0);
	LineTo(dcBmp, X2, Y2);

	DeleteDC(dcBmp);
}

void Bitmap_Draw_Rect(BitmapHandle Bmp, int X, int Y, int Width, int Height, unsigned int Color)
{
	HDC dc = CreateCompatibleDC(0);
	SelectObject(dc, Bmp->_data->hBmp);

	SetDCBrushColor(dc, Color);

	RECT rect;
	rect.left = X, rect.top = Y, rect.right = X + Width, rect.bottom = Y + Height;
	FillRect(dc, &rect, GetStockObject(DC_BRUSH));

	DeleteDC(dc);
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

	if (Wnd->_args.on_mouse)
		Wnd->_args.on_mouse(Wnd, x, y, btn, down);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	WndHandle wnd = 0;
	if (hWnd)
	{
		for (WndHandle _next = 0; _next = _Window_list_Next(_next);)
		{
			if (_next->_data->hWnd == hWnd)
			{
				wnd = _next;
				break;
			}
		}
	}

	switch (uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(EXIT_SUCCESS);
		return 0;
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
			if (wParam)
			{
				MenuItem* menu = Menu_FindId(wnd, (DWORD)wParam);
				if (menu && wnd->_args.on_menu)
					wnd->_args.on_menu(menu);
			}
			else if (lParam)
			{
				WndItem* item = Item_FindId(wnd, (DWORD)lParam);
				if (item && wnd->_args.on_itemmsg)
					wnd->_args.on_itemmsg(item, ItemMsg_Clicked);
			}
			break;
		case WM_NOTIFY:
		{
			NMHDR* hdr = (NMHDR*)lParam;
			WndItem* item = Item_FindId(wnd, hdr->hwndFrom);
			if (item && wnd->_args.on_itemmsg)
			{
				switch (wParam)
				{
				case UDN_DELTAPOS:
					wnd->_args.on_itemmsg(item, ItemMsg_ValueChanged);
					break;
				}
			}
		}
		case WM_PAINT:
			if (wnd->_args.on_wndmsg)
				wnd->_args.on_wndmsg(wnd, WndMsg_Draw);
			break;
		case WM_CLOSE:
			if (wnd->_args.on_wndmsg && wnd->_args.on_wndmsg(wnd, WndMsg_Closing) == WndCallback_Skip)
				return 0;
			break;
		}
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LPTSTR NativeString(const UniChar* Unicode)
{
#ifdef UNICODE
	return Unicode;
#else
	size_t len = wcslen(Unicode) + 1;
	LPTSTR text = malloc(len);
	WideCharToMultiByte(CP_UTF8, WC_COMPOSITECHECK, Unicode, -1, text, len, 0, 0);
	return text;
#endif;
}

void NativeCleanup(LPTSTR Text)
{
#ifndef UNICODE
	free(Text);
#endif
}

MenuItem* Menu_FindId_Recurse(WndHandle Wnd, MenuItem* Item, void* Id)
{
	if ((DWORD)Item->_id == Id)
		return Item;

	for (MenuItem* _next = 0; _next = _Window_Menu_Child_Next(Item, _next);)
	{
		MenuItem* found = Menu_FindId_Recurse(Wnd, _next, Id);
		if (found)
			return found;
	}
	
	return 0;
}

MenuItem* Menu_FindId(WndHandle Wnd, void* Id)
{
	for (MenuItem* _next = 0; _next = _Window_Menu_Next(Wnd, _next);)
	{
		MenuItem* found = Menu_FindId_Recurse(Wnd, _next, Id);
		if (found)
			return found;
	}
	
	return 0;
}

WndItem* Item_FindId(WndHandle Wnd, void* Id)
{
	for (WndItem* _next = 0; _next = Window_Items_Next(Wnd, _next);)
		if (_next->_id == Id)
			return _next;
	return 0;
}
