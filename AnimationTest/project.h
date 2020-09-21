#pragma once
#include "window.h"

typedef struct _FrameData FrameData;
typedef struct _FrameList FrameList;
typedef struct _FrameItem FrameItem;
typedef struct _UserStroke UserStroke;

FrameList* FrameList_Create();
void FrameList_Destroy(FrameList* List);

FrameItem* FrameList_Next(FrameList* List, FrameItem* Item);
FrameItem* FrameList_At(FrameList* List, int Index);

void FrameList_Add(FrameList* List, FrameData* Data);
void FrameList_Insert(FrameList* List, FrameData* Data, int Index);

FrameItem* FrameList_Remove(FrameList* List, FrameItem* Item);
FrameData* FrameList_Remove_At(FrameList* List, int Index);
char FrameList_IsDataUsed(FrameList* List, FrameData* Data);

void FrameItem_Destroy(FrameItem* Frame);

FrameData* FrameData_Create(unsigned int Width, unsigned int Height, IntColor BkgColor);
void FrameData_Destroy(FrameData* Data);

void FrameData_AddStroke(FrameData* Data, UserStroke* Stroke);
UserStroke* FrameData_RemoveStroke(FrameData* Data, UserStroke* Stroke);

typedef struct _FrameData
{
	BitmapHandle bmp;
	BasicList* strokes;
} FrameData;

typedef struct _FrameItem
{
	struct _FrameItem* _next;
	FrameData* data;
} FrameItem;

typedef struct _FrameList
{
	FrameItem* head, * tail;
	int count;
} FrameList;
