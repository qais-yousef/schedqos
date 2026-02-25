/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Qais Yousef */
#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parse_argp.h"
#include "cJSON.h"

#define APP_CONFIGS_PATH	"app_configs/"
#define QOS_MAPPINGS_FILE	"qos_mappings.json"
#define SCHED_PROFILES_FILE	"sched_profiles.json"

/*
 * App config parsing function.
 */
static void __parse_app_config(const char *json_string)
{
	cJSON *root = cJSON_Parse(json_string);
	if (!root) {
		fprintf(stderr, "Error: Invalid JSON format\n");
		return;
	}

	/* We identify apps by their cmdline */
	cJSON *app_node = NULL;
	cJSON_ArrayForEach(app_node, root) {
		fprintf(stdout, "app cmdline: %s\n", app_node->string);

		cJSON *version = cJSON_GetObjectItemCaseSensitive(app_node, "version");
		cJSON *period = cJSON_GetObjectItemCaseSensitive(app_node, "period");

		if (cJSON_IsString(version))
			fprintf(stdout, "  Version: %s\n", version->valuestring);
		if (cJSON_IsNumber(period))
			fprintf(stdout, "  Period: %d\n", period->valueint);

		cJSON *thread_qos = cJSON_GetObjectItemCaseSensitive(app_node, "thread_qos");
		if (cJSON_IsObject(thread_qos)) {
			fprintf(stdout, "  Thread QoS Settings:\n");
			cJSON *thread = NULL;
			cJSON_ArrayForEach(thread, thread_qos) {
				if (cJSON_IsArray(thread)) {
					cJSON *qos = NULL;
					cJSON_ArrayForEach(qos, thread) {
						if (cJSON_IsString(qos)) {
							fprintf(stdout, "	- %s: %s\n", thread->string, qos->valuestring);
						}
					}
				} else if (cJSON_IsString(thread)) {
					fprintf(stdout, "	- %s: %s\n", thread->string, thread->valuestring);
				}
			}
		}
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
		fprintf(stderr, "Error: Invalid JSON format\n");
		return;
	}

	cJSON *qos_node = NULL;
	cJSON_ArrayForEach(qos_node, root) {
		fprintf(stdout, "QoS Tag: %s\n", qos_node->string);

		cJSON *runtime = cJSON_GetObjectItemCaseSensitive(qos_node, "runtime");
		cJSON *policy = cJSON_GetObjectItemCaseSensitive(qos_node, "policy");
		cJSON *uclamp_max = cJSON_GetObjectItemCaseSensitive(qos_node, "uclamp_max");

		if (cJSON_IsNumber(runtime))
			fprintf(stdout, "  runtime: %d\n", runtime->valueint);
		if (cJSON_IsString(policy))
			fprintf(stdout, "  policy: %s\n", policy->valuestring);
		if (cJSON_IsNumber(uclamp_max))
			fprintf(stdout, "  uclamp_max: %d\n", uclamp_max->valueint);
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
		fprintf(stderr, "Error: Invalid JSON format\n");
		return;
	}

	cJSON *profile = NULL;
	cJSON *node = NULL;
	cJSON_ArrayForEach(profile, root) {
		fprintf(stdout, "Sched Profile: %s\n", profile->string);

		cJSON_ArrayForEach(node, profile) {
			if (cJSON_IsArray(node)) {
				cJSON *val = NULL;
				cJSON_ArrayForEach(val, node) {
					if (cJSON_IsString(val)) {
						fprintf(stdout, "	- %s: %s\n", node->string, val->valuestring);
					} else if (cJSON_IsNumber(val)) {
						fprintf(stdout, "	- %s: %d\n", node->string, val->valueint);
					}
				}
			} else if (cJSON_IsString(node)) {
				fprintf(stdout, "	- %s: %s\n", node->string, node->valuestring);
			} else if (cJSON_IsNumber(node)) {
				fprintf(stdout, "	- %s: %d\n", node->string, node->valueint);
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
	fprintf(stdout, "Reading %s\n", full_path);

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

			fprintf(stdout, "processing: %s\n", full_path);

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
