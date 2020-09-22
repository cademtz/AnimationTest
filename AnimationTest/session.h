#pragma once
#include "project.h"
#include "threading.h"

typedef struct _NetUser NetUser;
typedef void(*SessionCallback)(int SessionMsg, UID Object);

// Session

typedef struct _NetSession
{
	SessionCallback on_seshmsg; // Required
	MutexHandle mtx_frames; // Lock when reading frame data
	MutexHandle mtx_users; // Lock when reading (NON-drawing related) user data

	BasicList* users;
	BasicList* _strokes;
	FrameList* _frames;
	FrameItem* _frame_active;
	IntColor bkgcol;
	unsigned int width, height, _index_active, undo_max;
	unsigned char fps;

} NetSession;

extern NetSession my_sesh;
extern NetUser* user_local;

void Session_Init(IntColor BkgCol, unsigned int Width, unsigned int Height, unsigned char fps);

void Session_SetFPS(int FPS);
void Session_SetFrame(int Index);
void Session_InsertFrame(int Index);
void Session_RemoveFrame(int Index);
inline int Session_FrameCount() { return my_sesh._frames->count; }
inline FrameItem* Session_ActiveFrame() { return my_sesh._frame_active; }
inline int Session_ActiveFrameIndex() { return my_sesh._index_active; }

NetUser* Session_GetUser(UID IdUser);

// - Lock to access frame/drawing data (But no user data from them)
inline void Session_LockFrames() { Mutex_Lock(my_sesh.mtx_frames); }

// - Lock to access user data (But no frame/drawing data from them)
inline void Session_LockUsers() { Mutex_Lock(my_sesh.mtx_users); }

inline void Session_UnlockFrames() { Mutex_Unlock(my_sesh.mtx_frames); }
inline void Session_UnlockUsers() { Mutex_Unlock(my_sesh.mtx_users); }

// Users

typedef struct _NetUser
{
	UID id;
	UniChar szName[32];
	BasicList* strokes, * undone;
	char bDrawing; // Actively adding to a stroke
} NetUser;

NetUser* NetUser_Create(UID Id, const UniChar* szName);
void NetUser_Destroy(NetUser* User);

void NetUser_BeginStroke(NetUser* User, const Vec2* Point, const DrawTool* Tool, FrameData* Frame);
void NetUser_AddToStroke(NetUser* User, const Vec2* Point);
void NetUser_EndStroke(NetUser* User);
void NetUser_UndoStroke(NetUser* User);
void NetUser_RedoStroke(NetUser* User);
void NetUser_RemoveStroke(UserStroke* Stroke); // Removes from both active & undone list

// Actions

typedef struct _UserStroke
{
	FrameData* framedat;
	NetUser* user;
	DrawTool tool;
	BasicList* points; // Vec2
} UserStroke;

UserStroke* UserStroke_Create(NetUser* User, const DrawTool* Tool, FrameData* Frame);
void UserStroke_Destroy(UserStroke* Stroke);
void UserStroke_AddPoint(UserStroke* Stroke, const Vec2* Line);

enum ESessionMsg
{
	SessionMsg_UserJoin,
	SessionMsg_UserChat,

	SessionMsg_UserStrokeUndo,
	SessionMsg_UserStrokeRedo,
	SessionMsg_UserStrokeAdd, // Extend current stroke or start new one
	SessionMsg_UserStrokeEnd,

	SessionMsg_ChangedFPS, 
	SessionMsg_ChangedFrame,
	SessionMsg_ChangedFrameCount,
};