#pragma once
#include "common.h"

typedef struct _WndHandle* WndHandle;
typedef struct _WndItem WndItem;
typedef struct _ItemMsgData ItemMsgData;
typedef struct _MenuItem MenuItem;
typedef struct _BitmapData* BitmapHandle;

typedef int(*MouseCallback)(WndHandle Wnd, int X, int Y, int MouseBtn, int Down);
typedef int(*KeyboardCallback)(WndHandle Wnd, char Key, char bDown);
typedef int(*WndCallback)(WndHandle Wnd, int WndMsg);
typedef int(*ItemCallback)(WndItem* Item, int ItemMsg, ItemMsgData* Data);
typedef int(*MenuCallback)(MenuItem* Item);

// Handy tools

// - Returns true if any "Yes" option was selected
extern char OpenDialog(int DialogType, UniChar* Text); 

// Windows

typedef struct _WindowCreationArgs
{
	unsigned int width, height;
	int x, y; // If top-level window, set -1 for default pos
	const UniChar* sztitle;
	char visible; // If true, start window as visible

	// ----- Optional ----- //

	void* userdata;

	MouseCallback on_mouse;
	KeyboardCallback on_keyboard;
	WndCallback on_wndmsg;
	ItemCallback on_itemmsg;
	MenuCallback on_menu;
} WindowCreationArgs;

// - Returns 0 on failure
// - Only use within a single designated window thread
WndHandle Window_Create(const WindowCreationArgs* Args);
WndHandle Window_Create_Child(WndHandle Parent, const WindowCreationArgs* Args);

// - Performs a blocking call, running all created windows so far
// - Must never be used outside the thread you have created windows in
extern int Window_RunAll();
extern int Window_Show(WndHandle Wnd, char bShow);
char Window_GetPos(WndHandle Wnd, int* X, int* Y);
char Window_SetPos(WndHandle Wnd, int X, int Y);
char Window_GetSize(WndHandle Wnd, int* Width, int* Height);
char Window_SetSize(WndHandle Wnd, int Width, int Height);

MenuItem* Window_Menu_Add(WndHandle Wnd, const char* Name);
MenuItem* Window_Menu_Add_Child(WndHandle Wnd, MenuItem* Parent, const char* Name);

// Drawing

extern void Window_Redraw(WndHandle Wnd, int* opt_xywh);
extern void Window_Draw_Rect(WndHandle Wnd, int X, int Y, int W, int H, IntColor Color);
extern void Window_Draw_Line(WndHandle Wnd, int X1, int Y1, int X2, int Y2, int Width, IntColor Color);
extern void Window_Draw_Bitmap(WndHandle Wnd, BitmapHandle Bmp, int X, int Y);

// Items

// - Returns 0 at the end
// - Set 'Item' to 0 for start of list
WndItem* Window_Items_Next(WndHandle Wnd, WndItem* Item);
WndItem* Window_Item_Add(WndHandle Wnd, int Type, int X, int Y, int W, int H, const UniChar* sztext);

extern void Window_Item_SetText(WndItem* Item, UniChar* szText);
extern void Window_Item_SetValuei(WndItem* Item, int Value);
extern void Window_Item_SetValuef(WndItem* Item, float Value);
extern int Window_Item_GetValuei(WndItem* Item);
extern float Window_Item_GetValuef(WndItem* Item);
void Window_IntBox_SetRange(WndItem* IntBox, int Min, int Max);
void Window_IntBox_GetRange(WndItem* IntBox, int* outMin, int* outMax);

// Bitmaps

BitmapHandle Bitmap_Create(unsigned int Width, unsigned int Height);
void Bitmap_Destroy(BitmapHandle Bmp);
extern void Bitmap_Draw_Line(BitmapHandle Bmp, int X1, int Y1, int X2, int Y2, int Width, IntColor Color);
extern void Bitmap_Draw_Rect(BitmapHandle Bmp, int X, int Y, int W, int H, IntColor Color);

// Enums

enum EMouseBtn
{
	MouseBtn_None = 0,
	MouseBtn_Left,
	MouseBtn_Right,
	MoustBtn_Middle
};

enum EItemType
{
	ItemType_Button = 0,
	ItemType_Label,
	ItemType_IntBox,
	ItemType_CheckBox,
};

