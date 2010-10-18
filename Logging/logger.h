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
        
        typedef void (*Callback)(const char * message, const char * file, int line);

        // An abstract logger action. Inherit your own action class from this
        // and implement `invoke' method, then provide an instance of it to
        // Logger::set_action. If you do not need any context, you can simply specify
        // a callback function using Logger::set_callback and avoid defining your own action.
        class Action
        {
        public:
            virtual void invoke(const char * message, const char * file, int line) = 0;
        };

        // An action to use when you don't need any context, just callback
        class CallbackAction : public Action
        {
        private:
            Callback callback;
            
        public:
            CallbackAction(Callback callback) { this->callback = callback; }

            virtual void invoke(const char * message, const char * file, int line)
            {
                if( 0 != callback )
                    callback(message, file, line);
            }
        };

        // This class is used for logging and error reporting all over the library.
        //
        // By default it writes messages to std::clog and on error throws an exception
        // of class ::CrashAndSqueeze::Logging::Error.
        //
        // You can override default log, warning and error behavior, by providing  your own
        // implementation of Action to Logger::set_action. If you don't need any context
        // and don't want to define a new class, consider using pre-defined CallbackAction.
        //
        // Note that after an error all functions return abnormally, so you are supposed to
        // either throw an exception in error action or check after each call that no error has occured.
        //
        // Once overridden, default behaviour can be restored with Logger::set_default_action.
        //
        // Use Logger::ingnore to ignore any level of messages except errors.
        class Logger
        {
        public:
            enum Level
            {
                LOG,
                WARNING,
                ERROR,
                // not to be used as a level, just shows number of levels
                _LEVELS_NUMBER
            };

        private:
            Action * actions[_LEVELS_NUMBER];
            
            // A class for default action used internally
            class DefaultAction : public Action
            {
            private:
                static const char * const prefixes[Logger::_LEVELS_NUMBER];
                Level level;

            public:
                DefaultAction();
                void set_level(int level);

                virtual void invoke(const char * message, const char * file, int line);
            } default_actions[_LEVELS_NUMBER];

        public:
            // -- construction --

            Logger();

            // -- setup --

            void set_action(Level level, Action * action);
            Action * get_action(Level level);
            void set_default_action(Level level);
            void ignore(Level level);

            // -- invocation --
            
            void log(const char * message, const char * file = "", int line = 0)
            {
                actions[LOG]->invoke(message, file, line);
            }

            void warning(const char * message, const char * file = "", int line = 0)
            {
                actions[WARNING]->invoke(message, file, line);
            }

            void error(const char * message, const char * file = "", int line = 0)
            {
                actions[ERROR]->invoke(message, file, line);
            }
        };

        extern Logger logger;
    }
}