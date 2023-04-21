#include "history.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HISTFILE ".fisop_history"

// shows the shells most recent 'n'
// lines ran. If argument 'n' is -1
// it shows the whole history.
int
show_history(int n)
{
	const char *home_dir = getenv("HOME");
	if (home_dir == NULL) {
		fprintf_debug(
		        stderr, "Error showing history: HOME environment variable is not set\n");
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
		fprintf_debug(stderr,
		              "Error showing history: could not open file\n");
		return 0;
	}

	// get amount of lines
	int num_lines = 0;
	char ch;
	while ((ch = fgetc(fp)) != EOF) {
		if (ch == '\n') {
			num_lines++;
		}
	}

	// calculate number of lines to skip
	int num_to_skip = num_lines - n;
	if (num_to_skip < 0 || n == -1) {  // skip no lines
		num_to_skip = 0;
	}

	// reset file pointer to beginning
	fseek(fp, 0, SEEK_SET);

	// skip lines
	int num_skipped = 0;
	while (num_skipped < num_to_skip) {
		if (fgets(cmd, sizeof(cmd), fp) == NULL) {
			break;
		}
		num_skipped++;
	}

	// print rest of lines
	while (fgets(cmd, sizeof(cmd), fp) != NULL) {
		printf("%s", cmd);
	}

	fclose(fp);
	// printf_debug("finished showing history\n");
	return 1;
}

char *
get_previous_command()
{
	const char *home_dir = getenv("HOME");
	if (home_dir == NULL) {
		fprintf_debug(
		        stderr, "Error getting previous command: HOME environment variable is not set\n");
		return NULL;
	}

	char path[512];
	strcpy(path, home_dir);
	strcat(path, "/");
	strcat(path, HISTFILE);

	FILE *fp = fopen(path, "r");
	if (fp == NULL) {
		fprintf_debug(
		        stderr, "Error getting previous command: could not open file\n");
		return NULL;
	}

	char cmd[1024];
	char last_cmd[1024] = "";
	while (fgets(cmd, sizeof(cmd), fp) != NULL) {
		strcpy(last_cmd, cmd);
	}

	fclose(fp);

	// Remove newline character at the end of the command
	size_t len = strlen(last_cmd);
	if (len > 0 && last_cmd[len - 1] == '\n') {
		last_cmd[len - 1] = '\0';
	}

	return strdup(last_cmd);
}


int
append_history(const char *cmd)
{
	const char *home_dir = getenv("HOME");
	if (home_dir == NULL) {
		fprintf_debug(
		        stderr, "Error appending to history: HOME environment variable is not set\n");
		return 0;
	}

	char path[512];
	strcpy(path, home_dir);
	strcat(path, "/");
	strcat(path, HISTFILE);
	// printf_debug("attempt open: '%s'\n", path);

	FILE *fp = fopen(path, "a");
	if (fp == NULL) {
		fprintf_debug(
		        stderr,
		        "Error appending to history: could not open file\n");
		return 0;
	}
	// printf_debug("appending: '%s'\n", cmd);
	fprintf_debug(fp, "%s\n", cmd);
	fclose(fp);

	return 1;
}
