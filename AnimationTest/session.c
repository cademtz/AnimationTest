#include "session.h"
#include "sockets.h"

NetSession my_sesh = { 0 };
NetUser* user_local = 0;
NetInterface my_netint = { 0 };

void Session_Init(IntColor BkgCol, unsigned int Width, unsigned int Height, unsigned char FPS)
{
	my_sesh.width = Width, my_sesh.height = Height, my_sesh.bkgcol = BkgCol;
	my_sesh.undo_max = 25, my_sesh.fps = FPS;

	if (!my_sesh.mtx_frames)
		my_sesh.mtx_frames = Mutex_Create();
	if (!my_sesh.mtx_users)
		my_sesh.mtx_users = Mutex_Create();

	Mutex_Lock(my_sesh.mtx_users);
	Mutex_Lock(my_sesh.mtx_frames);

	if (my_sesh.users)
	{
		for (BasicListItem* next = 0; next = BasicList_Next(my_sesh.users, next);)
			NetUser_Destroy((NetUser*)next->data);
		BasicList_Destroy(my_sesh.users);
	}
	if (my_sesh._strokes)
		BasicList_Destroy(my_sesh._strokes);
	if (my_sesh._frames)
		FrameList_Destroy(my_sesh._frames);

	my_sesh._strokes = BasicList_Create();
	my_sesh._frames = FrameList_Create();
	my_sesh.users = BasicList_Create();

	user_local = 0;

	Session_InsertFrame(0, 0);
	Session_SetFrame(0, 0);

	my_sesh.on_seshmsg(SessionMsg_SwitchedFrame, 0);
	my_sesh.on_seshmsg(SessionMsg_ChangedFramelist, 0);
	my_sesh.on_seshmsg(SessionMsg_ChangedFPS, 0);

	Mutex_Unlock(my_sesh.mtx_frames);
	Mutex_Unlock(my_sesh.mtx_users);
}

void Session_SetFPS(int FPS)
{
	my_sesh.fps = FPS;
	my_sesh.on_seshmsg(SessionMsg_ChangedFPS, 0);
}

void Session_SetFrame(int Index, NetUser* opt_User)
{
	if (Index >= my_sesh._frames->count)
		Index = 0;
	else if (Index < 0)
		Index = my_sesh._frames->count - 1;

	if (!opt_User || opt_User == user_local)
	{
		my_sesh._index_active = Index;
		my_sesh._frame_active = FrameList_At(my_sesh._frames, Index);
	}

	if (opt_User)
		opt_User->frame_active = Index;

	my_sesh.on_seshmsg(SessionMsg_SwitchedFrame, opt_User ? opt_User->id : 0);
}

void Session_InsertFrame(int Index, NetUser* opt_User)
{
	FrameData* data = FrameData_Create(my_sesh.width, my_sesh.height, my_sesh.bkgcol);
	FrameList_Insert(my_sesh._frames, data, Index);

	my_sesh.on_seshmsg(SessionMsg_ChangedFramelist, opt_User ? opt_User->id : 0);
}

void Session_RemoveFrame(int Index, NetUser* opt_User)
{
	if (my_sesh._frames->count < 2)
		return;

	UID id = opt_User ? opt_User->id : 0;

	FrameData* framedat = FrameList_Remove_At(my_sesh._frames, Index);
	if (!FrameList_IsDataUsed(my_sesh._frames, framedat))
	{	// Make sure these strokes are removed everywhere
		for (BasicListItem* next = 0; next = BasicList_Next(framedat->strokes, next);)
		{
			UserStroke* stroke = (UserStroke*)next->data;
			BasicList_Remove_FirstOf(my_sesh._strokes, stroke);
			if (stroke->user)
				BasicList_Remove_FirstOf(stroke->user->strokes, stroke);
		}
		FrameData_Destroy(framedat);
	}

	my_sesh.on_seshmsg(SessionMsg_ChangedFramelist, id);

	int count = my_sesh._frames->count;
	if (my_sesh._index_active >= count)
	{
		my_sesh._index_active = count - 1;
		my_sesh._frame_active = FrameList_At(my_sesh._frames, my_sesh._index_active);
		if (user_local)
			my_sesh.on_seshmsg(SessionMsg_SwitchedFrame, user_local->id);
	}
}

FrameItem* Session_GetFrame(int Index) {
	return FrameList_At(my_sesh._frames, Index);
}

int Session_FrameData_GetIndex(const FrameData* FrameDat)
{
	int i = 0;
	for (FrameItem* next = 0; next = FrameList_Next(my_sesh._frames, next); i++)
		if (next->data == FrameDat)
			return i;
	return -1;
}

void Session_AddUser(NetUser* User)
{
	BasicList_Add(my_sesh.users, User);
	my_sesh.on_seshmsg(SessionMsg_UserJoin, User->id);
}

