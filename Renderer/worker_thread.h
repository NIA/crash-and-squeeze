#pragma once
#include "main.h"
#include "Core/model.h"

class WorkerThread
{
private:
    HANDLE handle;
    volatile bool stopped;
    ::CrashAndSqueeze::Core::Model * model;
public:
    WorkerThread();

    void start(::CrashAndSqueeze::Core::Model * model);
    void stop() { stopped = true; }
    void wait(DWORD milliseconds = INFINITE);

    static DWORD WINAPI routine(void *param);
    void work();
    
    ~WorkerThread();
};
