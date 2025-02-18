/*
 *  log.c
 *
 *  brif
 *  	user log system
 *  
 *  (C) 2025.02.18 <hkdywg@163.com>
 *
 *  This program is free software; you can redistribute it and/r modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 * */
#include "log.h"

#define MAX_CALLBACKS 		32

struct call_back {
	base_log_fn fn;
	void *out_put;
	int level;
};

struct log_info {
	void *user_data;
	base_lock_fn lock;
	int level;
	bool quiet;
	struct call_back call_backs[MAX_CALLBACKS];
};

static const char *level_strings[] = {
	"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

static const char *level_colors[] = {
	"\x1b[94m", "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m"
};

static struct log_info local_log_info;

static void stdout_callback(struct log_event *ev)
{
	char buf[64];
	size_t len = strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", ev->time);
	buf[len] = '\0';
	fprintf(ev->out_put, "%s %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ",
			buf, level_colors[ev->level], level_strings[ev->level],
			ev->file, ev->line);
	vfprintf(ev->out_put, ev->fmt, ev->args);
	fprintf(ev->out_put, "\n");
	fflush(ev->out_put);
}

static void file_callback(struct log_event *ev)
{
	char buf[64];
	size_t len = strftime(buf, sizeof(buf), "%H:%M:%S", ev->time);
	buf[len] = '\0';
	fprintf(ev->out_put, "%s %-5s %s:%d",
			buf, level_strings[ev->level], ev->file, ev->line);
	vfprintf(ev->out_put, ev->fmt, ev->args);
	fprintf(ev->out_put, "\n");
	fflush(ev->out_put);
}

static void lock(void) 
{
	if(local_log_info.lock)
		local_log_info.lock(true, local_log_info.user_data);
}

static void unlock(void) 
{
	if(local_log_info.lock)
		local_log_info.lock(false, local_log_info.user_data);
}

void log_set_lock(base_lock_fn fn,void *user_data)
{
	local_log_info.lock = fn;
	local_log_info.user_data = user_data;
}

void log_set_level(int level)
{
	local_log_info.level = level;
}

void log_set_quiet(bool en)
{
	local_log_info.quiet = en;
}

int log_add_callback(base_log_fn fn, void *arg, int level)
{
	for(int i = 0; i < MAX_CALLBACKS; i++) {
		if(!local_log_info.call_backs[i].fn) {
			local_log_info.call_backs[i] = (struct call_back) {fn, arg, level};
			return 0;
		}
	}
	
	return -1;
}

int log_add_fp(FILE *fp, int level)
{
	return log_add_callback(file_callback, fp, level);
}

static void init_event(struct log_event *ev, void *out)
{
	if(!ev->time) {
		time_t t = time(NULL);
		ev->time = localtime(&t);
	}
	ev->out_put = out;
}

void base_log(int level, const char *file, int line, const char *fmt, ...)
{
	struct log_event ev = {
		.fmt   = fmt,
		.file  = file,
		.line  = line,
		.level = level,
	};

	lock();
	if(!local_log_info.quiet && level >= local_log_info.level) {
		init_event(&ev, stderr);
		va_start(ev.args, fmt);
		stdout_callback(&ev);
		va_end(ev.args);
	}

	for(int i = 0; i < MAX_CALLBACKS && local_log_info.call_backs[i].fn; i++) {
		struct call_back *cb = &local_log_info.call_backs[i]; 	
		if(level >= cb->level) {
			init_event(&ev, cb->out_put);
			va_start(ev.args, fmt);
			cb->fn(&ev);
			va_end(ev.args);
		}
	}
	unlock();
}


