#include "../threading.h"
#include <Windows.h>

typedef struct _OSThreadImpl
{
	HANDLE hThread;
	ThreadStartFn start;
	void* userdat;
} OSThread;

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
