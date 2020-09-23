#include "server.h"
#include "sockets.h"
#include "threading.h"
#include "session.h"
#include <stdio.h>

SocketHandle sock_server = 0;
SessionCallback client_onseshmsg = 0;
BasicList* list_clients;
MutexHandle mtx_clients;

typedef struct _NetClient
{
	SocketHandle sock;
	NetUser* user;
} NetClient;

int _Server_ClientThread(void* UserDat);
int _Server_OnAccept(SocketHandle Client);
void _Server_OnSessionMsg(int SessionMsg, UID Object);
void _Server_SendToAll(const NetMsg* Msg); // Will lock mtx_clients
// Functions for server-client hybrid (Hosting the server while simultaneously being a user)

void ServerClJoin(const UniChar* szName);
void ServerClSetFPS(int FPS);
void ServerClSetFrame(int Index);
void ServerClInsertFrame(int Index);
void ServerClRemoveFrame(int Index);
void ServerClChat(const UniChar* szText);
void ServerClBeginStroke(NetUser* User, const Vec2* Point, const DrawTool* Tool, FrameData* FrameDat);
void ServerClAddToStroke(NetUser* User, const Vec2* Point);
void ServerClEndStroke(NetUser* User);
void ServerClUndo();
void ServerClRedo();
void ServerClLeave();

void ServerClStrokeMsg(int SessionMsg, NetUser* User, int Frame, int Count, const Vec2* Points, const DrawTool* opt_Tool);
void ServerClChangeFramelist(int Index, int Flags);
void ServerClGenericUserMsg(int SessionMsg);

void Server_StartAndRun(const char* Port)
{
	if (sock_server)
	{
		printf("[Server] Server already running!\n");
		return;
	}
	printf("[Server] Starting...\n");

	memset(&my_netint, 0, sizeof(my_netint));

	my_netint.join = &ServerClJoin;
	my_netint.setFPS = &ServerClSetFPS;
	my_netint.setFrame = &ServerClSetFrame;
	my_netint.insertFrame = &ServerClInsertFrame;
	my_netint.removeFrame = &ServerClRemoveFrame;
	my_netint.beginStroke = &ServerClBeginStroke;
	my_netint.addToStroke = &ServerClAddToStroke;
	my_netint.endStroke = &ServerClEndStroke;
	my_netint.chat = &ServerClChat;
	my_netint.undoStroke = &ServerClUndo;
	my_netint.redoStroke = &ServerClRedo;
	my_netint.leave = &ServerClLeave;

	list_clients = BasicList_Create();
	mtx_clients = Mutex_Create();
	sock_server = Socket_Create(0, Port);

	Socket_Listen(sock_server);
	while (Socket_Accept(sock_server, &_Server_OnAccept));

	printf("[Server] Closing server...\n");

	my_sesh.on_seshmsg = client_onseshmsg;
	Socket_Destroy(sock_server);
	Mutex_Destroy(mtx_clients);
	BasicList_Destroy(list_clients);

	printf("[Server] Server closed.\n");
}

