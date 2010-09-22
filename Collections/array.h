#pragma once
#include "Logging/logger.h"

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
            
            bool check_index(int index) const;

        public:
            static const int INITIAL_ALLOCATED = 100;

            Array(int initial_allocated = INITIAL_ALLOCATED);

            int size() const { return items_num; }

            // adds a new item to array and returns it
            T & create_item();

            // requires operator= to be defined for T;
            // if none, use create_new() instead of push_back()
            void push_back(T const & item);
            
            static const int ITEM_NOT_FOUND_INDEX = -1;
            typedef bool (* CompareFunc)(T const &a, T const &b);

            // returns index of first item in array, equal to `item`
            // according to given comparing function;
            // returns -1 if there is none
            int index_of(T const & item, CompareFunc compare) const;

            // returns index of first item in array, equal to `item`
            // according to given comparing function;
            // adds it to array and returns its index, if there is none
            int find_or_add(T const & item, CompareFunc compare);

            T & operator[](int index);
            const T & operator[](int index) const;

            virtual ~Array() { delete[] items; }
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
                logger.error("Collections::Array index out of range");
                return false;
            }
            return true;
        }

        template<class T>
        Array<T>::Array(int initial_allocated = INITIAL_ALLOCATED)
            : items(NULL), items_num(0), allocated_items_num(initial_allocated)
        {
            if( initial_allocated < 0 )
            {
                logger.error("creating Collections::Array with initial_allocated < 0");
            }
            else
            {
                if( initial_allocated > 0 )
                    items = new T[allocated_items_num];
            }
        }

        template<class T>
        T & Array<T>::create_item()
        {
            if( items_num == allocated_items_num )
            {
                if(0 == allocated_items_num)
                    allocated_items_num = INITIAL_ALLOCATED;
                else
                    allocated_items_num *= 2;
                
                T *new_items = new T[allocated_items_num];
                
                // this can be only if allocated_items_num was 0
                if(0 != items)
                {
                    memcpy(new_items, items, items_num*sizeof(items[0]));
                    delete[] items;
                }
                
                items = new_items;
            }

            ++items_num;
            return items[items_num - 1];
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
        int Array<T>::find_or_add(T const & item, CompareFunc compare)
        {
            int index = index_of(item, compare);
            if(ITEM_NOT_FOUND_INDEX == index)
            {
                index = items_num;
                push_back(item);
            }
            return index;
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
    }
}
