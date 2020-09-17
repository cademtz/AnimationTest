#pragma once

typedef struct _WndHandle* WndHandle;
typedef struct _WndItem WndItem;
typedef struct _MenuItem MenuItem;
typedef struct _BitmapData* BitmapHandle;
typedef unsigned short UniChar; // UTF-16 UNICODE character

typedef int(*MouseCallback)(WndHandle Wnd, int X, int Y, int MouseBtn, int Down);
typedef int(*WndCallback)(WndHandle Wnd, int WndMsg);
typedef int(*ItemCallback)(WndItem* Item, int ItemMsg);
typedef int(*MenuCallback)(MenuItem* Item);

// Windows

typedef struct _WindowCreationArgs
{
	unsigned int width, height;
	const UniChar* sztitle;
	char visible; // If true, start window as visible

	// ----- Optional ----- //

	MouseCallback on_mouse;
	WndCallback on_wndmsg;
	ItemCallback on_itemmsg;
	MenuCallback on_menu;
} WindowCreationArgs;

// - Returns 0 on failure
// - Only use within a single designated window thread
WndHandle Window_Create(const WindowCreationArgs* Args);

// - Performs a blocking call, running all created windows so far
// - Must never be used outside the thread you have created windows in
extern int Window_RunAll();
extern int Window_Show(WndHandle Wnd, char bShow);

MenuItem* Window_Menu_Add(WndHandle Wnd, const char* Name);
MenuItem* Window_Menu_Add_Child(WndHandle Wnd, MenuItem* Parent, const char* Name);

// Drawing

extern void Window_Redraw(WndHandle Wnd);
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
extern void Bitmap_Draw_Line(BitmapHandle Bmp, int X1, int Y1, int X2, int Y2, unsigned int Color);
extern void Bitmap_Draw_Rect(BitmapHandle Bmp, int X, int Y, int Width, int Height, unsigned int Color);

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
	WndCallback_Skip = 0, // - Skip default behavior after return
	WndCallback_Close // - Returning this will close and destroy the window
};

// ----- Private functions and variables ----- //

// Windows

// OS-dependent data structs. Implement these how you like them.
typedef struct _OSDataImpl OSData; 
typedef struct _OSBitmapImpl OSBitmap;

typedef struct _MenuList MenuList;
typedef struct _WndItemList WndItemList;


struct _WndHandle
{
	MenuList* _menus;
	WndItemList* _items;
	WindowCreationArgs _args;
	WndHandle _next;
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

extern OSData* _Window_Create_Impl(const WindowCreationArgs* Args);

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

// - Returns 0 at the end
// - Set 'Item' to 0 for start of list
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