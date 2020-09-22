#include "../threading.h"
#include <Windows.h>
#include <stdio.h>

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
	DWORD result = WaitForSingleObject((HANDLE)Mutex, INFINITE);
	if (result != WAIT_OBJECT_0)
	{
		printf("[ERROR] 0x%X, Failed to lock Mutex %p\n", result, Mutex);
		DebugBreak();
	}
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
}
