#include "client.h"
#include "sockets.h"
#include "threading.h"
#include "session.h"
#include "window.h"
#include <stdio.h>

SocketHandle sock_client;
UserStroke* stroke_local = 0, * stroke_server = 0;

void ClientJoin(const UniChar* szName);
void ClientSetFPS(int FPS);
void ClientSetFrame(int Index);
void ClientInsertFrame(int Index, char bDup);
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

	stroke_local = stroke_server = 0;
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

	printf("[Client] Connecting to %s:%s...\n", Host, Port);
	sock_client = Socket_Create(Host, Port);
	if (sock_client && Socket_Connect(sock_client))
	{
		ClientJoin(szName);

		char* msgbuf = (char*)malloc(MAX_MSGLEN);
		int nextlen = 0, off = 0;

		NetMsg* msg = (NetMsg*)msgbuf;

		while (1)
		{
			// Recv for the next message will always be at off 0
			int rec = nextlen ? (nextlen - off) : (int)sizeof(NetMsg);
			if (rec + off > MAX_MSGLEN)
				rec = MAX_MSGLEN - off;

			int count = Socket_Recv(sock_client, msgbuf + off, rec);
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
				printf("[Client] wtheck, RUDE!!! Server was sending a large message of %d bytes! Leaving...\n", nextlen);
				break;
			}

			if (off >= nextlen)
			{
				int seshmsg = Net_ntohl(msg->seshmsg);
				switch (seshmsg)
				{
				case SessionMsg_Init:
				{
					int* data = (int*)msg->data;
					int width = Net_ntohl(data[0]), height = Net_ntohl(data[1]);
					int fps = Net_ntohl(data[2]);
					IntColor bkgcol = (IntColor)Net_ntohl(data[3]);

					Session_Init(bkgcol, width, height, fps);
				}
					break;
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

					Vec2* netpoints = (Vec2*)next;
					DrawTool* tool = 0;
					if (!dude || (dude->id == user_local->id || !dude->bDrawing))
						tool = (DrawTool*)&netpoints[count];

					Session_LockFrames();
					if (count > 0 && idxframe >= 0 && idxframe < Session_FrameCount())
					{
						FrameData* dat = Session_GetFrame(idxframe)->data;
						if (tool && dude)
						{
							Vec2 point = { Net_ntohl(netpoints->x), Net_ntohl(netpoints->y) };

							if (dude->id == user_local->id)
							{
								if (!stroke_server)
									stroke_server = UserStroke_Create(user_local, tool, dat);
								UserStroke_AddPoint(stroke_server, &point);
							}
							else
								NetUser_BeginStroke(dude, &point, tool, dat);

							netpoints++;
							count--;
						}
						if (count)
						{
							UserStroke* stroke = 0;
							if (dude && dude->id == user_local->id)
								stroke = stroke_server;
							else if (!dude)
								stroke = UserStroke_Create(0, tool, dat);

							for (int i = 0; i < count; i++)
							{
								Vec2 point = { Net_ntohl(netpoints[i].x), Net_ntohl(netpoints[i].y) };
								if (stroke)
									UserStroke_AddPoint(stroke, &point);
								else
									NetUser_AddToStroke(dude, &point);
							}

							if (!dude)
							{
								FrameData_AddStroke(stroke->framedat, stroke);
								BasicList_Add(my_sesh._strokes, stroke);
								my_sesh.on_seshmsg(SessionMsg_UserStrokeAdd, -1);
							}
						}
					}
					else
						printf("[Client] Invalid stroke data from \"%S\"\n", dude->szName);
					Session_UnlockFrames();
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
					{
						if (dude->id == user_local->id)
						{
							// Replace locally-drawn stroke with stroke data echoed from server
							BasicList_Remove_FirstOf(user_local->strokes, stroke_local);
							FrameData_RemoveStroke(stroke_local->framedat, stroke_local);

							BasicList_Add(user_local->strokes, stroke_server);
							FrameData_AddStroke(stroke_server->framedat, stroke_server);

							UserStroke_Destroy(stroke_local);
							stroke_local = 0, stroke_server = 0;
						}
						else
							NetUser_EndStroke(dude);
					}
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
				/*case SessionMsg_SwitchedFrame:
				{
					UID iduser = Net_ntohl(*(int*)msg->data);
					int idxframe = Net_ntohl(*(int*)(msg->data + sizeof(iduser)));

					Session_LockUsers();
					Session_LockFrames();

					NetUser* user = 0;
					if (iduser != user_local->id)
						user = Session_GetUser(iduser);
					Session_SetFrame(idxframe, user);

					Session_UnlockFrames();
					Session_UnlockUsers();
				}
					break;*/
				case SessionMsg_ChangedFramelist:
				{
					Session_LockUsers();
					Session_LockFrames();
					int* ints = (int*)msg->data;
					UID iduser = Net_ntohl(ints[0]);
					int idxframe = Net_ntohl(ints[1]);
					int flags = Net_ntohl(ints[2]);

					NetUser* dude = Session_GetUser(iduser);
					int active = Session_ActiveFrameIndex();

					if (idxframe >= 0)
					{
						char dup = 0;
						switch (flags)
						{
						case FrameFlag_Dup:
							dup = 1;
						case FrameFlag_Insert:
							Session_InsertFrame(idxframe, dup, dude);
							break;
						case FrameFlag_Remove:
							Session_RemoveFrame(idxframe, dude); break;
						}
					}

					if (dude == user_local)
					{
						int set = idxframe >= Session_FrameCount() ? Session_FrameCount() - 1 : idxframe;
						Session_SetFrame(set, user_local);
					}

					printf("[Client] FrameList: ");
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
					printf("[Client] Unhandled msg type %d\n", seshmsg);
				}

				off = nextlen = 0;
			}
		}

		printf("[Client] Closing client...\n");
		free(msgbuf);
	}
	else
	{
		printf("[Client] Failed to connect.\n");
		OpenDialog(DialogType_Error, L"Failed to connect to server\nSee console for details\n");
	}

	if (sock_client)
		Socket_Destroy(sock_client);
	sock_client = 0;

	Client_StartAndRun_Local();

	printf("[Client] Client closed.\n");
}

