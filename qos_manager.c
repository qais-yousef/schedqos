/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Qais Yousef */
#include <glib.h>
#include <stdbool.h>
#include <stdio.h>

#include "qos_manager.h"
#include "qos_tagging.h"


/*
 * An instance of an app that has been exec'ed.
 * Same app might have multiple instances being executed.
 */
struct app_instance {
	char *cmdline;
	pid_t tgid;
};

/*
 * App config thread_qos tags parsed from json files.
 */
struct thread_qos {
	char *comm;
	enum qos_tag qos_tag;
};

/*
 * App (cmdline) config settings parsed from json files.
 */
struct app_config {
	char *cmdline;
	GHashTable *threads_registry;
};

static GHashTable *app_config_registry;
static GHashTable *app_instance_registry;


static void free_thread_qos(gpointer data)
{
	struct thread_qos *thread = (struct thread_qos *)data;
	g_free(thread->comm);
	g_free(thread);
}

static void create_thread_registry(struct app_config *app)
{
	app->threads_registry = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, free_thread_qos);
}

static void destroy_threads_registry(struct app_config *app)
{
	g_hash_table_destroy(app_config_registry);
}

static void* lookup_thread(struct app_config *app, const char *comm)
{
	return g_hash_table_lookup(app->threads_registry, comm);
}

static void free_app_config(gpointer data)
{
	struct app_config *app = (struct app_config *)data;
	destroy_threads_registry(app);
	g_free(app->cmdline);
	g_free(app);
}

static void create_app_config_registry(void)
{
	app_config_registry = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, free_app_config);
}

static void destroy_app_config_registry(void)
{
	g_hash_table_destroy(app_config_registry);
}

static void* lookup_app_config(const char *cmdline)
{
	return g_hash_table_lookup(app_config_registry, cmdline);
}

static void free_app_instance(gpointer data)
{
	struct app_instance *app = (struct app_instance *)data;
	g_free(app->cmdline);
	g_free(app);
}

static void create_app_instance_registry(void)
{
	app_instance_registry = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, free_app_instance);
}

static void destroy_app_instance_registry(void)
{
	g_hash_table_destroy(app_instance_registry);
}

static void* lookup_app_instance(pid_t tgid)
{
	return g_hash_table_lookup(app_instance_registry, GINT_TO_POINTER(tgid));
}


void init_qos_manager(void)
{
	create_app_config_registry();
	create_app_instance_registry();
}

void deinit_qos_manager(void)
{
	destroy_app_config_registry();
	destroy_app_instance_registry();
}

bool add_thread_qos_tag(const void *app, const char *comm, char *qos_tag)
{
	struct app_config *_app = (struct app_config *)app;
	struct thread_qos *thread = g_new0(struct thread_qos, 1);

	if (!thread)
		return false;;

	thread->comm = g_strdup(comm);
	thread->qos_tag = char_to_qos_tag(qos_tag);

	LOG_INFO("Add QoS Tag for %s: %s", comm, qos_tag);

	g_hash_table_insert(_app->threads_registry, g_strdup(comm), thread);

	return true;
}

/*
 * Create app_config instance for a new app parsed by app_config.json file
 *
 * Returns a handle to the newly created structure. Handle can be used to add
 * more info as the json files is being parsed.
 */
void *create_app_config(const char *cmdline)
{
	struct app_config *app = g_new0(struct app_config, 1);

	if (!app)
		return NULL;

	LOG_INFO("Creating new app config for %s", cmdline);

	app->cmdline = g_strdup(cmdline);

	/* We can add (unique) aliases easily by inserting more names to point to app*/
	g_hash_table_insert(app_config_registry, g_strdup(cmdline), app);

	create_thread_registry(app);

	return app;
}

/*
 * Create app_config instance for a new app parsed by app_config.json file
 *
 * Returns a handle to the newly created structure. Handle can be used to add
 * more info as the json files is being parsed.
 */
void create_app_instance(const pid_t tgid)
{
	struct app_instance *app = g_new0(struct app_instance, 1);

	if (!app)
		return;

	if (lookup_app_instance(tgid)) {
		LOG_ERROR("App instance for %d was already created", tgid);
		return;
	}

	app->cmdline = get_cmdline_by_pid(tgid);
	if (!app->cmdline) {
		g_free(app);
		return;
	}
	app->tgid = tgid;

	g_hash_table_insert(app_instance_registry, GINT_TO_POINTER(tgid), app);

	LOG_INFO("New app instance of %d %s", tgid, app->cmdline);
}

void destroy_app_instance(const pid_t tgid)
{
	struct app_instance *app = lookup_app_instance(tgid);

	if (!app)
		return;

	LOG_INFO("Exit app instance of %d %s", tgid, app->cmdline);

	g_hash_table_remove(app_instance_registry, GINT_TO_POINTER(tgid));
}

bool apply_thread_qos(pid_t pid, pid_t tgid, const char *comm)
{
	enum qos_tag qos_tag = QOS_DEFAULT;
	struct app_instance *appi;
	struct thread_qos *thread;
	struct app_config *app;
	int ret = false;

	/* Only apply the tagging to fair tasks */
	if (!is_fair_task(pid))
		return false;

	appi = lookup_app_instance(tgid);
	if (!appi)
		goto out;

	app = lookup_app_config(appi->cmdline);
	if (!app)
		goto out;

	thread = lookup_thread(app, comm);
	if (!thread)
		goto out;

	qos_tag = thread->qos_tag;
	ret = true;

out:
	LOG_INFO("Applying QoS Tag %s for %d:%d %s", qos_tag_to_char(qos_tag), pid, tgid, comm);

	apply_thread_qos_tag(pid, comm, qos_tag);

	return ret;
}
