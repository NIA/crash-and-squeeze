#pragma once
#include "main.h"

// A wrapper to QueryPerfomanceCounter for easy measuring time intervals
class Stopwatch
{
private:
    double frequency;
    LARGE_INTEGER start_moment;
    
public:
    Stopwatch();

    // starts the stopwatch
    void start();
    // stops the stopwatch and returns measured time
    double stop();
};