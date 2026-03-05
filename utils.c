/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Qais Yousef */
#include <ctype.h>
#include <fcntl.h>
#include <glib.h>
#include <limits.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"

/*
 * Returns the short name for the cmdline. Callers must call g_free() on the
 * returned pointer.
 */
gchar* get_short_name(const gchar *cmdline) {
	gchar *basename;
	gchar **parts;

	if (!cmdline || *cmdline == '\0')
		return NULL;

	parts = g_strsplit(cmdline, " ", 2);
	basename = g_path_get_basename(parts[0]);

	g_strfreev(parts);

	return basename;
}

/*
 * Reads cmdline for a pid and returns a copy. Callers must ensure to g_free()
 * the copy.
 */
char *get_cmdline_by_pid(pid_t pid)
{
	ssize_t bytes_read;
	char buffer[4096];
	char path[64];
	int fd, i;

	snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return NULL;

	bytes_read = read(fd, buffer, sizeof(buffer) - 1);
	close(fd);

	if (bytes_read <= 0)
		return NULL;

	buffer[bytes_read] = '\0';

	/*
	 * Replace internal NULLs with spaces to create a single string
	 * The kernel puts a NULL after the last argument
	 */
	for (i = 0; i < bytes_read - 1; i++) {
		if (buffer[i] == '\0') {
			buffer[i] = ' ';
		}
	}

	return get_short_name(buffer);
}

bool get_comm_by_pid(pid_t pid, char *comm)
{
	char path[64];
	FILE *fp;

	snprintf(path, sizeof(path), "/proc/%d/comm", pid);

	fp = fopen(path, "r");
	if (!fp)
		return false;

	if (fgets(comm, TASK_COMM_LEN, fp) != NULL) {
		fclose(fp);
		/* Remove the trailing newline added by the kernel */
		comm[strcspn(comm, "\n")] = '\0';
		return true;
	}

	fclose(fp);
	return false;
}

bool is_numeric(const char *str)
{
	if (str == NULL || *str == '\0')
		return false;

	while (*str) {
		if (!isdigit((unsigned char)*str))
			return false;
		str++;
	}

	return true;
}

bool is_fair_task(pid_t pid)
{
	int policy = sched_getscheduler(pid);

	if (policy == -1) {
		LOG_ERROR("Failed to get policy for %d", pid);
		return false;
	}

	switch (policy & ~SCHED_RESET_ON_FORK) {
	case SCHED_OTHER:
	case SCHED_IDLE:
	case SCHED_BATCH:
		return true;
	default:
		return false;
	}
}

pid_t get_pid_by_name(const char *name)
{
	DIR *dir = opendir("/proc");
	struct dirent *entry;
	pid_t found_pid = -1;

	if (!dir)
		return -1;

	while ((entry = readdir(dir)) != NULL) {
		char path[PATH_MAX];
		char comm[256];
		FILE *fp;

		if (!isdigit(*entry->d_name))
			continue;

		snprintf(path, sizeof(path), "/proc/%s/comm", entry->d_name);

		fp = fopen(path, "r");
		if (fp) {
			if (fgets(comm, sizeof(comm), fp)) {
				/* Strip newline */
				comm[strcspn(comm, "\n")] = 0;

				if (strcmp(comm, name) == 0) {
					found_pid = strtoul(entry->d_name, NULL, 10);
					fclose(fp);
					/* Return the first match */
					break;
				}
			}
			fclose(fp);
		}
	}
	closedir(dir);
	return found_pid;
}

bool kill_by_pid(pid_t pid)
{
	if (pid <= 0)
		return false;

	if (kill(pid, SIGTERM) == 0)
		return true;

	LOG_ERROR("Failed to kill process: %d", pid);
		return false;
}
