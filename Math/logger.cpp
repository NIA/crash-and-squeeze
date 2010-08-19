#include "logger.h"
#include <iostream>

namespace CrashAndSqueeze
{
    namespace Logging
    {
        inline void default_report(const char * message, const char * file, int line)
        {
            std::clog << message;
            if(0 != file)
                std::clog << " at file " << file;
            if(0 != line)
                std::clog << " at line " << line;
            std::clog << std::endl;
        }

        void Logger::default_log_callback(const char * message, const char * file, int line)
        {
            std::clog << "[Crash-And-Squeeze]: ";
            default_report(message, file, line);
        }

        void Logger::default_warning_callback(const char * message, const char * file, int line)
        {
            std::clog << "WARNING [Crash-And-Squeeze]: ";
            default_report(message, file, line);
        }

        void Logger::default_error_callback(const char * message, const char * file, int line)
        {
            std::clog << "ERROR! [Crash-And-Squeeze]: ";
            default_report(message, file, line);
            throw ::CrashAndSqueeze::Logging::Error();
        }

        Logger logger;
    }
}