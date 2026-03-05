/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Qais Yousef */
#ifndef __PARSE_ARGS_H__
#define __PARSE_ARGS_H__
#include <argp.h>
#include <stdbool.h>

struct sqos_opts {
	/* main options */
	const char *command;

	/* common options */
	const char *configs_path;
	bool verbose;

	/* sched_profile options */
	const char *sched_profile;
};

extern struct sqos_opts sqos_opts;
extern const struct argp argp;

#endif /* __PARSE_ARGS_H__ */
