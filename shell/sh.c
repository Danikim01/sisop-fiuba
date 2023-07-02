#include "defs.h"
#include "types.h"
#include "readline.h"
#include "runcmd.h"

#include "history.h"

char prompt[PRMTLEN] = { 0 };

// pid_t background_pgid = 0;

void
handle_child_termination(int signal)
{
	int saved_errno = errno; //We save errno in order not to interfer with other functions
	//Obs: not doing this interfers with the behaviour of the read function in line 211 readline.c
	//and pid 1 is printed below

	pid_t pid;
	int status;

	if ((pid = waitpid(0, &status, WNOHANG)) > 0) {
		//waitpid(0, ...) beacuse then it catches any processes
		//with the same group id as the shell's id (and that was 
		//the one chosen for background processes)
		printf("==>  %d terminado.\n", pid);
	}

	errno = saved_errno;
}

// runs a shell command
static void
run_shell()
{
	// Sets the action for child termination
	struct sigaction sa;
	sa.sa_handler = handle_child_termination;  // Hanlder is assigned
	sigemptyset(&sa.sa_mask);  // I don't want to block any signal
	sa.sa_flags = SA_RESTART;  // No flags needed
	sigaction(SIGCHLD,
	          &sa,
	          NULL);  // Action for child termination in main process is set

    // setpgid(0, 0);  // Set the shell's own process group
    // background_pgid = getpid(); // store the shell's pid as the background process group id

	char *cmd;
	// Set the shell process as the process group leader here
	while ((cmd = read_line(prompt)) != NULL)
		if (run_cmd(cmd) == EXIT_SHELL)
			return;
}

// initializes the shell
// with the "HOME" directory
static void
init_shell()
{
	char buf[BUFLEN] = { 0 };
	char *home = getenv("HOME");

	if (chdir(home) < 0) {
		snprintf(buf, sizeof buf, "cannot cd to %s ", home);
		perror(buf);
	} else {
		snprintf(prompt, sizeof prompt, "(%s)", home);
	}
}

int
main(void)
{
	init_shell();

#ifndef SHELL_NO_INTERACTIVE
	history_init();
#endif

	run_shell();

#ifndef SHELL_NO_INTERACTIVE
	history_free();
#endif

	return 0;
}
