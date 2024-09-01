#include "libmmeimage.h"
#include "libmmeimg_error.h"
#include "libmmeimg_debug.h"
#include "hw_interface.h"
#include <jpeglib.h>
#include <setjmp.h>
#include <bpamem.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <semaphore.h>


typedef struct
{
	MMEData data;

	JPEGD_TransformParams_t       output_params;
	JPEGD_TransformReturnParams_t return_params;

} DecodeJPEGData;

// do scaling at decoding time (within the limts of the jpeg decoder)
void pre_scale(unsigned int src_width, unsigned int src_height, unsigned int dst_width, unsigned int dst_height, unsigned int *out_width, unsigned int *out_height, unsigned int *out_removeright, unsigned int *out_removebottom)
{
	unsigned int src_width_mc = (src_width + 15) & ~15; // macro block limit
	unsigned int src_height_mc = (src_height + 15) & ~15;
	unsigned int scale;
	unsigned int width_mc_diff = src_width_mc - src_width;
	unsigned int height_mc_diff = src_height_mc - src_height;

	if ((src_width_mc / 8 >= dst_width) && (src_height_mc / 8 >= dst_height))
		scale = 8;
	else if ((src_width_mc / 4 >= dst_width) && (src_height_mc / 4 >= dst_height))
		scale = 4;
	else if ((src_width_mc / 2 >= dst_width) && (src_height_mc / 2 >= dst_height))
		scale = 2;
	else
		scale = 1;

	*out_width = src_width_mc / scale;
	*out_height = src_height_mc / scale;
	*out_removeright = width_mc_diff / scale + (width_mc_diff & 1);
	*out_removebottom = height_mc_diff / scale + (height_mc_diff & 1);
}

void jpeg_transformer_callback(MME_Event_t event, MME_Command_t *callback_data, void *user_data)
{
	JPEGD_TransformReturnParams_t *returnparams = (JPEGD_TransformReturnParams_t *)callback_data->CmdStatus.AdditionalInfo_p;
	MMEData *data = (MMEData *)user_data;

	DEBUG_PRINT("transformer callback with event: %d - %s\n", event, get_mme_event_string(event));

	switch (event)
	{
		case MME_COMMAND_COMPLETED_EVT:
		{
			if (callback_data->CmdStatus.State == MME_COMMAND_FAILED)
			{
				if (callback_data->CmdStatus.AdditionalInfoSize != 0)
				{
					MME_ERROR res;

					DEBUG_PRINT("command failed, JPEG return code: %d \n", returnparams->ErrorType);

					res = MME_AbortCommand(data->transformer_handle, callback_data->CmdStatus.CmdId);

					if (res != MME_SUCCESS)
						DEBUG_PRINT("abort command failed: %d - %s\n", res, get_mme_error_string(res));

					sem_post(&data->decode_event);
				}
			}
			else
			{
				if (callback_data->CmdStatus.AdditionalInfoSize != 0)
				{

					DEBUG_PRINT("expanded bytes: %d\n", returnparams->bytes_written);
					DEBUG_PRINT("decode w/h: %dx%d\n", returnparams->decodedImageWidth, returnparams->decodedImageHeight);
					DEBUG_PRINT("JPEG result code: %d\n", returnparams->ErrorType);

					data->decode_success = 1;
					sem_post(&data->decode_event);
				}

			}
			break;
		}
		case MME_DATA_UNDERFLOW_EVT:
		{
			DEBUG_PRINT("data underflow (this is normal if it only occurs once)\n");
			break;
		}


		default:
		{
			DEBUG_PRINT("unhandled event %d - %s occured\n", event, get_mme_event_string(event));

			if (callback_data->CmdStatus.AdditionalInfoSize != 0)
			{
				DEBUG_PRINT(" -> CmdStatus.AdditionalInfoSize: %d", callback_data->CmdStatus.AdditionalInfoSize);
				DEBUG_PRINT(" -> returnparams->bytes_written: %d", returnparams->bytes_written);
				DEBUG_PRINT(" -> returnparams->decodedImageHeight: %d", returnparams->decodedImageHeight);
				DEBUG_PRINT(" -> returnparams->decodedImageWidth: %d", returnparams->decodedImageWidth);
				DEBUG_PRINT("JPEG result code: %d\n", returnparams->ErrorType);
			}
			break;
		}
	};
}

// sequentiallize the requests
static sem_t jpeg_sem;
static int jpeg_sem_initialized = 0;

LIBMMEIMG_ERROR decode_jpeg(FILE *fp, unsigned int original_width, unsigned int original_height, unsigned int dst_width, unsigned int dst_height, char **dest_data)
{
	LIBMMEIMG_ERROR ret;

	*dest_data = NULL;
	// look if we have enough mem before starting the decode
	*dest_data = (char *)malloc(dst_width * dst_height * 3);
	if (!*dest_data)
	{
		DEBUG_PRINT("could not alloc sys mem for image");
		return LIBMMEIMG_NOMEM;
	}

	ret = decode_jpeg_noalloc(fp, original_width, original_height, dst_width, dst_height, *dest_data, 0);

	if (ret != LIBMMEIMG_SUCCESS)
		free(*dest_data);
	return ret;
}

