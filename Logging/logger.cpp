#include "Logging/logger.h"
#include <iostream>

namespace CrashAndSqueeze
{
    namespace Logging
    {
        namespace
        {
            inline bool is_empty(const char *string)
            {
                return '\0' == string[0];
            }

            inline Logger::Level valid_level(int i)
            {
                if(i < 0)
                    i = 0;
                
                if(i >= Logger::_LEVELS_NUMBER)
                    i = Logger::_LEVELS_NUMBER - 1;

                return static_cast<Logger::Level>(i);
            }
        }

        Logger::DefaultAction::DefaultAction()
        {
            set_level(0);
        }

        void Logger::DefaultAction::set_level(int level)
        {
            this->level = valid_level(level);
        }

        void Logger::DefaultAction::invoke(const char * message, const char * file, int line)
        {
            std::clog << "[Crash-And-Squeeze]: " << prefixes[level] << ": " << message;
            if(0 != file && !is_empty(file))
            {
                std::clog << "; " << file;
                if(0 != line)
                    std::clog << "(" << line << ")";
            }
            std::clog << std::endl;

            if(Logger::ERROR == level)
                throw ::CrashAndSqueeze::Logging::Error();
        }

        const char * const Logger::DefaultAction::prefixes[Logger::_LEVELS_NUMBER] = {
            "Log",
            "Warning!",
            "Error!!!"
        };

        class NoAction : public Action
        {
        public:
            virtual void invoke(const char * message, const char * file, int line)
            {
                message; file; line; // avoid unreferenced parameter warning
            }
        } _no_action;

        Logger::Logger()
        {
            for(int i = 0; i < Logger::_LEVELS_NUMBER; ++i)
            {
                default_actions[i].set_level(i);
                set_default_action(valid_level(i));
            }
        }

        Logger & Logger::get_instance()
        {
            static Logger instance;
            return instance;
        }

        void Logger::invoke(Level level, const char * message, const char * file, int line)
        {
            actions[valid_level(level)]->invoke(message, file, line);
        }

        Action * Logger::get_action(Level level)
        {
            return actions[valid_level(level)];
        }

        void Logger::set_action(Level level, Action * action)
        {
            if(0 != action)
                actions[valid_level(level)] = action;
            else
                set_default_action(level);
        }

        void Logger::set_default_action(Level level)
        {
            level = valid_level(level);
            actions[level] = & default_actions[level];
        }
        
        void Logger::ignore(Level level)
        {
            actions[valid_level(level)] = & _no_action;
        }
    }
}
