#pragma once
#include "Parallel/abstract_task.h"
#include "Parallel/iprim_factory.h"

namespace CrashAndSqueeze
{
    namespace Parallel
    {
        // A queue (FIFO) of pointers to abstract tasks.
        // Only one thread pushes tasks to the queue, but they may
        // be popped from many worker threads.
        class TaskQueue
        {
        private:
            // array of AbstractTask * (in heap, but fixed size);
            AbstractTask ** tasks;
            int size;
            // index of first added item: it will be popped on the next call to pop()
            volatile int first;
            // index of last added item: the next pushed item will be stored after it
            volatile int last;
            
            // factory for creating lock objects
            IPrimFactory * prim_factory;
            // a lock object for limiting access to pop: only one thread pops at a time
            ILock * pop_lock;
            // an event signaling that queue has become empty
            IEvent * queue_emptied;
            volatile bool is_event_set;
        public:
            TaskQueue(int max_size, IPrimFactory * prim_factory);

            // this function should be called from a worker thread to obtain
            // the next task to be completed. Returns NULL if no task available.
            AbstractTask * pop();

            // this function is called from the main thread to add the task to the queue
            void push(AbstractTask *task);

            // this function is called from the main thread to mark all complete
            // tasks incomplete again, and return them back to the queue
            void reset();

            // remove tasks from queue without completing them
            void clear();
            
            // this function is called from the main thread to wait untill all tasks are completed
            void wait_till_emptied() { queue_emptied->wait(); }

            bool is_empty() { return first > last; }
            bool is_full() { return last == size - 1; }

            ~TaskQueue();
        };
    }
}
