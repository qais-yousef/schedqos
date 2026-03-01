/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Qais Yousef */
#include <string.h>

#include "qos_tagging.h"
#include "sched_attr.h"

static struct sched_attr sa_qos_user_interactive = {
	.size = sizeof(struct sched_attr),
	.sched_policy = SCHED_OTHER,
	.sched_flags = SCHED_FLAG_RESET_ON_FORK | SCHED_FLAG_UTIL_CLAMP_MAX,
	.sched_nice = 0,
	.sched_priority = 0,
	.sched_runtime = 0,
	.sched_deadline = 0,
	.sched_period = 0,
	.sched_util_min = 0,
	.sched_util_max = 1024
};

static struct sched_attr sa_qos_user_initiated = {
	.size = sizeof(struct sched_attr),
	.sched_policy = SCHED_OTHER,
	.sched_flags = SCHED_FLAG_RESET_ON_FORK | SCHED_FLAG_UTIL_CLAMP_MAX,
	.sched_nice = 0,
	.sched_priority = 0,
	.sched_runtime = 0,
	.sched_deadline = 0,
	.sched_period = 0,
	.sched_util_min = 0,
	.sched_util_max = 1024
};

static struct sched_attr sa_qos_utility = {
	.size = sizeof(struct sched_attr),
	.sched_policy = SCHED_BATCH,
	.sched_flags = SCHED_FLAG_RESET_ON_FORK | SCHED_FLAG_UTIL_CLAMP_MAX,
	.sched_nice = 0,
	.sched_priority = 0,
	.sched_runtime = 0,
	.sched_deadline = 0,
	.sched_period = 0,
	.sched_util_min = 0,
	.sched_util_max = 1024
};

static struct sched_attr sa_qos_background = {
	.size = sizeof(struct sched_attr),
	.sched_policy = SCHED_BATCH,
	.sched_flags = SCHED_FLAG_RESET_ON_FORK | SCHED_FLAG_UTIL_CLAMP_MAX,
	.sched_nice = 0,
	.sched_priority = 0,
	.sched_runtime = 0,
	.sched_deadline = 0,
	.sched_period = 0,
	.sched_util_min = 0,
	.sched_util_max = 1024
};

static struct sched_attr sa_qos_default = {
	.size = sizeof(struct sched_attr),
	.sched_policy = SCHED_BATCH,
	.sched_flags = SCHED_FLAG_RESET_ON_FORK | SCHED_FLAG_UTIL_CLAMP_MAX,
	.sched_nice = 0,
	.sched_priority = 0,
	.sched_runtime = 0,
	.sched_deadline = 0,
	.sched_period = 0,
	.sched_util_min = 0,
	.sched_util_max = 1024
};


static uint32_t char_to_policy(char *policy)
{
	if (!policy)
		return SCHED_OTHER;

	if (strcmp(policy, "SCHED_NORMAL") == 0)
		return SCHED_OTHER;
	if (strcmp(policy, "SCHED_OTHER") == 0)
		return SCHED_OTHER;
	else if (strcmp(policy, "SCHED_BATCH") == 0)
		return SCHED_BATCH;
	else if (strcmp(policy, "SCHED_IDLE") == 0)
		return SCHED_IDLE;
	else if (strcmp(policy, "SCHED_FIFO") == 0)
		return SCHED_FIFO;
	else if (strcmp(policy, "SCHED_RR") == 0)
		return SCHED_RR;
	else if (strcmp(policy, "SCHED_DEADLINE") == 0)
		return SCHED_DEADLINE;
	else
		return SCHED_OTHER;
}

