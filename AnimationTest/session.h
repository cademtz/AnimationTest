#pragma once
#include "project.h"
#include "threading.h"

typedef struct _NetUser NetUser;
typedef struct _NetInterface NetInterface;
typedef void(*SessionCallback)(int SessionMsg, UID Object);

// Session

typedef struct _NetSession
{
	SessionCallback on_seshmsg;
	MutexHandle mtx_frames; // Lock when reading frame data
	MutexHandle mtx_users; // Lock when reading (NON-drawing related) user data

	BasicList* users;
	BasicList* _strokes;
	BasicListItem* _stroke_last;
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
void Session_SetFrame(int Index, NetUser* opt_User);
void Session_InsertFrame(int Index, NetUser* opt_User);
void Session_RemoveFrame(int Index, NetUser* opt_User);
FrameItem* Session_GetFrame(int Index);
int Session_FrameData_GetIndex(const FrameData* FrameDat);
void Session_AddUser(NetUser* User);
void Session_RemoveUser(NetUser* User);
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

#define USERNAME_MAX 0x20

typedef struct _NetUser
{
	UID id;
	UniChar szName[USERNAME_MAX];
	BasicList* strokes, * undone;
	int frame_active;
	char bDrawing; // Actively adding to a stroke
} NetUser;

NetUser* NetUser_Create(UID Id, const UniChar* szName);
void NetUser_Destroy(NetUser* User);

void NetUser_BeginStroke(NetUser* User, const Vec2* Point, const DrawTool* Tool, FrameData* Frame);
void NetUser_AddToStroke(NetUser* User, const Vec2* Point);
void NetUser_EndStroke(NetUser* User);
char NetUser_UndoStroke(NetUser* User);
char NetUser_RedoStroke(NetUser* User);

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
	SessionMsg_Init,

	SessionMsg_UserJoin,
	SessionMsg_UserLeave,
	SessionMsg_UserChat,

	SessionMsg_UserStrokeUndo,
	SessionMsg_UserStrokeRedo,
	SessionMsg_UserStrokeAdd, // Extend current stroke or start new one
	SessionMsg_UserStrokeEnd,

	SessionMsg_ChangedFPS, 
	SessionMsg_SwitchedFrame,
	SessionMsg_ChangedFramelist,
};

// Networking

#define MAX_MSGLEN 0x800

typedef struct _NetMsg
{
	long length;	// Network int
	long seshmsg;	// Network int
	char data[];
} NetMsg;

NetMsg* NetMsg_Create(int SessionMsg, int DataLen);
void NetMsg_Destroy(NetMsg* Msg);

// - A wrapper for the Session functions which chooses where user input is sent
typedef struct _NetInterface
{
	void(*join)(const UniChar* szName);
	void(*setFPS)(int FPS);
	void(*setFrame)(int Index);
	void(*insertFrame)(int Index);
	void(*removeFrame)(int Index);
	void(*chat)(const UniChar* szText);
	void(*beginStroke)(NetUser* User, const Vec2* Point, const DrawTool* Tool, FrameData* FrameDat);
	void(*addToStroke)(NetUser* User, const Vec2* Point);
	void(*endStroke)(NetUser* User);
	void(*undoStroke)();
	void(*redoStroke)();
	void(*leave)();
} NetInterface;

extern NetInterface my_netint;