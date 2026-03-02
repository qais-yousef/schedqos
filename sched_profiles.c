/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Qais Yousef */
#include <errno.h>
#include <glib.h>
#include <mntent.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>

#include "sched_profiles.h"

#define DEBUFS_PATH	"/sys/kernel/debug"

struct sched_profile {
	char *profile;
	GTree *debugfs_settings;
	GTree *cpufreq_settings;
};

static GHashTable *sched_profiles_registry;


static bool mount_debugfs(const char *path)
{
	mkdir(path, 0755);

	if (mount("none", path, "debugfs", 0, NULL) != 0) {
		/* Already mounted */
		if (errno == EBUSY)
			return true;

		LOG_ERROR("Failed to mount debugfs: %d", errno);
		return false;
	}
	return true;
}

static bool unmount_debugfs(const char *path)
{
	if (umount(path) != 0) {
		LOG_ERROR("Failed to unmount debugfs: %d", errno);
		return false;
	}

	return true;
}

/*
 * Callers must free() the returned pointer.
 */
static char *find_debugfs_path(void)
{
	FILE *fp = setmntent("/proc/mounts", "r");
	struct mntent *ent;

	if (!fp)
		return NULL;

	while ((ent = getmntent(fp)) != NULL) {
		if (strcmp(ent->mnt_type, "debugfs") == 0) {
			char *path = strdup(ent->mnt_dir);
			endmntent(fp);
			return path;
		}
	}
	endmntent(fp);

	return NULL;
}

static void write_sched_debug(const char *debugfs_path, const char *node, const char *value)
{
	char path[256];
	FILE *fp;

	snprintf(path, sizeof(path), "%s/sched/%s", debugfs_path, node);

	fp = fopen(path, "w");
	if (!fp) {
		LOG_ERROR("Error opening %s: %m", path);
		return;
	}

	if (fprintf(fp, "%s", value) < 0) {
		LOG_ERROR("Failed to write to debugfs: %d", errno);
		fclose(fp);
		return;
	}

	fclose(fp);
}

static bool write_cpufreq_all_policies(const char *node, const char *value)
{
	const char *base_path = "/sys/devices/system/cpu/cpufreq";
	DIR *dir = opendir(base_path);
	struct dirent *entry;

	if (!dir) {
		LOG_ERROR("Error opening %s: %m", base_path);
		return false;
	}

	while ((entry = readdir(dir)) != NULL) {
		/* Look for directories named "policyX" */
		if (entry->d_type == DT_DIR || entry->d_type == DT_LNK) {
			if (strncmp(entry->d_name, "policy", 6) == 0) {
				char full_path[512];
				FILE *fp;

				snprintf(full_path, sizeof(full_path), "%s/%s/%s",
						 base_path, entry->d_name, node);

				fp = fopen(full_path, "w");
				if (fp) {
					if (fprintf(fp, "%s", value) < 0) {
						LOG_ERROR("Write failed to %s: %m", full_path);
					}
					fclose(fp);
				} else {
					// Only log if it's not a "File not found" error
					if (errno != ENOENT) {
						LOG_ERROR("Could not open %s: %m", full_path);
					}
				}
			}
		}
	}

	closedir(dir);
	return true;
}

static gint compare_keys(gconstpointer a, gconstpointer b, gpointer user_data)
{
	return g_strcmp0((const char *)a, (const char *)b);
}

static void create_settings_registries(struct sched_profile *profile)
{
	profile->debugfs_settings = g_tree_new_full(compare_keys, NULL, g_free, g_free);
	profile->cpufreq_settings = g_tree_new_full(compare_keys, NULL, g_free, g_free);
}

static void destroy_settings_registries(struct sched_profile *profile)
{
	g_tree_unref(profile->debugfs_settings);
	g_tree_unref(profile->cpufreq_settings);
}

static void free_sched_profile(gpointer data)
{
	struct sched_profile *profile = (struct sched_profile *)data;
	destroy_settings_registries(profile);
	g_free(profile->profile);
	g_free(profile);
}

static void create_sched_profiles_registry(void)
{
	sched_profiles_registry = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, free_sched_profile);
}

static void destroy_sched_profiles_registry(void)
{
	g_hash_table_destroy(sched_profiles_registry);
}

static void* lookup_sched_profile(const char *profile)
{
	return g_hash_table_lookup(sched_profiles_registry, profile);
}

void init_sched_profiles(void)
{
	create_sched_profiles_registry();
}

void deinit_sched_profiles(void)
{
	destroy_sched_profiles_registry();
}

void *create_sched_profile(const char *profile)
{
	struct sched_profile *pprofile = g_new0(struct sched_profile, 1);

	if (!pprofile) {
		LOG_ERROR("Failed to create sched profile %s", profile);
		return NULL;
	}

	LOG_INFO("Creating new sched profile %s", profile);

	pprofile->profile = g_strdup(profile);

	g_hash_table_insert(sched_profiles_registry, g_strdup(profile), pprofile);

	create_settings_registries(pprofile);

	return pprofile;
}

void sched_profiles_add_debugfs(void *profile, const char *node, const char *value)
{
	struct sched_profile *pprofile = (struct sched_profile *)profile;

	if (!profile) {
		LOG_ERROR("Invalid profile");
		return;
	}

	LOG_VERBOSE("Adding new debugfs setting %s: %s", node, value);

	/* use value as key as it is likely unique */
	g_tree_insert(pprofile->debugfs_settings, g_strdup(value), g_strdup(node));
}

void sched_profiles_add_cpufreq(void *profile, const char *node, const char *value)
{
	struct sched_profile *pprofile = (struct sched_profile *)profile;

	if (!profile) {
		LOG_ERROR("Invalid profile");
		return;
	}

	LOG_VERBOSE("Adding new cpufreq setting %s: %s", node, value);

	/* use value as key as it is likely unique */
	g_tree_insert(pprofile->cpufreq_settings, g_strdup(value), g_strdup(node));
}

static gboolean debugfs_iter(gpointer value, gpointer node, gpointer data)
{
	char *debugfs_path = (char *)data;

	LOG_INFO("  Writing debugfs setting %s: %s", (char *)node, (char *)value);
	write_sched_debug(debugfs_path, (char *)node, (char *)value);
	return FALSE;
}

static gboolean cpufreq_iter(gpointer value, gpointer node, gpointer data)
{
	LOG_INFO("  Writing cpufreq setting %s: %s", (char *)node, (char *)value);
	write_cpufreq_all_policies((char *)node, (char *)value);
	return FALSE;
}

void sched_profiles_apply_profile(const char *profile)
{
	char *debugfs_path = find_debugfs_path();
	struct sched_profile *pprofile;
	bool umount = false;

	pprofile = lookup_sched_profile(profile);
	if (!pprofile) {
		LOG_ERROR("Profile %s not found", profile);
		return;
	}

	LOG_INFO("Applying sched profile: %s", pprofile->profile);

	if (!debugfs_path) {
		if (!mount_debugfs(DEBUFS_PATH)) {
			LOG_ERROR("Can't access debugfs");
			return;
		}
		debugfs_path = DEBUFS_PATH;
		umount = true;
	}

	g_tree_foreach(pprofile->debugfs_settings, debugfs_iter, debugfs_path);

	if (umount) {
		if (!unmount_debugfs(DEBUFS_PATH))
			LOG_WARN("Failed to umount debugfs");
	} else {
		free(debugfs_path);
	}

	g_tree_foreach(pprofile->cpufreq_settings, cpufreq_iter, NULL);
}
