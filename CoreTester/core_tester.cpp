#include "core_tester.h"


void core_tester_err_callback(const char * message, const char * file, int line)
{
    message; file; line; // avoid unreferenced formal parameter warning
    throw CoreTesterException();
}

extern Logger::Callback old_err_callback = logger.error_callback;
extern Logger::Callback old_warn_callback = logger.warning_callback;
