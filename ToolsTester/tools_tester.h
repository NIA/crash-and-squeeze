#pragma once
#include <gtest/gtest.h>
#include "Math/matrix.h"

using namespace ::CrashAndSqueeze::Math;
using namespace ::CrashAndSqueeze::Logging;

inline std::ostream &operator<<(std::ostream &stream, const Vector &vector)
{
    return stream << "(" << vector[0] << ", " << vector[1] << ", " << vector[2] << ")";
}

inline std::ostream &operator<<(std::ostream &stream, const Matrix &m)
{
    return stream << "{{" << m.get_at(0,0) << ", " << m.get_at(0,1) << ", " << m.get_at(0,2) << "}, "
                  <<  "{" << m.get_at(1,0) << ", " << m.get_at(1,1) << ", " << m.get_at(1,2) << "}, "
                  <<  "{" << m.get_at(2,0) << ", " << m.get_at(2,1) << ", " << m.get_at(2,2) << "}}";
}

class ToolsTesterException {};

extern CallbackAction tools_tester_err_action;
extern Action *old_err_action;
extern Action *old_warn_action;

inline void set_tester_err_callback()
{
    old_err_action = Logger::get_instance().get_action(Logger::ERROR);
    Logger::get_instance().set_action(Logger::ERROR, &tools_tester_err_action);
}

inline void unset_tester_err_callback()
{
    Logger::get_instance().set_action(Logger::ERROR, old_err_action);
}

inline void suppress_warnings()
{
    old_warn_action = Logger::get_instance().get_action(Logger::WARNING);
    Logger::get_instance().ignore(Logger::WARNING);
}

inline void unsuppress_warnings()
{
    Logger::get_instance().set_action(Logger::WARNING, old_warn_action);
}
