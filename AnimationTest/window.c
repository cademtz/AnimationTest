#include "window.h"
#include <stdlib.h>
#include <string.h>

WindowList _Window_list = { 0 };

WndHandle Window_Create(const WindowCreationArgs* Args)
{
	WndHandle wnd = malloc(sizeof(*wnd));
	memset(wnd, 0, sizeof(*wnd));
	wnd->_menus = malloc(sizeof(*wnd->_menus));
	memset(wnd->_menus, 0, sizeof(*wnd->_menus));
	wnd->_args = *Args;
	wnd->_data = _Window_Create_Impl(Args);

	if (_Window_list.count)
		_Window_list.tail->_next = wnd;
	else
		_Window_list.head = wnd;

	_Window_list.tail = wnd;
	_Window_list.count++;
	return wnd;
}

int Window_RunAll()
{
	unsigned int living = 0;

	do
	{
		for (WndHandle next = 0; next = _Window_list_Next(next);)
			if (_Window_Run(next))
				living++;
	} while (living > 0);
}

MenuItem* Window_Menu_Add(WndHandle Wnd, const char* Name, MenuCallback* Callback)
{
	MenuItem* item = malloc(sizeof(*item));
	memset(item, 0, sizeof(*item));
	item->szname = Name, item->callback = Callback;

	MenuList* list = Wnd->_menus;

	if (list->count)
		list->tail->next = item;
	else
		list->head = item;

	list->tail = item;
	list->count++;
	_Window_Menu_Add_Impl(Wnd, item);

	return item;
}

MenuItem* Window_Menu_Add_Child(WndHandle Wnd, MenuItem* Parent, const char* Name, MenuCallback* Callback)
{
	MenuItem* item = malloc(sizeof(*item));
	memset(item, 0, sizeof(*item));
	item->parent = Parent, item->szname = Name, item->callback = Callback;

	if (!Parent->children)
	{
		size_t size = sizeof(*Parent->children);
		Parent->children = malloc(size);
		memset(Parent->children, 0, size);
	}

	if (Parent->children->count)
		Parent->children->tail->next = item;
	else
		Parent->children->head = item;

	Parent->children->tail = item;
	Parent->children->count++;
	_Window_Menu_Add_Child_Impl(Wnd, Parent, item);

	return item;
}

WndHandle _Window_list_Next(WndHandle Wnd)
{
	if (!Wnd)
		return _Window_list.head;
	return Wnd->_next;
}

MenuItem* _Window_Menu_Next(WndHandle Wnd, MenuItem* Item)
{
	if (!Item)
		return Wnd->_menus->head;
	return Item->next;
}

MenuItem* _Window_Menu_Child_Next(MenuItem* Parent, MenuItem* Item)
{
	if (!Item)
	{
		if (Parent->children)
			return Parent->children->head;
		return 0;
	}
	return Item->next;
}