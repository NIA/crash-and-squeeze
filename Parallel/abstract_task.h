#pragma once

namespace CrashAndSqueeze
{
    namespace Parallel
    {
        class AbstractTask
        {
        public:
            // Custom implementation of task execution process
            virtual void execute() = 0;

            virtual ~AbstractTask() {}
        };
    }
}
