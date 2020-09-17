#include "project.h"
#include <stdlib.h>

Project my_project = { 0 };

UID GenerateUID()
{
	static UID i = 0;
	return ++i; // ;)
}

void InitProject(int Width, int Height)
{
	if (my_project.frames)
		free(my_project.frames);

	my_project.frames = malloc(sizeof(FrameList));
}

FrameItem* FrameList_Next(FrameItem* Item)
{
	if (!Item)
		return my_project.frames->head;
	return Item->_next;
}

void FrameList_Add(FrameData* Data)
{
	FrameItem* frame = malloc(sizeof(*frame));
	frame->_next = 0, frame->uid = GenerateUID(), frame->data = Data;

	if (my_project.frames->count)
		my_project.frames->tail->_next = frame;
	else
		my_project.frames->head = frame;
	my_project.frames->tail = frame;
	my_project.frames->count++;
}

void FrameList_Insert(FrameData* Data, int Index)
{
	FrameItem* frame = malloc(sizeof(*frame));
	frame->_next = 0, frame->uid = GenerateUID(), frame->data = Data;

	if (!Index)
	{
		if (!my_project.frames->count)
			return FrameList_Add(frame);
		frame->_next = my_project.frames->head;
		my_project.frames->head = frame;
	}
	else
	{
		FrameItem* prev = my_project.frames->head;
		for (int i = 0; i < Index - 1; i++, prev = prev->_next);
		frame->_next = prev->_next;
		prev->_next = frame;

		if (!frame->_next)
			my_project.frames->tail = frame;
	}
	my_project.frames->count++;
}

FrameItem* FrameList_Remove(FrameItem* Item)
{
	if (my_project.frames->head == Item)
	{
		my_project.frames->head = Item->_next;
		if (my_project.frames->tail == Item)
			my_project.frames->tail = my_project.frames->head;
	}
	else
	{
		FrameItem* prev = my_project.frames->head;
		for (int i = 0; prev->_next != Item; i++, prev = prev->_next);

		FrameItem* found = prev->_next;
		prev->_next = found->_next;
		if (!prev->_next)
			my_project.frames->tail = prev;
	}
	my_project.frames->count--;
	return Item;
}

FrameItem* FrameList_Remove_At(int Index)
{
	FrameItem* found = 0;
	if (!Index)
	{
		found = my_project.frames->head;
		my_project.frames->head = found->_next;
		if (my_project.frames->tail == found)
			my_project.frames->tail = my_project.frames->head;
	}
	else
	{
		FrameItem* prev = my_project.frames->head;
		for (int i = 0; i < Index - 1; i++, prev = prev->_next);

		found = prev->_next;
		prev->_next = found->_next;
		if (!prev->_next)
			my_project.frames->tail = prev;
	}

	my_project.frames->count--;
	return found;
}

FrameItem* FrameList_At(int Index)
{
	FrameItem* item = my_project.frames->head;
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

void Frame_Destroy(FrameItem* Frame)
{
	free(Frame->data);
	free(Frame);
}
