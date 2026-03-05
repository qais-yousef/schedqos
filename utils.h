/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Qais Yousef */
#include <stdbool.h>

#define TASK_COMM_LEN		16

char *get_cmdline_by_pid(pid_t pid);
bool get_comm_by_pid(pid_t pid, char *comm);
bool is_numeric(const char *str);
bool is_fair_task(pid_t pid);
pid_t get_pid_by_name(const char *name);
bool kill_by_pid(pid_t pid);
