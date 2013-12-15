
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <net/if_arp.h>

#include "radvd.h"
#include "pathnames.h"
#include "tuntap.h"

#define MICROWAIT 5000

void write_config(char const * dev, char const * mode);

void write_config(char const * dev, char const * mode)
{

	FILE * out = fopen("radvd.conf", mode);

	if (!out) {
		perror("fopen failed");
		exit(1);
	}

	fprintf(out, "interface %s {\n", dev);
	fprintf(out, "     AdvSendAdvert on;\n");
	fprintf(out, "     MinRtrAdvInterval 1;\n");
	fprintf(out, "     MaxRtrAdvInterval 3;\n");
	fprintf(out, "     AdvLinkMTU 1472;\n");
	fprintf(out, "     prefix 1234:5678:9abc::/64 {\n");
	fprintf(out, "             AdvOnLink off;\n");
	fprintf(out, "             AdvAutonomous on;\n");
	fprintf(out, "             AdvRouterAddr on; \n");
	fprintf(out, "             AdvPreferredLifetime 90;\n");
	fprintf(out, "             AdvValidLifetime 120;\n");
	fprintf(out, "     };\n");
	fprintf(out, "     RDNSS 2001:470:20::2\n");
	fprintf(out, "     {\n");
	fprintf(out, "             AdvRDNSSLifetime 60;\n");
	fprintf(out, "     };\n");
	fprintf(out, "};\n");

	fclose(out);
}

int main(int argc, char * argv[])
{
	pid_t pid;
	int fd;
	char dev[IFNAMSIZ] = {""};
	FILE * radvd_pid_file;
	

	unlink("radvd.conf");

	fd = tun_alloc(dev);
	if (fd < 0) {
		perror("tun_alloc failed");
		exit(1);
	}

	write_config(dev, "a");

	pid = fork();

	if (pid < 0) {
		perror("fork failed");
		exit(1);
	} else if (pid == 0) {
		char * args[] = {
			"./radvd",
			"--config",
			"radvd.conf",
			0,
		};
		execvp(args[0], args);
	} else {
		int status = -1;
		pid_t radvd_pid;
		pid_t rc;
		pid_t psea_pid1;
		pid_t psea_pid2;
		FILE * psea;

		/* radvd will daemon_fork, then exit.  Wait for that pid. */			
		rc = wait(&status);
		unlink("radvd.conf");

		if (rc != pid || status != 0) {
			fprintf(stderr, "radvd failed to daemonize");
			exit(1);
		}

		usleep(MICROWAIT);

		/* get the real radvd pid */
		radvd_pid_file = fopen(PATH_RADVD_PID, "r");

		if (!radvd_pid_file) {
			fprintf(stderr, "radvd pid file doesn't exist but should.\n");
			exit(1);
		}

		if (1 != fscanf(radvd_pid_file, "%d", &radvd_pid)) {
			fprintf(stderr, "radvd pid file doesn't contain a pid but should.\n");
			exit(1);
		}

		fclose(radvd_pid_file);

		if (0 != system("ps -ea | grep radvd | awk '{print $1}' | sort > check-pid.txt")) {
			fprintf(stderr, "shelling out failed.\n");
			exit(1);
		}

		psea = fopen("check-pid.txt", "r");

		if (!psea) {
			fprintf(stderr, "unable to ps -ea to get radvd PID's.\n");
			exit(1);
		}
		
		unlink("check-pid.txt");

		if (2 != fscanf(psea, " %d %d", &psea_pid1, &psea_pid2)) {
			fprintf(stderr, "unable to read ps -ea file to get radvd PID's.\n");
			exit(1);
		}

		if (psea_pid2 != radvd_pid) {
			fprintf(stderr, "ps -ea PID does not match %s.\n", PATH_RADVD_PID);
			exit(1);
		}

		kill(radvd_pid, SIGINT);

		usleep(MICROWAIT);

		radvd_pid_file = fopen(PATH_RADVD_PID, "r");

		if (radvd_pid_file) {
			fprintf(stderr, "radvd pid file still exist after radvd returned.\n");
			exit(1);
		}
	}

	return 0;
}

