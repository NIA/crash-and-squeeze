#pragma once
#include "main.h"
#include "Parallel/iprim_factory.h"

using CrashAndSqueeze::Parallel::ILock;
using CrashAndSqueeze::Parallel::IEvent;
using CrashAndSqueeze::Parallel::IEventSet;

class WinLock : public ILock
{
private:
    CRITICAL_SECTION cs;
public:
    WinLock() { InitializeCriticalSection(&cs); }

    void lock() { EnterCriticalSection(&cs); }
    void unlock() { LeaveCriticalSection(&cs); }

    ~WinLock() { DeleteCriticalSection(&cs); }
};

class WinFactory : public CrashAndSqueeze::Parallel::IPrimFactory
{
public:
    virtual ILock * create_lock();
    virtual void destroy_lock(ILock * lock) { delete lock; }

    virtual IEvent * create_event(bool initially_set);
    virtual void destroy_event(IEvent * event) { delete event; }
    
    virtual IEventSet * create_event_set(int size, bool initially_set);
    virtual void destroy_event_set(IEventSet * event_set) { delete event_set; }
};