static void log_qos_tag_attr(enum qos_tag qos_tag)
{
	struct sched_attr *sa;

	if (!sqos_opts.verbose)
		return;

	switch (qos_tag) {
	case QOS_USER_INTERACTIVE:
		sa = &sa_qos_user_interactive;
		break;
	case QOS_USER_INITIATED:
		sa = &sa_qos_user_initiated;
		break;
	case QOS_UTILITY:
		sa = &sa_qos_utility;
		break;
	case QOS_BACKGROUND:
		sa = &sa_qos_background;
		break;
	case QOS_DEFAULT:
		sa = &sa_qos_default;
		break;
	default:
		LOG_ERROR("Unknown QoS Tag %d", qos_tag);
		return;
	}

	LOG_VERBOSE("QoS Tag %d sched_attr:", qos_tag);
	LOG_VERBOSE("\t.sched_policy: %u", sa->sched_policy);
	LOG_VERBOSE("\t.sched_flags: %lu", sa->sched_flags);
	LOG_VERBOSE("\t.sched_nice: %d", sa->sched_nice);
	LOG_VERBOSE("\t.sched_priority: %u", sa->sched_priority);
	LOG_VERBOSE("\t.sched_runtime: %lu", sa->sched_runtime);
	LOG_VERBOSE("\t.sched_deadline: %lu", sa->sched_deadline);
	LOG_VERBOSE("\t.sched_period: %lu", sa->sched_period);
	LOG_VERBOSE("\t.sched_util_min: %u", sa->sched_util_min);
	LOG_VERBOSE("\t.sched_util_max: %u", sa->sched_util_max);
}

static void log_thread_attr(pid_t pid)
{
	struct sched_attr sa = {};

	if (!sqos_opts.verbose)
		return;

	sched_getattr(pid, &sa, sizeof(struct sched_attr), 0);

	LOG_VERBOSE("%d sched_attr:", pid);
	LOG_VERBOSE("\t.sched_policy: %u", sa.sched_policy);
	LOG_VERBOSE("\t.sched_flags: %lu", sa.sched_flags);
	LOG_VERBOSE("\t.sched_nice: %d", sa.sched_nice);
	LOG_VERBOSE("\t.sched_priority: %u", sa.sched_priority);
	LOG_VERBOSE("\t.sched_runtime: %lu", sa.sched_runtime);
	LOG_VERBOSE("\t.sched_deadline: %lu", sa.sched_deadline);
	LOG_VERBOSE("\t.sched_period: %lu", sa.sched_period);
	LOG_VERBOSE("\t.sched_util_min: %u", sa.sched_util_min);
	LOG_VERBOSE("\t.sched_util_max: %u", sa.sched_util_max);
}

void parse_thread_qos_mapping(enum qos_tag qos_tag, char *policy,
			      uint64_t runtime, uint32_t uclamp_max)
{
	struct sched_attr *sa;

	switch (qos_tag) {
	case QOS_USER_INTERACTIVE:
		sa = &sa_qos_user_interactive;
		break;
	case QOS_USER_INITIATED:
		sa = &sa_qos_user_initiated;
		break;
	case QOS_UTILITY:
		sa = &sa_qos_utility;
		break;
	case QOS_BACKGROUND:
		sa = &sa_qos_background;
		break;
	case QOS_DEFAULT:
		sa = &sa_qos_default;
		break;
	default:
		LOG_ERROR("Unknown QoS Tag %d", qos_tag);
		return;
	}

	sa->sched_policy = char_to_policy(policy);
	sa->sched_runtime = runtime;
	sa->sched_util_max = uclamp_max;

	log_qos_tag_attr(qos_tag);
}

void apply_thread_qos_tag(pid_t pid, const char *comm, enum qos_tag qos_tag)
{
	struct sched_attr *sa;
	int ret;

	switch (qos_tag) {
	case QOS_USER_INTERACTIVE:
		sa = &sa_qos_user_interactive;
		break;
	case QOS_USER_INITIATED:
		sa = &sa_qos_user_initiated;
		break;
	case QOS_UTILITY:
		sa = &sa_qos_utility;
		break;
	case QOS_BACKGROUND:
		sa = &sa_qos_background;
		break;
	case QOS_DEFAULT:
		sa = &sa_qos_default;
		break;
	default:
		LOG_ERROR("Unknown QoS Tag %d", qos_tag);
		return;
	}

	log_qos_tag_attr(qos_tag);
	log_thread_attr(pid);

	LOG_INFO("Applying QoS Tag %s for %d %s", qos_tag_to_char(qos_tag), pid, comm);
	ret = sched_setattr(pid, sa, 0);
	if (ret)
		LOG_ERROR("Failed to change sched_attr for %d", pid);

	log_thread_attr(pid);
}
