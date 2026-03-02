/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Qais Yousef */
#include "configs_parser.h"
#include "netlink_monitor.h"
#include "qos_manager.h"
#include "sched_profiles.h"

int main(int argc, char **argv)
{
	int err;

	err = argp_parse(&argp, argc, argv, 0, NULL, NULL);
	if (err)
		return err;

	init_qos_manager();
	init_sched_profiles();
	parse_all_configs();
	sched_profiles_apply_profile("qos");
	start_netlink_monitor();
}
