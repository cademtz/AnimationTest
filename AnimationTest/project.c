#include "project.h"
#include <stdlib.h>
#include <string.h>

Project my_project = { 0 };

UID GenerateUID()
{
	static UID i = 0;
	return ++i; // ;)
}

void Project_Init(int Width, int Height, unsigned int BkgCol)
{
	my_project.width = Width, my_project.height = Height, my_project.bkgcol = BkgCol;
	if (!my_project._frames)
	{
		my_project._frames = malloc(sizeof(FrameList));
		memset(my_project._frames, 0, sizeof(FrameList));
	}

	FrameList_Reset();
}

void Project_ChangeFrame(int Index) {
	my_project.frame_active = FrameList_At(Index);
}

void Project_InsertFrame(int Index)
{
	FrameData* data = FrameData_Create();
	FrameList_Insert(data, Index);
}

void FrameList_Reset()
{
	FrameData* last = 0;
	for (int i = my_project._frames->count - 1; i >= 0; i--)
	{
		FrameItem* next = FrameList_At(i);
		if (!last || last != next->data)
		{
			last = next->data;
			FrameData_Destroy(next->data);
		}

		Frame_Destroy(next);
	}

	memset(my_project._frames, 0, sizeof(FrameList));
	FrameList_Add(FrameData_Create());
	my_project.frame_active = my_project._frames->head;
}

FrameItem* FrameList_Next(FrameItem* Item)
{
	if (!Item)
		return my_project._frames->head;
	return Item->_next;
}

FrameItem* FrameList_At(int Index)
{
	FrameItem* item = my_project._frames->head;
	for (int i = 0; i < Index; i++, item = item->_next);
	return item;
}

FrameItem* FrameList_FindUID(UID UId)
{
	for (FrameItem* item = 0; item = FrameList_Next(item);)
		if (item->uid == UId)
			return item;
	return 0;
}

void FrameList_Add(FrameData* Data)
{
	FrameItem* frame = malloc(sizeof(*frame));
	frame->_next = 0, frame->uid = GenerateUID(), frame->data = Data;

	if (my_project._frames->count)
		my_project._frames->tail->_next = frame;
	else
		my_project._frames->head = frame;
	my_project._frames->tail = frame;
	my_project._frames->count++;
}

void FrameList_Insert(FrameData* Data, int Index)
{
	FrameItem* frame = malloc(sizeof(*frame));
	frame->_next = 0, frame->uid = GenerateUID(), frame->data = Data;

	if (!Index)
	{
		if (!my_project._frames->count)
			return FrameList_Add(frame);
		frame->_next = my_project._frames->head;
		my_project._frames->head = frame;
	}
	else
	{
		FrameItem* prev = my_project._frames->head;
		for (int i = 0; i < Index - 1; i++, prev = prev->_next);
		frame->_next = prev->_next;
		prev->_next = frame;

		if (!frame->_next)
			my_project._frames->tail = frame;
	}
	my_project._frames->count++;
}

FrameItem* FrameList_Remove(FrameItem* Item)
{
	if (my_project._frames->head == Item)
	{
		my_project._frames->head = Item->_next;
		if (my_project._frames->tail == Item)
			my_project._frames->tail = my_project._frames->head;
	}
	else
	{
		FrameItem* prev = my_project._frames->head;
		for (int i = 0; prev->_next != Item; i++, prev = prev->_next);

		FrameItem* found = prev->_next;
		prev->_next = found->_next;
		if (!prev->_next)
			my_project._frames->tail = prev;
	}
	my_project._frames->count--;
	return Item;
}

FrameItem* FrameList_Remove_At(int Index)
{
	FrameItem* found = 0;
	if (!Index)
	{
		found = my_project._frames->head;
		my_project._frames->head = found->_next;
		if (my_project._frames->tail == found)
			my_project._frames->tail = my_project._frames->head;
	}
	else
	{
		FrameItem* prev = my_project._frames->head;
		for (int i = 0; i < Index - 1; i++, prev = prev->_next);

		found = prev->_next;
		prev->_next = found->_next;
		if (!prev->_next)
			my_project._frames->tail = prev;
	}

	my_project._frames->count--;
	return found;
}

FrameData* FrameData_Create()
{
	FrameData* data = malloc(sizeof(*data));
	memset(data, 0, sizeof(*data));

	data->bmp = Bitmap_Create(my_project.width, my_project.height);
	Bitmap_Draw_Rect(data->bmp, 0, 0, data->bmp->width, data->bmp->height, my_project.bkgcol);

	return data;
}

void FrameData_Destroy(FrameData* Data)
{
	Bitmap_Destroy(Data->bmp);
	free(Data);
}

void Frame_Destroy(FrameItem* Frame) {
	free(Frame);
}
