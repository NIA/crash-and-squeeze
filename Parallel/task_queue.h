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
            // an event object to notify when there are new tasks available
            IEvent * has_tasks_event;
        public:
            TaskQueue(int max_size, IPrimFactory * prim_factory);

            // this function should be called from a worker thread to obtain
            // the next task to be completed. Returns NULL if no task available.
            AbstractTask * pop();

            // this function is called from the main thread to add the task to the queue
            // NB: this function is not reentrant! It is assumed that pushing tasks happens only from one thread (but popping can be done from many threads).
            // If `set_event` is true, then `has_task_event` will be set after pushing. It is useful to pass false for all tasks except the last.
            void push(AbstractTask *task, bool set_event = true);

            // this function is called from the main thread to mark all complete
            // tasks incomplete again, and return them back to the queue
            void reset();

            // remove tasks from queue without completing them
            // TODO: avoid calling this function when all tasks are actually completed just to move `first` and `last` to the start.
            void clear();

            // wait until there are some tasks to pop
            void wait_for_tasks();

            bool is_empty() { return first > last; }
            bool is_full() { return last == size - 1; }

            ~TaskQueue();
        };
    }
}
