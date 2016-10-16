#include "worker_thread.h"

namespace {
    const unsigned int DEFAULT_MAX_WAIT_MS = 1;
}

WorkerThread::WorkerThread()
    : handle(NULL), logger(NULL), max_wait_ms(DEFAULT_MAX_WAIT_MS) {}

void WorkerThread::start(::CrashAndSqueeze::Parallel::ITaskExecutor * executor, unsigned max_wait_ms, Logger *logger)
{
    _ASSERT(NULL == handle);
    this->stopped = false;
    this->max_wait_ms = max_wait_ms;
    this->task_executors.push_back(executor);
    this->executors_count = 1;
    this->logger = logger;
    handle = CreateThread(NULL, 0, WorkerThread::routine, this, 0, NULL);
    if (NULL == handle)
        throw ThreadError();
}

void WorkerThread::add_executor(::CrashAndSqueeze::Parallel::ITaskExecutor * executor)
{
    executors_lock.lock();
    task_executors.push_back(executor);
    ++executors_count;
    executors_lock.unlock();
}

DWORD WorkerThread::routine(void *param)
{
    WorkerThread * instance = reinterpret_cast<WorkerThread*>(param);
    return instance->work();
}

DWORD WorkerThread::work()
{
    _ASSERT(NULL != logger);
    while (!stopped)
    {
        for (int i = 0; i < executors_count; ++i)
        {
            executors_lock.lock();
            auto executor = task_executors[i];
            executors_lock.unlock();
            try
            {

                logger->add_message("Waiting for tasks...");
                bool tasks_ready = executor->wait_for_tasks(max_wait_ms);
                if (!tasks_ready) {
                    continue;
                }
                
                logger->add_message("...the wait is over");

                logger->add_message("Task >>started>>");
                bool has_more_tasks = executor->complete_next_task();
                if (has_more_tasks)
                {
                    logger->add_message("Task <<finished<<");
                }
                else
                {
                    logger->add_message("ALL Tasks ==finished==");
                }
            }
            catch (PhysicsError)
            {
                logger->add_message("ERROR while completing task, worker thread terminated!");
                executor->abort();
                return 1;
            }
            catch (DeadLockError)
            {
                logger->add_message("DEADLOCK detected, worker thread terminated!");
                return 2;
            }
        }
    }
    return 0;
}

void WorkerThread::wait(DWORD milliseconds)
{
    _ASSERT(NULL != handle);
    WaitForSingleObject(handle, milliseconds);
}

WorkerThread::~WorkerThread()
{
    if (handle != NULL)
        CloseHandle(handle);
}
