#include "window.h"
#include <stdlib.h>
#include <string.h>

WindowList _Window_list = { 0 };

WndHandle Window_Create(const WindowCreationArgs* Args)
{
	WndHandle wnd = _Window_Create(0, Args);
	if (wnd->_args.on_wndmsg)
		wnd->_args.on_wndmsg(wnd, WndMsg_Created);
	return wnd;
}

WndHandle Window_Create_Child(WndHandle Parent, const WindowCreationArgs* Args)
{
	if (!Parent->_children)
	{
		Parent->_children = malloc(sizeof(WindowList));
		memset(Parent->_children, 0, sizeof(WindowList));
	}

	WndHandle wnd = _Window_Create(Parent, Args);
	wnd->_parent = Parent;
	if (wnd->_args.on_wndmsg)
		wnd->_args.on_wndmsg(wnd, WndMsg_Created);
	return wnd;
}

char Window_GetPos(WndHandle Wnd, int* X, int* Y)
{
	int xy[2];
	if (_Window_GetSetPos_Impl(Wnd, xy, 0))
	{
		*X = xy[0], * Y = xy[1];
		return 1;
	}
	return 0;
}

char Window_SetPos(WndHandle Wnd, int X, int Y)
{
	int xy[2] = { X, Y };
	return _Window_GetSetPos_Impl(Wnd, 0, xy);
}

char Window_GetSize(WndHandle Wnd, int* Width, int* Height)
{
	int wh[2];
	if (_Window_GetSetSize_Impl(Wnd, wh, 0))
	{
		*Width = wh[0], * Height = wh[1];
		return 1;
	}
	return 0;
}

char Window_SetSize(WndHandle Wnd, int Width, int Height)
{
	int wh[2] = { Width, Height };
	return _Window_GetSetSize_Impl(Wnd, 0, wh);
}

MenuItem* Window_Menu_Add(WndHandle Wnd, const char* Name)
{
	MenuItem* item = malloc(sizeof(*item));
	memset(item, 0, sizeof(*item));
	item->sztext = Name, item->wnd = Wnd;

	MenuList* list = Wnd->_menus;

	if (list->count)
		list->tail->_next = item;
	else
		list->head = item;

	list->tail = item;
	list->count++;
	_Window_Menu_Add_Impl(Wnd, item);

	return item;
}

MenuItem* Window_Menu_Add_Child(WndHandle Wnd, MenuItem* Parent, const char* Name)
{
	MenuItem* item = malloc(sizeof(*item));
	memset(item, 0, sizeof(*item));
	item->wnd = Wnd, item->_parent = Parent, item->sztext = Name;

	if (!Parent->_children)
	{
		size_t size = sizeof(*Parent->_children);
		Parent->_children = malloc(size);
		memset(Parent->_children, 0, size);
	}

	if (Parent->_children->count)
		Parent->_children->tail->_next = item;
	else
		Parent->_children->head = item;

	Parent->_children->tail = item;
	Parent->_children->count++;
	_Window_Menu_Add_Child_Impl(Wnd, Parent, item);

	return item;
}

WndHandle _Window_list_Next(WndHandle Wnd)
{
	if (!Wnd)
		return _Window_list.head;
	return Wnd->_next;
}

WndHandle _Window_Child_Next(WndHandle Wnd, WndHandle Next)
{
	if (!Next && Wnd->_children)
		return Wnd->_children->head;
	return Next->_next;
}

WndHandle _Window_Create(WndHandle Parent, const WindowCreationArgs* Args)
{
	WndHandle wnd = malloc(sizeof(*wnd));
	memset(wnd, 0, sizeof(*wnd));

	wnd->_menus = malloc(sizeof(*wnd->_menus));
	memset(wnd->_menus, 0, sizeof(*wnd->_menus));

	wnd->_items = malloc(sizeof(*wnd->_items));
	memset(wnd->_items, 0, sizeof(*wnd->_items));

	wnd->_args = *Args;
	wnd->_parent = Parent;
	wnd->userdata = wnd->_args.userdata;
	_Window_Create_Impl(wnd);

	WindowList* list = Parent ? Parent->_children : &_Window_list;
	if (list->count)
		list->tail->_next = wnd;
	else
		list->head = wnd;

	list->tail = wnd;
	list->count++;

	if (wnd->_args.on_wndmsg)
		wnd->_args.on_wndmsg(wnd, WndMsg_Created);

	return wnd;
}

MenuItem* _Window_Menu_Next(WndHandle Wnd, MenuItem* Item)
{
	if (!Item)
		return Wnd->_menus->head;
	return Item->_next;
}

MenuItem* _Window_Menu_Child_Next(MenuItem* Parent, MenuItem* Item)
{
	if (!Item)
	{
		if (Parent->_children)
			return Parent->_children->head;
		return 0;
	}
	return Item->_next;
}

WndItem* Window_Items_Next(WndHandle Wnd, WndItem* Item)
{
	if (!Item)
		return Wnd->_items->head;
	return Item->_next;
}

WndItem* Window_Item_Add(WndHandle Wnd, int Type, int X, int Y, int W, int H, UniChar* sztext)
{
	WndItem* item = malloc(sizeof(*item));
	memset(item, 0, sizeof(*item));

	item->wnd = Wnd, item->type = Type, item->sztext = sztext;
	item->x = X, item->y = Y, item->width = W, item->height = H;

	if (Wnd->_items->count)
		Wnd->_items->tail->_next = item;
	else
		Wnd->_items->head = item;

	Wnd->_items->tail = item;
	Wnd->_items->count++;
	_Window_Item_Add_Impl(item);

	return item;
}

void Window_Item_SetText(WndItem* Item, UniChar* szText)
{
	Item->sztext = szText;
	_Window_Item_UpdateText_Impl(Item);
}

void Window_IntBox_SetRange(WndItem* IntBox, int Min, int Max)
{
	int minmax[2] = { Min, Max };
	_Window_IntBox_GetSetRange_Impl(IntBox, 0, minmax);
}

void Window_IntBox_GetRange(WndItem* IntBox, int* outMin, int* outMax)
{
	int minmax[2] = { 0 };
	if (_Window_IntBox_GetSetRange_Impl(IntBox, minmax, 0))
		*outMin = minmax[0], * outMax = minmax[1];
}

BitmapHandle Bitmap_Create(unsigned int Width, unsigned int Height)
{
	BitmapData* data = malloc(sizeof(*data));
	memset(data, 0, sizeof(*data));
	data->width = Width, data->height = Height;

	_Bitmap_Create_Impl(data);

	return data;
}

void Bitmap_Destroy(BitmapHandle Bmp)
{
	_Bitmap_Destroy_Impl(Bmp->_data);
	free(Bmp);
}
