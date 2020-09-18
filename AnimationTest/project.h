#pragma once
#include "window.h"

typedef struct _FrameData FrameData;
typedef struct _FrameList FrameList;
typedef struct _FrameItem FrameItem;
typedef unsigned int UID; // Unique ID

typedef struct _Project
{
	unsigned int width, height, bkgcol;
	FrameList* _frames;
	FrameItem* frame_active;
} Project;

extern Project my_project;

void Project_Init(int Width, int Height, unsigned int BkgCol);
void Project_ChangeFrame(int Index);
void Project_InsertFrame(int Index);

void FrameList_Reset();

FrameItem* FrameList_Next(FrameItem* Item);
FrameItem* FrameList_At(int Index);
FrameItem* FrameList_FindUID(UID UId);

void FrameList_Add(FrameData* Data);
void FrameList_Insert(FrameData* Data, int Index);

FrameItem* FrameList_Remove(FrameItem* Item);
FrameItem* FrameList_Remove_At(int Index);

FrameData* FrameData_Create();
void FrameData_Destroy(FrameData* Data);
void Frame_Destroy(FrameItem* Frame);

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
