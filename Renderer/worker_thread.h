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
    int id;
    int tasks_completed;
    
    DWORD work();
public:
    WorkerThread();

    void start(::CrashAndSqueeze::Core::Model * model, Logger *logger, int id);
    void stop() { stopped = true; }
    void wait(DWORD milliseconds = INFINITE);
    int get_tasks_completed() { return tasks_completed; }

    static DWORD WINAPI routine(void *param);
    
    ~WorkerThread();
};
