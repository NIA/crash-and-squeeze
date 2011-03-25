#include "worker_thread.h"

WorkerThread::WorkerThread()
: handle(NULL), model(NULL) {}

void WorkerThread::start(CrashAndSqueeze::Core::Model *model)
{
    _ASSERT(NULL == handle);
    stopped = false;
	handle = CreateThread(NULL, 0, WorkerThread::routine, this, 0, NULL);
    if(NULL == handle)
        throw ThreadError();
    this->model = model;
}

DWORD WorkerThread::routine(void *param)
{
    WorkerThread * instance = reinterpret_cast<WorkerThread*>(param);
    instance->work();
    return 0;
}

void WorkerThread::work()
{
    _ASSERT(NULL != model);
    while(!stopped)
    {
        model->wait_for_tasks();
        
        bool has_more_tasks = true;
        while( !stopped && has_more_tasks)
        {
             has_more_tasks = model->complete_next_task();
        }
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
