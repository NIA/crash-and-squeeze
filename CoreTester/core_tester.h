#pragma once
#include <gtest/gtest.h>
#include "Core/core.h"
#include "Math/vector.h"

using namespace ::CrashAndSqueeze::Math;
using namespace ::CrashAndSqueeze::Core;
using namespace ::CrashAndSqueeze::Logging;

inline std::ostream &operator<<(std::ostream &stream, const Vector &vector)
{
    return stream << "(" << vector[0] << ", " << vector[1] << ", " << vector[2] << ")";
}

class CoreTesterException {};

extern CallbackAction core_tester_err_action;
extern Action *old_err_action;
extern Action *old_warn_action;

inline void set_tester_err_callback()
{
    old_err_action = logger.get_action(Logger::ERROR);
    logger.set_action(Logger::ERROR, &core_tester_err_action);
}

inline void unset_tester_err_callback()
{
    logger.set_action(Logger::ERROR, old_err_action);
}

inline void suppress_warnings()
{
    old_warn_action = logger.get_action(Logger::WARNING);
    logger.ignore(Logger::WARNING);
}

inline void unsuppress_warnings()
{
    logger.set_action(Logger::WARNING, old_warn_action);
}
