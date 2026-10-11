/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Qais Yousef */
#include "sched_attr.h"	/* Must before glib.h include to avoid compilation errors. */

#include <glib.h>
#include <string.h>

#include "qos_tagging.h"


static struct sched_attr sa_qos_default = {
	.size = sizeof(struct sched_attr),
	.sched_policy = SCHED_BATCH,
	.sched_flags = SCHED_FLAG_RESET_ON_FORK | SCHED_FLAG_UTIL_CLAMP_MIN | SCHED_FLAG_UTIL_CLAMP_MAX | SCHED_FLAG_QOS,
	.sched_util_max = 1024
};

/*
 * A QoS Class or tag that is defined by the user in qos_mappings.json
 */
struct qos_class {
	char *qos_tag;
	struct sched_attr sa;
};

static GHashTable *qos_class_registry;

/*
 * Create app_config instance for a new app parsed by app_config.json file
 *
 * Returns a handle to the newly created structure. Handle can be used to add
 * more info as the json files is being parsed.
 */
void *create_qos_class(const char *qos_tag)
{
	struct qos_class *class = g_new0(struct qos_class, 1);

	if (!class)
		return NULL;

	LOG_INFO("Creating new QoS Class for %s", qos_tag);

	class->qos_tag = g_strdup(qos_tag);
	class->sa = sa_qos_default;

	g_hash_table_insert(qos_class_registry, g_strdup(qos_tag), class);

	return class;
}

static void free_qos_class(gpointer data)
{
	struct qos_class *class = (struct qos_class *)data;
	g_free(class->qos_tag);
}

static void create_qos_class_registry(void)
{
	qos_class_registry = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, free_qos_class);
}

static void destroy_qos_class_registry(void)
{
	g_hash_table_destroy(qos_class_registry);
}

static void *lookup_qos_class(const char *qos_tag)
{
	return g_hash_table_lookup(qos_class_registry, qos_tag);
}


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

static uint32_t char_to_sched_qos_type(char *sched_qos_type)
{
	if (!sched_qos_type)
		return SCHED_QOS_NONE;

	if (strcmp(sched_qos_type, "SCHED_QOS_RAMPUP_MULTIPLIER") == 0)
		return SCHED_QOS_RAMPUP_MULTIPLIER;

	LOG_ERROR("Unknown sched_qos_type %s", sched_qos_type);
	return SCHED_QOS_NONE;
}

static void log_qos_tag_attr(char *qos_tag)
{
	struct qos_class *class;
	struct sched_attr *sa;

	if (!sqos_opts.verbose)
		return;

	class = lookup_qos_class(qos_tag);
	if (!class) {
		LOG_ERROR("Unknown QoS Class %s", qos_tag);
		return;
	}

	sa = &class->sa;

	LOG_VERBOSE("QoS Tag %s sched_attr:", qos_tag);
	LOG_VERBOSE("\t.sched_policy: %u", sa->sched_policy);
	LOG_VERBOSE("\t.sched_flags: %lu", sa->sched_flags);
	LOG_VERBOSE("\t.sched_nice: %d", sa->sched_nice);
	LOG_VERBOSE("\t.sched_priority: %u", sa->sched_priority);
	LOG_VERBOSE("\t.sched_runtime: %lu", sa->sched_runtime);
	LOG_VERBOSE("\t.sched_deadline: %lu", sa->sched_deadline);
	LOG_VERBOSE("\t.sched_period: %lu", sa->sched_period);
	LOG_VERBOSE("\t.sched_util_min: %u", sa->sched_util_min);
	LOG_VERBOSE("\t.sched_util_max: %u", sa->sched_util_max);
	LOG_VERBOSE("\t.sched_qos_type: %u", sa->sched_qos_type);
	LOG_VERBOSE("\t.sched_qos_value: %lu", sa->sched_qos_value);
	LOG_VERBOSE("\t.sched_qos_cookie: %u", sa->sched_qos_cookie);
}

