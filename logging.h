/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Qais Yousef */
#include <stdio.h>
#include <time.h>

#include "parse_argp.h"

#define __LOG_OUT(level, fmt, ...) do { \
	time_t t = time(NULL); \
	struct tm *tm_info = localtime(&t); \
	char time_buf[26]; \
	strftime(time_buf, 26, "%Y-%m-%d %H:%M:%S", tm_info); \
	fprintf(stdout, "[%s] [%s] [%s:%d] " fmt "\n", \
			time_buf, level, __FILE__, __LINE__, ##__VA_ARGS__); \
} while (0)

#define __LOG_ERR(level, fmt, ...) do { \
	time_t t = time(NULL); \
	struct tm *tm_info = localtime(&t); \
	char time_buf[26]; \
	strftime(time_buf, 26, "%Y-%m-%d %H:%M:%S", tm_info); \
	fprintf(stderr, "[%s] [%s] [%s:%d] " fmt "\n", \
			time_buf, level, __FILE__, __LINE__, ##__VA_ARGS__); \
} while (0)

#define LOG_INFO(fmt, ...)	__LOG_OUT("INFO",  fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)	__LOG_ERR("WARN",  fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)	__LOG_ERR("ERROR", fmt, ##__VA_ARGS__)

#define LOG_VERBOSE(fmt, ...) do {				\
	if (sqos_opts.verbose)					\
		__LOG_OUT("VERBOSE", fmt, ##__VA_ARGS__);	\
} while (0)
