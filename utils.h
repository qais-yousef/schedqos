/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Qais Yousef */
#include <stdbool.h>

#define TASK_COMM_LEN		16

char *get_cmdline_by_pid(pid_t pid);
bool get_comm_by_pid(pid_t pid, char *comm);
