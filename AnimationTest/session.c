#include "session.h"

NetSession my_sesh = { 0 };
NetUser* user_local = 0;

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
			free(next->data); // NetUser*
		BasicList_Destroy(my_sesh.users);
	}
	if (my_sesh._strokes)
	{
		for (BasicListItem* next = 0; next = BasicList_Next(my_sesh._strokes, next);)
			free(next->data); // UserStroke*
		BasicList_Destroy(my_sesh._strokes);
	}
	if (my_sesh._frames)
		FrameList_Destroy(my_sesh._frames);
	if (user_local)
		NetUser_Destroy(user_local);

	my_sesh._strokes = BasicList_Create();
	my_sesh._frames = FrameList_Create();
	my_sesh.users = BasicList_Create();

	user_local = NetUser_Create(GenerateUID(), L"reynante_gamer512");
	BasicList_Add(my_sesh.users, user_local);

	Session_InsertFrame(0);
	Session_SetFrame(0);

	my_sesh.on_seshmsg(SessionMsg_UserJoin, user_local->id);
	my_sesh.on_seshmsg(SessionMsg_ChangedFrame, 0);
	my_sesh.on_seshmsg(SessionMsg_ChangedFrameCount, 0);
	my_sesh.on_seshmsg(SessionMsg_ChangedFPS, 0);

	Mutex_Unlock(my_sesh.mtx_frames);
	Mutex_Unlock(my_sesh.mtx_users);
}

void Session_SetFrame(int Index)
{
	if (Index >= my_sesh._frames->count)
		Index = 0;
	else if (Index < 0)
		Index = my_sesh._frames->count - 1;

	my_sesh._index_active = Index;
	my_sesh._frame_active = FrameList_At(my_sesh._frames, Index);
	my_sesh.on_seshmsg(SessionMsg_ChangedFrame, 0);
}

void Session_InsertFrame(int Index)
{
	FrameData* data = FrameData_Create(my_sesh.width, my_sesh.height, my_sesh.bkgcol);
	FrameList_Insert(my_sesh._frames, data, Index);

	my_sesh.on_seshmsg(SessionMsg_ChangedFrameCount, 0);
}

void Session_RemoveFrame(int Index)
{
	if (!my_sesh._frames->count)
		return;

	FrameData* data = FrameList_Remove_At(my_sesh._frames, Index);
	if (!FrameList_IsDataUsed(my_sesh._frames, data))
		FrameData_Destroy(data);

	my_sesh.on_seshmsg(SessionMsg_ChangedFrame, 0);

	int count = my_sesh._frames->count;
	if (count >= Index)
	{
		my_sesh._index_active = count - 1;
		my_sesh._frame_active = FrameList_At(my_sesh._frames, my_sesh._index_active);
		my_sesh.on_seshmsg(SessionMsg_ChangedFrameCount, 0);
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

void NetUser_BeginStroke(NetUser* User, const Vec2* Point, const DrawTool* Tool, FrameData* Frame)
{
	User->bDrawing = 1;

	// Clear undone list
	for (BasicListItem* next = 0; next = BasicList_Next(User->undone, next);)
		UserStroke_Destroy((UserStroke*)next->data);
	BasicList_Clear(User->undone);

	UserStroke* stroke = UserStroke_Create(User, Tool, Frame);
	UserStroke_AddPoint(stroke, Point);
	BasicList_Add(User->strokes, stroke);
	BasicList_Add(Frame->strokes, stroke);

	my_sesh.on_seshmsg(SessionMsg_UserStrokeAdd, User->id);
}

void NetUser_AddToStroke(NetUser* User, const Vec2* Point)
{
	UserStroke_AddPoint((UserStroke*)User->strokes->tail->data, Point);
	my_sesh.on_seshmsg(SessionMsg_UserStrokeAdd, User->id);
}

void NetUser_EndStroke(NetUser* User) {
	User->bDrawing = 0;
	my_sesh.on_seshmsg(SessionMsg_UserStrokeEnd, User->id);
}

void NetUser_UndoStroke(NetUser* User)
{
	if (!User->bDrawing && User->strokes->count)
	{
		UserStroke* stroke = BasicList_Pop(User->strokes);
		BasicList_Add(User->undone, stroke);
		FrameData_RemoveStroke(stroke->framedat, stroke);
		my_sesh.on_seshmsg(SessionMsg_UserStrokeUndo, User);
	}
}

void NetUser_RedoStroke(NetUser* User)
{
	if (!User->bDrawing && User->undone->count)
	{
		UserStroke* stroke = BasicList_Pop(User->undone);
		BasicList_Add(User->strokes, stroke);
		FrameData_AddStroke(stroke->framedat, stroke);
		my_sesh.on_seshmsg(SessionMsg_UserStrokeRedo, User);
	}
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
