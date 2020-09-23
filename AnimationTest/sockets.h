#pragma once

typedef struct _OSSocketImpl* SocketHandle;
typedef int(*AcceptCallback)(SocketHandle Client);

// - Creates the desired type of socket
// - If 'Host' is null/zero, the created socket is a listener
extern SocketHandle Socket_Create(const char* Host, const char* Port);
extern void Socket_Destroy(SocketHandle Socket); // Also closes any existing connection(s)

extern char Socket_Listen(SocketHandle Socket);
extern char Socket_Connect(SocketHandle Socket);

// - Waits infinitely for a new client, or until the socket is closed
extern char Socket_Accept(SocketHandle Socket, AcceptCallback OnAccept);

// - Returns number of bytes sent
extern int Socket_Send(SocketHandle Socket, const char* Buffer, int Len);

// - Waits for incoming data and writes it into 'Buffer'
// - Returns number of bytes received into 'Buffer'
extern int Socket_Recv(SocketHandle Socket, char* Buffer, int MaxLen);

extern int Net_ntohl(long NetInt);
extern short Net_ntohs(short NetShort);
extern int Net_htonl(long HostInt);
extern short Net_htons(short HostShort);