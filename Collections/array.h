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

            bool check_index(int index) const
            {
                if(index < 0 || index >= items_num)
                {
                    logger.error("Collections::Array index out of range");
                    return false;
                }
                return true;
            }

        public:
            static const int INITIAL_ALLOCATED = 100;

            Array(int initial_allocated = INITIAL_ALLOCATED)
                : items(NULL), items_num(0), allocated_items_num(initial_allocated)
            {
                items = new T[allocated_items_num];
            }

            int size() const
            {
                return items_num;
            }

            void push_back(T const & item)
            {
                if( items_num == allocated_items_num )
                {
                    allocated_items_num *= 2;
                    T *new_items = new T[allocated_items_num];
                    memcpy(new_items, items, items_num*sizeof(items[0]));
                    delete[] items;
                    items = new_items;
                }

                items[items_num] = item;
                ++items_num;
            }

            T & operator[](int index)
            {
                if(check_index(index))
                    return items[index];
                else
                    return items[0];
            }
            
            const T & operator[](int index) const
            {
                if(check_index(index))
                    return items[index];
                else
                    return items[0];
            }

            ~Array()
            {
                delete[] items;
            }
        };
    }
}
