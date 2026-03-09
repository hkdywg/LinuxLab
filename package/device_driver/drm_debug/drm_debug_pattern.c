/*
* drm_debug_pattern.c 
*   pattern display in drm system
*
*  (C) 2026.03.02 <hkdywg@163.com>
*
*  This program is free software; you can redistribute it and/r modify
*  it under the terms of the GNU General Public License version 2 as
*  published by the Free Software Foundation.
*
*/

#include <linux/kernel.h>
#include <linux/string.h>
#include <drm/drm_fourcc.h>

#define MAKE_RGB_INFO(rl, ro, gl, go, bl, bo, al, ao) \
    .rgb = { {(rl), (ro)}, {(gl), (go)}, {(bl), (bo)}, {(al), (ao)}, }

#define MAKE_YUV_INFO(order, xsub, ysub, chroma_stride) \
    .yuv = { (order), (xsub), (ysub), (chroma_stride) }

#define MAKE_YUV_601_Y(r, g, b) \
    (((66 * (r) + 129 * (g) +  25 * (b) + 128) >> 8) + 16)

#define MAKE_YUV_601_U(r, g, b) \
    (((-38 * (r) -  74 * (g) + 112 * (b) + 128) >> 8) + 128)

#define MAKE_YUV_601_V(r, g, b) \
    (((112 * (r) -  94 * (g) -  18 * (b) + 128) >> 8) + 128)

#define MAKE_YUV_601(r, g, b) \
    { .y = MAKE_YUV_601_Y(r, g, b), \
      .u = MAKE_YUV_601_U(r, g, b), \
      .v = MAKE_YUV_601_V(r, g, b), }

#define MAKE_RGBA(rgb, r, g, b, a) \
    ((((r) >> (8 - (rgb)->red.length)) << (rgb)->red.offset) | \
     (((g) >> (8 - (rgb)->green.length)) << (rgb)->green.offset) | \
     (((b) >> (8 - (rgb)->blue.length)) << (rgb)->blue.offset) | \
     (((a) >> (8 - (rgb)->alpha.length)) << (rgb)->alpha.offset))

#define MAKE_RGB24(rgb, r, g, b) { .value = MAKE_RGBA(rgb, r, g, b, 0) }

struct color_rgb24 {
    unsigned int value : 24;
} __attribute__((__packed__));

struct color_yuv {
    unsigned char y;
    unsigned char u;
    unsigned char v;
};

struct util_color_component {
    unsigned int length;
    unsigned int offset;
};

struct util_rgb_info {
    struct util_color_component red;
    struct util_color_component green;
    struct util_color_component blue;
    struct util_color_component alpha;
};

enum util_yuv_order {
    YUV_YCbCr = 1,
    YUV_YCrCb = 2,
    YUV_YC = 4,
    YUV_CY = 8,
};

struct util_yuv_info {
    enum util_yuv_order order;
    unsigned int xsub;
    unsigned int ysub;
    unsigned int chroma_stride;
};

struct util_format_info {
    uint32_t format;
    const char *name;
    const struct util_rgb_info rgb;
    const struct util_yuv_info yuv;
};

