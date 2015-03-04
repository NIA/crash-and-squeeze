#include "Stopwatch.h"

using namespace std::chrono;

Stopwatch::Stopwatch()
{
}

void Stopwatch::start()
{
    start_moment = high_resolution_clock::now();
}

double Stopwatch::stop()
{
    Moment stop_moment = high_resolution_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(stop_moment - start_moment);
    
    return time_span.count();
}