#ifndef CMDLINE_ARGS_H_
#define CMDLINE_ARGS_H_

struct cmdline_args {
	int background;
	int loglevel;
	const char *logfile;
	const char *pidfile;
	int force;
};

extern struct cmdline_args cmdargs;

int parse_commandline(int argc, char **argv);

#endif /* CMDLINE_ARGS_H_ */
