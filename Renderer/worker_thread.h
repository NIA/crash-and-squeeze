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
    unsigned max_wait_ms; // how long to wait for one executor before trying another one

    std::vector<::CrashAndSqueeze::Parallel::ITaskExecutor*> task_executors;
    WinLock executors_lock;
    volatile int executors_count;

    Logger *logger;
    
    DWORD work();

public:
    WorkerThread();

    void start(::CrashAndSqueeze::Parallel::ITaskExecutor * executor, unsigned max_wait_ms, Logger *logger);
    void add_executor(::CrashAndSqueeze::Parallel::ITaskExecutor * executor);
    void stop() { stopped = true; }
    bool is_started() { return !stopped; }
    void wait(DWORD milliseconds = INFINITE);

    static DWORD WINAPI routine(void *param);
    
    ~WorkerThread();
};
