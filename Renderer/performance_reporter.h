#pragma once
#include "main.h"
#include "logger.h"

class PerformanceReporter
{
private:
    Logger &logger;
    char *description;

    int measurements_count;
    double last_measurement;
    
    double max_time;
    double min_time;
    double avg_time;

    void begin_report();
    void report_time(char *prefix, double time);

public:
    PerformanceReporter(Logger &logger, const char *description);
    
    void add_measurement(double time);
    
    double get_last_measurement() const { return last_measurement; }
    double get_last_fps() const;

    void report_results();
    
    ~PerformanceReporter();

private:
    // No copying!
    PerformanceReporter(const PerformanceReporter&);
    PerformanceReporter &operator=(const PerformanceReporter&);
};
