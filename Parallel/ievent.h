#pragma once

namespace CrashAndSqueeze
{
    namespace Parallel
    {
        // An abstract event, a synchronization primitive for notifying
        // a waiting thread. To provide custom functionality, implement
        // a subclass of IEvent and a subclass of IPrimitiveFactory that
        // creates instances of your IEvent implementation.
        class IEvent
        {
        public:
            virtual void set() = 0;
            virtual void unset() = 0;

            virtual void wait() = 0;

            virtual ~IEvent() {}
        };
    }
}