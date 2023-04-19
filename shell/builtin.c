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
		else
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
	if (strncmp(cmd, "cd", 2) == 0) {
		// las primeras dos letras son "cd"
		if (strlen(cmd) <= 3) {
			// entra en caso de que no haya un directorio especificado
			const char *home_dir = getenv("HOME");
			if (chdir(home_dir) == -1) {
				fprintf_debug(stderr,
				              "ERROR: Failed to set current "
				              "directory to home.\n");
				return 0;
			}
		} else {
			// salto al directorio
			char *directorio = cmd + 3;
			if (chdir(directorio) == -1) {
				fprintf_debug(stderr,
				              "ERROR: Failed to set current "
				              "directory to home.\n");
				return 0;
			}
		}

		char cwd[BUFLEN];
		if (getcwd(cwd, sizeof(cwd)) != NULL) {
			// actualizar prompt con nueva direccion
			snprintf(prompt, PRMTLEN, "%s%s", "", cwd);
		}
		return 1;
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
		else
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

// returns true if `history` was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
history(char *cmd)
{
	// Your code here
	int i = 0;              // esto es para que no se queje
	if (strlen(cmd) > 0) {  // el make run
		i = 1;          //
	}                       //
	i = i + 1;              //

	return 0;
}
