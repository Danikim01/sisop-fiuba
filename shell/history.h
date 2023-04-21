#ifndef HISTORY_H
#define HISTORY_H

#include "defs.h"
#include "types.h"
#include "utils.h"

int show_history(int n);

int append_history(const char *cmd);

#endif  // HISTORY_H
