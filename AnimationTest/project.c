#include "project.h"
#include "session.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

FrameList* FrameList_Create()
{
	FrameList* list = (FrameList*)malloc(sizeof(*list));
	memset(list, 0, sizeof(*list));
	return list;
}

void FrameList_Destroy(FrameList* List)
{
	for (FrameItem* next = List->head; next;)
	{
		FrameItem* prev = next;
		next = next->_next;
		if (!next || prev->data != next->data)
		{
			FrameData_Destroy(prev->data);
			FrameItem_Destroy(prev);
		}
	}

	free(List);
}

FrameItem* FrameList_Next(FrameList* List, FrameItem* Item)
{
	if (!Item)
		return List->head;
	return Item->_next;
}

FrameItem* FrameList_At(FrameList* List, int Index)
{
	FrameItem* item = List->head;
	for (int i = 0; i < Index; i++, item = item->_next);
	return item;
}

void FrameList_Add(FrameList* List, FrameData* Data)
{
	FrameItem* frame = malloc(sizeof(*frame));
	frame->_next = 0, frame->data = Data;

	if (List->count)
		List->tail->_next = frame;
	else
		List->head = frame;
	List->tail = frame;
	List->count++;
}

void FrameList_Insert(FrameList* List, FrameData* Data, int Index)
{
	if (!Index && !List->count)
		return FrameList_Add(List, Data);

	FrameItem* frame = malloc(sizeof(*frame));
	frame->_next = 0, frame->data = Data;

	if (!Index)
	{
		frame->_next = List->head;
		List->head = frame;
	}
	else
	{
		FrameItem* prev = List->head;
		for (int i = 0; i < Index - 1; i++, prev = prev->_next);
		frame->_next = prev->_next;
		prev->_next = frame;

		if (!frame->_next)
			List->tail = frame;
	}
	List->count++;
}

FrameItem* FrameList_Remove(FrameList* List, FrameItem* Item)
{
	if (List->head == Item)
	{
		List->head = Item->_next;
		if (List->tail == Item)
			List->tail = List->head;
	}
	else
	{
		FrameItem* prev = List->head;
		for (int i = 0; prev->_next != Item; i++, prev = prev->_next);

		FrameItem* found = prev->_next;
		prev->_next = found->_next;
		if (!prev->_next)
			List->tail = prev;
	}
	List->count--;
	return Item;
}

FrameData* FrameList_Remove_At(FrameList* List, int Index)
{
	FrameItem* found = 0;
	if (!Index)
	{
		found = List->head;
		List->head = found->_next;
		if (List->tail == found)
			List->tail = List->head;
	}
	else
	{
		FrameItem* prev = List->head;
		for (int i = 0; i < Index - 1; i++, prev = prev->_next);

		found = prev->_next;
		prev->_next = found->_next;
		if (!prev->_next)
			List->tail = prev;
	}

	FrameData* data = found->data;
	FrameItem_Destroy(found);

	List->count--;
	return data;
}

char FrameList_IsDataUsed(FrameList* List, FrameData* Data)
{
	for (FrameItem* next = 0; next = FrameList_Next(List, next);)
		if (next->data == Data)
			return 1;
	return 0;
}

void FrameItem_Destroy(FrameItem* Frame) {
	free(Frame);
}

FrameData* FrameData_Create(unsigned int Width, unsigned int Height, IntColor BkgColor)
{
	FrameData* data = malloc(sizeof(*data));
	memset(data, 0, sizeof(*data));

	data->strokes = BasicList_Create();
	data->strokes_temp = BasicList_Create();

	data->bmp = Bitmap_Create(Width, Height);
	data->saved = Bitmap_Create(Width, Height);
	data->active = Bitmap_Create(Width, Height);
	Bitmap_Draw_Rect(data->bmp, 0, 0, data->bmp->width, data->bmp->height, BkgColor);
	Bitmap_Draw_Bitmap(data->saved, 0, 0, data->bmp);
	Bitmap_Draw_Bitmap(data->active, 0, 0, data->bmp);

	return data;
}

void FrameData_Destroy(FrameData* Data)
{
	for (BasicListItem* next = 0; next = BasicList_Next(Data->strokes_temp, next);)
		UserStroke_Destroy((UserStroke*)next->data);
	for (BasicListItem* next = 0; next = BasicList_Next(Data->strokes, next);)
		UserStroke_Destroy((UserStroke*)next->data);

	BasicList_Destroy(Data->strokes_temp);
	BasicList_Destroy(Data->strokes);
	Bitmap_Destroy(Data->active);
	Bitmap_Destroy(Data->saved);
	Bitmap_Destroy(Data->bmp);
	free(Data);
}

