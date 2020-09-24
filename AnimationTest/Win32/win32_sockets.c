#include "../sockets.h"
//#include <WinSock2.h>
//#include <WS2tcpip.h>
#include <stdio.h>

#ifdef __clang__
#include "winsock2_stripped.h"
#else
#include <WinSock2.h>
#endif

#pragma comment(lib, "Ws2_32.lib")

typedef struct _OSSocketImpl
{
	SOCKET sock;
	PADDRINFOA info;
	char* Host, * Port;
} OSSocket;

char Socket_Connect(SocketHandle Socket);

SocketHandle Socket_Create(const char* Host, const char* Port)
{
	static char init = 1;
	if (init)
	{
		WSADATA data;
		int result = WSAStartup(MAKEWORD(2, 2), &data);
		if (result != 0)
		{
			printf("WSAStartup() failed: %d\n", result);
			return 0;
		}
		init = 0;
	}

	struct addrinfo hints = { 0 };
	hints.ai_family = Host ? AF_UNSPEC : AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	if (!Host) // Listener
		hints.ai_flags = AI_PASSIVE;

	PADDRINFOA info;
	int result = getaddrinfo(Host, Port, &hints, &info);
	if (result != 0)
	{
		printf("getaddrinfo(\"%s\", \"%s\", ...) failed: %d\n", Host, Port, result);
		return 0;
	}

	SOCKET sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
	if (sock == INVALID_SOCKET)
	{
		printf("socket() failed: %d\n", WSAGetLastError());
		DebugBreak();
		return 0;
	}

	OSSocket* ossock = (OSSocket*)malloc(sizeof(*ossock));
	ZeroMemory(ossock, sizeof(*ossock));

	ossock->sock = sock;
	ossock->info = info;

	size_t len_of_str_huehue = strlen(Port);
	ossock->Port = (char*)malloc(++len_of_str_huehue);
	strcpy_s(ossock->Port, len_of_str_huehue, Port);

	if (Host)
	{
		len_of_str_huehue = strlen(Host);
		ossock->Host = (char*)malloc(++len_of_str_huehue);
		strcpy_s(ossock->Host, len_of_str_huehue, Host);
	}

	return ossock;
}

void Socket_Destroy(SocketHandle Socket)
{
	shutdown(Socket->sock, SD_SEND);
	if (closesocket(Socket->sock) == SOCKET_ERROR)
		printf("closesocket() failed: %d\n", WSAGetLastError());

	if (Socket->info)
		freeaddrinfo(Socket->info);
	if (Socket->Host)
		free(Socket->Host);
	if (Socket->Port)
		free(Socket->Port);
	free(Socket);
}

char Socket_Listen(SocketHandle Socket)
{
	if (Socket->Host)
	{
		printf("A mishap in Socket_Listen! Did you mean to connect instead?\n");
		DebugBreak();
		return 0;
	}

	if (bind(Socket->sock, Socket->info->ai_addr, (int)Socket->info->ai_addrlen) == SOCKET_ERROR)
	{
		printf("bind() failed: %d\n", WSAGetLastError());
		return 0;
	}

	if (listen(Socket->sock, SOMAXCONN) == SOCKET_ERROR)
	{
		printf("listen() failed: %d\n", WSAGetLastError());
		return 0;
	}

	return 1;
}

char Socket_Connect(SocketHandle Socket)
{
	if (!Socket->Host)
	{
		printf("A mishap in Socket_Connect! Did you mean to listen instead?\n");
		DebugBreak();
		return 0;
	}

	int result = SOCKET_ERROR;
	for (struct addrinfo* next = Socket->info; next; next = next->ai_next)
	{
		result = connect(Socket->sock, next->ai_addr, (int)next->ai_addrlen);
		if (result != SOCKET_ERROR)
			break;
	}

	if (result == SOCKET_ERROR)
	{
		printf("All connect() attempts returned SOCKET_ERROR\n");
		return 0;
	}
	return 1;
}

char Socket_Accept(SocketHandle Socket, AcceptCallback OnAccept)
{
	SOCKET client = accept(Socket->sock, 0, 0);
	if (client == INVALID_SOCKET)
	{
		printf("accept() failed: %d\n", WSAGetLastError());
		return 0;
	}

	OSSocket* osclient = (OSSocket*)malloc(sizeof(*osclient));
	ZeroMemory(osclient, sizeof(*osclient));
	osclient->sock = client;
	OnAccept(osclient);

	return 1;
}

int Socket_Send(SocketHandle Socket, const char* Buffer, int Len)
{
	int result = send(Socket->sock, Buffer, Len, 0);
	if (result == SOCKET_ERROR)
	{
		printf("send() failed: %d\n", WSAGetLastError());
		return 0;
	}
	return result;
}

int Socket_Recv(SocketHandle Socket, char* Buffer, int MaxLen)
{
	int result = recv(Socket->sock, Buffer, MaxLen, 0);
	if (result == SOCKET_ERROR)
	{
		printf("recv() failed: %d\n", WSAGetLastError());
		return 0;
	}
	return result;
}

inline short mybswap_short(short Short) {
	return (Short << 8) | (Short >> 8);
}

inline long mybswap_int(long Int) {
	return ((Int << 24) & (0xFF << 24)) | ((Int << 8) & (0xFF << 16)) | ((Int >> 8) & (0xFF << 8)) | ((Int >> 24) & 0xFF);
}

int Net_ntohl(long NetInt) {
	return mybswap_int(NetInt);
}

short Net_ntohs(short NetShort) {
	return mybswap_short(NetShort);
}

int Net_htonl(long HostInt) {
	return mybswap_int(HostInt);
}

short Net_htons(short HostShort) {
	return mybswap_short(HostShort);
}
