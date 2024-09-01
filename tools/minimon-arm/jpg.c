#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "jpeglib.h"
#include "jerror.h"

#include "jpg.h"

typedef struct
{
	struct jpeg_destination_mgr pub; /* public fields */

	unsigned char ** outbuffer;	/* target buffer */
	unsigned long * outsize;
	unsigned char * newbuffer;	/* newly allocated buffer */
	JOCTET * buffer;		/* start of buffer */
	size_t bufsize;
} mem_dest_mgr;

typedef mem_dest_mgr * mem_dest_mgr_ptr;

static void init_mem_destination (j_compress_ptr cinfo)
{
}

static boolean empty_mem_output_buffer(j_compress_ptr cinfo)
{
	size_t nextsize;
	JOCTET * nextbuffer;
	mem_dest_mgr_ptr dest = (mem_dest_mgr_ptr) cinfo->dest;

	/* Try to allocate new buffer with double size */
	nextsize = dest->bufsize * 2;
	nextbuffer = malloc(nextsize);

	if (nextbuffer == NULL)
		ERREXIT1(cinfo, JERR_OUT_OF_MEMORY, 10);

	memcpy(nextbuffer, dest->buffer, dest->bufsize);

	if (dest->newbuffer != NULL)
		free(dest->newbuffer);

	dest->newbuffer = nextbuffer;

	dest->pub.next_output_byte = nextbuffer + dest->bufsize;
	dest->pub.free_in_buffer = dest->bufsize;

	dest->buffer = nextbuffer;
	dest->bufsize = nextsize;

	return TRUE;
}

static void term_mem_destination(j_compress_ptr cinfo)
{
	mem_dest_mgr_ptr dest = (mem_dest_mgr_ptr) cinfo->dest;

	*dest->outbuffer = dest->buffer;
	*dest->outsize = dest->bufsize - dest->pub.free_in_buffer;
}

static void my_jpeg_mem_dest(j_compress_ptr cinfo, unsigned char ** outbuffer, unsigned long * outsize)
{
	mem_dest_mgr_ptr dest;

	if (outbuffer == NULL || outsize == NULL)	/* sanity check */
		ERREXIT(cinfo, JERR_BUFFER_SIZE);

	/* The destination object is made permanent so that multiple JPEG images
	 * can be written to the same buffer without re-executing jpeg_mem_dest.
	 */
	if (cinfo->dest == NULL)  	/* first time for this JPEG object? */
	{
		cinfo->dest = (struct jpeg_destination_mgr *)
		              (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
		                      sizeof(mem_dest_mgr));
	}

	dest = (mem_dest_mgr_ptr) cinfo->dest;
	dest->pub.init_destination = init_mem_destination;
	dest->pub.empty_output_buffer = empty_mem_output_buffer;
	dest->pub.term_destination = term_mem_destination;
	dest->outbuffer = outbuffer;
	dest->outsize = outsize;
	dest->newbuffer = NULL;

	if (*outbuffer == NULL || *outsize == 0)
	{
		/* Allocate initial buffer */
		dest->newbuffer = *outbuffer = malloc(65536);
		if (dest->newbuffer == NULL)
			ERREXIT1(cinfo, JERR_OUT_OF_MEMORY, 10);
		*outsize = 65536;
	}

	dest->pub.next_output_byte = dest->buffer = *outbuffer;
	dest->pub.free_in_buffer = dest->bufsize = *outsize;
}

#ifdef HAVE_LIBJPEG_TURBO
jpg_buf_t build_jpg_from_fb(unsigned char *fb_mem, int fb_width, int fb_height, int bits_per_pixel, int mon_width, int mon_height)
{
	int scanline;
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	jpg_buf_t jpg_buf;
	jpg_buf.size = 1 << 20;
	jpg_buf.ptr = NULL;

	my_jpeg_mem_dest(&cinfo, &jpg_buf.ptr, &jpg_buf.size);

	cinfo.image_width = mon_width;
	cinfo.image_height = mon_height;
	cinfo.input_components = 4;
	cinfo.in_color_space = JCS_EXT_BGRX;
	jpeg_set_defaults(&cinfo);
	cinfo.dct_method = JDCT_FASTEST;
	jpeg_set_quality(&cinfo, 70, TRUE);

	jpeg_start_compress(&cinfo, TRUE);


	unsigned char buf[mon_width * 4];
	JSAMPROW row_ptr[1] = {buf};
	memset(buf, 0, mon_width*4);
	if (fb_width >= mon_width)
		while (cinfo.next_scanline < cinfo.image_height)
		{
			row_ptr[0] = &fb_mem[cinfo.next_scanline * fb_width * 4];
			jpeg_write_scanlines(&cinfo, row_ptr, 1);
		}
	else
	{
		for (scanline = 0; scanline < MIN(fb_height, mon_height); scanline++)
		{
			memcpy(buf, &fb_mem[scanline * fb_width * 4], fb_width * 4);
			jpeg_write_scanlines(&cinfo, row_ptr, 1);
		}
	}
	memset(buf, 0, mon_width*4);
	for (scanline = fb_height; scanline < mon_height; scanline++)
	{
		jpeg_write_scanlines(&cinfo, row_ptr, 1);
	}

	jpeg_finish_compress(&cinfo);

	jpeg_destroy_compress(&cinfo);

	return jpg_buf;
}
#else
static void convert_rgba_to_rgb(unsigned char *buf, unsigned char *fb_mem, int width)
{
	int i;
	for (i = 0; i < width; i++)
	{
// BGRX
		unsigned int x = *(unsigned int *)fb_mem;
		*buf++ = (x >> 16) & 0xff;
		*buf++ = (x >> 8) & 0xff;
		*buf++ = (x) & 0xff;
		fb_mem += 4;
	}
}

jpg_buf_t build_jpg_from_fb(unsigned char *fb_mem, int fb_width, int fb_height, int bits_per_pixel, int mon_width, int mon_height)
{
	int scanline;
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	jpg_buf_t jpg_buf;
	jpg_buf.size = 1 << 20;
	jpg_buf.ptr = NULL;

	my_jpeg_mem_dest(&cinfo, &jpg_buf.ptr, &jpg_buf.size);

	cinfo.image_width = mon_width;
	cinfo.image_height = mon_height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;
	jpeg_set_defaults(&cinfo);
	cinfo.dct_method = JDCT_FASTEST;
	jpeg_set_quality(&cinfo, 70, TRUE);

	jpeg_start_compress(&cinfo, TRUE);

	JSAMPLE buf[mon_width * 3];
	JSAMPROW row_ptr[1] = {buf};
	memset(buf, 0, mon_width*3);
	for (scanline = 0; scanline < MIN(fb_height, mon_height); scanline++)
	{
		convert_rgba_to_rgb(buf, &fb_mem[scanline * fb_width * 4], mon_width);
		jpeg_write_scanlines(&cinfo, row_ptr, 1);
	}
	memset(buf, 0, mon_width*3);
	for (scanline = fb_height; scanline < mon_height; scanline++)
	{
		jpeg_write_scanlines(&cinfo, row_ptr, 1);
	}

	jpeg_finish_compress(&cinfo);

	jpeg_destroy_compress(&cinfo);

	return jpg_buf;
}
#endif
