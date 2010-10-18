#include "tools_tester.h"


void tools_tester_err_callback(const char * message, const char * file, int line)
{
    message; file; line; // avoid unreferenced formal parameter warning
    throw ToolsTesterException();
}

CallbackAction tools_tester_err_action(tools_tester_err_callback);
Action *old_err_action;
Action *old_warn_action;