void FrameData_AddStroke(FrameData* Data, const UserStroke* Stroke)
{
	BasicList_Remove_FirstOf(Data->strokes_temp, Stroke);

	BasicList_Add(Data->strokes, Stroke);
	_FrameData_DrawStroke(Data->saved, Stroke, 0);
	_FrameData_RedrawActive(Data);
	Data->last_point = Stroke->points->count - 1;
}

UserStroke* FrameData_RemoveStroke(FrameData* Data, const UserStroke* Stroke)
{
	assert(!BasicList_Remove_FirstOf(Data->strokes_temp, Stroke));

	UserStroke* removed = (UserStroke*)BasicList_Remove_FirstOf(Data->strokes, Stroke);
	_FrameData_RedrawSave(Data);
	_FrameData_RedrawActive(Data);
	return removed;
}

/*void FrameData_UpdateStroke(FrameData* Data, const UserStroke* Stroke)
{
	int idx = BasicList_IndexOfFirst(Data->strokes, Stroke);
	if (idx == -1)
		return;

	if (idx == Data->strokes->count - 1) // Continue last drawn point in last stroke
	{
		_FrameData_DrawStroke(Data->saved, Stroke, Data->last_point);
		Data->last_point = Stroke->points->count - 1;
	}
	else
		_FrameData_RedrawSave(Data);
}*/

void FrameData_ApplyStroke(FrameData* Data, const UserStroke* Stroke)
{
	int idx = BasicList_IndexOfFirst(Data->strokes, Stroke);
	if (idx == -1)
		return;

	BasicList_Remove_At(Data->strokes, idx);
	_FrameData_DrawStroke(Data->bmp, Stroke, 0);

	if (idx != 0) // A stroke has been re-ordered
		_FrameData_RedrawSave(Data);
	_FrameData_RedrawActive(Data); // Order of strokes are unknown
}

void FrameData_PointsAdded(FrameData* Data, const UserStroke* Stroke, int Count)
{
	int idx = BasicList_IndexOfFirst(Data->strokes_temp, Stroke);
	if (idx == -1)
		BasicList_Add(Data->strokes_temp, Stroke);

	_FrameData_DrawStroke(Data->active, Stroke, Stroke->points->count - Count - 1);
}

void _FrameData_DrawStroke(BitmapHandle Bmp, const UserStroke* Stroke, int StartPoint)
{
	const DrawTool* tool = &Stroke->tool;

	Vec2* vec = (Vec2*)Stroke->points->head->data;
	int x1 = vec->x, y1 = vec->y, iPoint = 0;

	if (Stroke->points->count == 1)
		Bitmap_Draw_Line(Bmp, x1, y1, x1, y1, tool->Width, tool->Color);
	else
	{
		iPoint++;
		for (BasicListItem* next = Stroke->points->head; next = BasicList_Next(Stroke->points, next); iPoint++)
		{
			if (iPoint < StartPoint)
				continue;
			
			vec = (Vec2*)next->data;
			if (iPoint == StartPoint)
			{
				x1 = vec->x, y1 = vec->y;
				if (BasicList_Next(Stroke->points, next))
					continue; // Not a single point, so we want 'iPoint' to act as the previous point
			}

			int x2 = vec->x, y2 = vec->y;
			Bitmap_Draw_Line(Bmp, x1, y1, x2, y2, tool->Width, tool->Color);
			x1 = x2, y1 = y2;
		}
	}
}

void _FrameData_RedrawSave(FrameData* Data)
{
	Bitmap_Draw_Bitmap(Data->saved, 0, 0, Data->bmp);
	for (BasicListItem* next = 0; next = BasicList_Next(Data->strokes, next);)
		_FrameData_DrawStroke(Data->saved, (UserStroke*)next->data, 0);
}

void _FrameData_RedrawActive(FrameData* Data)
{
	Bitmap_Draw_Bitmap(Data->active, 0, 0, Data->saved);
	for (BasicListItem* next = 0; next = BasicList_Next(Data->strokes_temp, next);)
		_FrameData_DrawStroke(Data->active, (UserStroke*)next->data, 0);
}
