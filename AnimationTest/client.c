#include "client.h"
#include "sockets.h"
#include "threading.h"
#include "session.h"
#include <stdio.h>

SocketHandle sock_client;

void ClientJoin(const UniChar* szName);
void ClientSetFPS(int FPS);
void ClientSetFrame(int Index);
void ClientInsertFrame(int Index);
void ClientRemoveFrame(int Index);
void ClientChat(const UniChar* szText);
void ClientBeginStroke(NetUser* User, const Vec2* Point, const DrawTool* Tool, FrameData* FrameDat);
void ClientAddToStroke(NetUser* User, const Vec2* Point);
void ClientEndStroke(NetUser* User);
void ClientUndo();
void ClientRedo();
void ClientLeave();

void ClientChangeFramelist(int Index, int Flags);
void ClientStrokeMsg(int SessionMsg, NetUser* User, int Frame, int Count, const Vec2* Points, const DrawTool* opt_Tool);
void ClientGenericUserMsg(int SessionMsg);

void Client_StartAndRun(const UniChar* szName, const char* Host, const char* Port)
{
	if (sock_client)
	{
		printf("[Client] Client already running!\n");
		return;
	}
	printf("[Client] Starting...\n");

	memset(&my_netint, 0, sizeof(my_netint));

	my_netint.join = &ClientJoin;
	my_netint.setFPS = &ClientSetFPS;
	my_netint.setFrame = &ClientSetFrame;
	my_netint.insertFrame = &ClientInsertFrame;
	my_netint.removeFrame = &ClientRemoveFrame;
	my_netint.beginStroke = &ClientBeginStroke;
	my_netint.addToStroke = &ClientAddToStroke;
	my_netint.endStroke = &ClientEndStroke;
	my_netint.chat = &ClientChat;
	my_netint.undoStroke = &ClientUndo;
	my_netint.redoStroke = &ClientRedo;
	my_netint.leave = &ClientLeave;

	sock_client = Socket_Create(Host, Port);
	if (Socket_Connect(sock_client))
	{
		ClientJoin(szName);

		char* msgbuf = (char*)malloc(MAX_MSGLEN);
		int nextlen = 0, off = 0;

		NetMsg* msg = (NetMsg*)msgbuf;

		while (1)
		{
			// Recv for the next message will always be at off 0
			int count = Socket_Recv(sock_client, msgbuf + off, (nextlen ? nextlen : MAX_MSGLEN) - off);
			if (!count)
				break;

			if (!nextlen)
			{
				if (off >= sizeof(NetMsg))
					nextlen = Net_ntohl(msg->length);
			}
			if (nextlen > MAX_MSGLEN)
			{
				printf("[Client] wtheck, RUDE!!! Server was sending a large message of %d bytes! Kicking...\n", nextlen);
				break;
			}

			if (off >= nextlen)
			{
				int seshmsg = Net_ntohl(msg->seshmsg);
				switch (seshmsg)
				{
				case SessionMsg_UserJoin:
				{
					UID iduser = Net_ntohl(*(UID*)msg->data);
					UniChar* szName = (UniChar*)(msg->data + sizeof(iduser));

					NetUser* newuser = NetUser_Create(iduser, szName);

					if (!user_local)
						user_local = newuser;

					printf("[Client] Adding user \"%S\" (0x%X)\n", newuser->szName, newuser->id);

					Session_LockUsers();
					Session_AddUser(newuser);
					Session_UnlockUsers();
				}
					break;
				case SessionMsg_UserChat:
				{
					Session_LockUsers();
					UID iduser = Net_ntohl(*(UID*)msg->data);
					NetUser* dude = Session_GetUser(iduser);
					UniChar* szText = (UniChar*)(msg->data + sizeof(iduser));
					if (dude)
						printf("%S: %S\n", dude->szName, szText);
					else
						printf("[Client] Failed to find user 0x%X\n", iduser);
					Session_UnlockUsers();
				}
					break;
				case SessionMsg_UserLeave:
				{
					Session_LockUsers();
					UID iduser = Net_ntohl(*(UID*)msg->data);
					NetUser* dude = Session_GetUser(iduser);
					UniChar* szText = (UniChar*)(msg->data + sizeof(iduser));
					if (dude)
						Session_RemoveUser(dude);
					else
						printf("[Client] Failed to find user 0x%X\n", iduser);
					Session_UnlockUsers();
				}
					break;
				case SessionMsg_UserStrokeAdd:
				{
					Session_LockUsers();

					UID iduser = Net_ntohl(*(UID*)msg->data);
					NetUser* dude = Session_GetUser(iduser);
					char* next = msg->data + sizeof(iduser);

					int idxframe = Net_ntohl(*(int*)next);
					next += sizeof(idxframe);

					int count = Net_ntohl(*(int*)next);
					next += sizeof(count);

					if (dude)
					{
						Vec2* netpoints = (Vec2*)next;

						Session_LockFrames();
						if (count > 0 && idxframe >= 0 && idxframe < Session_FrameCount())
						{
							if (!dude->bDrawing)
							{
								DrawTool* tool = (DrawTool*)&netpoints[count];
								Vec2 point = { Net_ntohl(netpoints->x), Net_ntohl(netpoints->y) };
								NetUser_BeginStroke(dude, &point, tool, Session_GetFrame(idxframe)->data);

								netpoints++;
								count--;
							}
							if (count)
							{
								for (int i = 0; i < count; i++)
								{
									Vec2 point = { Net_ntohl(netpoints[i].x), Net_ntohl(netpoints[i].y) };
									NetUser_AddToStroke(dude, &point);
								}
							}
						}
						else
							printf("[Client] Invalid stroke data from \"%S\"\n", dude->szName);
						Session_UnlockFrames();
					}
					else
						printf("[Client] Failed to find user 0x%X\n", iduser);
					Session_UnlockUsers();
				}
					break;
				case SessionMsg_UserStrokeEnd:
				{
					Session_LockUsers();
					Session_LockFrames();

					UID iduser = Net_ntohl(*(UID*)msg->data);
					NetUser* dude = Session_GetUser(iduser);
					if (dude)
						NetUser_EndStroke(dude);
					else
						printf("[Client] Failed to find user 0x%X\n", iduser);

					Session_UnlockFrames();
					Session_UnlockUsers();
				}
					break;
				case SessionMsg_UserStrokeUndo:
				case SessionMsg_UserStrokeRedo:
				{
					Session_LockUsers();
					UID iduser = Net_ntohl(*(UID*)msg->data);
					NetUser* dude = Session_GetUser(iduser);

					if (dude)
					{
						Session_LockFrames();
						if (seshmsg == SessionMsg_UserStrokeUndo)
							NetUser_UndoStroke(dude);
						else
							NetUser_RedoStroke(dude);
						Session_UnlockFrames();
					}
					else
						printf("[Client] Failed to find user 0x%X\n", iduser);

					Session_UnlockUsers();
				}
					break;
				case SessionMsg_ChangedFPS:
				{
					int fps = Net_ntohl(*(int*)msg->data);
					Session_LockFrames();
					Session_SetFPS(fps);
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

					if (idxframe > 0)
					{
						if (flags) // Boolean for now. True = add, false = remove
							Session_InsertFrame(idxframe);
						else // Remove
							Session_RemoveFrame(idxframe);
					}
					Session_UnlockFrames();
				}
					break;
				default:
					printf("[Client] Unhandled msg type %d\n", seshmsg);
				}

				off = nextlen = 0;
			}
			else
				off += count;
		}

		printf("[Client] Closing client...\n");
		free(msgbuf);
	}
	else
		printf("[Client] Failed to connect");

	Socket_Destroy(sock_client);
	printf("[Client] Client closed.\n");
}

