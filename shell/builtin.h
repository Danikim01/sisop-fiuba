#ifndef BUILTIN_H
#define BUILTIN_H

#include "defs.h"
#include "utils.h"
#include "history.h"

int change_directory(char *dir);

int print_cwd(void);

extern char prompt[PRMTLEN];

int cd(char *cmd);

int exit_shell(char *cmd);

int pwd(char *cmd);

int history(char *cmd);

#endif  // BUILTIN_H
