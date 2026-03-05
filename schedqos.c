/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Qais Yousef */
#include <string.h>

#include "configs_parser.h"
#include "netlink_monitor.h"
#include "qos_manager.h"
#include "sched_profiles.h"

int main(int argc, char **argv)
{
	int err;

	err = argp_parse(&argp, argc, argv, ARGP_NO_EXIT | ARGP_IN_ORDER, NULL, NULL);
	if (err)
		return err;

	if (strcmp(sqos_opts.command, "start") == 0) {
		init_qos_manager();
		parse_app_configs();
		parse_qos_mappings();
		start_netlink_monitor();
		return 0;
	} else if (strcmp(sqos_opts.command, "stop") == 0) {
	} else if (strcmp(sqos_opts.command, "restart") == 0) {
	} else if (strcmp(sqos_opts.command, "sched_profile") == 0) {
		init_sched_profiles();
		parse_sched_profiles();
		sched_profiles_apply_profile(sqos_opts.sched_profile);
	}
}
