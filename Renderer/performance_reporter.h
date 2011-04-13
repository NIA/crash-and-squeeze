#pragma once
#include "main.h"
#include "logger.h"

class PerformanceReporter
{
private:
    Logger &logger;
    char *description;

    int measurements_count;
    
    double max_time;
    double min_time;
    double avg_time;

    void report_time(double time);

public:
    PerformanceReporter(Logger &logger, const char *description);
    
    void add_measurement(double time);

    void report_results();
    
    ~PerformanceReporter();

private:
    // No copying!
    PerformanceReporter(const PerformanceReporter&);
    PerformanceReporter &operator=(const PerformanceReporter&);
};
