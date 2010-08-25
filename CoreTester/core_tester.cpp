#include "core_tester.h"


void core_tester_err_callback(const char * message, const char * file, int line)
{
    ignore_unreferenced(message);
    ignore_unreferenced(file);
    ignore_unreferenced(line);
    throw CoreTesterException();
}

extern Logger::Callback old_err_callback = logger.error_callback;
extern Logger::Callback old_warn_callback = logger.warning_callback;
