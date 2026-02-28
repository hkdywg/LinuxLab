#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>

#include "parameter_parser.h"

static void usage(char *name)
{
    printf("usage: %s [-Moisth]\n", name);
    printf("\t-M <drm-module>\tset DRM module\n");
    printf("\t-o <connector_id>:<crtc_id>:<mode>\tset a module\n");
    printf("\t-i <video-node>\tsupport to select 1-6 camera devices and set like -i 84,66,75,93,102,111 \n");
    printf("\t-s <widthxheight>\tset input resolution\n");
    printf("\t-f <fourcc>\tset input format using 4cc\n");
    printf("\t-F <fourcc>\tset output format using 4cc\n");
    printf("\t-b buffer_count\tset numer of buffers\n");
    printf("\t-h\tshow this help\n");
}

int parse_args(int argc, char *argv[], struct setup *config)
{

}
