
#include "config.h"
#include "fork_radvd.h"
#include "radvd_pid.h"

#include <stdarg.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

pid_t fork_radvd(char * arg, ...)
{
	pid_t pid;

	pid = fork();

	if (pid < 0) {
		perror("fork failed");
		exit(1);
	} else if (pid == 0) {
		int used = 1;
		char * argv[50] = {"./radvd"};
		va_list va;
		va_start(va, arg);
		argv[used++] = arg;
		arg = va_arg(va, char *);
		while (arg && used < sizeof(argv)/sizeof(argv[0])) {
			argv[used++] = arg;
			arg = va_arg(va, char *);
		}
		if (arg) {
			fprintf(stderr, "ran out of space in argv\n");
			exit(1);
		}
		execv("./radvd", argv);
	}

	usleep(25000);

	pid = radvd_pid();

	return pid;
}

