#include "history.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HISTFILE ".fisop_history"

int show_history(int n) {
    const char *home_dir = getenv("HOME");
    if (home_dir == NULL) {
        fprintf_debug(stderr, "Error showing history: HOME environment variable is not set\n");
        return 0;
    }

    char path[512];
    strcpy(path, home_dir);
    strcat(path, "/");
    strcat(path, HISTFILE);
    // printf_debug("attempt open to show: '%s'\n", path);

    char cmd[1024];
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        fprintf_debug(stderr, "Error showing history: could not open file\n");
        return 0;
    }


    int line_num = 0;
    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        if (n == 0 || line_num >= (line_num - n)) {
            printf("%d %s", line_num, line);
        }
    }

    fclose(fp);
    printf_debug("finished showing history\n");
    return 1;
}

int append_history(const char *cmd) {
    const char *home_dir = getenv("HOME");
    if (home_dir == NULL) {
        fprintf_debug(stderr, "Error appending to history: HOME environment variable is not set\n");
        return 0;
    }

    char path[512];
    strcpy(path, home_dir);
    strcat(path, "/");
    strcat(path, HISTFILE);
    // printf_debug("attempt open: '%s'\n", path);

    FILE *fp = fopen(path, "a");
    if (fp == NULL) {
        fprintf_debug(stderr, "Error appending to history: could not open file\n");
        return 0;
    }
    printf_debug("appending: '%s'\n", cmd);
    fprintf_debug(fp, "%s\n", cmd);
    fclose(fp);

    return 1;
}