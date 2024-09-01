#ifndef _LIBMMEIMAGE_H
#define _LIBMMEIMAGE_H

#include "libmmeimg_error.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

// FILE pointers provided to any of these functions need to be opened!!

// This function is only provided for conveniency because the hardware is a quite fast in
// resizing images at decode time. However for providing the correct scaled image sizes one
// will need the original image sizes quite often.
LIBMMEIMG_ERROR get_jpeg_img_size(FILE *fp, unsigned int *width, unsigned int *height);

// output is in BGR/user space (3 bytes per pixel)
LIBMMEIMG_ERROR decode_jpeg(FILE *fp, unsigned int original_width, unsigned int original_height, unsigned int dst_width, unsigned int dst_height, char **dest_data);

// output is in BGR (3 bytes per pixel), but allocs no memory
LIBMMEIMG_ERROR decode_jpeg_noalloc(FILE *fp, unsigned int original_width, unsigned int original_height, unsigned int dst_width, unsigned int dst_height, char *dest_data, int mem_is_hw_writeable);

#ifdef __cplusplus
}
#endif

#endif
