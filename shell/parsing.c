#include "parsing.h"

// parses an argument of the command stream input
static char *
get_token(char *buf, int idx)
{
	char *tok;
	int i;

	tok = (char *) calloc(ARGSIZE, sizeof(char));
	i = 0;

	while (buf[idx] != SPACE && buf[idx] != END_STRING) {
		tok[i] = buf[idx];
		i++;
		idx++;
	}

	return tok;
}

// parses and changes stdin/out/err if needed
static bool
parse_redir_flow(struct execcmd *c, char *arg)
{
	int inIdx, outIdx;

	// flow redirection for output
	if ((outIdx = block_contains(arg, '>')) >= 0) {
		switch (outIdx) {
		// stdout redir
		case 0: {
			strcpy(c->out_file, arg + 1);
			break;
		}
		// stderr redir
		case 1: {
			strcpy(c->err_file, &arg[outIdx + 1]);
			break;
		}
		}

		free(arg);
		c->type = REDIR;

		return true;
	}

	// flow redirection for input
	if ((inIdx = block_contains(arg, '<')) >= 0) {
		// stdin redir
		strcpy(c->in_file, arg + 1);

		c->type = REDIR;
		free(arg);

		return true;
	}

	return false;
}

// parses and sets a pair KEY=VALUE
// environment variable
static bool
parse_environ_var(struct execcmd *c, char *arg)
{
	// sets environment variables apart from the
	// ones defined in the global variable "environ"
	if (block_contains(arg, '=') > 0) {
		// checks if the KEY part of the pair
		// does not contain a '-' char which means
		// that it is not a environ var, but also
		// an argument of the program to be executed
		// (For example:
		// 	./prog -arg=value
		// 	./prog --arg=value
		// )
		if (block_contains(arg, '-') < 0) {
			c->eargv[c->eargc++] = arg;
			return true;
		}
	}

	return false;
}

// this function will be called for every token, and it should
// expand environment variables. In other words, if the token
// happens to start with '$', the correct substitution with the
// environment value should be performed. Otherwise the same
// token is returned. If the variable does not exist, an empty string should be
// returned within the token
//
// Hints:
// - check if the first byte of the argument contains the '$'
// - expand it and copy the value in 'arg'
// - remember to check the size of variable's value
//		It could be greater than the current size of 'arg'
//		If that's the case, you should realloc 'arg' to the new size.
static char *
expand_environ_var(char *arg)
{
	// Your code here

	if (arg[0] == '$') {
		// the correct substitution with the environment value should be
		// performed advance to the next character
		arg++;

		// get the environment variable name
		char *auxiliar = arg;
		char *env_var_name = arg;

		while (*auxiliar != '\0') {
			auxiliar++;
		}

		// iterate until the end of the environment variable name
		while (isalnum(*arg) || (*arg == '_')) {
			arg++;
		}

		// save the end position of the environment variable name
		char *env_var_name_end = arg;

		// replace the environment variable name with the corresponding value
		if (env_var_name_end > env_var_name) {
			// copy the environment variable name into a null-terminated string
			size_t env_var_len = env_var_name_end - env_var_name;
			char env_var[env_var_len + 1];
			strncpy(env_var, env_var_name, env_var_len);
			env_var[env_var_len] = '\0';


			printf_debug("El valor de env_var es:%s\n", env_var);

			// get the value of the environment variable
			char *env_value = getenv(env_var);

			printf_debug("El valor de getenv(env_var) es %s\n",
			             env_value);

			// copy the value into a new string
			if (env_value != NULL) {
				char *new_arg =
				        malloc(strlen(env_value) +
				               (auxiliar - env_var_name_end) + 1);
				strcpy(new_arg, env_value);
				strcat(new_arg, arg);

				printf_debug("Concateno %s con %s (arg)\n",
				             new_arg,
				             arg);

				printf_debug("El nuevo argumento es %s\n",
				             new_arg);
				return new_arg;
			} else {
				printf_debug("Error: la variable de entorno %s "
				             "no existe\n",
				             env_var);
				return strdup("");
			}
		}  // if the environment variable name is empty, return an empty string
		else {
			return strdup("");
		}
	}  // if the argument doesn't start with '$', just return the same argument
	else {
		return strdup(arg);
	}
}

// parses one single command having into account:
// - the arguments passed to the program
// - stdin/stdout/stderr flow changes
// - environment variables (expand and set)
static struct cmd *
parse_exec(char *buf_cmd)
{
	struct execcmd *c;
	char *tok;
	int idx = 0, argc = 0;

	// expand_environ_var(buf_cmd);

	c = (struct execcmd *) exec_cmd_create(buf_cmd);

	while (buf_cmd[idx] != END_STRING) {
		tok = get_token(buf_cmd, idx);
		idx = idx + strlen(tok);

		if (buf_cmd[idx] != END_STRING)
			idx++;

		if (parse_redir_flow(c, tok))
			continue;

		if (parse_environ_var(c, tok))
			continue;

		printf_debug("El token es %s\n", tok);
		tok = expand_environ_var(tok);

		c->argv[argc++] = tok;
	}

	c->argv[argc] = (char *) NULL;
	c->argc = argc;

	return (struct cmd *) c;
}

// parses a command knowing that it contains the '&' char
static struct cmd *
parse_back(char *buf_cmd)
{
	int i = 0;
	struct cmd *e;

	while (buf_cmd[i] != '&')
		i++;

	buf_cmd[i] = END_STRING;

	e = parse_exec(buf_cmd);

	return back_cmd_create(e);
}

// parses a command and checks if it contains the '&'
// (background process) character
static struct cmd *
parse_cmd(char *buf_cmd)
{
	if (strlen(buf_cmd) == 0)
		return NULL;

	int idx;

	// checks if the background symbol is after
	// a redir symbol, in which case
	// it does not have to run in in the 'back'
	if ((idx = block_contains(buf_cmd, '&')) >= 0 && buf_cmd[idx - 1] != '>')
		return parse_back(buf_cmd);

	return parse_exec(buf_cmd);
}

// parses the command line
// looking for the pipe character '|'

// struct cmd *
// parse_line(char *buf)
// {
// 	struct cmd *r, *l;

// 	char *right = split_line(buf, '|');

// 	l = parse_cmd(buf);


// 	r = parse_cmd(right);


// 	return pipe_cmd_create(l, r);
// }


struct cmd *
parse_line(char *buf)
{
	struct cmd *r, *l;

	char *right = split_line(buf, '|');

	l = parse_cmd(buf);
	if (block_contains(right, '|') >= 0) {
		r = parse_line(right);
	} else {
		r = parse_cmd(right);
	}
	return pipe_cmd_create(l, r);
}
