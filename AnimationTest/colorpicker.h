#pragma once
#include "window.h"

typedef struct _ColorPicker
{
	WndHandle wnd;
	IntColor color;
	void(*on_changed)(struct _ColorPicker* Picker);
	WndItem* _int_r, * _int_g, * _int_b;
	BitmapHandle _bmp;
} ColorPicker;

ColorPicker* ColorPicker_Create(WndHandle opt_Parent, UniChar* szName);
void ColorPicker_SetColor(ColorPicker* Picker, unsigned int Color);