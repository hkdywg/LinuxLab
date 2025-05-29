#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

void save_ppm(const char *filename, int width, int height, uint32_t *buffer) 
{
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("fopen");
        return;
    }

    fprintf(fp, "P6\n%d %d\n255\n", width, height);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint32_t pixel = buffer[y * width + x];
            fputc((pixel >> 16) & 0xFF, fp); // Red
            fputc((pixel >> 8) & 0xFF, fp);  // Green
            fputc(pixel & 0xFF, fp);         // Blue
        }
    }
    fclose(fp);
}

void print_drm_info(struct drm_mode_fb_cmd *info)
{
    printf(" frame buffer id: %d\n", info->fb_id);
    printf(" frame buffer width: %d\n", info->width);
    printf(" frame buffer height: %d\n", info->height);
    printf(" frame buffer pitch: %d\n", info->pitch);
    printf(" frame buffer bpp: %d\n", info->bpp);
    printf(" frame buffer depth: %d\n", info->depth);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <output.ppm>\n", argv[0]);
        return 1;
    }
    const char *filename = argv[1];

    int fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    drmModeRes *res = drmModeGetResources(fd);
    if (!res) {
        perror("drmModeGetResources");
        close(fd);
        return 1;
    }

    drmModeCrtc *crtc = drmModeGetCrtc(fd, res->crtcs[0]);
    if (!crtc) {
        perror("drmModeGetCrtc");
        drmModeFreeResources(res);
        close(fd);
        return 1;
    }

    struct drm_mode_fb_cmd fb_cmd;
    memset(&fb_cmd, 0, sizeof(fb_cmd));
    fb_cmd.fb_id = crtc->buffer_id;

    if (ioctl(fd, DRM_IOCTL_MODE_GETFB, &fb_cmd) < 0) {
        perror("DRM_IOCTL_MODE_GETFB");
        drmModeFreeCrtc(crtc);
        drmModeFreeResources(res);
        close(fd);
        return 1;
    }
    print_drm_info(&fb_cmd);

    struct drm_mode_map_dumb map_dumb;
    memset(&map_dumb, 0, sizeof(map_dumb));
    map_dumb.handle = fb_cmd.handle;

    if (ioctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map_dumb) < 0) {
        perror("DRM_IOCTL_MODE_MAP_DUMB");
        drmModeFreeCrtc(crtc);
        drmModeFreeResources(res);
        close(fd);
        return 1;
    }

    size_t buffer_size = fb_cmd.pitch * fb_cmd.height;
    uint32_t *buffer = mmap(NULL, buffer_size, PROT_READ, MAP_SHARED, fd, map_dumb.offset);
    if (buffer == MAP_FAILED) {
        perror("mmap");
        drmModeFreeCrtc(crtc);
        drmModeFreeResources(res);
        close(fd);
        return 1;
    }

    save_ppm(filename, fb_cmd.width, fb_cmd.height, buffer);

    munmap(buffer, buffer_size);
    drmModeFreeCrtc(crtc);
    drmModeFreeResources(res);
    close(fd);

    printf("Screenshot saved to %s\n", filename);
    return 0;
}

