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
            has_tasks_event = prim_factory->create_event(false);
        }

        void TaskQueue::push(AbstractTask * task, bool set_event /*= true*/)
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

            if (set_event)
            {
                has_tasks_event->set();
            }
            pop_lock->unlock();
        }

        AbstractTask * TaskQueue::pop()
        {
            AbstractTask * task = NULL;

            pop_lock->lock();
            if( ! is_empty() )
            {
                task = tasks[first++];

                if (is_empty())
                {
                    // if it was the las task => call clear to reset queue to its initial state with pointers at the beginning
                    clear_unsafe();
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
            has_tasks_event->set();

            pop_lock->unlock();
        }

        void TaskQueue::clear()
        {
            pop_lock->lock();
            clear_unsafe();
            pop_lock->unlock();
        }

        // internal version without locking
        void TaskQueue::clear_unsafe()
        {
            // To make the queue empty:
            // 1) move pointers back to the beginning
            first = 0;
            last = -1;
            // 2) unset notification
            has_tasks_event->unset();
        }

        void TaskQueue::wait_for_tasks()
        {
            has_tasks_event->wait();
        }

        TaskQueue::~TaskQueue()
        {
            has_tasks_event->unset();
            delete[] tasks;
            prim_factory->destroy_lock(pop_lock);
            prim_factory->destroy_event(has_tasks_event);
        }
    }
}