LIBMMEIMG_ERROR decode_jpeg_noalloc(FILE *fp, unsigned int original_width, unsigned int original_height, unsigned int dst_width, unsigned int dst_height, char *dest_data, int mem_is_hw_writeable)
{
	DecodeJPEGData decode_data;
	unsigned int pre_scaled_width, pre_scaled_height, removeright, removebottom;
	int fd_bpa;
	unsigned long filesize;
	BPAMemAllocMemData bpa_data;
	int res, i;
	LIBMMEIMG_ERROR res_img;

	char bpa_mem_device[30];
	char *decode_surface;

	if (!jpeg_sem_initialized)
	{
		sem_init(&jpeg_sem, 0, 1);
		jpeg_sem_initialized = 1;
	}

	sem_wait(&jpeg_sem);

	res_img = mme_loadlib();
	if (res_img != LIBMMEIMG_SUCCESS)
	{
		sem_post(&jpeg_sem);
		return res_img;
	}

	fseek(fp, 0, SEEK_END);
	filesize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	pre_scale(original_width, original_height, dst_width, dst_height, &pre_scaled_width, &pre_scaled_height, &removeright, &removebottom);

	DEBUG_PRINT("pre-scaling to width %d height %d from width %d height %d, macro block limit border width = %d, height = %d\n", pre_scaled_width, pre_scaled_height, original_width, original_height, removeright, removebottom);

	fd_bpa = open("/dev/bpamem0", O_RDWR);

	if (fd_bpa < 0)
	{
		DEBUG_PRINT("cannot access /dev/bpamem0! err = %d", fd_bpa);
		sem_post(&jpeg_sem);
		return LIBMMEIMG_NO_DEVACCESS;
	}

	bpa_data.bpa_part = "LMI_VID";		// TODO: good for ufs910 - please adapt this for other boxes
	bpa_data.mem_size = pre_scaled_width * pre_scaled_height * 2;	// we get the image in UYVY which is 2 byte per pixel
	if (dst_width * dst_height * 3 > filesize)
		if (mem_is_hw_writeable)
			bpa_data.mem_size += filesize;
		else
			bpa_data.mem_size += dst_width * dst_height * 3;
	else
		bpa_data.mem_size += filesize;		// temporarily store the whole file in mem - streaming is slow (at least on ufs910)

	res = ioctl(fd_bpa, BPAMEMIO_ALLOCMEM, &bpa_data); // request memory from bpamem
	if (res)
	{
		DEBUG_PRINT("cannot alloc required mem");
		sem_post(&jpeg_sem);
		return LIBMMEIMG_NOMEM;
	}

	sprintf(bpa_mem_device, "/dev/bpamem%d", bpa_data.device_num);
	close(fd_bpa);

	fd_bpa = open(bpa_mem_device, O_RDWR);

	// if somebody forgot to add all bpamem devs then this gets really bad here
	if (fd_bpa < 0)
	{
		DEBUG_PRINT("cannot access %s! err = %d", bpa_mem_device, fd_bpa);
		sem_post(&jpeg_sem);
		return LIBMMEIMG_NO_DEVACCESS;
	}

	decode_surface = (char *)mmap(0, bpa_data.mem_size, PROT_WRITE | PROT_READ, MAP_SHARED, fd_bpa, 0);

	if (decode_surface == MAP_FAILED)
	{
		DEBUG_PRINT("could not map bpa mem");
		ioctl(fd_bpa, BPAMEMIO_FREEMEM);
		close(fd_bpa);
		sem_post(&jpeg_sem);
		return LIBMMEIMG_NOMEM;
	}

	if (mme_init_multi() != LIBMMEIMG_SUCCESS)
	{
		DEBUG_PRINT("mme did not init");
		munmap(decode_surface, bpa_data.mem_size);
		ioctl(fd_bpa, BPAMEMIO_FREEMEM);
		close(fd_bpa);
		sem_post(&jpeg_sem);
		return LIBMMEIMG_NOMEM;
	}

	// read in the file
	fread(decode_surface + pre_scaled_width * pre_scaled_height * 2, 1, filesize, fp);

	// clear cache
	msync(decode_surface + pre_scaled_width * pre_scaled_height * 2, filesize, MS_SYNC);

	static const char *transformers[] = {	"JPEG_DECODER0",
						"JPEG_Transformer0",
						"JPEG_DECODER1",
						"JPEG_DECODER2",
						"JPEG_DECODER3",
						"JPEG_DECODER4",
						"JPEG_DECODER",
						"JPEG_Transformer1",
						"JPEG_Transformer2",
						"JPEG_Transformer3",
						"JPEG_Transformer4",
						"JPEG_Transformer",
						NULL
					    };

	decode_data.data.decode_success = 0;
	decode_data.data.transform_command = NULL;
	res_img = mme_init_transformer(&decode_data.data, transformers, &jpeg_transformer_callback);
	if (res_img == LIBMMEIMG_SUCCESS)
	{
		// init output settings
		decode_data.output_params.outputSettings.xvalue0 = 0;
		decode_data.output_params.outputSettings.xvalue1 = 0;
		decode_data.output_params.outputSettings.yvalue0 = 0;
		decode_data.output_params.outputSettings.yvalue1 = 0;

		decode_data.output_params.outputSettings.outputWidth    = 0;
		decode_data.output_params.outputSettings.outputHeight   = 0;
		decode_data.output_params.outputSettings.ROTATEFLAG     = 0;
		decode_data.output_params.outputSettings.Rotatedegree   = 0;
		decode_data.output_params.outputSettings.HorizantalFlip = 0;
		decode_data.output_params.outputSettings.VerticalFlip   = 0;
#ifndef STM22
		decode_data.output_params.outputSettings.ROTATEFLAG     = 0x80000000;
		decode_data.output_params.outputSettings.Pitch          = pre_scaled_width * 2;
#endif

		res_img = mme_start_transformer(&decode_data.data, sizeof(JPEGD_TransformReturnParams_t), (void *)&decode_data.return_params, sizeof(JPEGD_TransformParams_t), (void *)&decode_data.output_params, decode_surface, pre_scaled_width * pre_scaled_height * 2);

		if (res_img == LIBMMEIMG_SUCCESS)
			res_img = mme_send_data(&decode_data.data, decode_surface + pre_scaled_width * pre_scaled_height * 2, filesize);
	}

	if (res_img != LIBMMEIMG_SUCCESS || !decode_data.data.decode_success)
	{
		mme_abort_transformer(&decode_data.data);
		mme_deinit_transformer(&decode_data.data);
		munmap(decode_surface, bpa_data.mem_size);
		ioctl(fd_bpa, BPAMEMIO_FREEMEM);
		close(fd_bpa);
		sem_post(&jpeg_sem);
		return LIBMMEIMG_DECODE_ERROR;
	}

	mme_deinit_transformer(&decode_data.data);
	mme_term_multi();
	sem_post(&jpeg_sem);

	if (mem_is_hw_writeable)
		res_img = blit_decoder_result(decode_surface, bpa_data.mem_size, dest_data, dst_width * dst_height * 3, pre_scaled_width, pre_scaled_height, dst_width, dst_height, removeright, removebottom);
	else
		res_img = blit_decoder_result(decode_surface, bpa_data.mem_size, decode_surface + pre_scaled_width * pre_scaled_height * 2, dst_width * dst_height * 3, pre_scaled_width, pre_scaled_height, dst_width, dst_height, removeright, removebottom);

	if (res_img != LIBMMEIMG_SUCCESS)
	{
		munmap(decode_surface, bpa_data.mem_size);
		ioctl(fd_bpa, BPAMEMIO_FREEMEM);
		close(fd_bpa);
		return LIBMMEIMG_DECODE_ERROR;
	}

	if (mem_is_hw_writeable)
		// clear cache
		msync(dest_data, dst_width * dst_height * 3, MS_SYNC);
	else
	{
		// clear cache
		msync(decode_surface + pre_scaled_width * pre_scaled_height * 2, dst_width * dst_height * 3, MS_SYNC);

		// memcpy doesn't work
		for (i = 0; i < dst_width * dst_height * 3; i++)
			*(dest_data + i) = *(decode_surface + pre_scaled_width * pre_scaled_height * 2 + i);
	}

	DEBUG_PRINT("JPEG decode finished");
	munmap(decode_surface, bpa_data.mem_size);
	ioctl(fd_bpa, BPAMEMIO_FREEMEM);
	close(fd_bpa);


	return LIBMMEIMG_SUCCESS;
}


