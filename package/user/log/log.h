/*
 *  log.h
 *
 *  brif
 *  	user log system file header
 *  
 *  (C) 2025.02.18 <hkdywg@163.com>
 *
 *  This program is free software; you can redistribute it and/r modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 * */
#ifndef __LOG_H_
#define __LOG_H_

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <time.h>

#define LOG_VERSION 	"1.0.0"

struct log_event  {
	va_list args;
	const char *fmt;
	const char *file;
	struct tm *time;
	void *out_put;
	int line;
	int level;
};

typedef void (*base_log_fn)(struct log_event *event);
typedef void (*base_lock_fn)(bool lock, void *user_data);

enum {
	LOG_TRACE,
	LOG_DEBUG,
	LOG_INFO,
	LOG_WARN,
	LOG_ERROR,
	LOG_FATAL
};

#define log_trace(...) base_log(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define log_debug(...) base_log(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define log_info(...)  base_log(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...)  base_log(LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...) base_log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define log_fatal(...) base_log(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

void log_set_lock(base_lock_fn fn,void *user_data);
void log_set_level(int level);
void log_set_quiet(bool en);
int log_add_callback(base_log_fn fn, void *arg, int level);
int log_add_fp(FILE *fp, int level);
void base_log(int level, const char *file, int linne, const char *fmt, ...);


#endif
