/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Qais Yousef */
#include <stdint.h>

/*
 * Per Task Thread QoS Tag
 *
 * QOS_DEFAULT:
 *
 *	System default behavior, likely mapped to QOS_UTILITY
 *
 * QOS_USER_INTERACTIVE:
 *
 *	Tasks that require immediate response
 *
 * QOS_USER_INITIATED:
 *
 *	Tasks that can tolerate short delays, but require fast response
 *	otherwise
 *
 * QOS_UTILITY:
 *
 *	Tasks that can tolerate long delays, but not prolonged ones
 *
 * QOS_BACKGROUND:
 *
 *	Tasks that don't mind prolonged delays
 */
enum qos_tag {
	QOS_DEFAULT,
	QOS_USER_INTERACTIVE,
	QOS_USER_INITIATED,
	QOS_UTILITY,
	QOS_BACKGROUND,
};

static inline enum qos_tag char_to_qos_tag(char *qos_tag)
{
	if (!qos_tag)
		return QOS_DEFAULT;

	if (strcmp(qos_tag, "QOS_USER_INTERACTIVE") == 0)
		return QOS_USER_INTERACTIVE;
	else if (strcmp(qos_tag, "QOS_USER_INITIATED") == 0)
		return QOS_USER_INITIATED;
	else if (strcmp(qos_tag, "QOS_UTILITY") == 0)
		return QOS_UTILITY;
	else if (strcmp(qos_tag, "QOS_BACKGROUND") == 0)
		return QOS_BACKGROUND;
	else
		return QOS_DEFAULT;
}

void parse_thread_qos_mapping(enum qos_tag qos_tag, char *policy,
			      uint64_t runtime, uint32_t uclamp_max);
void apply_thread_qos_tag(pid_t pid, enum qos_tag qos_tag);
