/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Qais Yousef */
#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "qos_manager.h"
#include "qos_tagging.h"
#include "sched_profiles.h"

#define APP_CONFIGS_PATH	"app_configs/"
#define QOS_MAPPINGS_FILE	"qos_mappings.json"
#define SCHED_PROFILES_FILE	"sched_profiles.json"


static void __parse_thread_qos(const void *app, const cJSON *thread_qos)
{
	LOG_INFO("  Thread QoS Settings:");
	cJSON *thread = NULL;
	cJSON_ArrayForEach(thread, thread_qos) {
		if (cJSON_IsArray(thread)) {
			cJSON *qos = NULL;
			cJSON_ArrayForEach(qos, thread) {
				if (cJSON_IsString(qos)) {
					LOG_INFO("	- %s: %s", thread->string, qos->valuestring);
					add_thread_qos_tag(app, thread->string, qos->valuestring);
				}
			}
		} else if (cJSON_IsString(thread)) {
			LOG_INFO("	- %s: %s", thread->string, thread->valuestring);
			add_thread_qos_tag(app, thread->string, thread->valuestring);
		}
	}
}

/*
 * App config parsing function.
 */
static void __parse_app_config(const char *json_string)
{
	cJSON *root = cJSON_Parse(json_string);
	if (!root) {
		LOG_ERROR("Invalid JSON format");
		return;
	}

	/* We identify apps by their cmdline */
	cJSON *app_node = NULL;
	cJSON_ArrayForEach(app_node, root) {
		void *app;

		LOG_INFO("app cmdline: %s", app_node->string);

		app = create_app_config(app_node->string);
		if (!app)
			continue;

		cJSON *version = cJSON_GetObjectItemCaseSensitive(app_node, "version");
		cJSON *period = cJSON_GetObjectItemCaseSensitive(app_node, "period");

		if (cJSON_IsString(version))
			LOG_INFO("  Version: %s", version->valuestring);
		if (cJSON_IsNumber(period))
			LOG_INFO("  Period: %d", period->valueint);

		cJSON *thread_qos = cJSON_GetObjectItemCaseSensitive(app_node, "thread_qos");
		if (cJSON_IsObject(thread_qos))
			__parse_thread_qos(app, thread_qos);
	}

	cJSON_Delete(root);
}

/*
 * QoS Mappings parsing function.
 */
static void __parse_qos_mappings(const char *json_string)
{
	cJSON *root = cJSON_Parse(json_string);
	if (!root) {
		LOG_ERROR("Invalid JSON format");
		return;
	}

	cJSON *qos_node = NULL;
	cJSON_ArrayForEach(qos_node, root) {
		LOG_INFO("QoS Tag: %s", qos_node->string);

		cJSON *attr = NULL;
		cJSON_ArrayForEach(attr, qos_node) {
			if (cJSON_IsNumber(attr)) {
				LOG_INFO("  %s: %d", attr->string, attr->valueint);
				parse_thread_qos_mapping_int(char_to_qos_tag(qos_node->string),
							     attr->string, attr->valueint);
			} else if (cJSON_IsString(attr)) {
				LOG_INFO("  %s: %s", attr->string, attr->valuestring);
				parse_thread_qos_mapping_str(char_to_qos_tag(qos_node->string),
							     attr->string, attr->valuestring);
			} else {
				LOG_ERROR(" Unknown sched_attr type: %s", attr->string);
			}
		}
	}

	cJSON_Delete(root);
}

/*
 * Sched Profiles parsing function.
 */
