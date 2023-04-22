#include "readline.h"
#include "history.h"


#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include "defs.h"

static char buffer[BUFLEN];
// stores the original terminal settings so they can be restored later
struct termios saved_attributes;

void
reset_input_mode(void)
{
	tcsetattr(STDIN_FILENO, TCSANOW, &saved_attributes);
}

void
set_input_mode(void)
{
	struct termios tattr;

	/* Save the terminal attributes so we can restore them later. */
	tcgetattr(STDIN_FILENO, &saved_attributes);

	/* Set the funny terminal modes. */
	tcgetattr(STDIN_FILENO, &tattr);
	/* Clear ICANON and ECHO. We'll do a manual echo! */
	tattr.c_lflag &= ~(ICANON | ECHO);
	/* Read one char at a time */
	tattr.c_cc[VMIN] = 1;
	tattr.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &tattr);
}

void
delete_char()
{
	assert(write(STDOUT_FILENO, "\b \b", 3) > 0);
}

char *
input_from_test(const char *prompt)
{
	int i = 0, c = 0;

#ifndef SHELL_NO_INTERACTIVE
	fprintf(stdout, "%s %s %s\n", COLOR_RED, prompt, COLOR_RESET);
	fprintf(stdout, "%s", "$ ");
#endif

	memset(buffer, 0, BUFLEN);

	c = getchar();

	while (c != END_LINE && c != EOF) {
		buffer[i++] = c;
		c = getchar();
	}

	// if the user press ctrl+D
	// just exit normally
	if (c == EOF)
		return NULL;

	buffer[i] = END_STRING;

	return buffer;
}

char *
input_from_stdin(const char *prompt)
{
	char c;
	int current_command_index = 0;
	int top_index = 0;

	bool keep_reading = true;

	set_input_mode();

#ifndef SHELL_NO_INTERACTIVE
	fprintf(stdout, "%s %s %s\n", COLOR_RED, prompt, COLOR_RESET);
	fprintf(stdout, "%s", "$ ");
#endif

	while (keep_reading) {
		assert(read(STDIN_FILENO, &c, 1) > 0);

		switch (c) {
		case CHAR_NEW_LINE:
			// enter key input
			buffer[current_command_index] = '\0';
			assert(write(STDOUT_FILENO, &c, 1) > 0);
			keep_reading = false;
			break;
		case CHAR_EOF:
			// ctrl+d input
			keep_reading = false;
			break;
		case CHAR_DEL:
			// backspace input
			if (current_command_index > 0) {
				delete_char();
				buffer[current_command_index--] = '\0';
				top_index--;
			}
			break;
		case CHAR_ESCSEQ:
			// arrow key input
			handle_escape_sequence(&current_command_index, top_index);
			break;
		default:
			// normal input
			if (isprint(c)) {
				assert(write(STDOUT_FILENO, &c, 1) > 0);
				buffer[current_command_index++] = c;
				top_index++;
			}
			break;
		}
	}

	reset_input_mode();

	if (c == CHAR_EOF) {
		return NULL;
	} else {
		return strdup(buffer);
	}
}

void
handle_escape_sequence(int *current_command_index, int top_index)
{
	char esc_seq;
	assert(read(STDIN_FILENO, &esc_seq, 1) > 0);
	if (esc_seq != '[') {
		return;
	}

	assert(read(STDIN_FILENO, &esc_seq, 1) > 0);

	switch (esc_seq) {
	case 'A':
		handle_up_arrow(current_command_index, top_index);
		break;
	case 'B':
		handle_down_arrow(current_command_index);
		break;
	case 'C':  // Right arrow
		handle_right_arrow(current_command_index, top_index);
		break;

	case 'D':  // Left arrow
		handle_left_arrow(current_command_index);
		break;
	}
}

void
handle_up_arrow(int *current_command_index, int *top_index)
{
	char *previous_command = history_get_move_index_up();

	if (previous_command != NULL) {
		while (*current_command_index > 0) {
			delete_char();
			buffer[--(*current_command_index)] = '\0';
		}

		strcpy(buffer, previous_command);
		*current_command_index = strlen(previous_command);
		top_index = current_command_index;

		assert(write(STDOUT_FILENO,
		             previous_command,
		             *current_command_index) > 0);

		// free(previous_command);
	}
}

void
handle_down_arrow(int *current_command_index)
{
	char *next_command = history_get_move_index_down();

	if (next_command != NULL) {
		while (*current_command_index > 0) {
			delete_char();
			buffer[--(*current_command_index)] = '\0';
		}

		strcpy(buffer, next_command);
		*current_command_index = strlen(next_command);

		assert(write(STDOUT_FILENO, next_command, *current_command_index) >
		       0);

		// free(previous_command);
	}
}

void
handle_right_arrow(int *command_index, int top_index)
{
	if (*command_index < top_index) {
		assert(write(STDOUT_FILENO, "\x1b[C", 3) > 0);
		(*command_index)++;
	}
}

void
handle_left_arrow(int *command_index)
{
	if (*command_index > 0) {
		assert(write(STDOUT_FILENO, "\x1b[D", 3) > 0);
		(*command_index)--;
	}
}


// reads a line from the standard input
// and prints the prompt
char *
read_line(const char *prompt)
{
	if (isatty(STDIN_FILENO))
		return input_from_stdin(prompt);
	else
		return input_from_test(prompt);
}
