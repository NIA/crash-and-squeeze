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

double Stopwatch::stop()
{
    LARGE_INTEGER stop_moment;
    QueryPerformanceCounter(&stop_moment);
    
    return static_cast<double>(stop_moment.QuadPart - start_moment.QuadPart)/frequency;
}