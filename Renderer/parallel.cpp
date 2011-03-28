#include "parallel.h"

namespace
{
    const int MAX_WAIT = 2000;  // 2 sec

    class WinEventSet : public IEventSet
    {
    private:
        HANDLE *events;
        int size;

        void check_index(int index)
        {
            if(index < 0 || index >= size)
                throw OutOfRangeError();
        }
    public:
        WinEventSet(int size, bool initially_set)
            : size(size)
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
            if( WAIT_TIMEOUT == WaitForSingleObject(events[index], MAX_WAIT) )
                throw DeadLockError();
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
            if( WAIT_TIMEOUT == WaitForMultipleObjects(size, events, TRUE, MAX_WAIT) )
                throw DeadLockError();
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

ILock * WinFactory::create_lock()
{
    return new WinLock();
}

IEvent * WinFactory::create_event(bool initially_set)
{
    return new WinEventSet(1, initially_set);
}

IEventSet * WinFactory::create_event_set(int size, bool initially_set)
{
    return new WinEventSet(size, initially_set);
}
