#pragma once
#include <stdlib.h>
#include <string.h>

#define COLOR_INT(r, g, b) (r | (g << 8) | (b << 16))
typedef unsigned int IntColor; // 32-bit color, RGBA (in reverse order)
typedef unsigned short UniChar; // UTF-16 UNICODE character

typedef unsigned int UID; // Unique ID
UID GenerateUID();

typedef struct _Vec2 {
	int x, y;
} Vec2;

typedef struct _Line2D {
	Vec2 a, b;
} Line2D;

typedef struct _DrawTool
{
	int Width;
	IntColor Color; // RGB
} DrawTool;

// Basic list

typedef struct _BasicListItem BasicListItem;
typedef struct _BasicList
{
	BasicListItem* head, * tail;
	int count;
} BasicList;

BasicList* BasicList_Create();
void BasicList_Destroy(BasicList* List);

void BasicList_Add(BasicList* List, void* Data);
void BasicList_Insert(BasicList* List, void* Data, int Index);
void* BasicList_At(BasicList* List, int Index);
BasicListItem* BasicList_Next(BasicList* List, BasicListItem* Item);

void* BasicList_Remove_At(BasicList* List, int Index);
void* BasicList_Remove_FirstOf(BasicList* List, const void* Data);
void* BasicList_Remove(BasicList* List, BasicListItem* Item);
void BasicList_Clear(BasicList* List);

inline void* BasicList_Pop(BasicList* List) {
	return BasicList_Remove_At(List, List->count - 1);
}

// - Returns an index for the first occurence of 'Data' pointer
// - If not found, the return is -1
int BasicList_IndexOfFirst(const BasicList* List, const void* Data);

typedef struct _BasicListItem
{
	BasicListItem* _next;
	void* data;
} BasicListItem;