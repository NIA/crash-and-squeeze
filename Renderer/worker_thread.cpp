#include "worker_thread.h"

WorkerThread::WorkerThread()
: handle(NULL), model(NULL), logger(NULL) {}

void WorkerThread::start(CrashAndSqueeze::Core::Model *model, Logger *logger, int id)
{
    _ASSERT(NULL == handle);
    stopped = false;
    this->model = model;
    this->logger = logger;
    this->id = id;
    handle = CreateThread(NULL, 0, WorkerThread::routine, this, 0, NULL);
    if(NULL == handle)
    {
        throw ThreadError();
    }
    
    int affinity_mask = 1 << (id - 1);
    if(NULL == SetThreadAffinityMask(handle, affinity_mask))
    {
        throw AffinityError();
    }
}

DWORD WorkerThread::routine(void *param)
{
    WorkerThread * instance = reinterpret_cast<WorkerThread*>(param);
    return instance->work();
}

DWORD WorkerThread::work()
{
    try
    {
        _ASSERT(NULL != model);
        _ASSERT(NULL != logger);
        while(!stopped)
        {
            logger->add_message("Waiting for tasks...", id);
            model->wait_for_tasks();
            logger->add_message("...the wait is over", id);
            
            bool has_more_tasks = true;
            while( !stopped && has_more_tasks)
            {
                logger->add_message("Task >>started>>", id);
                has_more_tasks = model->complete_next_task();
                if(has_more_tasks)
                    logger->add_message("Task <<finished<<", id);
                else
                    logger->add_message("): No more tasks :(", id);
            }
        }
        return 0;
    }
    catch(PhysicsError)
    {
        logger->add_message("ERROR while completing task, worker thread terminated!");
        model->abort();
        return 1;
    }
    catch(DeadLockError)
    {
        logger->add_message("DEADLOCK detected, worker thread terminated!");
        return 2;
    }
}

void WorkerThread::wait(DWORD milliseconds)
{
    _ASSERT(NULL != handle);
    WaitForSingleObject(handle, milliseconds);
}

WorkerThread::~WorkerThread()
{
	if( handle != NULL)
	    CloseHandle( handle );
}