void LocalJoin(const UniChar* szName);
void LocalLeave() { }
void LocalChat(const UniChar* szText) { }
void LocalUndo();
void LocalRedo();

void Client_StartAndRun_Local()
{
	memset(&my_netint, 0, sizeof(my_netint));

	my_netint.join = &LocalJoin;
	my_netint.setFPS = &Session_SetFPS;
	my_netint.setFrame = &Session_SetFrame;
	my_netint.insertFrame = &Session_InsertFrame;
	my_netint.removeFrame = &Session_RemoveFrame;
	my_netint.beginStroke = &NetUser_BeginStroke;
	my_netint.addToStroke = &NetUser_AddToStroke;
	my_netint.endStroke = &NetUser_EndStroke;
	my_netint.chat = &LocalChat;
	my_netint.undoStroke = &LocalUndo;
	my_netint.redoStroke = &LocalRedo;
	my_netint.leave = &LocalLeave;
}

void ClientJoin(const UniChar* szName)
{
	size_t namelen = wcslen(szName) + 1;
	if (namelen >= USERNAME_MAX)
		namelen = USERNAME_MAX - 1;
	namelen *= sizeof(szName[0]);

	NetMsg* msg = NetMsg_Create(SessionMsg_UserJoin, sizeof(UID) + namelen);
	memcpy(msg->data + sizeof(UID), szName, namelen);
	Socket_Send(sock_client, (char*)msg, Net_ntohl(msg->length));
	NetMsg_Destroy(msg);
}

