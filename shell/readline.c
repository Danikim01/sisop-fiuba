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

	set_input_mode();

#ifndef SHELL_NO_INTERACTIVE
	fprintf(stdout, "%s %s %s\n", COLOR_RED, prompt, COLOR_RESET);
	fprintf(stdout, "%s", "$ ");
#endif

	while (true) {
		assert(read(STDIN_FILENO, &c, 1) > 0);
		if (c == CHAR_NEW_LINE) {
			// Enter input
			buffer[command_index++] = CHAR_NEW_LINE;
			buffer[command_index] = '\0';
			assert(write(STDOUT_FILENO, &c, 1) > 0);
			reset_input_mode();
			return strdup(buffer);
		}

		if (c == CHAR_EOF) {
			// CTRL-d input
			return NULL;
		}

		if (c == CHAR_DEL) {
			// input "Backspace"
			if (command_index == 0) {
				// beginning
				continue;
			}

			delete_char();
			buffer[command_index--] = '\0';
		}

		if (c == CHAR_ESCSEQ) {
			// checks if escape sequence is an arrow
			char esc_seq;
			assert(read(STDIN_FILENO, &esc_seq, 1) > 0);
			if (esc_seq != '[') {
				// we do nothing sicen its not an arrow
			} else {
				// now we check which of the 4 arrows is it
				assert(read(STDIN_FILENO, &esc_seq, 1) > 0);

				switch (esc_seq) {
				case 'A':
					// interact with history (get the previous command)
					char *previous_command =
					        get_previous_command();

					if (previous_command != NULL) {
						// Delete the current input in the buffer
						while (command_index > 0) {
							delete_char();
							buffer[--command_index] =
							        '\0';
						}

						// Copy the previous command into the buffer
						strcpy(buffer, previous_command);
						command_index =
						        strlen(previous_command);

						// Write the previous command to the terminal
						assert(write(STDOUT_FILENO,
						             previous_command,
						             command_index) > 0);

						// Free the memory allocated by strdup in get_previous_command
						free(previous_command);
					}
					return "up";

				case 'B':
					// interact with history
					return "down";

				case 'C':
					// interact with command
					return "right";

				case 'D':
					// interact with command
					return "left";
				}
			}
		}

		if (isprint(c)) {
			assert(write(STDOUT_FILENO, &c, 1) > 0);
			buffer[command_index++] = c;
		}
	}

	reset_input_mode();
	return buffer;
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
