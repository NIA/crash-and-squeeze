#include "Parallel/task_queue.h"
#include "Logging/logger.h"

namespace CrashAndSqueeze
{
    using Logging::Logger;

    namespace Parallel
    {
        TaskQueue::TaskQueue(int max_size, ILockFactory * lock_factory)
            : size(max_size), first(0), last(-1), lock_factory(lock_factory)
        {
            tasks = new AbstractTask*[size];
            pop_lock = lock_factory->create_lock();
        }

        void TaskQueue::push(AbstractTask * task)
        {
            if( is_full())
            {
                Logger::error("in TaskQueue::push: queue is full", __FILE__, __LINE__);
                return;
            }
            tasks[++last] = task;
        }

        AbstractTask * TaskQueue::pop()
        {
            pop_lock->lock();
            if( is_empty() )
            {
                Logger::error("in TaskQueue::pop: queue is empty", __FILE__, __LINE__);
                pop_lock->unlock();
                return NULL;
            }
            pop_lock->unlock();
            return tasks[first++];
        }

        void TaskQueue::reset()
        {
            pop_lock->lock();
            if( !is_empty() )
            {
                Logger::error("in TaskQueue::push: queue is full", __FILE__, __LINE__);
                pop_lock->unlock();
                return;
            }
            first = 0;
            last = -1;
            pop_lock->unlock();
        }

        TaskQueue::~TaskQueue()
        {
            delete[] tasks;
            lock_factory->destroy_lock(pop_lock);
        }
    }
}
