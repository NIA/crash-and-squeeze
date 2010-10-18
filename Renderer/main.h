#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#undef ERROR // because WinGDI.h defines this and it's awful!
#include <cstdlib>
#include <ctime>
#include <crtdbg.h>
#include "Error.h"
#include <fstream>

#define BONES_COUNT 2

// a helper to release D3D interface if it is not NULL
inline void release_interface(IUnknown* iface)
{
    if( iface != NULL )
        iface->Release();
}

// a helper to call delete[] on pointer to an array(!) if it is not NULL
template<class Type> void delete_array(Type **dynamic_array)
{
    _ASSERT(dynamic_array != NULL);
    if( *dynamic_array != NULL)
    {
        delete[] *dynamic_array;
        *dynamic_array = NULL;
    }
}

// a helper to find out a size of an array defined with `array[]={...}' without doing `sizeof(array)/sizeof(array[0])'
template<size_t SIZE, class T> inline size_t array_size(T (&array)[SIZE])
{
    UNREFERENCED_PARAMETER(array);
    return SIZE;
}

class Logger
{
private:
    std::ofstream log_file;

public:
    Logger(const char * log_filename)
    {
        log_file.open(log_filename, std::ios::app);
    }

    void log(const char *prefix, const char * message, const char * file = "", int line = 0);

    ~Logger()
    {
        if(log_file.is_open())
            log_file.close();
    }

private:
    // No copying!
    Logger(const Logger &logger);
    Logger & operator=(const Logger &logger);
};