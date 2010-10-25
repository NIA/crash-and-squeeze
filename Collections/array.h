#pragma once
#include "Logging/logger.h"
#include <new>
#include <cstdlib>

namespace CrashAndSqueeze
{
    namespace Collections
    {
        template<class T> class Array
        {
        private:
            T *items;
            int items_num;
            int allocated_items_num;
            bool frozen;
            
            bool check_index(int index) const;
            bool check_not_frozen() const;
            bool reallocate(int new_allocated_num);

        public:
            static const int INITIAL_ALLOCATED = 100;

            Array(int initial_allocated = INITIAL_ALLOCATED);

            int size() const { return items_num; }

            // adds a new item to array and returns it
            T & create_item();

            // adds new_items_num new items to array
            void create_items(int new_items_num);

            // requires operator= to be defined for T;
            // if none, use create_item() instead of push_back()
            void push_back(T const & item);
            
            static const int ITEM_NOT_FOUND_INDEX = -1;
            typedef bool (* CompareFunc)(T const &a, T const &b);

            // returns index of first item in array, equal to `item`
            // according to given comparing function;
            // returns -1 if there is none
            int index_of(T const & item, CompareFunc compare) const;

            // returns first item in array, equal to `item`
            // according to given comparing function;
            // adds it to array and returns it, if there is none
            T & find_or_add(T const & item, CompareFunc compare);

            T & operator[](int index);
            const T & operator[](int index) const;

            // after call to this function array cannot grow anymore
            void freeze() { frozen = true; }
            bool is_frozen() { return frozen; }

            virtual ~Array();
        private:
            // No copying!
            Array(const Array<T> &);
            Array<T> & operator=(const Array<T> &);
        };

        // -- I M P L E M E N T A T I O N --

        template<class T>
        bool Array<T>::check_index(int index) const
        {
            if(index < 0 || index >= items_num)
            {
                Logging::Logger::error("Collections::Array index out of range");
                return false;
            }
            return true;
        }

        template<class T>
        bool Array<T>::check_not_frozen() const
        {
            if(frozen)
            {
                Logging::Logger::error("Collections::Array is frozen, cannot add new item(s)");
                return false;
            }
            return true;
        }

        template<class T>
        Array<T>::Array(int initial_allocated = INITIAL_ALLOCATED)
            : items(NULL), items_num(0), frozen(false), allocated_items_num(initial_allocated)
        {
            if( initial_allocated < 0 )
            {
                Logging::Logger::error("creating Collections::Array with initial_allocated < 0");
            }
            else
            {
                if( initial_allocated > 0 )
                {
                    items = reinterpret_cast<T*>( malloc( sizeof(items[0])*allocated_items_num ) );
                    
                    if(NULL == items)
                    {
                        Logging::Logger::error("creating Collections::Array: not enough memory!");
                    }
                }
            }
        }
        
        template<class T>
        bool Array<T>::reallocate(int new_allocated_num)
        {
            if( new_allocated_num <= allocated_items_num )
                return true;

            T *new_items = reinterpret_cast<T*>( realloc(items, sizeof(items[0])*new_allocated_num) );
            
            // if realloc failed
            if(NULL == new_items)
            {
                Logging::Logger::error("in Collections::Array::reallocate: not enough memory!");
                return false;
            }

            items = new_items;
            allocated_items_num = new_allocated_num;
            return true;
        }

        template<class T>
        T & Array<T>::create_item()
        {
            if( false == check_not_frozen() )
                return items[items_num - 1];

            if( items_num == allocated_items_num )
            {
                int new_allocated_num = (0 == allocated_items_num) ? INITIAL_ALLOCATED : allocated_items_num*2;
                if( false == reallocate(new_allocated_num) )
                    return items[items_num - 1];
            }

            ++items_num;
            T &new_item = items[items_num - 1];
            // construct it in its place
            new (&new_item) T;
            return new_item;
        }

        template<class T>
        void Array<T>::create_items(int new_items_num)
        {
            if( false == check_not_frozen() )
                return;

            if(new_items_num <= 0)
                return;
            
            int new_size = items_num + new_items_num;
            
            if( new_size > allocated_items_num )
            {
                if( false == reallocate(new_size) )
                    return;
            }
            
            // construct new items
            for(int i = items_num; i < new_size; ++i)
                new (&items[i]) T;
            items_num = new_size;
        }

        template<class T>
        void Array<T>::push_back(T const & item)
        {
            create_item() = item;
        }

        template<class T>
        int Array<T>::index_of(T const & item, CompareFunc compare) const
        {
            for(int i = 0; i < items_num; ++i)
            {
                if(compare(item, items[i]))
                    return i;
            }
            return ITEM_NOT_FOUND_INDEX;
        }

        template<class T>
        T & Array<T>::find_or_add(T const & item, CompareFunc compare)
        {
            int index = index_of(item, compare);
            if(ITEM_NOT_FOUND_INDEX == index)
            {
                index = items_num;
                push_back(item);
            }
            return items[index];
        }

        template<class T>
        T & Array<T>::operator[](int index)
        {
            if(check_index(index))
                return items[index];
            else
                return items[0];
        }
        
        template<class T>
        const T & Array<T>::operator[](int index) const
        {
            if(check_index(index))
                return items[index];
            else
                return items[0];
        }

        template<class T>
        Array<T>::~Array()
        {
            for(int i = 0; i < items_num; ++i)
            {
                items[i].~T();
            }
            free(items);
        }
    }
}
