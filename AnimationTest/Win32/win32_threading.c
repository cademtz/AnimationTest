#include "../threading.h"
#include <Windows.h>
#include <stdio.h>

//#define DEBUG_MUTEX

#ifdef DEBUG_MUTEX
#define DEBUG_MUTEX_PRINT(fmtstr, ...) printf(fmtstr, __VA_ARGS__)
#include <intrin.h>
#else
#define DEBUG_MUTEX_PRINT(fmtstr, ...)
#endif

typedef struct _OSThreadImpl
{
	HANDLE hThread;
	ThreadStartFn start;
	void* userdat;
} OSThread;

typedef struct _OSMutexImpl {
	int unused;
};

DWORD WINAPI DispatchThread(OSThread* Thread) {
	return Thread->start(Thread->userdat);
}

ThreadHandle Thread_Create(ThreadStartFn StartFunc, void* opt_UserData)
{
	OSThread* thread = (OSThread*)malloc(sizeof(*thread));
	ZeroMemory(thread, sizeof(*thread));

	thread->start = StartFunc, thread->userdat = opt_UserData;
	thread->hThread = CreateThread(0, 0, &DispatchThread, thread, CREATE_SUSPENDED, 0);
	return thread;
}

void Thread_Resume(ThreadHandle Thread) {
	ResumeThread(Thread->hThread);
}

void Thread_Suspend(ThreadHandle Thread) {
	SuspendThread(Thread->hThread);
}

char Thread_Wait(ThreadHandle Thread, int* opt_outReturn)
{
	WaitForSingleObject(Thread->hThread, INFINITE);
	if (opt_outReturn)
		return Thread_GetReturn(Thread, opt_outReturn);
	return 1;
}

char Thread_IsAlive(ThreadHandle Thread) {
	return WaitForSingleObject(Thread->hThread, 0) != WAIT_OBJECT_0;
}

void Thread_Terminate(ThreadHandle Thread) {
	TerminateThread(Thread->hThread, -1);
}

char Thread_GetReturn(ThreadHandle Thread, int* outReturn)
{
	DWORD code;
	if (GetExitCodeThread(Thread->hThread, &code))
	{
		outReturn = code;
		return 1;
	}
	return 0;
}

void Thread_Destroy(ThreadHandle Thread)
{
	TerminateThread(Thread->hThread, -1);
	CloseHandle(Thread->hThread);
	free(Thread);
}

void Thread_Current_Sleep(unsigned int Milliseconds) {
	Sleep(Milliseconds);
}

void Thread_Current_Exit(int ThreadReturn) {
	ExitThread(ThreadReturn);
}

MutexHandle Mutex_Create() {
	return (MutexHandle)CreateMutex(0, 0, 0);
}

void Mutex_Destroy(MutexHandle Mutex) {
	CloseHandle((HANDLE)Mutex);
}

void Mutex_Lock(MutexHandle Mutex)
{
#ifdef DEBUG_MUTEX
	// Pasted stack overflow solution :)

	typedef USHORT(WINAPI* CaptureStackBackTraceType)(__in ULONG, __in ULONG, __out PVOID*, __out_opt PULONG);
	CaptureStackBackTraceType func = (CaptureStackBackTraceType)(GetProcAddress(LoadLibrary(L"kernel32.dll"), "RtlCaptureStackBackTrace"));

	if (func == NULL)
		return; // WOE 29.SEP.2010

	// Quote from Microsoft Documentation:
	// ## Windows Server 2003 and Windows XP:  
	// ## The sum of the FramesToSkip and FramesToCapture parameters must be less than 63.
	const int kMaxCallers = 3;

	void* callers[3];
	int count = (func)(0, kMaxCallers, callers, NULL);

	DEBUG_MUTEX_PRINT("Locking Mutex\t%p at %p %p\n", Mutex, callers[1], callers[2]);
#endif
	DWORD result = WaitForSingleObject((HANDLE)Mutex, INFINITE);
	if (result != WAIT_OBJECT_0)
	{
		printf("[ERROR] 0x%X, Failed to lock Mutex %p\n", result, Mutex);
		DebugBreak();
	}
	else
		DEBUG_MUTEX_PRINT("Lockedd Mutex\t%p\n", Mutex);
}

void Mutex_Unlock(MutexHandle Mutex)
{
	if (!ReleaseMutex((HANDLE)Mutex))
	{
		DWORD code = GetLastError();
		char buf[256];
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, 0, code, 0, &buf, sizeof(buf), 0);
		printf("[ERROR] 0x%X, Failed to release mutex %p: %s\n", code, Mutex, buf);
		DebugBreak();
	}
	else
		DEBUG_MUTEX_PRINT("Unlockd Mutex\t%p\n", Mutex);
}
