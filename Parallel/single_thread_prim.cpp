#include "Parallel/single_thread_prim.h"
#include "Logging/logger.h"

namespace CrashAndSqueeze
{
    using Logging::Logger;

    namespace Parallel
    {
        SingleThreadFactory SingleThreadFactory::instance;

        void SingleThreadLock::lock()
        {
            if(is_locked)
                Logger::error("in SingleThreadLock::lock: unexpected attempt to lock already locked lock from the same thread!", __FILE__, __LINE__);
            else
                is_locked = true;
        }

        void SingleThreadEvent::wait()
        {
            if( ! is_set )
                Logger::error("in SingleThreadEvent::wait: unexpected attempt to wait for unset event from the same thread!", __FILE__, __LINE__);
        }

        SingleThreadEventSet::SingleThreadEventSet(int size, bool initially_set)
            : size(size)
        {
            are_set = new bool[size];
            
            for(int i = 0; i < size; ++i)
                are_set[i] = initially_set;
        }
        
        bool SingleThreadEventSet::check_index(int index)
        {
            if(index < 0 || index >= size)
            {
                Logger::error("in SingleThreadEventSet::check_index: incorrect index passed to set(int), unset(int) or wait(int)", __FILE__, __LINE__);
                return false;
            }
            return true;
        }

        void SingleThreadEventSet::set(int index)
        {
            if(check_index(index))
                are_set[index] = true;
        }

        void SingleThreadEventSet::unset(int index)
        {
            if(check_index(index))
                are_set[index] = false;
        }
        void SingleThreadEventSet::wait(int index)
        {
            if( ! are_set[index] )
            {
                Logger::error("in SingleThreadEventSet::wait(int): unexpected attempt to wait for unset event from the same thread!", __FILE__, __LINE__);
            }
        }

        void SingleThreadEventSet::set()
        {
            for(int i = 0; i < size; ++i)
                are_set[i] = true;
        }

        void SingleThreadEventSet::unset()
        {
            for(int i = 0; i < size; ++i)
                are_set[i] = false;
        }

        void SingleThreadEventSet::wait()
        {
            for(int i = 0; i < size; ++i)
            {
                if( ! are_set[i] )
                {
                    Logger::error("in SingleThreadEventSet::wait(): one of events is unset, but trying to wait for them from the same thread!", __FILE__, __LINE__);
                    return;
                }
            }
        }
    }
}
