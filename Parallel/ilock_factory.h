#pragma once
#include "Parallel/ilock.h"

namespace CrashAndSqueeze
{
    namespace Parallel
    {
        // An abstract factory for creating lock objects.
        class ILockFactory
        {
        public:
            virtual ILock * create_lock() = 0;
            virtual void destroy_lock(ILock * lock) = 0;
            
            virtual ~ILockFactory() {}
        };
    }
}