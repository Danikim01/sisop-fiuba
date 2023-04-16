#ifndef PARSING_H
#define PARSING_H

#include "defs.h"
#include "types.h"
#include "createcmd.h"
#include "utils.h"

struct cmd *parse_line(char *b);
extern int status;  // add by jm

#endif  // PARSING_H
