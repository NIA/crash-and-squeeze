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
        }

        void TaskQueue::push(AbstractTask * task)
        {
            if( is_full())
            {
                Logger::error("in TaskQueue::push: queue is full: it should be cleared after each step", __FILE__, __LINE__);
                return;
            }

            // first store task, then increment index to make it accessible
            tasks[last+1] = task;
            pop_lock->lock();
            ++last;
            pop_lock->unlock();
        }

        AbstractTask * TaskQueue::pop()
        {
            AbstractTask * task = NULL;

            pop_lock->lock();
            if( ! is_empty() )
            {
                task = tasks[first++];
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
            
            // reseting `first' index to 0 returns all popped items back...
            first = 0;
            
            // ...if there were any
            if( !is_empty() )
            {
                for(int i = first; i <= last; ++i)
                {
                    tasks[i]->reset();
                }
            }

            pop_lock->unlock();
        }

        void TaskQueue::clear()
        {
            pop_lock->lock();
            first = 0;
            last = -1;
            pop_lock->unlock();
        }

        TaskQueue::~TaskQueue()
        {
            delete[] tasks;
            prim_factory->destroy_lock(pop_lock);
        }
    }
}