void _Server_ProcessMsg(SocketHandle Client, NetUser** pUser, NetMsg* Msg)
{
	NetUser* user = *pUser;
	NetMsg* msg = Msg;
	int seshmsg = Net_ntohl(Msg->seshmsg);

	switch (seshmsg)
	{
	case SessionMsg_UserJoin:
	{
		if (user)
		{
			printf("[Server] User \"%S\" attempted to send multiple join msgs\n", user->szName);
			break;
		}

		user = *pUser = NetUser_Create(GenerateUID(), (UniChar*)(Msg->data + sizeof(UID)));
		*(int*)Msg->data = Net_htonl(user->id); // Assign the Msg's UID to our generated one

		printf("[Server] Adding user \"%S\"\n", user->szName);
		Session_LockUsers();

		_Server_SendToAll(Msg);

		if (Client) // Zero if in the context of local user
		{
			Socket_Send(Client, (char*)msg, Net_ntohl(msg->length)); // Send the client their local user first
			for (BasicListItem* next = 0; next = BasicList_Next(my_sesh.users, next);)
			{
				NetUser* dude = (NetUser*)next->data;

				size_t textlen = (wcslen(dude->szName) + 1) * sizeof(dude->szName[0]);
				NetMsg* nextmsg = NetMsg_Create(SessionMsg_UserJoin, sizeof(UID) + textlen);
				*(UID*)nextmsg->data = Net_htonl(dude->id);
				memcpy(nextmsg->data + sizeof(UID), dude->szName, textlen);

				Socket_Send(Client, (char*)nextmsg, Net_ntohl(nextmsg->length));
				free(nextmsg);
			}

			NetClient* netcl = (NetClient*)malloc(sizeof(*netcl));
			netcl->sock = Client, netcl->user = user;
			BasicList_Add(list_clients, netcl);
		}

		Session_AddUser(user);

		Session_UnlockUsers();

	}
	break;
	case SessionMsg_UserChat:
		if (user)
		{
			*(UID*)Msg->data = Net_htonl(user->id);
			_Server_SendToAll(Msg);
		}
		break;
	case SessionMsg_UserLeave:
		if (user)
		{
			*(UID*)Msg->data = Net_htonl(user->id);
			_Server_SendToAll(Msg);
		}
		break;
	case SessionMsg_UserStrokeAdd:
	{
		if (!user)
			break;

		Session_LockUsers();

		*(UID*)msg->data = Net_htonl(user->id);

		char* next = msg->data + sizeof(UID);
		int idxframe = Net_ntohl(*(int*)next);
		next += sizeof(idxframe);
		int count = Net_ntohl(*(int*)next);
		next += sizeof(count);

		Vec2* netpoints = (Vec2*)next;

		Session_LockFrames();
		if (count > 0 && idxframe >= 0 && idxframe < Session_FrameCount())
		{
			if (!user->bDrawing)
			{
				DrawTool* tool = (DrawTool*)&netpoints[count];
				Vec2 point = { Net_ntohl(netpoints->x), Net_ntohl(netpoints->y) };
				NetUser_BeginStroke(user, &point, tool, Session_GetFrame(idxframe)->data);

				netpoints++;
				count--;
			}
			if (count)
			{
				for (int i = 0; i < count; i++)
				{
					Vec2 point = { Net_ntohl(netpoints[i].x), Net_ntohl(netpoints[i].y) };
					NetUser_AddToStroke(user, &point);
				}
			}

			_Server_SendToAll(msg);
		}
		else
			printf("[Server] Invalid stroke data from \"%S\"\n", user->szName);
		Session_UnlockFrames();
		Session_UnlockUsers();
	}
	break;
	case SessionMsg_UserStrokeEnd:
	{
		if (!user)
			break;

		Session_LockUsers();
		Session_LockFrames();

		*(UID*)msg->data = Net_htonl(user->id);
		NetUser_EndStroke(user);
		_Server_SendToAll(msg);

		Session_UnlockFrames();
		Session_UnlockUsers();
	}
	break;
	case SessionMsg_UserStrokeUndo:
	case SessionMsg_UserStrokeRedo:
	{
		if (!user)
			break;

		Session_LockUsers();
		*(UID*)msg->data = Net_htonl(user->id);

		Session_LockFrames();
		if (seshmsg == SessionMsg_UserStrokeUndo)
			NetUser_UndoStroke(user);
		else
			NetUser_RedoStroke(user);

		_Server_SendToAll(msg);

		Session_UnlockFrames();
		Session_UnlockUsers();
	}
	break;
	case SessionMsg_ChangedFPS:
	{
		int fps = Net_ntohl(*(int*)msg->data);
		if (fps < 0 || fps > 100)
			break;

		Session_LockFrames();
		Session_SetFPS(fps);
		_Server_SendToAll(msg);
		Session_UnlockFrames();
	}
	break;
	case SessionMsg_ChangedFrame:
		// TO DO: Users can send their current frame so that other users may follow them
		break;
	case SessionMsg_ChangedFramelist:
	{
		Session_LockFrames();
		int idxframe = Net_ntohl(*(int*)msg->data);
		int flags = Net_ntohl(*(int*)(msg->data + sizeof(idxframe)));

		int framecount = Session_FrameCount();
		char valid = 1;
		if (idxframe > 0)
		{
			if (flags && idxframe <= framecount) // Boolean for now. True = add, false = remove
				Session_InsertFrame(idxframe);
			else if (!flags && idxframe < framecount) // Remove
				Session_RemoveFrame(idxframe);
			else valid = 0;
		}
		if (valid)
			_Server_SendToAll(msg);
		Session_UnlockFrames();
	}
	break;
	default:
		printf("[Server] Unhandled msg type %d\n", seshmsg);
	break;
	}
}

