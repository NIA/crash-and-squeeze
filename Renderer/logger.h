#pragma once
#include "main.h"
#include "parallel.h"
#include <fstream>

class Logger
{
private:
    std::ofstream log_file;
    static const int MESSAGES_SIZE =500;
    const char * messages[MESSAGES_SIZE];
    int next_message_index;
    WinLock lock;
    bool messages_enabled;

public:
    Logger(const char * log_filename, bool messages_enabled = false);

    void log(const char *prefix, const char * message, const char * file = "", int line = 0);
    void newline();
    
    // WARNING! message is not copied but saved as pointer. DO NOT pass local
    // variables as `message'! For best safety `message' should be constant string
    // literal, so there would be a guarantee that it is not freed before dumping.
    void add_message(const char* message);
    void dump_messages();
    void clear_messages();

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