void LocalJoin(const UniChar* szName);
void LocalSetFrame(int Index);
void LocaLInsertFrame(int Index, char bDup);
void LocaLRemoveFrame(int Index);
void LocalLeave() { }
void LocalChat(const UniChar* szText) { }
void LocalUndo();
void LocalRedo();

void Client_StartAndRun_Local()
{
	memset(&my_netint, 0, sizeof(my_netint));

	my_netint.join = &LocalJoin;
	my_netint.setFPS = &Session_SetFPS;
	my_netint.setFrame = &LocalSetFrame;
	my_netint.insertFrame = &LocaLInsertFrame;
	my_netint.removeFrame = &LocaLRemoveFrame;
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
	Socket_Send(sock_client, (char*)sendmsg, Net_ntohl(sendmsg->length));
	free(sendmsg);
}

void ClientSetFrame(int Index)
{
	//Session_SetFrame(Index, user_local);

	NetMsg* sendmsg = NetMsg_Create(SessionMsg_SwitchedFrame, sizeof(UID) + sizeof(Index));
	int* ints = (int*)sendmsg->data;
	ints[1] = Net_htonl(Index);
	Socket_Send(sock_client, (char*)sendmsg, Net_ntohl(sendmsg->length));
	free(sendmsg);
}

void ClientInsertFrame(int Index, char bDup) {
	ClientChangeFramelist(Index, bDup ? FrameFlag_Dup : FrameFlag_Insert);
}

void ClientRemoveFrame(int Index) {
	ClientChangeFramelist(Index, FrameFlag_Remove);
}

void ClientChat(const UniChar* szText)
{
	size_t textlen = (wcslen(szText) + 1) * sizeof(szText[0]);
	NetMsg* msg = NetMsg_Create(SessionMsg_UserChat, sizeof(UID) + textlen);
	memcpy(msg->data + sizeof(UID), szText, textlen);
	Socket_Send(sock_client, (char*)msg, Net_ntohl(msg->length));
	NetMsg_Destroy(msg);
}

void ClientBeginStroke(NetUser* User, const Vec2* Point, const DrawTool* Tool, FrameData* FrameDat)
{
	NetUser_BeginStroke(user_local, Point, Tool, FrameDat);
	stroke_local = (UserStroke*)user_local->strokes->tail->data;

	ClientStrokeMsg(SessionMsg_UserStrokeAdd, user_local, Session_ActiveFrameIndex(), 1, Point, Tool);
}

void ClientAddToStroke(NetUser* User, const Vec2* Point)
{
	NetUser_AddToStroke(user_local, Point);
	ClientStrokeMsg(SessionMsg_UserStrokeAdd, User, Session_ActiveFrameIndex(), 1, Point, 0);
}

void ClientEndStroke(NetUser* User)
{
	NetUser_EndStroke(user_local);

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

void LocalSetFrame(int Index) {
	Session_SetFrame(Index, user_local);
}

void LocaLInsertFrame(int Index, char bDup)
{
	Session_InsertFrame(Index, bDup, user_local);
	Session_SetFrame(Index, user_local);
}

void LocaLRemoveFrame(int Index)
{
	Session_RemoveFrame(Index, user_local);
	Session_SetFrame(Index >= Session_FrameCount() ? Index : Session_FrameCount() - 1, user_local);
}

void LocalUndo() {
	NetUser_UndoStroke(user_local);
}

void LocalRedo() {
	NetUser_RedoStroke(user_local);
}

void ClientChangeFramelist(int Index, int Flags)
{
	NetMsg* msg = NetMsg_Create(SessionMsg_ChangedFramelist, sizeof(UID) + sizeof(int) + sizeof(int));
	int* ints = (int*)msg->data;
	ints[1] = Net_htonl(Index);
	ints[2] = Net_htonl(Flags);

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