int _Server_ClientThread(void* UserDat)
{
	SocketHandle Client = (SocketHandle)UserDat;
	
	char* msgbuf = (char*)malloc(MAX_MSGLEN);
	int nextlen = 0, off = 0;

	NetMsg* msg = (NetMsg*)msgbuf;
	NetUser* user = 0;

	while (1)
	{
		// Recv for the next message will always be at off 0
		int count = Socket_Recv(Client, msgbuf + off, (nextlen ? nextlen : MAX_MSGLEN) - off);
		if (!count)
			break;

		if (!nextlen)
		{
			if (off >= sizeof(NetMsg))
				nextlen = Net_ntohl(msg->length);
		}
		if (nextlen > MAX_MSGLEN)
		{
			printf("[Server] wtheck, RUDE!!! Client %p was sending a large message of %d bytes! Kicking...\n", Client, nextlen);
			break;
		}

		if (off >= nextlen)
		{
			_Server_ProcessMsg(Client, &user, msg);

			off = nextlen = 0;
		}
		else
			off += count;
	}

	Mutex_Lock(mtx_clients);
	for (BasicListItem* next = 0; next = BasicList_Next(list_clients, next);)
	{
		NetClient* netcl = (NetClient*)next->data;
		if (netcl->sock == Client)
		{
			BasicList_Remove_FirstOf(list_clients, next->data);
			break;
		}
	}
	Mutex_Unlock(mtx_clients);

	Socket_Destroy(Client);
	free(msgbuf);
	printf("[Server] Client %p disconnected\n", Client);
	return 0;
}

int _Server_OnAccept(SocketHandle Client)
{
	printf("[Server] Accepting client %p\n", Client);
	Thread_Resume(Thread_Create(&_Server_ClientThread, Client));
	return 0;
}

void _Server_OnSessionMsg(int SessionMsg, UID Object)
{
	switch (SessionMsg)
	{
	case SessionMsg_UserJoin:
		printf("[Server] User %S has joined\n", Session_GetUser(Object)->szName);
		break;
	}
	if (client_onseshmsg)
		client_onseshmsg(SessionMsg, Object);
}

void _Server_SendToAll(const NetMsg* Msg)
{
	Mutex_Lock(mtx_clients);
	for (BasicListItem* next = 0; next = BasicList_Next(list_clients, next);)
	{
		NetClient* netcl = (NetClient*)next->data;
		Socket_Send(netcl->sock, (const char*)Msg, Net_ntohl(Msg->length));
	}
	Mutex_Unlock(mtx_clients);
}

void ServerClJoin(const UniChar* szName)
{
	size_t textlen = (wcslen(szName) + 1) * sizeof(szName[0]);
	NetMsg* msg = NetMsg_Create(SessionMsg_UserJoin, sizeof(UID) + textlen);
	*(UID*)msg->data = Net_htonl(GenerateUID());
	memcpy(msg->data + sizeof(UID), szName, textlen);

	_Server_ProcessMsg(0, &user_local, msg);

	NetMsg_Destroy(msg);
}

