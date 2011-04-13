#include "Stopwatch.h"

Stopwatch::Stopwatch()
{
    LARGE_INTEGER large_int;
    QueryPerformanceFrequency(&large_int);
    
    frequency = static_cast<double>(large_int.QuadPart);
    if(0 == frequency)
        throw PerformanceFrequencyError();
}

void Stopwatch::start()
{
    QueryPerformanceCounter(&start_moment);
}

double Stopwatch::get_time()
{
    LARGE_INTEGER current_moment;
    QueryPerformanceCounter(&current_moment);
    
    return static_cast<double>(current_moment.QuadPart - start_moment.QuadPart)/frequency;
}
