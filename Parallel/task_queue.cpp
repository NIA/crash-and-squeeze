#include "Parallel/task_queue.h"
#include "Logging/logger.h"

namespace CrashAndSqueeze
{
    using Logging::Logger;

    namespace Parallel
    {
        TaskQueue::TaskQueue(int max_size)
            : size(max_size), first(0), last(-1)
        {
            tasks = new AbstractTask*[size];
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
            if( is_empty() )
            {
                Logger::error("in TaskQueue::pop: queue is empty", __FILE__, __LINE__);
                return NULL;
            }
            return tasks[first++];
        }

        void TaskQueue::reset()
        {
            if( !is_empty() )
            {
                Logger::error("in TaskQueue::push: queue is full", __FILE__, __LINE__);
                return;
            }
            first = 0;
            last = -1;
        }

        TaskQueue::~TaskQueue()
        {
            delete[] tasks;
        }
    }
}
