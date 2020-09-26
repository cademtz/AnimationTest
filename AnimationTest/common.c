#include "common.h"

UID GenerateUID()
{
	static UID i = 0;
	return ++i; // ;)
}

char* UniStr_ToUTF8(const UniChar* szText)
{
	int len = wcslen(szText) + 1;
	char* utf8 = (char*)malloc(len);

	size_t yawn;
	wcstombs_s(&yawn, utf8, len, szText, len * sizeof(szText[0]));
	return utf8;
}

UniChar* UTF8_ToUni(const char* szText)
{
	int len = strlen(szText) + 1;
	UniChar* uni = (UniChar*)malloc(len * sizeof(uni[0]));

	size_t yawn;
	wcstombs_s(&yawn, uni, len * sizeof(uni[0]), szText, len);
	return uni;
}

BasicList* BasicList_Create()
{
	BasicList* list = (BasicList*)malloc(sizeof(*list));
	memset(list, 0, sizeof(*list));
	return list;
}

void BasicList_Destroy(BasicList* List)
{
	for (BasicListItem* next = List->head; next;)
	{
		BasicListItem* prev = next;
		next = next->_next;
		free(prev);
	}
	free(List);
}

void BasicList_Add(BasicList* List, void* Data)
{
	BasicListItem* item = (BasicListItem*)malloc(sizeof(*item));
	item->_next = 0, item->data = Data;

	if (!List->count)
		List->head = item;
	else
		List->tail->_next = item;
	List->tail = item;
	List->count++;
}

void BasicList_Insert(BasicList* List, void* Data, int Index)
{
	if (!Index && !List->count)
		return BasicList_Add(List, Data);

	BasicListItem* item = (BasicListItem*)malloc(sizeof(*item));
	item->_next = 0, item->data = Data;

	if (!Index)
	{
		item->_next = List->head;
		List->head = item;
	}
	else
	{
		BasicListItem* prev = List->head;
		for (int i = 0; i < Index - 1; i++, prev = prev->_next);
		item->_next = prev->_next;
		prev->_next = item;

		if (!item->_next)
			List->tail = item;
	}
	List->count++;
}

void* BasicList_At(BasicList* List, int Index)
{
	BasicListItem* next = 0;
	for (int i = 0; i <= Index; next = BasicList_Next(List, 0));
	return next;
}

BasicListItem* BasicList_Next(BasicList* List, BasicListItem* Item)
{
	if (!Item)
		return List->head;
	return Item->_next;
}

void* BasicList_Remove_At(BasicList* List, int Index)
{
	BasicListItem* prev = 0, * item = List->head;
	for (int i = 0; i < Index; prev = item, item = item->_next, i++);
	if (!prev)
		List->head = item->_next;
	else
		prev->_next = item->_next;

	if (!item->_next)
		List->tail = prev;
	List->count--;

	void* data = item->data;
	free(item);
	return data;
}

void* BasicList_Remove_FirstOf(BasicList* List, const void* Data)
{
	if (!List->head)
		return 0;

	for (BasicListItem* prev = 0, * item = List->head; item; prev = item, item = item->_next)
	{
		if (item->data == Data)
		{
			if (!prev)
				List->head = item->_next;
			else
				prev->_next = item->_next;

			if (!item->_next)
				List->tail = prev;
			List->count--;

			void* data = item->data;
			free(item);
			return data;
		}
	}
	return 0;
}

void* BasicList_Remove(BasicList* List, BasicListItem* Item)
{
	for (BasicListItem* prev = 0, * item = List->head;; prev = item, item = prev->_next)
	{
		if (item == Item)
		{
			if (!prev)
				List->head = item->_next;
			else
				prev->_next = item->_next;

			if (!item->_next)
				List->tail = prev;
			List->count--;

			void* data = item->data;
			free(Item);
			return data;
		}
	}
	return 0;
}

void BasicList_Clear(BasicList* List)
{
	for (BasicListItem* next = List->head; next;)
	{
		BasicListItem* prev = next;
		next = next->_next;
		free(prev);
	}
	List->count = 0;
	List->head = List->tail = 0;
}

int BasicList_IndexOfFirst(const BasicList* List, const void* Data)
{
	int i = 0;
	for (BasicListItem* next = 0; next = !next ? List->head : next->_next; i++)
		if (next->data == Data)
			return i;
	return -1;
}
