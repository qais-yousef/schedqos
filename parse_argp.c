/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Qais Yousef */
#include <stdlib.h>
#include <string.h>

#include "parse_argp.h"

#define XSTR(x) STR(x)
#define STR(x) #x

const char *argp_program_version = "schedqos " XSTR(SA_VERSION);
const char *argp_program_bug_address = "<qyousef@layalina.io>";

static char doc[] =
"Sched QoS Daemon";

struct sqos_opts sqos_opts = {
	.configs_path = "/var/run/sched_qos",
};

enum sqos_opts_flags {
	OPT_DUMMY_START = 0x80,

	OPT_CONFIGS_PATH,
};

static const struct argp_option options[] = {
	{ "configs-path", OPT_CONFIGS_PATH, "PATH", 0, "Path to configs file, /var/run/sched_qos/ by default." },
	{ 0 },
};

static error_t parse_arg(int key, char *arg, struct argp_state *state)
{
	switch (key) {
	case OPT_CONFIGS_PATH:
		sqos_opts.configs_path = arg;
		break;
	case ARGP_KEY_ARG:
		argp_usage(state);
		break;
	case ARGP_KEY_END:
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

const struct argp argp = {
	.options = options,
	.parser = parse_arg,
	.doc = doc,
};
