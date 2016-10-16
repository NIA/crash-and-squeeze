#pragma once
#include "Parallel/iprim_factory.h"

//
// Defined here are synchronization primitives that doesn't do
// any interthread synchronization and are supposed to be used
// in a single-threaded application.
//

// TODO: separate tests for them

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
            virtual void lock();
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

            virtual void wait();
            // waits for a given amount of time, returns true if event happened and false - if time elapsed, but event did not happen
            virtual bool wait_for(unsigned milliseconds);
        };

        class SingleThreadEventSet : public IEventSet
        {
        private:
            int size;
            bool *are_set;
            bool check_index(int index);
        public:
            SingleThreadEventSet(int size, bool initially_set);
            virtual void set(int index);
            virtual void unset(int index);
            virtual void wait(int index);
            // waits for a given amount of time, returns true if event happened and false - if time elapsed, but event did not happen
            virtual bool wait_for(int index, unsigned milliseconds);
            virtual void set();
            virtual void unset();
            virtual void wait();
            // waits for a given amount of time, returns true if event happened and false - if time elapsed, but event did not happen
            virtual bool wait_for(unsigned milliseconds);
        };

        // A factory for these primitives which allocates them dynamically in heap
        class SingleThreadFactory : public IPrimFactory
        {
        public:
            virtual ILock * create_lock() { return new SingleThreadLock(); }
            virtual void destroy_lock(ILock * lock) { delete lock; }
            
            virtual IEvent * create_event(bool initially_set) { return new SingleThreadEvent(initially_set); }
            virtual void destroy_event(IEvent * event) { delete event; }
            
            virtual IEventSet * create_event_set(int size, bool initially_set) { return new SingleThreadEventSet(size, initially_set); }
            virtual void destroy_event_set(IEventSet * event_set) { delete event_set; }

            static SingleThreadFactory instance;
        };
    }
}
