#include "Parallel/task_queue.h"
#include "Logging/logger.h"

namespace CrashAndSqueeze
{
    using Logging::Logger;

    namespace Parallel
    {
        TaskQueue::TaskQueue(int max_size, IPrimFactory * prim_factory)
            : size(max_size), first(0), last(-1), prim_factory(prim_factory), is_event_set(false)
        {
            tasks = new AbstractTask*[size];
            pop_lock = prim_factory->create_lock();
            queue_emptied = prim_factory->create_event(false);
        }

        void TaskQueue::push(AbstractTask * task)
        {
            if( is_full())
            {
                Logger::error("in TaskQueue::push: queue is full", __FILE__, __LINE__);
                return;
            }
            // clear queue_emptied event, if it is set yet
            if(is_event_set)
            {
                is_event_set = false;
                queue_emptied->unset();
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
                    is_event_set = true;
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

        TaskQueue::~TaskQueue()
        {
            delete[] tasks;
            prim_factory->destroy_lock(pop_lock);
            prim_factory->destroy_event(queue_emptied);
        }
    }
}
