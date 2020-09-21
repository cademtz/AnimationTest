#pragma once

typedef struct _OSThreadImpl* ThreadHandle;
typedef struct _OSMutexImpl* MutexHandle;
typedef int(*ThreadStartFn)(void* UserData);

// Threads

// - Creates a suspended thread
// - Start it at any time by calling Thread_Resume
extern ThreadHandle Thread_Create(ThreadStartFn StartFunc, void* opt_UserData);
extern void Thread_Resume(ThreadHandle Thread);
extern void Thread_Suspend(ThreadHandle Thread);
extern char Thread_Wait(ThreadHandle Thread, int* opt_outReturn);

// - Terminates the thread, setting its return to -1
// - Does NOT free the handle, see Thread_Destroy
extern void Thread_Terminate(ThreadHandle Thread); 

// - Gets the returned int (Typically exit code) of the thread
// - Returns 0 on failure, or if the thread has not yet exited
// - Must be used on a valid ThreadHandle before it has been destroyed
extern char Thread_GetReturn(ThreadHandle Thread, int* outReturn);

// - Frees the handle and ensures the thread is stopped or terminated
extern void Thread_Destroy(ThreadHandle Thread);

// Functions for the current thread

extern void Thread_Current_Sleep(unsigned int Milliseconds);
extern void Thread_Current_Exit(int ThreadReturn);

// Mutexes

extern MutexHandle Mutex_Create();
extern void Mutex_Destroy(MutexHandle Mutex);
extern void Mutex_Lock(MutexHandle Mutex);
extern void Mutex_Unlock(MutexHandle Mutex);
