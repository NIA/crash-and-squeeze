#include "worker_thread.h"

WorkerThread::WorkerThread()
    : handle(NULL), logger(NULL) {}

void WorkerThread::start(CrashAndSqueeze::Core::Model *model, Logger *logger)
{
    _ASSERT(NULL == handle);
    stopped = false;
    handle = CreateThread(NULL, 0, WorkerThread::routine, this, 0, NULL);
    if (NULL == handle)
        throw ThreadError();
    this->models.push_back(model);
    this->models_count = 1;
    this->logger = logger;
}

void WorkerThread::addModel(::CrashAndSqueeze::Core::Model * model)
{
    models_lock.lock();
    models.push_back(model);
    ++models_count;
    models_lock.unlock();
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
        for (int i = 0; i < models_count; ++i)
        {
            models_lock.lock();
            auto model = models[i];
            models_lock.unlock();
            try
            {

                logger->add_message("Waiting for tasks...");
                model->wait_for_tasks();
                logger->add_message("...the wait is over");

                bool has_more_tasks = true;
                while (!stopped && has_more_tasks)
                {
                    logger->add_message("Task >>started>>");
                    has_more_tasks = model->complete_next_task();
                    if (has_more_tasks)
                        logger->add_message("Task <<finished<<");
                    else
                        logger->add_message("ALL Tasks ==finished==");
                }
            }
            catch (PhysicsError)
            {
                logger->add_message("ERROR while completing task, worker thread terminated!");
                model->abort();
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
