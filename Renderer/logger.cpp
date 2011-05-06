#include "logger.h"
#include <ctime>
#include <fstream>
#include <iomanip>

using namespace std;

Logger::Logger(const char * log_filename, bool messages_enabled)
: next_message_index(0), messages_enabled(messages_enabled)
{
    log_file.open(log_filename, ios::app);
    stopwatch.start();
}

void Logger::log(const char *prefix, const char * message, const char * file, int line)
{
    if(!log_file.is_open())
        return;

    static const int DATETIME_BUF_SIZE = 80;
    char datetime_buffer[DATETIME_BUF_SIZE];
    time_t rawtime;
    struct tm * timeinfo;
    time( &rawtime );
#pragma warning( disable : 4996 )
    timeinfo = localtime( &rawtime );
#pragma warning( default : 4996 ) 
    strftime(datetime_buffer, DATETIME_BUF_SIZE-1, "%Y-%m-%d %H:%M:%S", timeinfo);

    log_file << datetime_buffer << ' ' << prefix << ' ' << message;
    if(0 != file && '\0' != file[0])
    {
        log_file << "; " << file;
        if(0 != line)
            log_file << "(" << line << ")";
    }
    newline();
}
void Logger::newline()
{
    if(!log_file.is_open())
        return;

    log_file << endl;
    log_file.flush();
}

void Logger::add_message(const char* message, int thread_id)
{
    if(messages_enabled)
    {
        int write_here;
        lock.lock();
        write_here = next_message_index;
        if(write_here >= MESSAGES_SIZE)
        {
            // reset
            write_here = 0;
        }
        next_message_index = write_here + 1;
        lock.unlock();
        messages[write_here] = message;
        timestamps[write_here] = stopwatch.get_time();
        thread_ids[write_here] = thread_id;
    }
}

void Logger::dump_messages()
{
    if(messages_enabled)
    {
        log_file << endl << "Messages dump:" << endl;
        log_file << setiosflags(ios::fixed) << setprecision(2);
        lock.lock();
        for(int i = 0; i < next_message_index; ++i)
        {
            log_file << setw(7) << timestamps[i]*1000 << "ms"
                     << " #" << thread_ids[i] << ':'
                     << ' '  << messages[i] << endl;
        }
        lock.unlock();
        log_file.flush();
    }
}