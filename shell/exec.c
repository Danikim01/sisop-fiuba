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
	                      (flags == O_RDONLY ? 0 : O_CREAT | O_TRUNC),
	              S_IRUSR | S_IWUSR);
	if (fd == -1) {
		printf_debug("Fallo open con file:\n", file);
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
		// changes the input/output/stderr flow
		//
		// To check if a redirection has to be performed
		// verify if file name's length (in the execcmd struct)
		// is greater than zero
		//
		// Your code here
		r = (struct execcmd *) cmd;

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


		if (strlen(r->out_file) > 0 && strlen(r->err_file) == 0) {
			int fd_abierto = open_redir_fd(r->out_file, O_RDWR);
			printf("El fd abierto es %d\n", fd_abierto);
			dup2(fd_abierto, 1);
			close(fd_abierto);
			execvp(r->argv[0], r->argv);
			perror("execvp");
		} else if (strlen(r->in_file) > 0) {
			int fd_abierto = open_redir_fd(r->in_file, O_RDONLY);
			printf("El fd abierto es%d\n", fd_abierto);
			dup2(fd_abierto, 0);
			close(fd_abierto);
			execvp(r->argv[0], r->argv);
			perror("execvp");
		} else if (strlen(r->err_file) > 0 && strlen(r->out_file) > 0) {
			int index = block_contains(r->err_file, '&');
			printf("El index es %d\n", index);
			if (index == 0) {
				int fd = open_redir_fd(r->out_file, O_RDWR);
				dup2(fd, STDOUT_FILENO);
				dup2(fd, STDERR_FILENO);
				close(fd);
				execvp(r->argv[0], r->argv);
				perror("execvp");
			}

			int fd_abierto = open_redir_fd(r->err_file, O_RDWR);
			printf("El fd abierto es %d\n", fd_abierto);
			dup2(fd_abierto, 2);

			int fd_abierto2 = open_redir_fd(r->out_file, O_RDWR);
			printf("El fd abierto es %d\n", fd_abierto2);
			dup2(fd_abierto2, 1);

			close(fd_abierto);
			close(fd_abierto2);
			execvp(r->argv[0], r->argv);
			perror("execvp");
		}
		_exit(1);
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
