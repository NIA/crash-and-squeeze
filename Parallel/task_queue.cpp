#include "Parallel/task_queue.h"
#include "Logging/logger.h"

namespace CrashAndSqueeze
{
    using Logging::Logger;

    namespace Parallel
    {
        TaskQueue::TaskQueue(int max_size, IPrimFactory * prim_factory)
            : size(max_size), first(0), last(-1), prim_factory(prim_factory)
        {
            tasks = new AbstractTask*[size];
            pop_lock = prim_factory->create_lock();
            queue_emptied = prim_factory->create_event();
        }

        void TaskQueue::push(AbstractTask * task)
        {
            if( is_full())
            {
                Logger::error("in TaskQueue::push: queue is full", __FILE__, __LINE__);
                return;
            }
            // first store task, then increment index to make it accessible
            tasks[last+1] = task;
            ++last;
        }

        AbstractTask * TaskQueue::pop()
        {
            pop_lock->lock();

            AbstractTask * task;
            if( is_empty() )
            {
                task = NULL;
            }
            else
            {
                task = tasks[first++];

                if( is_empty() )
                {
                    queue_emptied->set();
                }
            }

            pop_lock->unlock();
            return task;
        }

        void TaskQueue::reset()
        {
            pop_lock->lock();
            if( !is_empty() )
            {
                Logger::error("in TaskQueue::reset: queue is not empty", __FILE__, __LINE__);
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
            prim_factory->destroy_lock(pop_lock);
            prim_factory->destroy_event(queue_emptied);
        }
    }
}
