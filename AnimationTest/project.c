#include "project.h"
#include "session.h"
#include <stdlib.h>
#include <string.h>

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
	data->bmp = Bitmap_Create(Width, Height);
	Bitmap_Draw_Rect(data->bmp, 0, 0, data->bmp->width, data->bmp->height, BkgColor);

	return data;
}

void FrameData_Destroy(FrameData* Data)
{
	for (BasicListItem* next = 0; next = BasicList_Next(Data->strokes, next);)
		UserStroke_Destroy((UserStroke*)next->data);
	BasicList_Destroy(Data->strokes);
	Bitmap_Destroy(Data->bmp);
	free(Data);
}

void FrameData_AddStroke(FrameData* Data, UserStroke* Stroke) {
	BasicList_Add(Data->strokes, Stroke);
}

UserStroke* FrameData_RemoveStroke(FrameData* Data, UserStroke* Stroke) {
	return (UserStroke*)BasicList_Remove_FirstOf(Data->strokes, Stroke);
}