void ClientSetFPS(int FPS)
{
	NetMsg* sendmsg = NetMsg_Create(SessionMsg_ChangedFPS, sizeof(FPS));
	*(int*)sendmsg->data = Net_htonl(FPS);
	Socket_Send(sock_client, sendmsg, Net_ntohl(sendmsg->length));
	free(sendmsg);
}

void ClientSetFrame(int Index)
{
	Session_SetFrame(Index);

	NetMsg* sendmsg = NetMsg_Create(SessionMsg_ChangedFrame, sizeof(Index));
	*(int*)sendmsg->data = Net_htonl(Index);
	Socket_Send(sock_client, sendmsg, Net_ntohl(sendmsg->length));
	free(sendmsg);
}

void ClientInsertFrame(int Index) {
	ClientChangeFramelist(Index, 1);
}

void ClientRemoveFrame(int Index) {
	ClientChangeFramelist(Index, 0);
}

void ClientChat(const UniChar* szText)
{
	size_t textlen = (wcslen(szText) + 1) * sizeof(szText[0]);
	NetMsg* msg = NetMsg_Create(SessionMsg_UserChat, sizeof(UID) + textlen);
	memcpy(msg->data + sizeof(UID), szText, textlen);
	Socket_Send(sock_client, (char*)msg, Net_ntohl(msg->length));
	NetMsg_Destroy(msg);
}

void ClientBeginStroke(NetUser* User, const Vec2* Point, const DrawTool* Tool, FrameData* FrameDat) {
	ClientStrokeMsg(SessionMsg_UserStrokeAdd, User, Session_ActiveFrameIndex(), 1, Point, Tool);
}

void ClientAddToStroke(NetUser* User, const Vec2* Point)
{
	ClientStrokeMsg(SessionMsg_UserStrokeAdd, User, Session_ActiveFrameIndex(), 1, Point, 0);
}

void ClientEndStroke(NetUser* User)
{
	NetMsg* msg = NetMsg_Create(SessionMsg_UserStrokeEnd, sizeof(UID));
	Socket_Send(sock_client, (char*)msg, Net_ntohl(msg->length));
	NetMsg_Destroy(msg);
}

void ClientUndo() {
	ClientGenericUserMsg(SessionMsg_UserStrokeUndo);
}

void ClientRedo() {
	ClientGenericUserMsg(SessionMsg_UserStrokeRedo);
}

void ClientLeave() {
	ClientGenericUserMsg(SessionMsg_UserLeave);
}

void LocalJoin(const UniChar* szName)
{
	user_local = NetUser_Create(GenerateUID(), szName);
	Session_AddUser(user_local);
}

void LocalUndo() {
	NetUser_UndoStroke(user_local);
}

void LocalRedo() {
	NetUser_RedoStroke(user_local);
}

void ClientChangeFramelist(int Index, int Flags)
{
	NetMsg* msg = NetMsg_Create(SessionMsg_ChangedFramelist, sizeof(int) + sizeof(int));
	*(int*)msg->data = Net_htonl(Index);
	*(int*)(msg->data + sizeof(Index)) = Net_htonl(Flags);

	Socket_Send(sock_client, (char*)msg, Net_ntohl(msg->length));

	NetMsg_Destroy(msg);
}

void ClientStrokeMsg(int SessionMsg, NetUser* User, int Frame, int Count, const Vec2* Points, const DrawTool* opt_Tool)
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

	Socket_Send(sock_client, (char*)msg, Net_ntohl(msg->length));
	NetMsg_Destroy(msg);
}

void ClientGenericUserMsg(int SessionMsg)
{
	NetMsg* msg = NetMsg_Create(SessionMsg, sizeof(UID));
	Socket_Send(sock_client, (char*)msg, Net_ntohl(msg->length));
	NetMsg_Destroy(msg);
}
