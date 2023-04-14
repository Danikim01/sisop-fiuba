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
	int fd = open(file,
	              flags | O_CLOEXEC |
	                      ((flags == O_RDONLY || strcmp("&1", file) == 0)
	                               ? 0
	                               : O_CREAT | O_TRUNC),
	              S_IRUSR | S_IWUSR);
	if (fd == -1) {
		fprintf_debug(stderr, "ERROR: Open failed:\n");
	}

	return fd;
}


void
run_pipe(struct pipecmd *p)
{
	int fd[2];
	if (pipe(fd) < 0) {
		perror("pipe");
		return;
	}

	pid_t pid = fork();
	if (pid < 0) {
		perror("fork");
		return;
	} else if (pid == 0) {
		close(fd[0]);
		dup2(fd[1], STDOUT_FILENO);
		close(fd[1]);

		struct execcmd *left_cmd = (struct execcmd *) p->leftcmd;
		printf_debug("El comando izquierdo a ejecutar es %s\n",
		             left_cmd->argv[0]);
		for (int i = 1; i < (left_cmd->argc); i++) {
			printf_debug(
			        "El argumento del comando izquierdo es: %s\n",
			        left_cmd->argv[i]);
		}
		execvp(left_cmd->argv[0], left_cmd->argv);
		perror("execvp");
		exit(EXIT_FAILURE);
	}

	pid_t pid2 = fork();
	if (pid2 < 0) {
		perror("fork");
		kill(pid, SIGKILL);
		return;
	} else if (pid2 == 0) {
		close(fd[1]);
		dup2(fd[0], STDIN_FILENO);
		close(fd[0]);

		if (p->rightcmd->type == PIPE) {
			printf_debug("El comando derecho antes de la llamada "
			             "recursiva es %s\n",
			             p->rightcmd->scmd);
			run_pipe((struct pipecmd *) p->rightcmd);
			printf_debug("El comando derecho despues de la llamada "
			             "recursiva es %s\n",
			             p->rightcmd->scmd);
		} else {
			struct execcmd *right_cmd = (struct execcmd *) p->rightcmd;
			printf_debug("El comando derecho a ejecutar es %s\n",
			             right_cmd->argv[0]);
			for (int i = 1; i < (right_cmd->argc); i++) {
				printf_debug("El argumento del comando derecho "
				             "es: %s\n",
				             right_cmd->argv[i]);
			}
			execvp(right_cmd->argv[0], right_cmd->argv);
			perror("execvp");
			exit(EXIT_FAILURE);
		}
	}

	close(fd[0]);
	close(fd[1]);
	waitpid(pid, NULL, 0);
	waitpid(pid2, NULL, 0);
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
				fprintf(stderr, "ERROR: Open failed:\n");
				exit(EXIT_FAILURE);
			}
		}

		// Redirect output (stdout)
		if (strlen(r->out_file) > 0) {
			// O_WRONLY = write only
			output_fd = open_redir_fd(r->out_file, O_WRONLY);

			if (output_fd < 0) {
				fprintf(stderr, "ERROR: Open failed:\n");
				exit(EXIT_FAILURE);
			}
		}

		// Redirect errors (stderr)
		if (strlen(r->err_file) > 0) {
			error_fd = open_redir_fd(r->err_file, O_RDWR);

			if (error_fd < 0) {
				fprintf(stderr, "ERROR: Open failed:\n");
				exit(EXIT_FAILURE);
			}
		}

		if (output_fd != -1 && error_fd != -1) {
			dup2(output_fd, STDOUT_FILENO);
			if (block_contains(r->err_file, '&') == 0) {
				// Caso ls > out.txt 2>&1
				dup2(output_fd, STDERR_FILENO);
			} else {
				// Caso ls > out.txt 2>err.txt
				dup2(error_fd, STDERR_FILENO);
			}
			close(output_fd);
			close(error_fd);

		} else if (output_fd != -1) {
			// Caso ls > out.txt
			dup2(output_fd, STDOUT_FILENO);
			close(output_fd);
		} else if (error_fd != -1) {
			// ls 2>err.txt
			dup2(error_fd, STDERR_FILENO);
			close(error_fd);
		}

		if (input_fd != -1) {
			// Cualquiera de los casos de arriba mezclado con <,
			// o exclusivamente <
			dup2(input_fd, STDIN_FILENO);
			close(input_fd);
		}
		execvp(r->argv[0], r->argv);
		fprintf(stderr, "ERROR: execvp failed\n");
		exit(EXIT_FAILURE);

		break;
	}

	case PIPE: {
		// p = (struct pipecmd *) cmd;
		multi_pipes(p);
		run_pipe(cmd);
		// int fd[2];
		// if (pipe(fd) < 0) {
		// 	perror("pipe");
		// 	return;
		// }

		// int left_pid = fork();
		// if (left_pid < 0) {
		// 	perror("fork");
		// 	return;
		// } else if (left_pid == 0) {  // hijo izquierdo
		// 	close(fd[0]);  // cierra extremo de lectura del pipe
		// 	dup2(fd[1], STDOUT_FILENO);
		// 	close(fd[1]);  // cierra extremo de escritura del pipe

		// 	struct execcmd *left_cmd = (struct execcmd *) p->leftcmd;
		// 	execvp(left_cmd->argv[0], left_cmd->argv);
		// 	perror("execvp");  // si falla execvp, termina el proceso hijo izquierdo
		// 	exit(EXIT_FAILURE);
		// }

		// int right_pid = fork();
		// if (right_pid < 0) {
		// 	perror("fork");
		// 	kill(left_pid,
		// 	     SIGKILL);  // si falla fork, mata al hijo izquierdo
		// 	return;
		// } else if (right_pid == 0) {  // hijo derecho
		// 	close(fd[1]);  // cierra extremo de escritura del pipe
		// 	dup2(fd[0], STDIN_FILENO);
		// 	close(fd[0]);  // cierra extremo de lectura del pipe

		// 	struct execcmd *right_cmd = (struct execcmd *) p->rightcmd;
		// 	execvp(right_cmd->argv[0], right_cmd->argv);
		// 	perror("execvp");  // si falla execvp, termina el proceso hijo derecho
		// 	exit(EXIT_FAILURE);
		// }

		// // proceso padre
		// close(fd[0]);  // cierra extremo de lectura del pipe
		// close(fd[1]);  // cierra extremo de escritura del pipe
		// waitpid(left_pid, NULL, 0);  // espera a que el hijo izquierdo termine
		// waitpid(right_pid, NULL, 0);  // espera a que el hijo derecho termine

		// break;


		// struct pipecmd *left_cmd = (struct pipecmd *)p->leftcmd;
		// struct pipecmd *right_cmd = (struct pipecmd *)p->rightcmd;

		// printf("%s\n",left_cmd->scmd);
		// printf("%s\n",right_cmd->scmd);

		// struct pipecmd *nieto = right_cmd->rightcmd;

		// printf("%s\n",nieto->scmd);

		// struct pipecmd *nieto_nieto = nieto->rightcmd;

		// printf("%s\n",nieto_nieto->scmd);
		break;
	}
	}
}
