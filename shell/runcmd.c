#include "runcmd.h"
#include "history.h"

int status = 0;
struct cmd *parsed_pipe;

extern struct history_t *global_hist;

void
handle_event_designators(char **buf)
{
	// Supports exclusively !! or !-n, if there are any other command limes either separated
	// by a space or right next to either ! or -n they are not taken into account as
	//!! and !-n event dessignators are used to execute a previous command rather than modify them
	if ((*buf)[1] == '!') {
		if (block_contains(*buf, SPACE) == -1 && strlen(*buf) > 2)
			return;
		char *aux = history_get_move_index_up();
		strcpy(*buf, aux);
		history_get_move_index_down();
	}

	if ((*buf)[1] == '-') {
		// Dynamic memory is exclusively used for strcpy to have a valid
		// memory location to copy to
		int num_size = sizeof(int) * strlen(*buf) - 2 +
		               1;  // +1 for null terminator
		char *aux = malloc(num_size);
		if (aux == NULL) {
			fprintf_debug(stderr, "Malloc failed\n.");
			return;
		}

		strcpy(aux, *buf + 2);  // Get the number in the !-number command

		if (!isNumeric(aux))
			return;

		int n = atoi(aux);
		// OBS: Atoi fails if the num aux represents is greater than
		// 2^32 and returns some negative number, didn`t take it into
		// consideration because by convention history size is at max
		// 2000
		if (n <= 0) {
			fprintf_debug(stderr, "Atoi failed.\n");
			return;
		}

		if (n > MAX_HIST_SIZE) {
			fprintf_debug(stderr, "Index %d is out of bounds.\n", n);
			return;
		}

		// Get the N-th last command
		for (int i = 0; i < n; i++) {
			char *temp = history_get_move_index_up();
			if (strlen(temp) > strlen(aux)) {
				aux = realloc(aux,
				              sizeof(char) * strlen(temp) + 1);
				if (aux == NULL) {
					fprintf_debug(stderr, "Realloc failed.\n");
					free(aux);
					return;
				}
			}
			strcpy(aux, temp);
		}

		strcpy(*buf, aux);

		// Return index to its original position
		for (int i = 0; i < n; i++) {
			history_get_move_index_down();
		}

		free(aux);
	}
}


// runs the command in 'cmd'
int
run_cmd(char *cmd)
{
	pid_t p;
	struct cmd *parsed;


	// if the "enter" key is pressed
	// just print the prompt again
	if (cmd[0] == END_STRING)
		return 0;

	// "history" built-in call
	if (history(cmd))
		return 0;

// append to history
#ifndef SHELL_NO_INTERACTIVE
	if (cmd[0] != '!')
		append_history(cmd);
#endif

	// Event designators are handled here because its needed to handle them
	// before parsing and before checks for built in command are made, but
	// after appending in history because commands runned by using an event
	// dessignator are not added to history
	if (cmd[0] == '!') {
		handle_event_designators(&cmd);
		printf("%s\n",
		       cmd);  // it prints the command to be executed before doing so
	}

	// "cd" built-in call
	if (cd(cmd))  //
		return 0;

	// "exit" built-in call
	if (exit_shell(cmd))
		return EXIT_SHELL;

	// "pwd" built-in call
	if (pwd(cmd))
		return 0;

	// parses the command line
	parsed = parse_line(cmd);

	// forks and run the command
	if ((p = fork()) == 0) {
		// keep a reference
		// to the parsed pipe cmd
		// so it can be freed later
		if (parsed->type == PIPE)
			parsed_pipe = parsed;

		exec_cmd(parsed);
	}

	// stores the pid of the process
	parsed->pid = p;

	// background process special treatment
	// Hint:
	// - check if the process is
	//		going to be run in the 'back'
	// - print info about it with
	// 	'print_back_info()'
	//
	// Your code here

	// Antes de cada prompt, checkea si algun hijo temrino (pid = -1)
	waitpid(-1, &status, WNOHANG);

	if (parsed->type == BACK) {
		// Imprimo la info del proceso corriendo en back
		print_back_info(parsed);
		// Hay que poner los procesos de back en un grupo
		// constantemente (fuera de aca) checkear si termino alguno de los procesos en back
	} else {
		// waits for the process to finish
		waitpid(p, &status, 0);
		print_status_info(parsed);
	}

#ifndef SHELL_NO_INTERACTIVE
	reset_history_index();
#endif

	free_command(parsed);
	return 0;
}