static void log_thread_attr(pid_t pid)
{
	struct sched_attr sa = {};

	if (!sqos_opts.verbose)
		return;

	sa.sched_qos_type = SCHED_QOS_RAMPUP_MULTIPLIER;
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
	LOG_VERBOSE("\t.sched_qos_type: %u", sa.sched_qos_type);
	LOG_VERBOSE("\t.sched_qos_value: %lu", sa.sched_qos_value);
	LOG_VERBOSE("\t.sched_qos_cookie: %u", sa.sched_qos_cookie);
}

void parse_thread_qos_mapping_str(char *qos_tag, char *attr, char *value)
{
	struct qos_class *class;
	struct sched_attr *sa;

	class = lookup_qos_class(qos_tag);
	if (!class) {
		class = create_qos_class(qos_tag);
		if (!class) {
			LOG_ERROR("Failed to create QoS Class %s", qos_tag);
			return;
		}
	}

	sa = &class->sa;

	if (strcmp(attr, "sched_policy") == 0)
		sa->sched_policy = char_to_policy(value);
	else if (strcmp(attr, "sched_qos_type") == 0)
		sa->sched_qos_type = char_to_sched_qos_type(value);
	else
		LOG_ERROR("Unknown str sched_attr: %s", attr);
}

void parse_thread_qos_mapping_int(char *qos_tag, char *attr, int value)
{
	struct qos_class *class;
	struct sched_attr *sa;

	class = lookup_qos_class(qos_tag);
	if (!class) {
		class = create_qos_class(qos_tag);
		if (!class) {
			LOG_ERROR("Failed to create QoS Class %s", qos_tag);
			return;
		}
	}

	sa = &class->sa;

	if (strcmp(attr, "sched_nice") == 0)
		sa->sched_nice = value;
	else if (strcmp(attr, "sched_priority") == 0)
		sa->sched_priority = value;
	else if (strcmp(attr, "sched_runtime") == 0)
		sa->sched_runtime = value;
	else if (strcmp(attr, "sched_deadline") == 0)
		sa->sched_deadline = value;
	else if (strcmp(attr, "sched_period") == 0)
		sa->sched_period = value;
	else if (strcmp(attr, "sched_util_min") == 0)
		sa->sched_util_min = value;
	else if (strcmp(attr, "sched_util_max") == 0)
		sa->sched_util_max = value;
	else if (strcmp(attr, "sched_qos_value") == 0)
		sa->sched_qos_value = value;
	else
		LOG_ERROR("Unknown int sched_attr: %s", attr);
}

void apply_thread_qos_tag(pid_t pid, const char *comm, char *qos_tag, uint64_t period)
{
	struct qos_class *class;
	struct sched_attr *sa;
	int ret;

	class = lookup_qos_class(qos_tag);
	if (!class) {
		LOG_ERROR("QoS Class %s doesn't exist", qos_tag);
	}

	sa = &class->sa;

	log_qos_tag_attr(qos_tag);
	log_thread_attr(pid);

	LOG_INFO("Applying QoS Tag %s for %d %s", qos_tag, pid, comm);
retry:
	ret = sched_setattr(pid, sa, 0);
	if (ret && errno == E2BIG) {
		switch (sa->size) {
		case SCHED_ATTR_SIZE_VER1:
			LOG_VERBOSE("Kernel doesn't support SCHED_ATTR_SIZE_VER2, reverting to VER1");
			sa->sched_flags &= ~SCHED_FLAG_QOS;
			goto retry;
		case SCHED_ATTR_SIZE_VER0:
			LOG_VERBOSE("Kernel doesn't support SCHED_ATTR_SIZE_VER1, reverting to VER0");
			sa->sched_flags &= ~SCHED_FLAG_UTIL_CLAMP_MIN;
			sa->sched_flags &= ~SCHED_FLAG_UTIL_CLAMP_MAX;
			sa->sched_flags &= ~SCHED_FLAG_QOS;
			goto retry;
		default:
			LOG_ERROR("Unknown sched_attr version");
			break;
		}
	}
	if (ret)
		LOG_ERROR("Failed to change sched_attr for %d: %d", pid, errno);

	log_thread_attr(pid);
}

void init_qos_tagging(void)
{
	create_qos_class_registry();
}

void deinit_qos_tagging(void)
{
	destroy_qos_class_registry();
}
