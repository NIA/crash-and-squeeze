#pragma once
#include "Parallel/abstract_task.h"

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
        public:
            TaskQueue(int max_size);

            void push(AbstractTask *task);
            AbstractTask * pop();

            // if the queue is empty it's `first' and `last' indices can be reset
            void reset();

            bool is_empty() { return first > last; }
            bool is_full() { return last == size - 1; }

            ~TaskQueue();
        };
    }
}