static const struct util_format_info format_info[] = {
    /* YUV semi-planer */
    { DRM_FORMAT_NV12, "NV12", MAKE_YUV_INFO(YUV_YCbCr, 2, 2, 2) },
    { DRM_FORMAT_NV21, "NV21", MAKE_YUV_INFO(YUV_YCrCb, 2, 2, 2) },
    { DRM_FORMAT_NV21, "NV16", MAKE_YUV_INFO(YUV_YCbCr, 2, 1, 2) },
    { DRM_FORMAT_NV21, "NV21", MAKE_YUV_INFO(YUV_YCrCb, 2, 1, 2) },
    /* RGB565 */
    { DRM_FORMAT_RGB565, "RG16", MAKE_RGB_INFO(5, 11, 6, 5, 5, 0, 0, 0) },
    { DRM_FORMAT_BGR565, "BG16", MAKE_RGB_INFO(5, 0, 6, 5, 5, 11, 0, 0) },
    /* RGB888 */
    { DRM_FORMAT_RGB888, "RG24", MAKE_RGB_INFO(8, 16, 8, 8, 8, 0, 0, 0) },
    { DRM_FORMAT_BGR888, "BG24", MAKE_RGB_INFO(8, 0, 8, 8, 8, 16, 0, 0) },
    /* ARGB8888 */
    { DRM_FORMAT_ARGB8888, "AR24", MAKE_RGB_INFO(8, 16, 8, 8, 8, 0, 8, 24) },
    { DRM_FORMAT_XRGB8888, "XR24", MAKE_RGB_INFO(8, 16, 8, 8, 8, 0, 0, 0) },
    { DRM_FORMAT_ABGR8888, "AB24", MAKE_RGB_INFO(8, 0, 8, 8, 8, 16, 8, 24) },
    { DRM_FORMAT_XBGR8888, "XB24", MAKE_RGB_INFO(8, 0, 8, 8, 8, 16, 0, 0) },
    { DRM_FORMAT_RGBA8888, "RA24", MAKE_RGB_INFO(8, 24, 8, 16, 8, 8, 8, 0) },
    { DRM_FORMAT_RGBX8888, "RX24", MAKE_RGB_INFO(8, 24, 8, 16, 8, 8, 0, 0) },
    { DRM_FORMAT_BGRA8888, "BA24", MAKE_RGB_INFO(8, 8, 8, 16, 8, 24, 8, 0) },
    { DRM_FORMAT_BGRX8888, "BX24", MAKE_RGB_INFO(8, 8, 8, 16, 8, 24, 0, 0) },
};

static const struct util_format_info *util_format_info_find(uint32_t format)
{
    unsigned int i;

    for(i = 0; i < ARRAY_SIZE(format_info); i++) {
        if(format_info[i].format == format)
            return &format_info[i];
    }

    return NULL;
}

static void fill_smpte_yuv_planar(const struct util_yuv_info *yuv,
                            unsigned char *y_mem, unsigned char *u_mem, unsigned char *v_mem,
                            unsigned int width, unsigned int height, unsigned int stride)
{
    const struct color_yuv colors_pattern[] = {
        MAKE_YUV_601(191, 192, 192),  /* grey */
        MAKE_YUV_601(192, 192, 0),    /* yellow */
        MAKE_YUV_601(0, 192, 192),    /* cyan */
        MAKE_YUV_601(0, 192, 0),      /* green */
        MAKE_YUV_601(192, 0, 192),    /* magenta */
        MAKE_YUV_601(192, 0, 0),      /* red */
        MAKE_YUV_601(0, 0, 192),      /* red */
    };

    unsigned int cs = yuv->chroma_stride;
    unsigned int xsub = yuv->xsub;
    unsigned int x, y;

    for(y = 0; y < height; ++y) {
        for(x = 0; x < width; ++x) {
            y_mem[x] = colors_pattern[x * 7 / width].y;
            u_mem[x * cs / xsub] = colors_pattern[x * 7 / width].u;
            v_mem[x * cs / xsub] = colors_pattern[x * 7 / width].v;
        }
        y_mem += stride;
        u_mem += stride * cs / xsub;
        v_mem += stride * cs / xsub;
    }

}

static void fill_smpte_rgb16(const struct util_rgb_info *rgb, void *mem,
                    unsigned int width, unsigned int height, unsigned int stride)
{
	const uint16_t colors_pattern[] = {
		MAKE_RGBA(rgb, 192, 192, 192, 255), /* grey */
		MAKE_RGBA(rgb, 192, 192, 0, 255),   /* yellow */
		MAKE_RGBA(rgb, 0, 192, 192, 255),   /* cyan */
		MAKE_RGBA(rgb, 0, 192, 0, 255),     /* green */
		MAKE_RGBA(rgb, 192, 0, 192, 255),   /* magenta */
		MAKE_RGBA(rgb, 192, 0, 0, 255),     /* red */
		MAKE_RGBA(rgb, 0, 0, 192, 255),     /* blue */
	};

    unsigned int x, y;

    for(y = 0; y < height; ++y) {
        for(x = 0; x < width; ++x) {
            ((uint16_t *)mem)[x] = colors_pattern[x * 7 / width];
        }
        mem += stride;
    }
}


