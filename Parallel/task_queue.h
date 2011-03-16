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
            int first;
            // index of last added item: the next pushed item will be stored after it
            int last;
            
            // factory for creating lock objects
            IPrimFactory * prim_factory;
            // a lock object for limiting access to pop: only one thread pops at a time
            ILock * pop_lock;
            // an event signaling that queue has become empty
            IEvent * queue_emptied;
        public:
            TaskQueue(int max_size, IPrimFactory * prim_factory);

            void push(AbstractTask *task);
            // returns NULL if no task available
            AbstractTask * pop();

            void wait_till_emptied() { queue_emptied->wait(); }

            // if the queue is empty it's `first' and `last' indices can be reset
            void reset();

            bool is_empty() { return first > last; }
            bool is_full() { return last == size - 1; }

            ~TaskQueue();
        };
    }
}
