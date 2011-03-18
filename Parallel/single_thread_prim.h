#pragma once
#include "Parallel/iprim_factory.h"
#include "Logging/logger.h"

//
// Defined here are synchronization primitives that doesn't do
// any interthread synchronization and are supposed to be used
// in a single-threaded application.
//

namespace CrashAndSqueeze
{
    namespace Parallel
    {
        // This implementation of ILock reports a failure if locked twice.
        // Good for detecting deadlocks inside a single thread.
        class SingleThreadLock : public ILock
        {
        private:
            bool is_locked;
        public:
            SingleThreadLock() : is_locked(false) {}
            virtual void lock()
            {
                if(is_locked)
                    Logging::Logger::error("in SingleThreadLock::lock: unexpected attempt to lock already locked lock from the same thread!", __FILE__, __LINE__);
                else
                    is_locked = true;
            }
            virtual void unlock() { is_locked = false; }
        };

        // This implementation of IEvent reports a failure if wait()
        // is called when the event not set, because in a single-threaded
        // application it will be a "deadlock"
        class SingleThreadEvent : public IEvent
        {
        private:
            bool is_set;
        public:
            SingleThreadEvent(bool initially_set) : is_set(initially_set) {}
            
            virtual void set()   { is_set = true;  }
            virtual void unset() { is_set = false; }

            virtual void wait()
            {
                if( ! is_set )
                    Logging::Logger::error("in SingleThreadEvent::wait: unexpected attempt to wait for unset event from the same thread!", __FILE__, __LINE__);
            }
        };

        // A factory for these primitives which allocates them dynamically in heap
        class SingleThreadFactory : public IPrimFactory
        {
        public:
            virtual ILock * create_lock() { return new SingleThreadLock(); }
            virtual void destroy_lock(ILock * lock) { delete lock; }
            
            virtual IEvent * create_event(bool initially_set) { return new SingleThreadEvent(initially_set); }
            virtual void destroy_event(IEvent * event) { delete event; }
        };
    }
}
