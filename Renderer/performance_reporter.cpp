#include "performance_reporter.h"
#include <cstdio>

PerformanceReporter::PerformanceReporter(Logger &logger, const char *description)
    : measurements_count(0), logger(logger)
{
    if(NULL == description)
    {
        description = "<no description>";
    }
    
    int size = strlen(description) + 1;
    this->description = new char[size];
    strcpy_s(this->description, size, description);
}

void PerformanceReporter::report_time(double time)
{
    if(0 == time)
        return;
    
    static const int BUF_SIZE = 512;
    static char buf[BUF_SIZE];
    sprintf_s(buf, BUF_SIZE,
              "%s:%7.2f ms/frame (%4.0f fps)",
              description, time*1000, 1/time);
    logger.log("        [Renderer]", buf);
}

void PerformanceReporter::add_measurement(double time)
{
    if(measurements_count <= 0)
    {
        max_time = min_time = avg_time = time;
        measurements_count = 1;
    }
    else
    {
        avg_time = (avg_time*measurements_count + time)/(measurements_count + 1);
        
        if( time > max_time )
            max_time = time;

        if( time < min_time )
            min_time = time;

        ++measurements_count;
    }
}

void PerformanceReporter::report_results()
{
    if( 0 != measurements_count )
    {
        report_time(avg_time);
    }
}
    
PerformanceReporter::~PerformanceReporter()
{
    delete[] description;
}
