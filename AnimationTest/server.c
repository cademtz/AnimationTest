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
NetMsg* _Server_MakeStrokeMsg(const UserStroke* Stroke);
NetMsg* _Server_MakeGenericUserMsg(int SessionMsg, const NetUser* User);

// Functions for server-client hybrid (Hosting the server while simultaneously being a user)

void ServerClJoin(const UniChar* szName);
void ServerClSetFPS(int FPS);
void ServerClSetFrame(int Index);
void ServerClInsertFrame(int Index, char bDup);
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
void ServerOnMsgStub(int SessionMsg, UID Object) { }

void Server_StartAndRun(const char* Port, char bDedicated)
{
	if (sock_server)
	{
		printf("[Server] Server already running!\n");
		return;
	}
	printf("[Server] Starting...\n");

	if (bDedicated)
		my_sesh.on_seshmsg = &ServerOnMsgStub;

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

	if (sock_server && Socket_Listen(sock_server))
	{
		printf("[Server] Listening on port %s\n", Port);
		while (Socket_Accept(sock_server, &_Server_OnAccept));
	}
	else
		printf("[Server] Error while attempting to host on port %s\n", Port);

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

		if (Client) // Zero if in the context of host's local user
		{
			NetMsg* init = NetMsg_Create(SessionMsg_Init, sizeof(int) * 4);
			int* data = (int*)init->data;
			data[0] = Net_htonl(my_sesh.width), data[1] = Net_htonl(my_sesh.height);
			data[2] = Net_htonl(my_sesh.fps);
			data[3] = Net_htonl(my_sesh.bkgcol);

			Socket_Send(Client, (char*)init, Net_ntohl(init->length));

			NetMsg_Destroy(init);

			Socket_Send(Client, (char*)Msg, Net_ntohl(Msg->length)); // Send the client their local user first

			// Send all users
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

			Mutex_Lock(mtx_clients);
			BasicList_Add(list_clients, netcl);
			Mutex_Unlock(mtx_clients);

			Session_LockFrames();

			// Send all frames

			int framelen = sizeof(UID) + sizeof(int) + sizeof(int);
			NetMsg* framemsg = NetMsg_Create(SessionMsg_ChangedFramelist, framelen);
			data = (int*)framemsg->data;
			data[0] = Net_htonl(0); // User 0 (null)
			data[1] = Net_htonl(0); // Index 0
			data[2] = Net_htonl(FrameFlag_Insert);

			// TODO: Instead of spamming the client with frames, how about implement framecount in SessionMsg_Init
			FrameItem* frame = my_sesh._frames->head;
			char dup = frame->_next && frame->_next->data == frame->data;
			for (int i = 1; frame = frame->_next; i++)
			{
				data[1] = Net_htonl(i);
				data[2] = Net_htonl(dup ? FrameFlag_Dup : FrameFlag_Insert);

				Socket_Send(Client, (char*)framemsg, Net_ntohl(framemsg->length));

				FrameItem* next = frame->_next;
				dup = next && next->data == frame->data;
			}

			NetMsg_Destroy(framemsg);

			// Send all strokes

			for (BasicListItem* next = 0; next = BasicList_Next(my_sesh._strokes, next);)
			{
				UserStroke* stroke = (UserStroke*)next->data;

				NetMsg* strokemsg = _Server_MakeStrokeMsg(stroke);
				Socket_Send(Client, (char*)strokemsg, Net_ntohl(strokemsg->length));
				NetMsg_Destroy(strokemsg);

				if (stroke->user)
				{
					strokemsg = _Server_MakeGenericUserMsg(SessionMsg_UserStrokeEnd, stroke->user);
					Socket_Send(Client, (char*)strokemsg, Net_ntohl(strokemsg->length));
					NetMsg_Destroy(strokemsg);
				}
			}

			Session_UnlockFrames();
		}

		Session_AddUser(user);

		Session_UnlockUsers();
	}
	break;
	case SessionMsg_UserChat:
		if (user)
		{
			Session_LockUsers();
			*(UID*)Msg->data = Net_htonl(user->id);
			_Server_SendToAll(Msg);
			Session_UnlockUsers();
		}
		break;
	case SessionMsg_UserLeave:
		if (user)
		{
			Session_LockUsers();
			*(UID*)Msg->data = Net_htonl(user->id);
			_Server_SendToAll(Msg);
			Session_RemoveUser(user);
			user = *pUser = 0;
			Session_UnlockUsers();
		}
		break;
	case SessionMsg_UserStrokeAdd:
	{
		if (!user)
			break;

		Session_LockUsers();
		Session_LockFrames();

		*(UID*)Msg->data = Net_htonl(user->id);

		char* next = Msg->data + sizeof(UID);
		int idxframe = Net_ntohl(*(int*)next);
		next += sizeof(idxframe);
		int count = Net_ntohl(*(int*)next);
		next += sizeof(count);

		Vec2* netpoints = (Vec2*)next;

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

			_Server_SendToAll(Msg);
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

		if (!user->bDrawing)
		{
			Session_UnlockFrames();
			Session_UnlockUsers();
			break;
		}

		*(UID*)Msg->data = Net_htonl(user->id);
		NetUser_EndStroke(user);
		_Server_SendToAll(Msg);

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
		Session_LockFrames();

		*(UID*)Msg->data = Net_htonl(user->id);

		if (seshmsg == SessionMsg_UserStrokeUndo)
			NetUser_UndoStroke(user);
		else
			NetUser_RedoStroke(user);

		_Server_SendToAll(Msg);

		Session_UnlockFrames();
		Session_UnlockUsers();
	}
	break;
	case SessionMsg_ChangedFPS:
	{
		/*int fps = Net_ntohl(*(int*)Msg->data);
		if (fps < 0 || fps > 100)
			break;

		Session_LockFrames();
		Session_SetFPS(fps);
		_Server_SendToAll(Msg);
		Session_UnlockFrames();*/
	}
	break;
	/*case SessionMsg_SwitchedFrame:
		if (user)
		{
			Session_LockFrames();

			int* ints = (int*)Msg->data;
			ints[0] = Net_htonl(user->id);
			int idxframe = Net_ntohl(ints[1]);
			
			if (idxframe >= 0 && idxframe < Session_FrameCount())
				_Server_SendToAll(Msg);

			Session_UnlockFrames();
		}
		// TO DO: Users can send their current frame so that other users may follow them
		break;*/
	case SessionMsg_ChangedFramelist:
		if (user)
		{
			Session_LockUsers();
			Session_LockFrames();
			int* ints = (int*)Msg->data;
			ints[0] = Net_htonl(user->id);
			int idxframe = Net_ntohl(ints[1]);
			int flags = Net_ntohl(ints[2]);

			int framecount = Session_FrameCount();
			char valid = 0;
			if (idxframe >= 0)
			{
				char dup = 0;
				switch (flags)
				{
				case FrameFlag_Dup:
					dup = 1;
				case FrameFlag_Insert:
					if (idxframe <= framecount)
						valid = Session_InsertFrame(idxframe, dup, user);
					break;
				case FrameFlag_Remove:
					if (idxframe < framecount)
					{
						char isUsed = 0;
						FrameItem* frame = Session_GetFrame(idxframe);
						for (BasicListItem* next = 0; next = BasicList_Next(my_sesh.users, next);)
						{
							NetUser* user = (NetUser*)next->data;
							if (user->bDrawing)
							{
								UserStroke* stroke = (UserStroke*)user->strokes->tail->data;
								if (stroke->framedat == frame->data)
								{
									isUsed = 1;
									break;
								}
							}
						}
						if (isUsed)
							break; // Don't remove frames while another broski is drawing on it. Saves them (and I) trouble

						Session_RemoveFrame(idxframe, user);
						valid = 1;
					}
					break;
				}
			}
			if (valid)
			{
				_Server_SendToAll(Msg);
				if (user_local)
				{
					if (user->id == user_local->id)
					{
						int set = idxframe > Session_FrameCount() ? Session_FrameCount() - 1 : idxframe;
						Session_SetFrame(set, user_local);
					}
				}
			}

			printf("[Server] FrameList: ");
			for (FrameItem* frame = 0; frame = FrameList_Next(my_sesh._frames, frame);)
			{
				if (frame->_next && frame->_next->data == frame->data)
					printf("-");
				else
					printf("o");
			}
			printf("\n");

			Session_UnlockFrames();
			Session_UnlockUsers();
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
		int count = Socket_Recv(Client, msgbuf + off, nextlen ? (nextlen - off) : (int)sizeof(NetMsg));
		if (!count)
			break;

		off += count;

		if (!nextlen)
		{
			nextlen = Net_ntohl(msg->length);
			continue; // Receive the rest of the data, now that the length is known
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
	}

	Mutex_Lock(mtx_clients);
	for (BasicListItem* next = 0; next = BasicList_Next(list_clients, next);)
	{
		NetClient* netcl = (NetClient*)next->data;
		if (netcl->sock == Client)
		{
			BasicList_Remove_FirstOf(list_clients, netcl);
			free(netcl);
			break;
		}
	}
	Mutex_Unlock(mtx_clients);

	if (user)
	{
		NetMsg* leave = _Server_MakeGenericUserMsg(SessionMsg_UserLeave, user);
		_Server_SendToAll(leave);
		NetMsg_Destroy(leave);
		Session_RemoveUser(user);
		NetUser_Destroy(user);
	}

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

NetMsg* _Server_MakeStrokeMsg(const UserStroke* Stroke)
{
	int frame = Session_FrameData_GetIndex(Stroke->framedat);
	int cpoints = Stroke->points->count;
	int length = sizeof(UID) + sizeof(frame) + sizeof(cpoints) + (sizeof(Vec2) * cpoints) + sizeof(DrawTool);

	NetMsg* msg = NetMsg_Create(SessionMsg_UserStrokeAdd, length);
	int* ints = (int*)msg->data;
	ints[0] = Net_htonl(Stroke->user ? Stroke->user->id : -1);
	ints[1] = Net_htonl(frame);
	ints[2] = Net_htonl(cpoints);

	Vec2* points = (Vec2*)&ints[3];
	int i = 0;
	for (BasicListItem* next = 0; next = BasicList_Next(Stroke->points, next); i++)
	{
		Vec2* vec = (Vec2*)next->data;
		points[i].x = Net_htonl(vec->x), points[i].y = Net_htonl(vec->y);
	}
	*(DrawTool*)&points[cpoints] = Stroke->tool;

	return msg;
}

NetMsg* _Server_MakeGenericUserMsg(int SessionMsg, const NetUser* opt_User)
{
	NetMsg* msg = NetMsg_Create(SessionMsg, sizeof(UID));
	*(UID*)msg->data = Net_htonl(opt_User ? opt_User->id : -1);
	return msg;
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
	Session_SetFrame(Index, user_local);

	NetMsg* sendmsg = NetMsg_Create(SessionMsg_SwitchedFrame, sizeof(UID) + sizeof(Index));
	int* ints = (int*)sendmsg->data;
	ints[1] = Net_htonl(Index);
	_Server_ProcessMsg(0, &user_local, sendmsg);
	free(sendmsg);
}

void ServerClInsertFrame(int Index, char bDup) {
	ServerClChangeFramelist(Index, bDup ? FrameFlag_Dup : FrameFlag_Insert);
}

void ServerClRemoveFrame(int Index) {
	ServerClChangeFramelist(Index, FrameFlag_Remove);
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
	NetMsg* msg = NetMsg_Create(SessionMsg_ChangedFramelist, sizeof(UID) + sizeof(int) + sizeof(int));
	int* ints = (int*)msg->data;
	ints[1] = Net_htonl(Index);
	ints[2] = Net_htonl(Flags);

	_Server_ProcessMsg(0, &user_local, msg);

	NetMsg_Destroy(msg);
}

void ServerClGenericUserMsg(int SessionMsg)
{
	NetMsg* msg = NetMsg_Create(SessionMsg, sizeof(UID));
	_Server_ProcessMsg(0, &user_local, msg);
	NetMsg_Destroy(msg);
}
