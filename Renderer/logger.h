#pragma once
#include "main.h"
#include "parallel.h"
#include "stopwatch.h"

class Logger
{
private:
    std::ofstream log_file;
    static const int MESSAGES_SIZE =50000;
    const char * messages[MESSAGES_SIZE];
    double timestamps[MESSAGES_SIZE];
    int thread_ids[MESSAGES_SIZE];

    int next_message_index;
    WinLock lock;
    Stopwatch stopwatch;
    bool messages_enabled;

public:
    Logger(const char * log_filename, bool messages_enabled = false);

    void log(const char *prefix, const char * message, const char * file = "", int line = 0);
    void newline();
    
    // WARNING! message is not copied but saved as pointer. DO NOT pass local
    // variables as `message'! For best safety `message' should be a global constant (incl. string
    // literal), so there would be a guarantee that it is neither freed nor changed before dumping.
    void add_message(const char* message, int thread_id = 0);
    void dump_messages();

    ~Logger()
    {
        if(log_file.is_open())
            log_file.close();
    }

private:
    // No copying!
    Logger(const Logger &logger);
    Logger & operator=(const Logger &logger);
};

