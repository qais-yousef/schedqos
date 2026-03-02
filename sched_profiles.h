/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Qais Yousef */

void init_sched_profiles(void);
void deinit_sched_profiles(void);
void *create_sched_profile(const char *profile);
void sched_profiles_add_debugfs(void *profile, const char *node, const char *value);
void sched_profiles_add_cpufreq(void *profile, const char *node, const char *value);
void sched_profiles_apply_profile(const char *profile);
