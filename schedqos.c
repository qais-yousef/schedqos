/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Qais Yousef */
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "configs_parser.h"
#include "netlink_monitor.h"
#include "qos_manager.h"
#include "sched_profiles.h"

#define DAEMON_NAME		"schedqosd"
#define LOG_FILE		"/var/log/schedqosd.log"
#define MAX_PATH		1024


static int save_old_log(const char *loogfile)
{
	char new_path[MAX_PATH];
	struct stat st;
	int i = 1;

	if (stat(LOG_FILE, &st) != 0 || st.st_size == 0) {
		/* Nothing to rotate */
		return 0;
	}

	while (1) {
		snprintf(new_path, MAX_PATH, "%s.%d", LOG_FILE, i);
		if (access(new_path, F_OK) != 0) {
			/* Found an available suffix */
			break;
		}
		i++;
	}

	if (rename(LOG_FILE, new_path) == 0) {
		FILE *fp = fopen(LOG_FILE, "w");
		if (fp) {
			fclose(fp);
			chmod(LOG_FILE, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		}
		LOG_INFO("Rotated %s to %s\n", LOG_FILE, new_path);
	} else {
		LOG_WARN("Error renaming log file");
		return -1;
	}

	return 0;
}

static int redirect_to_log(const char *logfile)
{
	int fd;

	save_old_log(logfile);

	fd = open(logfile, O_WRONLY | O_CREAT | O_APPEND, 0644);
	if (fd == -1) {
		LOG_ERROR("Failed to open logfile");
		return -1;
	}

	/* Redirect stdout (1) to the log file */
	if (dup2(fd, STDOUT_FILENO) == -1) {
		LOG_ERROR("Failed to redirect stdout to log file");
		return -1;
	}

	/* Redirect stderr (2) to the log file */
	if (dup2(fd, STDERR_FILENO) == -1) {
		LOG_ERROR("Failed to redirect stderr to log file");
		return -1;
	}

	close(fd);

	return 0;
}

static void apply_sched_profile(const char *profile)
{
	init_sched_profiles();
	parse_sched_profiles();
	sched_profiles_apply_profile(profile);
}

static int start_schedqosd(void)
{
	pid_t pid = get_pid_by_name("schedqosd");
	if (pid > 0) {
		LOG_FATAL("schedqosd already running, pid: %d", pid);
		return -1;
	}

	if (sqos_opts.daemon) {
		if (daemon(0, 1) == -1) {
			LOG_FATAL("Failed to start %s", DAEMON_NAME);
			return -1;
		}

		if (redirect_to_log(LOG_FILE)) {
			LOG_FATAL("Failed to redirect to log file %s", LOG_FILE);
			return -1;
		}
	}

	if (pthread_setname_np(pthread_self(), DAEMON_NAME) != 0) {
		LOG_ERROR("Failed to change name to %s", DAEMON_NAME);
		return -1;
	}

	init_qos_manager();
	parse_app_configs();
	parse_qos_mappings();
	/* schedqosd assumes qos sched_profile, apply automatically */
	apply_sched_profile("qos");
	start_netlink_monitor();
	return 0;
}

static int stop_schedqosd(void)
{
	pid_t pid = get_pid_by_name("schedqosd");
	int ret;

	ret = kill_by_pid(pid);
	sleep(1);
	return ret;
}

int main(int argc, char **argv)
{
	int err;

	err = argp_parse(&argp, argc, argv, ARGP_NO_EXIT | ARGP_IN_ORDER, NULL, NULL);
	if (err)
		return err;

	if (!sqos_opts.command)
		return 0;

	if (strcmp(sqos_opts.command, "start") == 0) {
		return start_schedqosd();
	} else if (strcmp(sqos_opts.command, "stop") == 0) {
		return stop_schedqosd();
	} else if (strcmp(sqos_opts.command, "restart") == 0) {
		/* we probably should verify we stopped correctly */
		stop_schedqosd();
		start_schedqosd();
	} else if (strcmp(sqos_opts.command, "sched_profile") == 0) {
		apply_sched_profile(sqos_opts.sched_profile);
	}

	return 0;
}
