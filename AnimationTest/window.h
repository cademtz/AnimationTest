#pragma once

enum EMouseBtn
{
	MouseBtn_None = 0,
	MouseBtn_Left,
	MouseBtn_Right,
	MoustBtn_Middle
};

typedef struct _WndHandle* WndHandle;
typedef int(*MouseCallback)(WndHandle Wnd, int X, int Y, int MouseBtn, int Down);
typedef int(*MenuCallback)();
typedef struct _MenuList MenuList;
typedef struct _MenuItem MenuItem;

typedef struct _WindowCreationArgs
{
	unsigned int width, height;
	const unsigned short* sztitle;
	MouseCallback onmouse; // (Optional) Mouse drag & click callback
	char visible; // If true, start window as visible
} WindowCreationArgs;

// - Returns 0 on failure
// - Only use within a single designated window thread
WndHandle Window_Create(const WindowCreationArgs* Args);

// - Performs a blocking call, running all created windows so far
// - Must never be used outside the thread you have created windows in
int Window_RunAll();

MenuItem* Window_Menu_Add(WndHandle Wnd, const char* Name, MenuCallback* Callback);
MenuItem* Window_Menu_Add_Child(WndHandle Wnd, MenuItem* Parent, const char* Name, MenuCallback* Callback);

extern void Window_Draw_Dot(WndHandle Wnd, int X, int Y);

// ----- Private functions and variables ----- //

// Windows

typedef struct _OSDataImpl OSData; // OS-dependent data struct, must be implemented

struct _WndHandle
{
	MenuList* _menus;
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
extern int _Window_Run(WndHandle Wnd);

// Menu list and items

typedef struct _MenuItem
{
	const unsigned short* szname;
	MenuCallback callback;
	MenuItem* next;
	MenuItem* parent;
	struct _MenuList* children;
	void* id; // OS-dependent identifier
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