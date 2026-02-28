/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Qais Yousef */
#include <stdbool.h>
#include <sys/types.h>

enum qos_tag;

void init_qos_manager(void);
void deinit_qos_manager(void);
bool add_thread_qos_tag(const void *app, const char *comm, char *qos_tag);
void *create_app_config(const char *cmdline);
void create_app_instance(const pid_t tgid);
void destroy_app_instance(const pid_t tgid);
bool apply_thread_qos(pid_t pid, pid_t tgid, const char *comm);
