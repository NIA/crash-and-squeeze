#pragma once

namespace CrashAndSqueeze
{
    namespace Parallel
    {
        // An abstract event, a synchronization primitive for notifying
        // a waiting thread. To provide custom functionality, implement
        // a subclass of IEvent and a subclass of IPrimitiveFactory that
        // creates instances of your IEvent implementation. Note that
        // you may use your implementation of IEventSet as IEvent too,
        // or you can make separate efficient implementation of IEvent.
        class IEvent
        {
        public:
            virtual void set() = 0;
            virtual void unset() = 0;

            virtual void wait() = 0;
            // waits for a given amount of time, returns true if event happened and false - if time elapsed, but event did not happen
            virtual bool wait_for(unsigned milliseconds) = 0;

            virtual ~IEvent() {}
        };

        // A set of events, which may be set/unset either separately or
        // together. Waiting also may be done for all events or just
        // for one of them. To provide custom functionality, implement
        // a subclass of IEventSet and a subclass of IPrimitiveFactory that
        // creates instances of your IEvent implementation.
        class IEventSet : public IEvent
        {
        public:
            virtual void set(int index) = 0;
            virtual void unset(int index) = 0;
            virtual void wait(int index) = 0;
            // waits for a given amount of time, returns true if event happened and false - if time elapsed, but event did not happen
            virtual bool wait_for(int index, unsigned milliseconds) = 0;
            
            virtual void set() = 0;
            virtual void unset() = 0;
            virtual void wait() = 0;
            // waits for a given amount of time, returns true if event happened and false - if time elapsed, but event did not happen
            virtual bool wait_for(unsigned milliseconds) = 0;
        };
    }
}