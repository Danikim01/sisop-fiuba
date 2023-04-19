#include "exec.h"

void run_pipe(struct pipecmd *p);

// sets "key" with the key part of "arg"
// and null-terminates it
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  key = "KEY"
//
static void
get_environ_key(char *arg, char *key)
{
	int i;
	for (i = 0; arg[i] != '='; i++)
		key[i] = arg[i];

	key[i] = END_STRING;
}

// sets "value" with the value part of "arg"
// and null-terminates it
// "idx" should be the index in "arg" where "=" char
// resides
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  value = "value"
//
static void
get_environ_value(char *arg, char *value, int idx)
{
	size_t i, j;
	for (i = (idx + 1), j = 0; i < strlen(arg); i++, j++)
		value[j] = arg[i];

	value[j] = END_STRING;
}

// sets the environment variables received
// in the command line
//
// Hints:
// - use 'block_contains()' to
// 	get the index where the '=' is
// - 'get_environ_*()' can be useful here
static void
set_environ_vars(char **eargv, int eargc)
{
	for (int i = 0; i < eargc; i++) {
		int equal_index = block_contains(eargv[i], '=');
		if (equal_index == -1) {
			fprintf_debug(
			        stderr, "ERROR: block_contains failed in set_environ_vars.\n");

			return;
		}

		int key_length = equal_index + 1;  //+1 for \0
		char key[key_length];
		get_environ_key(eargv[i], key);

		int value_length = strlen(eargv[i]) - equal_index;
		// EJ: HOLA=algo => length = 9 - 4 = 5 (already considers \0)
		char value[value_length];
		get_environ_value(eargv[i], value, equal_index);

		if (setenv(key, value, 1) != 0) {
			fprintf_debug(
			        stderr,
			        "ERROR: Failed setting enviroment variable\n.");
			return;
		}
	}

	return;
}

// opens the file in which the stdin/stdout/stderr
// flow will be redirected, and returns
// the file descriptor
//
// Find out what permissions it needs.
// Does it have to be closed after the execve(2) call?
//
// Hints:
// - if O_CREAT is used, add S_IWUSR and S_IRUSR
// 	to make it a readable normal file
static int
open_redir_fd(char *file, int flags)
{
	int fd = open(file,
	              flags | O_CLOEXEC |
	                      ((flags == O_WRONLY) ? (O_CREAT | O_TRUNC) : 0),
	              S_IRUSR | S_IWUSR);
	if (fd == -1) {
		fprintf_debug(stderr,
		              "ERROR: Failed opening redirection"
		              "file descriptor in open_redir_fd()\n");
	}

	return fd;
}

void
run_pipe(struct pipecmd *p)
{
	int fd[2];
	if (pipe(fd) < 0) {
		fprintf_debug(stderr, "Pipe failed.\n");
		return;
	}

	pid_t left = fork();
	if (left < 0) {
		fprintf_debug(stderr, "Fork failed.\n");
		return;
	}
	if (left == 0) {
		// Child Process
		dup2(fd[WRITE], STDOUT_FILENO);
		close(fd[WRITE]);
		close(fd[READ]);

		exec_cmd(p->leftcmd);
	}

	pid_t right = fork();
	if (right < 0) {
		fprintf_debug(stderr, "Fork failed.\n");
		kill(left, SIGKILL);
		return;
	}
	if (right == 0) {
		dup2(fd[READ], STDIN_FILENO);
		close(fd[WRITE]);
		close(fd[READ]);

		exec_cmd(p->rightcmd);
	}


	close(fd[READ]);
	close(fd[WRITE]);

	waitpid(left, NULL, 0);
	waitpid(right, NULL, 0);
}

// executes a command - does not return
//
// Hint:
// - check how the 'cmd' structs are defined
// 	in types.h
// - casting could be a good option
void
exec_cmd(struct cmd *cmd)
{
	// To be used in the different cases
	struct execcmd *e;
	struct backcmd *b;
	struct execcmd *r;
	struct pipecmd *p;

	switch (cmd->type) {
	case EXEC: {
		// TODO: later add env var
		//  spawns a command
		e = (struct execcmd *) cmd;

		set_environ_vars(e->eargv, e->eargc);
		execvp(e->argv[0], e->argv);
		fprintf_debug(stderr, "ERROR: Execvp failed.\n");
		exit(EXIT_FAILURE);

		break;
	}

	case BACK: {
		// runs a command in background
		b = (struct backcmd *) cmd;
		exec_cmd(b->c);

		break;
	}

	case REDIR: {
		r = (struct execcmd *) cmd;

		int input_fd = -1;
		int output_fd = -1;
		int error_fd = -1;

		// Redirect input (stdin)
		if (strlen(r->in_file) > 0) {
			input_fd = open_redir_fd(r->in_file, O_RDONLY);
			if (input_fd < 0) {
				fprintf_debug(
				        stderr,
				        "ERROR: Falied opening input file\n");
				exit(EXIT_FAILURE);
			}
		}

		// Redirect output (stdout)
		if (strlen(r->out_file) > 0) {
			// O_WRONLY = write only
			output_fd = open_redir_fd(r->out_file, O_WRONLY);

			if (output_fd < 0) {
				fprintf_debug(
				        stderr,
				        "ERROR: Failed opening output file\n");
				exit(EXIT_FAILURE);
			}
		}

		// Redirect errors (stderr)
		if (strlen(r->err_file) > 0 && strcmp(r->err_file, "&1") != 0) {
			error_fd = open_redir_fd(r->err_file,
			                         O_WRONLY);  // Falla

			if (error_fd < 0) {
				fprintf_debug(
				        stderr,
				        "ERROR: Failed opening error file\n");
				exit(EXIT_FAILURE);
			}
		}

		bool special_case = block_contains(r->err_file, '&') == 0;
		if (error_fd != -1 || special_case) {
			if (special_case) {
				// Caso ls > out.txt 2>&1
				dup2(output_fd, STDERR_FILENO);
			} else {
				// Caso ls > out.txt 2>err.txt
				dup2(error_fd, STDERR_FILENO);
				close(error_fd);
			}
		}

		if (output_fd != -1) {
			dup2(output_fd, STDOUT_FILENO);
			close(output_fd);
		}

		if (input_fd != -1) {
			dup2(input_fd, STDIN_FILENO);
			close(input_fd);
		}

		execvp(r->argv[0], r->argv);
		fprintf_debug(stderr, "ERROR: execvp failed.\n");
		exit(EXIT_FAILURE);

		break;
	}

	case PIPE: {
		p = (struct pipecmd *) cmd;
		run_pipe(p);
		exit(EXIT_SUCCESS);

		break;
	}
	}
}
