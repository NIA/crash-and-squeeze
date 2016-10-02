#pragma once
#include "main.h"
#include "Core/model.h"
#include "logger.h"
#include "parallel.h"
#include <vector>

class WorkerThread
{
private:
    HANDLE handle;
    volatile bool stopped;

    std::vector<::CrashAndSqueeze::Core::Model *> models;
    WinLock models_lock;
    volatile int models_count;

    Logger *logger;
    
    DWORD work();

public:
    WorkerThread();

    void start(::CrashAndSqueeze::Core::Model * model, Logger *logger);
    void addModel(::CrashAndSqueeze::Core::Model * model);
    void stop() { stopped = true; }
    bool is_started() { return !stopped; }
    void wait(DWORD milliseconds = INFINITE);

    static DWORD WINAPI routine(void *param);
    
    ~WorkerThread();
};