//------------------------------------- non hw stuff

struct r_jpeg_error_mgr
{
	struct jpeg_error_mgr pub;
	jmp_buf envbuffer;
};

void jpeg_cb_error_exit(j_common_ptr cinfo)
{
	struct r_jpeg_error_mgr *mptr;
	mptr = (struct r_jpeg_error_mgr *) cinfo->err;
	(*cinfo->err->output_message)(cinfo);
	longjmp(mptr->envbuffer, 1);
}

LIBMMEIMG_ERROR get_jpeg_img_size(FILE *fp, unsigned int *width, unsigned int *height)
{
	struct jpeg_decompress_struct cinfo;
	struct jpeg_decompress_struct *ciptr = &cinfo;
	struct r_jpeg_error_mgr emgr;

	fseek(fp, 0, SEEK_SET);

	ciptr->err = jpeg_std_error(&emgr.pub);
	emgr.pub.error_exit = jpeg_cb_error_exit;
	if (setjmp(emgr.envbuffer) == 1)
	{
		jpeg_destroy_decompress(ciptr);
		return LIBMMEIMG_MISC_ERROR;
	}

	jpeg_create_decompress(ciptr);
	jpeg_stdio_src(ciptr, fp);
	jpeg_read_header(ciptr, TRUE);

	jpeg_start_decompress(ciptr);

	*width = ciptr->output_width;
	*height = ciptr->output_height;

	jpeg_abort_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	fseek(fp, 0, SEEK_SET);
	return LIBMMEIMG_SUCCESS;
}
