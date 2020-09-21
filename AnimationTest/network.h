#pragma once

typedef struct _OSSocketImpl OSSocket;

typedef struct _NetMsg
{
	unsigned int length;
	const char* data;
} NetMsg;