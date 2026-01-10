/* Copyright 2019 Tronlong Elec. Tech. Co. Ltd. All Rights Reserved. */

#ifndef PARAMETER_PARSER_H
#define PARAMETER_PARSER_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#if defined (__cplusplus)
extern "C" {
#endif

struct _Params {
    
    char    *location;
    char    *h26x;
    char    *rtsp_url;
    int     connector_id;
    int     plane_id;
    bool    replay;
    bool    is_rtsp;
};

bool parse_parameter(struct _Params *params, int argc, char **argv);

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */

#endif /* PARAMETER_PARSER_H */

