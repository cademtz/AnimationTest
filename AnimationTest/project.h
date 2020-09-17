#pragma once
#include "window.h"

typedef struct _FrameList FrameList;
typedef unsigned int UID; // Unique ID

typedef struct _Project
{
	unsigned int width, height;
	FrameList* frames;
} Project;

extern Project my_project;

void InitProject(int Width, int Height);

typedef struct _FrameData
{
	BitmapHandle bmp;
} FrameData;

typedef struct _FrameItem
{
	struct _FrameItem* _next;
	UID uid;
	FrameData* data;
} FrameItem;

typedef struct _FrameList
{
	FrameItem* head, * tail;
	int count;
} FrameList;

FrameItem* FrameList_Next(FrameItem* Item);

void FrameList_Add(FrameData* Data);
void FrameList_Insert(FrameData* Data, int Index);

FrameItem* FrameList_Remove(FrameItem* Item);
FrameItem* FrameList_Remove_At(int Index);

FrameItem* FrameList_At(int Index);
FrameItem* FrameList_FindUID(UID UId);

void Frame_Destroy(FrameItem* Frame);