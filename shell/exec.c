#include "exec.h"

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
	// Your code here
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
#define FILE_MODE (S_IRUSR | S_IWUSR)
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
	case EXEC:
		//spawns a command
		int fk = fork();
		if (fk == -1) {
			perror("error al crear el fork");
			exit(-1);
		}

		if (fk == 0) {
			e = (struct execcmd *) cmd;
			execvp(e->argv[0], e->argv);
			perror("execvp");
			exit(-1);
		} else
			wait(NULL);

		break;

	case BACK: {
		// runs a command in background
		//
		// esperado Your code here

		int fk = fork();
		if (fk == -1) {
			perror("error al crear el fork");
			exit(-1);
		}

		if (fk == 0) {
			b = (struct backcmd *) cmd;
			e = (struct execcmd *) b->c;
			printf("[PID=%d]\n", getpid());
			execvp(e->argv[0], e->argv);

			perror("execvp");
			exit(-1);
		} else {
			waitpid(fk, NULL, WNOHANG);
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



		if(strlen(r->out_file) > 0 && strlen(r->err_file) == 0){
			int fd_abierto = open_redir_fd(r->out_file,O_RDWR);
			printf("El fd abierto es %d\n",fd_abierto);
			dup2(fd_abierto,1);
			close(fd_abierto);
			execvp(r->argv[0], r->argv);
		}else if(strlen(r->in_file) > 0){
			int fd_abierto = open_redir_fd(r->in_file,O_RDONLY);
			printf("El fd abierto es%d\n",fd_abierto);
			dup2(fd_abierto,0);
			close(fd_abierto);
			execvp(r->argv[0], r->argv);
		}else if(strlen(r->err_file) > 0 && strlen(r->out_file) > 0){
			/*
			int index = block_contains(r->err_file, '&');
			printf("El index es %d\n",index);
			if(index == 0){
				printf("Entraaaaaaaaaaaaaaa\n");
				int fd_abierto = open_redir_fd(r->err_file,O_RDWR);
				
				int fd_abierto2 = open_redir_fd(r->out_file,O_RDWR);
				printf("El fd abierto es %d\n",fd_abierto2);
				dup2(fd_abierto2,1);
				dup2(fd_abierto2,2);

				close(fd_abierto);
				close(fd_abierto2);
				execvp(r->argv[0], r->argv);
				_exit(1);
			}
			*/
			
			int fd_abierto = open_redir_fd(r->err_file,O_RDWR);
			printf("El fd abierto es %d\n", fd_abierto);
			dup2(fd_abierto, 2);

			int fd_abierto2 = open_redir_fd(r->out_file,O_RDWR);
			printf("El fd abierto es %d\n",fd_abierto2);
			dup2(fd_abierto2,1);

			close(fd_abierto);
			close(fd_abierto2);
			execvp(r->argv[0], r->argv);
			
		}
    	_exit(1);
		break;
	}

	case PIPE: {
		// pipes two commands
		//
		// Your code here
		printf("Pipes are not yet implemented\n");

		// free the memory allocated
		// for the pipe tree structure
		free_command(parsed_pipe);

		break;
	}
	}
}
