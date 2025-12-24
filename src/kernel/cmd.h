#ifndef CMD_H
#define CMD_H

#include "gui/terminal.h"

// FEATURE 1: Currently active terminal instance for command output
extern terminal_instance_t* active_terminal;

void cmd_process(const char* cmd);

#endif
