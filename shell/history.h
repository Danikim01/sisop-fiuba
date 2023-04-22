#ifndef HISTORY_H
#define HISTORY_H

#include "defs.h"
#include "types.h"
#include "utils.h"


int show_history(int n);
int append_history(const char *cmd);
void history_init(); // should be int for error checking
void history_free(); // should be int for error checking

// used for arrow keys navigation
char* history_get_move_index_up();
char* history_get_move_index_down();

#endif  // HISTORY_H
