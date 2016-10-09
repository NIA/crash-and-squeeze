#pragma once
namespace CrashAndSqueeze
{
    namespace Parallel
    {
        // Common interface for anything that contains tasks and knows how to execute them, being called from worker threads
        class ITaskExecutor {
        public:
            // waits until a task is available
            virtual void wait_for_tasks() = 0;
            
            // wait for a given time until a task is available (returns true if the task become available, false if time elapsed)
            virtual bool wait_for_tasks(unsigned milliseconds) = 0;

            // executes next task in caller thread, returns true if there is more
            virtual bool complete_next_task() = 0;

            // aborts all tasks
            virtual void abort() = 0;
        };
    }
}