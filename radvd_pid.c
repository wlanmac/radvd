
#include "config.h"
#include "radvd_pid.h"
#include "pathnames.h"


pid_t radvd_pid()
{
	FILE * radvd_pid_file;
	pid_t radvd_pid;

	/* get the real radvd pid */
	radvd_pid_file = fopen(PATH_RADVD_PID, "r");

	if (!radvd_pid_file) {
		return -1;
	}

	if (1 != fscanf(radvd_pid_file, "%d", &radvd_pid)) {
		fclose(radvd_pid_file);
		return -1;
	}

	fclose(radvd_pid_file);

	return radvd_pid;
}

