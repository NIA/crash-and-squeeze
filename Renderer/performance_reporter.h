#pragma once
#include "main.h"

class PerformanceReporter
{
private:
    char *description;

    int measurements_count;
    
    double max_time;
    double min_time;
    double avg_time;

    void begin_report();
    void report_time(char *prefix, double time);

public:
    PerformanceReporter(const char *description);
    
    void add_measurement(double time);

    void report_results();
    
    ~PerformanceReporter();
};
