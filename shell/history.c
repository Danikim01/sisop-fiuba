#include "history.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HISTFILE ".fisop_history"

int append_history_list(const char *cmd);

history_t *global_hist = NULL;

int
append_history_list(const char *cmd)
{
	if (!global_hist) {
		return -1;
	}

	node_t *new_node = calloc(1, sizeof(node_t));
	if (!new_node) {
		printf_debug("Error allocating memory for new node\n");
		return -1;
	}

	new_node->line = strdup(cmd);
	new_node->next = NULL;

	if (global_hist->size == 0) {
		global_hist->node_first = new_node;
	} else {
		global_hist->node_last->next = new_node;
	}

	global_hist->node_last = new_node;
	global_hist->size++;
	global_hist->index++;
	return 0;
}

// loads history file into RAM
void
history_init()
{
	global_hist = calloc(1, sizeof(history_t));
	if (!global_hist) {
		printf_debug("Error allocating memory for history\n");
		return;
	}

	global_hist->node_first = NULL;
	global_hist->node_last = NULL;
	global_hist->size = 0;
	global_hist->index = 0;

	const char *home_dir = getenv("HOME");
	if (home_dir == NULL) {
		printf_debug("Error appending to history: HOME environment "
		             "variable is not set\n");
		history_free();
		return;
	}

	char path[512];
	strcpy(path, home_dir);
	strcat(path, "/");
	strcat(path, HISTFILE);

	// open the history file for reading
	FILE *fp = fopen(path, "r");
	if (!fp) {
		printf_debug("Error opening history file '%s'\n", HISTFILE);
		history_free();
		return;
	}

	// read the file line by line
	char *line = NULL;
	size_t len = 0;
	ssize_t read = 0;
	while ((read = getline(&line, &len, fp)) != -1) {
		// remove trailing newline
		if (line[read - 1] == '\n') {
			line[read - 1] = '\0';
		}

		// append the line to the history list
		int ret = append_history_list(line);
		if (ret != 0) {
			printf_debug("Error appending line to history: %s\n",
			             line);
		}
	}

	// set index to be last (most recent) element on list
	global_hist->index = (int) global_hist->size;

	// free memory and close file
	free(line);
	fclose(fp);

	return;
}

void
history_free()
{
	node_t *curr_node = global_hist->node_first;
	node_t *next_node;

	while (curr_node != NULL) {
		next_node = curr_node->next;
		free(curr_node->line);
		free(curr_node);
		curr_node = next_node;
	}

	free(global_hist);
	global_hist = NULL;
}

char *
history_get_current_index()
{
	// Check if the history is empty
	if (global_hist->node_first == NULL || global_hist->node_last == NULL) {
		return NULL;
	}

	// Check if the current index is out of bounds
	if (global_hist->index < 0 ||
	    (size_t) global_hist->index >= global_hist->size) {
		return NULL;
	}

	// Traverse the linked list to find the current node
	node_t *current_node = global_hist->node_first;
	for (int i = 0; i < global_hist->index; i++) {
		current_node = current_node->next;
	}

	// Return the line from the current node
	return current_node->line;
}

// decreases index (arrow up)
// and return string on index
char *
history_get_move_index_up()
{
	// decrease idx
	if (global_hist->index > 0) {
		global_hist->index--;
	}

	return history_get_current_index();
}

// increases index (arrow down)
// and return string on index
char *
history_get_move_index_down()
{
	// increase idx
	if (global_hist->index < (int) global_hist->size) {
		global_hist->index++;
	}

	return history_get_current_index();
}

// resets index
void
reset_history_index()
{
	global_hist->index = global_hist->size;
}

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
	return 1;
}

// appends cmd to both file and ram
int
append_history(const char *cmd)
{
	// TODO: avoid appending empty line

	// append to FILE
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

	FILE *fp = fopen(path, "a");
	if (fp == NULL) {
		fprintf_debug(
		        stderr,
		        "Error appending to history: could not open file\n");
		return 0;
	}
	fprintf_debug(fp, "%s\n", cmd);
	fclose(fp);

	// append to RAM -- eval output
	append_history_list(cmd);

	return 1;
}
