#pragma once
#include "main.h"
#include "Core/model.h"
#include "logger.h"

class WorkerThread
{
private:
    HANDLE handle;
    volatile bool stopped;
    ::CrashAndSqueeze::Core::Model * model;
    Logger *logger;
    
    void work();
public:
    WorkerThread();

    void start(::CrashAndSqueeze::Core::Model * model, Logger *logger);
    void stop() { stopped = true; }
    void wait(DWORD milliseconds = INFINITE);

    static DWORD WINAPI routine(void *param);
    
    ~WorkerThread();
};