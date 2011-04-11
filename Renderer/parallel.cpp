#include "parallel.h"

namespace
{
    const int MAX_WAIT = 2000;  // 2 sec

    class WinEventSet : public IEventSet
    {
    private:
        HANDLE *events;
        int size;
        bool detect_dead_locks;

        void check_index(int index)
        {
            if(index < 0 || index >= size)
                throw OutOfRangeError();
        }
    public:
        WinEventSet(int size, bool initially_set, bool detect_dead_locks)
            : size(size), detect_dead_locks(detect_dead_locks)
        {
            events = new HANDLE[size];
            for(int i = 0; i < size; ++i)
            {
                events[i] = CreateEvent(NULL, TRUE, initially_set, NULL);
            }
        }

        virtual void set(int index)
        {
            check_index(index);
            SetEvent(events[index]);
        }

        virtual void unset(int index)
        {
            check_index(index);
            ResetEvent(events[index]);
        }

        virtual void wait(int index)
        {
            check_index(index);
            if( detect_dead_locks )
            {
                if( WAIT_TIMEOUT == WaitForSingleObject(events[index], MAX_WAIT) )
                    throw DeadLockError();
            }
            else
            {
                WaitForSingleObject(events[index], INFINITE);
            }
        }

        virtual void set()
        {
            for(int i = 0; i < size; ++i)
            {
                SetEvent(events[i]);
            }
        }

        virtual void unset()
        {
            for(int i = 0; i < size; ++i)
            {
                ResetEvent(events[i]);
            }
        }
        
        virtual void wait()
        {
            if( detect_dead_locks )
            {
                if( WAIT_TIMEOUT == WaitForMultipleObjects(size, events, TRUE, MAX_WAIT) )
                    throw DeadLockError();
            }
            else
            {
                WaitForMultipleObjects(size, events, TRUE, INFINITE);
            }
        }

        ~WinEventSet()
        {
            for(int i = 0; i < size; ++i)
            {
                CloseHandle(events[i]);
            }
            delete[] events;
        }
    };
}

// -- Factory --
WinFactory::WinFactory(bool detect_dead_locks)
: detect_dead_locks(detect_dead_locks)
{
}

ILock * WinFactory::create_lock()
{
    return new WinLock();
}

IEvent * WinFactory::create_event(bool initially_set)
{
    return new WinEventSet(1, initially_set, detect_dead_locks);
}

IEventSet * WinFactory::create_event_set(int size, bool initially_set)
{
    return new WinEventSet(size, initially_set, detect_dead_locks);
}