void ServerClSetFPS(int FPS)
{
	NetMsg* msg = NetMsg_Create(SessionMsg_ChangedFPS, sizeof(FPS));
	*(int*)msg->data = Net_htonl(FPS);
	_Server_ProcessMsg(0, &user_local, msg);
	NetMsg_Destroy(msg);
}

void ServerClSetFrame(int Index)
{
	Session_SetFrame(Index);

	NetMsg* sendmsg = NetMsg_Create(SessionMsg_ChangedFrame, sizeof(Index));
	*(int*)sendmsg->data = Net_htonl(Index);
	_Server_ProcessMsg(0, &user_local, sendmsg);
	free(sendmsg);
}

void ServerClInsertFrame(int Index) {
	ServerClChangeFramelist(Index, 1);
}

void ServerClRemoveFrame(int Index) {
	ServerClChangeFramelist(Index, 0);
}

void ServerClChat(const UniChar* szText)
{
	size_t textlen = (wcslen(szText) + 1) * sizeof(szText[0]);
	NetMsg* msg = NetMsg_Create(SessionMsg_UserChat, sizeof(UID) + textlen);
	memcpy(msg->data + sizeof(UID), szText, textlen);

	_Server_ProcessMsg(0, &user_local, msg);

	NetMsg_Destroy(msg);
}

void ServerClBeginStroke(NetUser* User, const Vec2* Point, const DrawTool* Tool, FrameData* FrameDat) {
	ServerClStrokeMsg(SessionMsg_UserStrokeAdd, user_local, Session_ActiveFrameIndex(), 1, Point, Tool);
}

void ServerClAddToStroke(NetUser* User, const Vec2* Point) {
	ServerClStrokeMsg(SessionMsg_UserStrokeAdd, user_local, Session_ActiveFrameIndex(), 1, Point, 0);
}

void ServerClEndStroke(NetUser* User) {
	ServerClGenericUserMsg(SessionMsg_UserStrokeEnd);
}

void ServerClUndo() {
	ServerClGenericUserMsg(SessionMsg_UserStrokeUndo);
}

void ServerClRedo() {
	ServerClGenericUserMsg(SessionMsg_UserStrokeRedo);
}

void ServerClLeave() {
	ServerClGenericUserMsg(SessionMsg_UserLeave);
}

void ServerClStrokeMsg(int SessionMsg, NetUser* User, int Frame, int Count, const Vec2* Points, const DrawTool* opt_Tool)
{
	int len = sizeof(UID) + sizeof(Frame) + sizeof(Count) + (sizeof(Points[0]) * Count);
	if (opt_Tool)
		len += sizeof(DrawTool);

	NetMsg* msg = NetMsg_Create(SessionMsg, len);

	char* next = msg->data + sizeof(UID);
	*(int*)next = Net_htonl(Frame);
	next += sizeof(Frame);
	*(int*)next = Net_htonl(Count);
	next += sizeof(Count);

	for (int i = 0; i < Count; i++)
	{
		Vec2* point = &((Vec2*)next)[i];
		point->x = Net_htonl(Points[i].x);
		point->y = Net_htonl(Points[i].y);
	}
	next += sizeof(Points[0]) * Count;

	if (opt_Tool)
		*(DrawTool*)next = *opt_Tool;

	_Server_ProcessMsg(0, &user_local, msg);

	NetMsg_Destroy(msg);
}

void ServerClChangeFramelist(int Index, int Flags)
{
	NetMsg* msg = NetMsg_Create(SessionMsg_ChangedFramelist, sizeof(int) + sizeof(int));
	*(int*)msg->data = Net_htonl(Index);
	*(int*)(msg->data + sizeof(Index)) = Net_htonl(Flags);

	_Server_ProcessMsg(0, &user_local, msg);

	NetMsg_Destroy(msg);
}

void ServerClGenericUserMsg(int SessionMsg)
{
	NetMsg* msg = NetMsg_Create(SessionMsg, sizeof(UID));
	_Server_ProcessMsg(0, &user_local, msg);
	NetMsg_Destroy(msg);
}
