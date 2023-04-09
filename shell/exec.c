#include "exec.h"

// ls -l | grep f
static void
multi_pipes(struct pipecmd *p)
{
	// izquierdo
	int fk_l = fork();
	if (fk_l < 0) {
		perror("fork");
		exit(-1);
	}
	//  leo de stdin
	// 	escribe a stdout

	// derecho
	//  creo pipe
	// 	llamo recursivamente
}

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
	printf("eargc %d \n", eargc);
	for (int i = 0; i < eargc; i++) {
		int equal_index = -1;
		printf("hola1 \n");
		if ((equal_index = block_contains(eargv[i], '=')) == -1) {
			printf_debug("Error obtaining = index in "
			             "set_eviron_vars.\n");
			break;
		}
		printf("hola2 \n");

		char *key = (char *) malloc(equal_index);
		char *value = (char *) malloc(strlen(eargv[i]) - equal_index);
		get_environ_key(eargv[i], key);
		get_environ_value(eargv[i], value, equal_index);

		printf("key: %s", key);
		printf("value: %s", value);

		if (setenv(key, value, 1) == 0) {
			printf("&%s set to: %s\n", key, value);
		} else {
			printf_debug("Error setting enviroment var.\n");
		}
	}
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
	int fd;
	if (flags == O_RDONLY) {
		fd = open(file, flags | O_CLOEXEC);
	} else {
		fd = open(file,
		          flags | O_CLOEXEC | O_CREAT | O_TRUNC,
		          S_IRUSR | S_IWUSR);
	}
	if (fd == -1) {
		printf_debug("Fallo open con file:\n", file);
	}
	return fd;
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
		execvp(e->argv[0], e->argv);
		perror("execvp");
		exit(-1);

		break;
	}

	case BACK: {
		// runs a command in background

		int fk = fork();
		if (fk == -1) {
			perror("fork");
			exit(-1);
		}

		if (fk == 0) {
			// child process
			b = (struct backcmd *) cmd;
			e = (struct execcmd *) cmd;
			printf_debug("[PID=%d]\n", b->pid);
			execvp(e->argv[0], e->argv);

			printf_debug("execvp");
			exit(-1);
		} else {
			// father process
			waitpid(fk, NULL, WNOHANG);  // TODO: Fix this
		}

		// printf("Background process are not yet implemented\n");
		// _exit(-1);
		break;
	}

	case REDIR: {
		// Changes the input/output/stderr flow
		//
		// To check if a redirection has to be performed,
		// verify if the file name's length (in the execcmd struct)
		// is greater than zero

		r = (struct execcmd *) cmd;
		set_environ_vars(r->eargv, r->eargc);

		printf("type: %d\n", r->type);
		printf("pid: %d\n", r->pid);
		printf("scmd: %s\n", r->scmd);
		printf("argc: %d\n", r->argc);
		printf("eargc: %d\n", r->eargc);

		// Imprimir los elementos del arreglo argv
		printf("argv:\n");
		for (int i = 0; i < r->argc; i++) {
			printf("\targv[%d]: %s\n", i, r->argv[i]);
		}

		// Imprimir los elementos del arreglo eargv
		printf("eargv:\n");
		for (int i = 0; i < r->eargc; i++) {
			printf("\teargv[%d]: %s\n", i, r->eargv[i]);
		}

		printf("out_file: %s\n", r->out_file);
		printf("in_file: %s\n", r->in_file);
		printf("err_file: %s\n", r->err_file);

		int input_fd = -1;
		int output_fd = -1;
		int error_fd = -1;

		// Redirect input (stdin)
		if (strlen(r->in_file) > 0) {
			// O_RDONLY = read only
			input_fd = open_redir_fd(r->in_file, O_RDONLY);
			if (input_fd < 0) {
				perror("open");
				exit(EXIT_FAILURE);
			}
		}

		// Redirect output (stdout)
		if (strlen(r->out_file) > 0) {
			// O_WRONLY = write only
			output_fd = open_redir_fd(r->out_file, O_WRONLY);

			if (output_fd < 0) {
				perror("open");
				exit(EXIT_FAILURE);
			}
		}

		// Redirect errors (stderr)
		if (strlen(r->err_file) > 0) {
			error_fd = open_redir_fd(r->err_file, O_WRONLY);

			if (error_fd < 0) {
				perror("open");
				exit(EXIT_FAILURE);
			}
		}


		if (input_fd != -1) {
			dup2(input_fd, STDIN_FILENO);
			close(input_fd);
		}

		if (output_fd != -1) {
			dup2(output_fd, STDOUT_FILENO);
			close(output_fd);
		}

		if (error_fd != -1) {
			dup2(error_fd, STDERR_FILENO);
			close(error_fd);
		}

		if (input_fd != -1 && error_fd != -1) {
			dup2(input_fd, STDOUT_FILENO);
			dup2(error_fd, STDERR_FILENO);
			close(input_fd);
			close(error_fd);
		}

		execvp(r->argv[0], r->argv);
		perror("execvp");

		break;
	}

	case PIPE: {
		p = (struct pipecmd *) cmd;
		multi_pipes(p);

		// free_command(parsed_pipe);
		break;
	}
	}
}