static void __parse_sched_profiles(const char *json_string)
{
	cJSON *root = cJSON_Parse(json_string);
	if (!root) {
		LOG_ERROR("Invalid JSON format");
		return;
	}

	cJSON *profile = NULL;
	cJSON *node = NULL;
	cJSON_ArrayForEach(profile, root) {
		void *sched_profile;

		LOG_INFO("Sched Profile: %s", profile->string);

		sched_profile = create_sched_profile(profile->string);
		if (!sched_profile)
			continue;

		cJSON *debugfs = cJSON_GetObjectItemCaseSensitive(profile, "debugfs");
		cJSON *cpufreq = cJSON_GetObjectItemCaseSensitive(profile, "cpufreq");

		if (!debugfs || !cpufreq) {
			LOG_ERROR("Incomplete sched profile spec");
			continue;
		}

		LOG_INFO("  debugfs");
		cJSON_ArrayForEach(node, debugfs) {
			if (cJSON_IsArray(node)) {
				cJSON *val = NULL;
				cJSON_ArrayForEach(val, node) {
					if (cJSON_IsString(val)) {
						LOG_INFO("	- %s: %s", node->string, val->valuestring);
						sched_profiles_add_debugfs(sched_profile, node->string, val->valuestring);
					} else {
						LOG_ERROR("%s value must be a string", node->string);
					}
				}
			} else if (cJSON_IsString(node)) {
				LOG_INFO("	- %s: %s", node->string, node->valuestring);
				sched_profiles_add_debugfs(sched_profile, node->string, node->valuestring);
			} else {
				LOG_ERROR("%s value must be a string", node->string);
			}
		}

		LOG_INFO("  cpufreq");
		cJSON_ArrayForEach(node, cpufreq) {
			if (cJSON_IsArray(node)) {
				cJSON *val = NULL;
				cJSON_ArrayForEach(val, node) {
					if (cJSON_IsString(val)) {
						LOG_INFO("	- %s: %s", node->string, val->valuestring);
						sched_profiles_add_cpufreq(sched_profile, node->string, val->valuestring);
					} else {
						LOG_ERROR("%s value must be a string", node->string);
					}
				}
			} else if (cJSON_IsString(node)) {
				LOG_INFO("	- %s: %s", node->string, node->valuestring);
				sched_profiles_add_cpufreq(sched_profile, node->string, node->valuestring);
			} else {
				LOG_ERROR("%s value must be a string", node->string);
			}
		}
	}

	cJSON_Delete(root);
}

/* TODO can we do better file type checking? */
static bool is_json_file(const char *filename) {
	const char *ext = strrchr(filename, '.');

	return (ext && strcmp(ext, ".json") == 0);
}

/* Convert json to a string to be consumed by cJSON */
static char *read_config_file(const char *config) {
	char full_path[1024];
	FILE *fp;

	snprintf(full_path, sizeof(full_path), "%s/%s", sqos_opts.configs_path, config);
	LOG_INFO("Reading %s", full_path);

	fp = fopen(full_path, "rb");
	if (!fp)
		return NULL;

	fseek(fp, 0, SEEK_END);
	long length = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	char *buffer = malloc(length + 1);
	if (buffer) {
		size_t ret = fread(buffer, 1, length, fp);
		if (ret != length)
			goto error;
		buffer[length] = '\0';
	}
	fclose(fp);
	return buffer;
error:
	free(buffer);
	return NULL;
}

/*
 * Iterate and read app config files.
 */
void parse_app_configs(void)
{
	char full_path[1024];
	struct dirent *entry;
	DIR *dp;

	snprintf(full_path, sizeof(full_path), "%s/%s", sqos_opts.configs_path, APP_CONFIGS_PATH);

	dp = opendir(full_path);
	if (dp == NULL) {
		perror("opendir");
		return;
	}

	while ((entry = readdir(dp))) {
		if (is_json_file(entry->d_name)) {

			snprintf(full_path, sizeof(full_path), "%s/%s", APP_CONFIGS_PATH, entry->d_name);

			LOG_INFO("processing: %s", full_path);

			char *app_config = read_config_file(full_path);
			if (app_config) {
				__parse_app_config(app_config);
				free(app_config);
			}
		}
	}
	closedir(dp);
}

/*
 * Read qos mappings file.
 */
void parse_qos_mappings(void)
{
	char *qos_mapping = read_config_file(QOS_MAPPINGS_FILE);
	if (qos_mapping) {
		__parse_qos_mappings(qos_mapping);
		free(qos_mapping);
	}
}


/*
 * Read sched profiles file.
 */
void parse_sched_profiles(void)
{
	char *profiles = read_config_file(SCHED_PROFILES_FILE);
	if (profiles) {
		__parse_sched_profiles(profiles);
		free(profiles);
	}
}

void parse_all_configs(void)
{
	parse_app_configs();
	parse_qos_mappings();
	parse_sched_profiles();
}
