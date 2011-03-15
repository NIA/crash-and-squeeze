#pragma once

namespace CrashAndSqueeze
{
    namespace Parallel
    {
        // An abstract locking primitive.
        // To provide custom functionality, implement a subclass of ILock
        // and a subclass of ILockFactory that creates instances
        // of your ILock implementation.
        class ILock
        {
        public:
            virtual void lock() = 0;
            virtual void unlock() = 0;

            virtual ~ILock() {}
        };
    }
}