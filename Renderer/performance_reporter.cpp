#include "performance_reporter.h"
#include <cstdio>

namespace
{
    const char * RELEASE_OR_DEBUG = 
#ifdef NDEBUG
        "Release"
#else
        "Debug  "
#endif
        ;
}

PerformanceReporter::PerformanceReporter(const char *description) : measurements_count(0)
{
    if(NULL == description)
    {
        description = "<no description>";
    }
    
    int size = strlen(description) + 1;
    this->description = new char[size];
    strcpy_s(this->description, size, description);
}

void PerformanceReporter::begin_report()
{
    static const int BUF_SIZE = 512;
    static char buf[BUF_SIZE];
    sprintf_s(buf, BUF_SIZE, "Performance for %s in %s:", description, RELEASE_OR_DEBUG);
    my_log("        [Renderer]", buf);
}

void PerformanceReporter::report_time(char *prefix, double time)
{
    if(0 == time)
        return;
    
    static const int BUF_SIZE = 128;
    static char buf[BUF_SIZE];
    sprintf_s(buf, BUF_SIZE,
              "%3s:%7.2f ms/frame (%6.1f fps)",
              prefix, time*1000, 1/time, description, RELEASE_OR_DEBUG);
    my_log("        [Renderer]", buf);
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
        begin_report();
        report_time("AVG", avg_time);
        report_time("MAX", max_time);
        report_time("MIN", min_time);
    }
}
    
PerformanceReporter::~PerformanceReporter()
{
    delete[] description;
}
