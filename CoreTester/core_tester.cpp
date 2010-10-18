#include "core_tester.h"


void core_tester_err_callback(const char * message, const char * file, int line)
{
    ignore_unreferenced(message);
    ignore_unreferenced(file);
    ignore_unreferenced(line);
    throw CoreTesterException();
}

CallbackAction core_tester_err_action(core_tester_err_callback);
Action *old_err_action;
Action *old_warn_action;
