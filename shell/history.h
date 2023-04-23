#ifndef HISTORY_H
#define HISTORY_H

#define MAX_HIST_SIZE 2000

#include "defs.h"
#include "types.h"
#include "utils.h"

void history_init(void);  // should be int for error checking
void history_free(void);  // should be int for error checking
int show_history(int n);
int append_history(const char *cmd);

// used for arrow keys navigation
char *history_get_move_index_up(void);
char *history_get_move_index_down(void);
char *history_get_current_index(void);
void reset_history_index(void);

#endif  // HISTORY_H
