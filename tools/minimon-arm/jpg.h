#ifndef JPG_H
#define JPG_H

#ifndef MIN
#define MIN(x, y) ((x < y) ? (x) : (y))
#endif

typedef struct
{
	unsigned long size;
	unsigned char *ptr;
} jpg_buf_t;

jpg_buf_t build_jpg_from_fb(unsigned char *fb_mem, int fb_width, int fb_height, int bits_per_pixel, int mon_width, int mon_height);

#endif