void Session_RemoveUser(NetUser* User)
{
	my_sesh.on_seshmsg(SessionMsg_UserLeave, User->id);
	BasicList_Remove_FirstOf(my_sesh.users, User);

	for (BasicListItem* next = 0; next = BasicList_Next(User->strokes, next);)
	{
		UserStroke* stroke = (UserStroke*)next->data;
		stroke->user = 0;
	}
}

NetUser* Session_GetUser(UID IdUser)
{
	for (BasicListItem* next = 0; next = BasicList_Next(my_sesh.users, next);)
	{
		NetUser* user = (NetUser*)next->data;
		if (user->id == IdUser)
			return user;
	}
	return 0;
}

NetUser* NetUser_Create(UID Id, const UniChar* szName)
{
	NetUser* user = (NetUser*)malloc(sizeof(*user));
	memset(user, 0, sizeof(*user));

	user->id = Id;
	wcscpy_s(user->szName, sizeof(user->szName) / sizeof(user->szName[0]), szName);
	user->strokes = BasicList_Create(), user->undone = BasicList_Create();

	return user;
}

void NetUser_Destroy(NetUser* User)
{
	for (BasicListItem* next = 0; next = BasicList_Next(User->undone, next);)
		UserStroke_Destroy((UserStroke*)next->data);

	BasicList_Destroy(User->undone);
	free(User);
}

void NetUser_BeginStroke(NetUser* User, const Vec2* Point, const DrawTool* Tool, FrameData* FrameDat)
{
	User->bDrawing = 1;

	// Clear undone list
	for (BasicListItem* next = 0; next = BasicList_Next(User->undone, next);)
		UserStroke_Destroy((UserStroke*)next->data);
	BasicList_Clear(User->undone);

	UserStroke* stroke = UserStroke_Create(User, Tool, FrameDat);
	UserStroke_AddPoint(stroke, Point);
	BasicList_Add(User->strokes, stroke);
	BasicList_Add(my_sesh._strokes, stroke);
	FrameData_PointsAdded(FrameDat, stroke, 1);

	my_sesh.on_seshmsg(SessionMsg_UserStrokeAdd, User->id);
}

void NetUser_AddToStroke(NetUser* User, const Vec2* Point)
{
	UserStroke* stroke = (UserStroke*)User->strokes->tail->data;
	UserStroke_AddPoint(stroke, Point);
	FrameData_PointsAdded(stroke->framedat, stroke, 1);
	my_sesh.on_seshmsg(SessionMsg_UserStrokeAdd, User->id);
}

void NetUser_EndStroke(NetUser* User)
{
	User->bDrawing = 0;
	UserStroke* stroke = (UserStroke*)User->strokes->tail->data;
	FrameData_AddStroke(stroke->framedat, stroke);
	my_sesh.on_seshmsg(SessionMsg_UserStrokeEnd, User->id);
}

char NetUser_UndoStroke(NetUser* User)
{
	if (!User->bDrawing && User->strokes->count)
	{
		UserStroke* stroke = BasicList_Pop(User->strokes);
		BasicList_Add(User->undone, stroke);
		FrameData_RemoveStroke(stroke->framedat, stroke);
		BasicList_Remove_FirstOf(my_sesh._strokes, stroke);
		my_sesh.on_seshmsg(SessionMsg_UserStrokeUndo, User->id);
		return 1;
	}
	return 0;
}

char NetUser_RedoStroke(NetUser* User)
{
	if (!User->bDrawing && User->undone->count)
	{
		UserStroke* stroke = BasicList_Pop(User->undone);
		BasicList_Add(User->strokes, stroke);
		FrameData_AddStroke(stroke->framedat, stroke);
		BasicList_Add(my_sesh._strokes, stroke);
		my_sesh.on_seshmsg(SessionMsg_UserStrokeRedo, User->id);
		return 1;
	}
	return 0;
}

UserStroke* UserStroke_Create(NetUser* User, const DrawTool* Tool, FrameData* Frame)
{
	UserStroke* stroke = (UserStroke*)malloc(sizeof(*stroke));
	memset(stroke, 0, sizeof(*stroke));

	stroke->user = User;
	stroke->tool = *Tool;
	stroke->points = BasicList_Create();
	stroke->framedat = Frame;
	return stroke;
}

void UserStroke_Destroy(UserStroke* Stroke)
{
	for (BasicListItem* next = 0; next = BasicList_Next(Stroke->points, next);)
		free(next->data); // Vec2*
	BasicList_Destroy(Stroke->points);
	free(Stroke);
}

void UserStroke_AddPoint(UserStroke* Stroke, const Vec2* Point)
{
	Vec2* point = (Vec2*)malloc(sizeof(*point));
	*point = *Point;
	BasicList_Add(Stroke->points, point);
}

NetMsg* NetMsg_Create(int SessionMsg, int DataLen)
{
	int len = sizeof(NetMsg) + DataLen;
	NetMsg* msg = (NetMsg*)malloc(len);

	msg->length = Net_htonl(len);
	msg->seshmsg = Net_htonl(SessionMsg);

	return msg;
}

void NetMsg_Destroy(NetMsg* Msg) {
	free(Msg);
}
