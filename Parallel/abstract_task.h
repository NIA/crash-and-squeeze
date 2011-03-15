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
            // Implement task execution process in this method in your subclass
            virtual void execute() = 0;
        
        public:
            AbstractTask() : _is_complete(false) {}
            bool is_complete() { return _is_complete; }
            
            void complete() 
            {
                execute();
                _is_complete = true;
            }

            virtual ~AbstractTask() {}
        };
    }
}