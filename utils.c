/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Qais Yousef */
#include <fcntl.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"

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

	return g_strdup(buffer);
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

