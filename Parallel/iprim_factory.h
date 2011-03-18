#pragma once
#include "Parallel/ilock.h"
#include "Parallel/ievent.h"

namespace CrashAndSqueeze
{
    namespace Parallel
    {
        // An abstract factory for creating synchronization primitives
        class IPrimFactory
        {
        public:
            virtual ILock * create_lock() = 0;
            virtual void destroy_lock(ILock * lock) = 0;

            virtual IEvent * create_event(bool initially_set) = 0;
            virtual void destroy_event(IEvent * event) = 0;
            
            virtual IEventSet * create_event_set(int size, bool initially_set) = 0;
            virtual void destroy_event_set(IEventSet * event_set) = 0;

            virtual ~IPrimFactory() {}
        };
    }
}