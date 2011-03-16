#pragma once

namespace CrashAndSqueeze
{
    namespace Parallel
    {
        // An abstract locking synchronization primitive. To provide custom
        // functionality, implement a subclass of ILock and a subclass of
        // IPrimFactory that creates instances of your ILock implementation.
        class ILock
        {
        public:
            virtual void lock() = 0;
            virtual void unlock() = 0;

            virtual ~ILock() {}
        };
    }
}