#pragma once
#include <exception>

namespace CrashAndSqueeze
{
    namespace Logging
    {
        // This exception is thrown on error by default. You should catch it or
        // you may override such behavior by setting your error_callbak in logger
        class Error : public std::exception
        {
            virtual const char * what() const
            {
                return "Crash-And-Squeeze deformation system internal error! See log.";
            }
        };
        
        // TODO: does it have to be here?
        class Logger
        {
        public:
            typedef void (*Callback)(const char * message, const char * file, int line);
            
        private:
            static void default_log_callback(const char * message, const char * file, int line);
            static void default_warning_callback(const char * message, const char * file, int line);
            static void default_error_callback(const char * message, const char * file, int line);

        public:
            Callback log_callback;
            Callback warning_callback;
            Callback error_callback;

            Logger() : log_callback(default_log_callback),
                       warning_callback(default_warning_callback),
                       error_callback(default_error_callback) {}

            void log(const char * message, const char * file = 0, int line = 0)
            {
                if( 0 != log_callback )
                    log_callback(message, file, line);
            }

            void warning(const char * message, const char * file = 0, int line = 0)
            {
                if( 0 != warning_callback )
                    warning_callback(message, file, line);
            }

            void error(const char * message, const char * file = 0, int line = 0)
            {
                if( 0 != error_callback )
                {
                    error_callback(message, file, line);
                }
                else
                {
                    // error handling cannot be disabled!
                    default_error_callback(message, file, line);
                }
            }
        };

        extern Logger logger;
    }
}