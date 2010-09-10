/*
 *   Copyright (C) 2010 Michael Buesch <mb@bu3sch.de>
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation; either version 2
 *   of the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 */

#include "args.h"

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>


struct cmdline_args cmdargs;


static void usage(FILE *fd, int argc, char **argv)
{
	fprintf(fd, "Usage: %s [OPTIONS]\n", argv[0]);
	fprintf(fd, "\n");
	fprintf(fd, "  -B|--background             Fork into background\n");
	fprintf(fd, "  -P|--pidfile PATH           Create a PID-file\n");
	fprintf(fd, "  -l|--loglevel LEVEL         Set logging level\n");
	fprintf(fd, "                              0=error, 1=info, 2=debug\n");
	fprintf(fd, "  -L|--logfile PATH           Write log to file\n");
	fprintf(fd, "  -f|--force                  Force mode\n");
	fprintf(fd, "\n");
	fprintf(fd, "  -h|--help                   Print this help text\n");
}

int parse_commandline(int argc, char **argv)
{
	static struct option long_options[] = {
		{ "help", no_argument, 0, 'h' },
		{ "background", no_argument, 0, 'B' },
		{ "pidfile", required_argument, 0, 'P' },
		{ "loglevel", required_argument, 0, 'l' },
		{ "logfile", required_argument, 0, 'L' },
		{ "force", no_argument, 0, 'f' },
	};
	int c, idx;

	while (1) {
		c = getopt_long(argc, argv, "hBP:l:L:f",
				long_options, &idx);
		if (c == -1)
			break;
		switch (c) {
		case 'h':
			usage(stdout, argc, argv);
			return 1;
		case 'B':
			cmdargs.background = 1;
			break;
		case 'P':
			cmdargs.pidfile = optarg;
			break;
		case 'l':
			if (sscanf(optarg, "%d", &cmdargs.loglevel) != 1) {
				fprintf(stderr, "Failed to parse --loglevel argument.\n");
				return -1;
			}
			break;
		case 'L':
			cmdargs.logfile = optarg;
			break;
		case 'f':
			cmdargs.force = 1;
			break;
		default:
			return -1;
		}
	}

	return 0;
}
