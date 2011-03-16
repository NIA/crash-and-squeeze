#pragma once

namespace CrashAndSqueeze
{
    namespace Parallel
    {
        class AbstractTask
        {
        private:
            volatile bool _is_complete;
        
        protected:
            // Custom implementation of task execution process
            virtual void execute() = 0;
            // Custom actions when task is reset
            virtual void on_reset() {}
        
        public:
            AbstractTask() : _is_complete(false) {}
            bool is_complete() { return _is_complete; }
            
            // This function should be called from a worker thread
            // in order to complete the task
            void complete() 
            {
                execute();
                _is_complete = true;
            }

            // This functions is called from the main thread to
            // specify that the task has to be completed again
            void reset()
            {
                _is_complete = false;
                on_reset();
            }

            virtual ~AbstractTask() {}
        };
    }
}
