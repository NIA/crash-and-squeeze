#pragma once
#include "main.h"
#include <chrono>

// A wrapper to std::chrono::high_resolution_clock for easy measuring time intervals
class Stopwatch
{
private:
    typedef std::chrono::high_resolution_clock::time_point Moment;
    Moment start_moment;
    
public:
    Stopwatch();

    // starts the stopwatch
    void start();
    // stops the stopwatch and returns measured time
    double stop();
};