enum EMsgs
{
	WndMsg_Created = 0,
	WndMsg_Closing,
	WndMsg_Hidden,
	WndMsg_Visible,
	WndMsg_Draw,

	ItemMsg_Clicked = 0,
	ItemMsg_ValueChanged, // - One or many values have changed
};

enum EWndCallback
{
	WndCallback_None = 0,
	WndCallback_Skip, // - Skip default behavior after return
	WndCallback_Close // - Returning this will close and destroy the window
};

enum EKey
{
	Key_None = 0,
	Key_Comma,
	Key_Period,
	Key_Ctrl,
	Key_Shift,
	Key_Alt,
	Key_Space,
	Key_LBracket, // Can be '[' or '{'
	Key_RBracket, // Can be ']' or '}'
	// Keys for numbers 0-9 and letters A-Z correspond with their ASCII values ('A', 'B', '7', etc...)
};

enum EDialogType
{
	DialogType_Ok = 0,
	DialogType_YesNo,
	DialogType_Error,
	DialogType_Warning,
};

// ----- Private functions and variables ----- //

// Windows

// OS-dependent data structs. Implement these how you like them.
typedef struct _OSDataImpl OSData; 
typedef struct _OSBitmapImpl OSBitmap;

typedef struct _MenuList MenuList;
typedef struct _WndItemList WndItemList;
typedef struct _WindowList WindowList;

struct _WndHandle
{
	void* userdata;
	MenuList* _menus;
	WndItemList* _items;
	WindowCreationArgs _args;
	WndHandle _next;
	WndHandle _parent;
	WindowList* _children;
	OSData* _data;
};

typedef struct _WindowList
{
	WndHandle head, tail;
	unsigned int count;
} WindowList;

extern WindowList _Window_list;

// - Returns 0 at the end
// - Set 'Item' to 0 for start of list
WndHandle _Window_list_Next(WndHandle Wnd);
WndHandle _Window_Child_Next(WndHandle Wnd, WndHandle Next);
WndHandle _Window_Create(WndHandle Parent, const WindowCreationArgs* Args);
extern OSData* _Window_Create_Impl(WndHandle Data);

extern char _Window_GetSetPos_Impl(WndHandle Wnd, int* opt_getXY, int* opt_setXY);
extern char _Window_GetSetSize_Impl(WndHandle Wnd, int* opt_getWH, int* opt_setWH);

// Menu list and menu items

typedef struct _MenuItem
{
	const UniChar* sztext;
	WndHandle wnd;
	MenuItem* _next;
	MenuItem* _parent;
	struct _MenuList* _children;
	void* _id; // OS-dependent identifier
} MenuItem;

typedef struct _MenuList
{
	MenuItem* head, * tail;
	unsigned int count;
} MenuList;

extern int _Window_Menu_Add_Impl(WndHandle Wnd, MenuItem* Item);
extern int _Window_Menu_Add_Child_Impl(WndHandle Wnd, MenuItem* Parent, MenuItem* Item);

// - Returns 0 at the end
// - Set 'Item' to 0 for start of list
MenuItem* _Window_Menu_Next(WndHandle Wnd, MenuItem* Item);
MenuItem* _Window_Menu_Child_Next(MenuItem* Parent, MenuItem* Item);

// Window items

typedef struct _WndItem
{
	const UniChar* sztext;
	int x, y, width, height;
	int type;
	WndHandle wnd;
	WndItem* _next;
	void* _id; // OS-dependent identifier
} WndItem;

typedef struct _WndItemList
{
	WndItem* head, * tail;
	int count;
} WndItemList;

typedef struct _ItemMsgData
{
	union
	{
		int i;
		float fl;
	} newval;
	union
	{
		int i;
		float fl;
	} oldval;
} ItemMsgData;

extern int _Window_Item_Add_Impl(WndItem* Item);
extern char _Window_Item_UpdateText_Impl(WndItem* Item);
extern char _Window_IntBox_GetSetRange_Impl(WndItem* IntBox, int* opt_getMinMax, int* opt_setMinMax);

// Bitmaps

typedef struct _BitmapData
{
	unsigned int width, height;
	OSBitmap* _data;
} BitmapData;

extern int _Bitmap_Create_Impl(BitmapData* Data);
extern int _Bitmap_Destroy_Impl(OSBitmap* osData);