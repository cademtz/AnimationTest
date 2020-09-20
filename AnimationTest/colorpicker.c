#include "colorpicker.h"
#include <stdlib.h>
#include <string.h>

int ColorPicker_OnItemMsg(WndItem* Item, int ItemMsg, ItemMsgData* Data)
{
	ColorPicker* picker = (ColorPicker*)Item->wnd->userdata;
	switch (ItemMsg)
	{
	case ItemMsg_ValueChanged:
		if (Item == picker->_int_r ||
			Item == picker->_int_g ||
			Item == picker->_int_b)
		{
			int r = Window_Item_GetValuei(picker->_int_r);
			int g = Window_Item_GetValuei(picker->_int_g);
			int b = Window_Item_GetValuei(picker->_int_b);
			int* poo = &b;
			if (Item == picker->_int_r)
				poo = &r;
			else if (Item == picker->_int_g)
				poo = &g;
			*poo = Data->newval.i;

			ColorPicker_SetColor(picker, COLOR_INT(r, g, b));
		}
	}
	return WndCallback_None;
}

int ColorPicker_OnWndMsg(WndHandle Wnd, int WndMsg)
{
	ColorPicker* picker = (ColorPicker*)Wnd->userdata;
	switch (WndMsg)
	{
	case WndMsg_Draw:
		Window_Draw_Bitmap(Wnd, picker->_bmp, 5, 5);
		break;
	}
	return WndCallback_None;
}

ColorPicker* ColorPicker_Create(WndHandle opt_Parent, UniChar* szName)
{
	ColorPicker* picker = malloc(sizeof(*picker));
	memset(picker, 0, sizeof(*picker));

	WindowCreationArgs args = { 0 };
	args.sztitle = L"ColorPicker";
	args.width = 100, args.height = 110;
	args.visible = 1;
	args.userdata = picker;
	args.on_wndmsg = &ColorPicker_OnWndMsg;
	args.on_itemmsg = &ColorPicker_OnItemMsg;

	WndHandle wnd;
	if (opt_Parent)
		wnd = Window_Create_Child(opt_Parent, &args);
	else
		wnd = Window_Create(&args);

	picker->wnd = wnd;
	picker->_bmp = Bitmap_Create(50, 50);

	Window_Item_Add(wnd, ItemType_Label, 60, 5, 50, 70, szName);

	int x = 40, nexty = 50 + 5 * 2, w = 70 - 5 * 2, h = 23;;
	picker->_int_r = Window_Item_Add(wnd, ItemType_IntBox, x, nexty, w, h, L"");
	Window_Item_Add(wnd, ItemType_Label, 5, nexty, 35, 23, L"Red");
	nexty += picker->_int_r->height + 5;
	picker->_int_g = Window_Item_Add(wnd, ItemType_IntBox, x, nexty, w, h, L"");
	Window_Item_Add(wnd, ItemType_Label, 5, nexty, 35, 23, L"Green");
	nexty += picker->_int_g->height + 5;
	picker->_int_b = Window_Item_Add(wnd, ItemType_IntBox, x, nexty, w, h, L"");
	Window_Item_Add(wnd, ItemType_Label, 5, nexty, 35, 23, L"Blue");

	Window_IntBox_SetRange(picker->_int_r, 0, 255);
	Window_IntBox_SetRange(picker->_int_g, 0, 255);
	Window_IntBox_SetRange(picker->_int_b, 0, 255);

	ColorPicker_SetColor(picker, COLOR_INT(255, 255, 255));

	return picker;
}

void ColorPicker_SetColor(ColorPicker* Picker, unsigned int Color)
{
	Picker->color = Color;
	Window_Item_SetValuei(Picker->_int_r, Color & 0xFF);
	Window_Item_SetValuei(Picker->_int_g, (Color >> 8) & 0xFF);
	Window_Item_SetValuei(Picker->_int_b, (Color >> 16) & 0xFF);

	if (Picker->on_changed)
		Picker->on_changed(Picker);

	Bitmap_Draw_Rect(Picker->_bmp, 0, 0, Picker->_bmp->width, Picker->_bmp->height, Picker->color);
	Window_Redraw(Picker->wnd, 0);
}
