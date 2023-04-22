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
	// genera una secuencia de bytes
	// que indican que se debe borrar un byte
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
	int command_index = 0;
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
			buffer[command_index] = '\0';
			assert(write(STDOUT_FILENO, &c, 1) > 0);
			keep_reading = false;
			break;
		case CHAR_EOF:
			keep_reading = false;
			break;
		case CHAR_DEL:
			if (command_index > 0) {
				delete_char();
				buffer[command_index--] = '\0';
			}
			break;
		case CHAR_ESCSEQ:
			handle_escape_sequence(&command_index);
			break;
		default:
			if (isprint(c)) {
				assert(write(STDOUT_FILENO, &c, 1) > 0);
				buffer[command_index++] = c;
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
handle_escape_sequence(int *command_index)
{
	char esc_seq;
	assert(read(STDIN_FILENO, &esc_seq, 1) > 0);
	if (esc_seq != '[') {
		return;
	}

	assert(read(STDIN_FILENO, &esc_seq, 1) > 0);

	switch (esc_seq) {
	case 'A':
		handle_up_arrow(command_index);
		break;
	case 'B':
		// interact with history
		break;
	case 'C':
		// interact with command
		break;
	case 'D':
		// interact with command
		break;
	}
}

void
handle_up_arrow(int *command_index)
{
	char *previous_command = get_previous_command();

	if (previous_command != NULL) {
		while (*command_index > 0) {
			delete_char();
			buffer[--(*command_index)] = '\0';
		}

		strcpy(buffer, previous_command);
		*command_index = strlen(previous_command);

		assert(write(STDOUT_FILENO, previous_command, *command_index) > 0);

		free(previous_command);
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
