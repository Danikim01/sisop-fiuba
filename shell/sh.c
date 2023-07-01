#include "defs.h"
#include "types.h"
#include "readline.h"
#include "runcmd.h"

#include "history.h"

char prompt[PRMTLEN] = { 0 };

void handle_child_termination(int signal) {
	int saved_errno = errno;
    pid_t pid;
    int status;

    while ((pid = waitpid((pid_t)(-1), &status, WNOHANG)) > 0) {
        printf("==>  %d terminado.\n", pid);
    }

    errno = saved_errno;
}

// runs a shell command
static void
run_shell()
{
	//Sets the action for child termination
	struct sigaction sa;
	sa.sa_handler = handle_child_termination; //Hanlder is assigned
	sigemptyset(&sa.sa_mask); //I don't want to block any signal
	sa.sa_flags = SA_RESTART; //No flags needed
	sigaction(SIGCHLD, &sa, NULL);//Action for child termination in main process is set
	
	char *cmd;
	//Set the shell process as the process group leader here
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
