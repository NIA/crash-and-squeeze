#pragma once
#include <exception>

namespace CrashAndSqueeze
{
    namespace Logging
    {
        // This exception is thrown on error by default. You should catch it or
        // you may override such behavior by setting your error_callback in logger
        // (see ::CrashAndSqueeze::Logging::Logger for details)
        class Error : public std::exception
        {
            virtual const char * what() const
            {
                return "Crash-And-Squeeze deformation system internal error! See log.";
            }
        };
        
        // This class is used for logging and error reporting all over the library.
        //
        // You can override default log, warning and error behavior, by substituting 
        // log_callback, warning_callback and error_callback members of logger object
        // with your own functions.
        //
        // You may set log_callback or/and warning_callback to NULL
        // in order to disable logging or/and warnings.
        //
        // Default error_callback throws an exception of class ::CrashAndSqueeze::Logging::Error,
        // so if it is not desired, override such behavior with your own error_callback.
        // Note that you CANNOT disable error handling by setting error_callback to NULL,
        // and that if your error_callback doesn't interrupt execution, furhter behaviour of library
        // is unpredictable.
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