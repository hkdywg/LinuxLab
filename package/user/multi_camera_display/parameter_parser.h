#ifndef __PARAMETER_PARSER_H_
#define __PARAMETER_PARSER_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#define FETAL_MSG(cond, ...) \
    do { \
        if(cond) { \
            int errsv = errno; \
            fprintf(stderr, "ERROR(%s:%d) : ", \
                __FILE__, __LINE__); \
            errno = errsv; \
            fprintf(stderr, __VA_ARGS__); \
            abort(); \
        } \
    } while(0);

static inline int warn(const char *file, int line, const char *fmt, ...) 
{
    int errsv = errno;
    va_list va;
    va_start(va, fmt);
    fprintf(stderr, "Warn(%s:%d): ", file, line);
    vfprintf(stderr, fmt, va);
    va_end(va);
    errno = errsv;

    return 1;
}

#define WARN_MSG(cond, ...) \
    ((cond) ? warn(__FILE__, __LINE__, __VA_ARGS__) : 0)

#define MAX_DEVICES 6

struct setup {
    char module[32];
    int conn_id;
    uint32_t crtc_id;
    int crtc_idx;
    uint32_t plane_id;
    char mode_str[32];
    char videos[32];
    uint32_t width, height;
    uint32_t in_fourcc, out_fourcc;
    uint32_t buffer_count;
    uint32_t size, pitch;
    uint32_t device_num;
    char devs[MAX_DEVICES][15];
};

int parse_args(int argc, char *argv[], struct setup *config);

#endif
