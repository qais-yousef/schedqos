/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Qais Yousef */
#include <stdlib.h>
#include <string.h>

#include "parse_argp.h"

#define XSTR(x) STR(x)
#define STR(x) #x

const char *argp_program_version = "schedqos " XSTR(SA_VERSION);
const char *argp_program_bug_address = "<qyousef@layalina.io>";

struct sqos_opts sqos_opts = {
	/* main options */
	.command = NULL,

	/* common options */
	.configs_path = "/var/run/schedqos",
	.verbose = false,

	/* start/restart options*/
	.daemon = false,

	/* sched_profile options */
	.sched_profile = NULL,
};

enum sqos_opts_flags {
	OPT_DUMMY_START = 0x80,

	OPT_CONFIGS_PATH,
};

/* subcommand parser: start/restart */
static error_t start_parse_arg(int key, char *arg, struct argp_state *state)
{
	switch (key) {
	case OPT_CONFIGS_PATH:
		sqos_opts.configs_path = arg;
		break;
	case 'd':
		sqos_opts.daemon = true;
		break;
	case 'v':
		sqos_opts.verbose = true;
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

static char start_doc[] =
"Start schedqosd daemon";

static char restart_doc[] =
"Restart schedqosd daemon";

static const struct argp_option start_options[] = {
	{ "configs-path", OPT_CONFIGS_PATH, "PATH", 0, "Path to configs file, /var/run/sched_qos/ by default." },
	{ "daemon", 'd', 0, 0, "Start/restart as a daemon." },
	{ "verbose", 'v', 0, 0, "Enable verbose logging." },
	{ 0 },
};

const struct argp start_argp = {
	.options = start_options,
	.parser = start_parse_arg,
	.doc = start_doc,
};

const struct argp restart_argp = {
	.options = start_options,
	.parser = start_parse_arg,
	.doc = restart_doc,
};

/* subcommand parser: stop */
static error_t stop_parse_arg(int key, char *arg, struct argp_state *state)
{
	switch (key) {
	case 'v':
		sqos_opts.verbose = true;
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

static char stop_doc[] =
"Stop schedqosd daemon";

static const struct argp_option stop_options[] = {
	{ "verbose", 'v', 0, 0, "Enable verbose logging." },
	{ 0 },
};

const struct argp stop_argp = {
	.options = stop_options,
	.parser = stop_parse_arg,
	.doc = stop_doc,
};

/* subcommand parser: sched_profile */
static error_t sched_profile_parse_arg(int key, char *arg, struct argp_state *state)
{
	switch (key) {
	case OPT_CONFIGS_PATH:
		sqos_opts.configs_path = arg;
		LOG_INFO("configs_path: %s", sqos_opts.configs_path);
		break;
	case 'v':
		sqos_opts.verbose = true;
		break;
	case ARGP_KEY_ARG:
		if (state->arg_num == 0) {
			sqos_opts.sched_profile = arg;
		}
		break;
	case ARGP_KEY_END:
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static char sched_profile_doc[] =
"Apply sched profile";

static const struct argp_option sched_profile_options[] = {
	{ "configs-path", OPT_CONFIGS_PATH, "PATH", 0, "Path to configs file, /var/run/sched_qos/ by default." },
	{ "verbose", 'v', 0, 0, "Enable verbose logging." },
	{ 0 },
};

const struct argp sched_profile_argp = {
	.options = sched_profile_options,
	.parser = sched_profile_parse_arg,
	.doc = sched_profile_doc,
};

/* main parser */
static error_t parse_arg(int key, char *arg, struct argp_state *state)
{
	switch (key) {
	case 'v':
		sqos_opts.verbose = true;
		break;
	case ARGP_KEY_ARG:
		if (state->arg_num == 0) {
			int argc = state->argc - state->next + 1;
			char **argv = &state->argv[state->next - 1];
			const struct argp *argp = NULL;

			sqos_opts.command = arg;

			if (strcmp(arg, "start") == 0) {
				argp = &start_argp;
			} else if (strcmp(arg, "stop") == 0) {
				argp = &stop_argp;
			} else if (strcmp(arg, "restart") == 0) {
				argp = &restart_argp;
			} else if (strcmp(arg, "sched_profile") == 0) {
				argp = &sched_profile_argp;
			}

			if (argp) {
				int err = argp_parse(argp, argc, argv, ARGP_IN_ORDER, NULL, NULL);
				if (err)
					return err;
			}
			state->next = state->argc;
		}
		break;
	case ARGP_KEY_END:
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static char doc[] =
"Sched QoS";

static const struct argp_option options[] = {
	{ "verbose", 'v', 0, 0, "Enable verbose logging." },
	{ 0 },
};

/* main argp */
const struct argp argp = {
	.options = options,
	.parser = parse_arg,
	.doc = doc,
};
