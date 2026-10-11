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

void parse_thread_qos_mapping_str(char *qos_tag, char *attr, char *value);
void parse_thread_qos_mapping_int(char *qos_tag, char *attr, int value);
void apply_thread_qos_tag(pid_t pid, const char *comm, char *qos_tag, uint64_t period);
void init_qos_tagging(void);
void deinit_qos_tagging(void);
