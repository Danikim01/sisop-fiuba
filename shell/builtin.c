#include "builtin.h"

// returns true if the 'exit' call
// should be performed
//
// (It must not be called from here)
int
exit_shell(char *cmd)
{
	int space_index = block_contains(cmd, ' ');
	if (space_index == -1) {
		// CASE:
		// exit
		int is_exit_alone = strcmp(cmd, "exit") == 0;

		if (is_exit_alone)
			return 1;

		return 0;
	}

	// Enters here exlusively if theres a space in the command
	if (strncmp(cmd, "exit", space_index) == 0) {
		// CASE:
		// exit randomstrings

		return 1;
	}

	return 0;
}

int
change_directory(char *dir)
{
	// printf_debug("change dir to: '%s'\n", dir);
	if (chdir(dir) == -1) {
		fprintf_debug(stderr,
		              "ERROR: Failed to set current "
		              "directory to %s.\n",
		              dir);
		return 0;
	}

	char *aux = getcwd(NULL, 0);
	// dir = aux + dir;
	snprintf(prompt, sizeof prompt, "(%s)", aux);

	return 1;
}


// returns true if "chdir" was performed
//  this means that if 'cmd' contains:
// 	1. $ cd directory (change to 'directory')
// 	2. $ cd (change to $HOME)
//  it has to be executed and then return true
//
//  Remember to update the 'prompt' with the
//  	new directory.
//
// Examples:
//  1. cmd = ['c','d', ' ', '/', 'b', 'i', 'n', '\0']
//  2. cmd = ['c','d', '\0']
int
cd(char *cmd)
{
	int space_index = block_contains(cmd, ' ');
	if (space_index == -1) {
		// CASE:
		// cd
		int is_cd_alone = strcmp(cmd, "cd") == 0;
		if (is_cd_alone) {
			const char *home_dir = getenv("HOME");

			return change_directory(home_dir);
		}

		return 0;
	}

	// Enters here exlusively if theres a space in the command
	if (strncmp(cmd, "cd", space_index) == 0) {
		// CASE:
		// cd randomstrings, cd . and cd ..
		char *dir = split_line(cmd, ' ');

		return change_directory(dir);
	}

	return 0;
}

int
print_cwd()
{
	char *cwd =
	        getcwd(NULL, 0);  // getcwd(NULL, 0) dynamically allocates memory
	if (cwd == NULL) {
		fprintf_debug(stderr, "ERROR: Getcwd failed.\n");

		return 0;
	}

	printf_debug("%s\n", cwd);
	free(cwd);  // free the allocated memory from getcwd

	return 1;
}

// returns true if 'pwd' was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
pwd(char *cmd)
{
	int space_index = block_contains(cmd, ' ');
	if (space_index == -1) {
		// CASE:
		// pwd
		int is_pwd_alone = strcmp(cmd, "pwd") == 0;

		if (is_pwd_alone)
			return print_cwd();

		return 0;
	}

	// Enters here exlusively if theres a space in the command
	if (strncmp(cmd, "pwd", space_index) == 0) {
		// CASE:
		// pwd randomstring
		return print_cwd();
	}

	return 0;
}

// https://www.w3schools.blog/check-if-string-is-number-c
int
isNumeric(char s[])
{
	for (int i = 0; s[i] != '\0'; i++) {
		if (isdigit(s[i]) == 0)
			return 0;
	}
	return 1;
}

// returns true if `history` was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
history(char *cmd)
{
	char *cmd_copy = strdup(cmd);
	if (cmd_copy == NULL) {
		return 0;
	}

	char *token = strtok(cmd_copy, " ");
	if (strcmp(token, "history") == 0) {
		int n = 0;
		token = strtok(NULL, " ");
		if (token == NULL) {
			n = -1;  // ran history without argument n
		} else if (isNumeric(token)) {
			n = atoi(token);
		} else {
			printf_debug("usage: history n\n");
			free(cmd_copy);
			return 1;
		}

		// printf_debug("argument is '%i'\n", n);
		free(cmd_copy);
		return show_history(n);
	}

	// printf_debug("cmd: '%s' is not history\n", cmd);
	free(cmd_copy);
	return 0;
}