static void fill_smpte_rgb24(const struct util_rgb_info *rgb, void *mem,
                unsigned int width, unsigned int height, unsigned int stride)
{
	const struct color_rgb24 colors_pattern[] = {
		MAKE_RGB24(rgb, 192, 192, 192),		/* grey */
		MAKE_RGB24(rgb, 192, 192, 0),		/* yellow */
		MAKE_RGB24(rgb, 0, 192, 192),		/* cyan */
		MAKE_RGB24(rgb, 0, 192, 0),		    /* green */
		MAKE_RGB24(rgb, 192, 0, 192),		/* magenta */
		MAKE_RGB24(rgb, 192, 0, 0),		    /* red */
		MAKE_RGB24(rgb, 0, 0, 192),		    /* blue */
	};
    unsigned int x, y;

    if(!mem)
        return;

    for(y = 0; y < height; ++y) {
        for(x = 0; x < width; ++x) {
            ((struct color_rgb24 *)mem)[x] = colors_pattern[x * 7 / width];
        }
        mem += stride;
    }
}


static void fill_smpte_rgb32(const struct util_rgb_info *rgb, void *mem,
                unsigned int width, unsigned int height, unsigned int stride)
{
	const uint32_t colors_pattern[] = {
		MAKE_RGBA(rgb, 192, 192, 192, 255),	/* grey */
		MAKE_RGBA(rgb, 192, 192, 0, 255),	/* yellow */
		MAKE_RGBA(rgb, 0, 192, 192, 255),	/* cyan */
		MAKE_RGBA(rgb, 0, 255, 0, 255),		/* green */
		MAKE_RGBA(rgb, 192, 0, 192, 255),	/* magenta */
		MAKE_RGBA(rgb, 255, 0, 0, 255),		/* red */
		MAKE_RGBA(rgb, 0, 0, 255, 255),		/* blue */
	};
    unsigned int x, y;

    for(y = 0; y < height; ++y) {
        for(x = 0; x < width; ++x) {
            ((uint32_t *)mem)[x] = colors_pattern[x * 7 / width];
        }
        mem = (uint8_t *)mem + stride;
    }
}

static void fill_smpte(const struct util_format_info *info, void *planes[3],
                unsigned int width, unsigned int height, unsigned int stride)
{
    unsigned char *u, *v;

    if(width < 7)
        width = 7;

    switch(info->format) {
        case DRM_FORMAT_NV12:
        case DRM_FORMAT_NV21:
        case DRM_FORMAT_NV16:
        case DRM_FORMAT_NV61:
            u = info->yuv.order & YUV_YCbCr ? planes[1] : planes[1] + 1;
            v = info->yuv.order & YUV_YCrCb ? planes[1] : planes[1] + 1;
            return fill_smpte_yuv_planar(&info->yuv, planes[0], u, v,
                                width, height, stride);
        case DRM_FORMAT_RGB565:
        case DRM_FORMAT_BGR565:
            return fill_smpte_rgb16(&info->rgb, planes[0], width, height, stride);
        case DRM_FORMAT_RGB888:
        case DRM_FORMAT_BGR888:
            return fill_smpte_rgb24(&info->rgb, planes[0], width, height, stride);
        case DRM_FORMAT_ARGB8888:
        case DRM_FORMAT_XRGB8888:
        case DRM_FORMAT_ABGR8888:
        case DRM_FORMAT_XBGR8888:
        case DRM_FORMAT_RGBA8888:
        case DRM_FORMAT_RGBX8888:
        case DRM_FORMAT_BGRA8888:
        case DRM_FORMAT_BGRX8888:
            return fill_smpte_rgb32(&info->rgb, planes[0], width, height, stride);
    }
}

void drm_fill_color_bar(uint32_t format, void *planes[3], unsigned int width,
                unsigned int height, unsigned int stride)
{
    const struct util_format_info *info;

    if(!planes)
        return;

    info = util_format_info_find(format);
    if(info == NULL)
        return;

    return fill_smpte(info, planes, width, height, stride);
}










