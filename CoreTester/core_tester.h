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

extern void core_tester_err_callback(const char * message, const char * file, int line);
extern Logger::Callback old_err_callback;
extern Logger::Callback old_warn_callback;

inline void set_tester_err_callback()
{
    old_err_callback = logger.error_callback;
    logger.error_callback = core_tester_err_callback;
}

inline void unset_tester_err_callback()
{
    logger.error_callback = old_err_callback;
}

inline void suppress_warnings()
{
    old_warn_callback = logger.warning_callback;
    logger.warning_callback = 0;
}

inline void unsuppress_warnings()
{
    logger.warning_callback = old_warn_callback;
}