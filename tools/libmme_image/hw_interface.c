#include "hw_interface.h"
#include "libmmeimg_debug.h"
#include <malloc.h>
#include <string.h>
#include <asm/types.h> // was forgotten in stmfb.h
#include <linux/stmfb.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>

// debug strings
const char *mme_event_strings[] =
{
	"MME_COMMAND_COMPLETED_EVT",
	"MME_DATA_UNDERFLOW_EVT",
	"MME_NOT_ENOUGH_MEMORY_EVT",
	"MME_NEW_COMMAND_EVT",
};

const char *mme_error_type_strings[] =
{
	"MME_SUCCESS",
	"MME_DRIVER_NOT_INITIALIZED",
	"MME_DRIVER_ALREADY_INITIALIZED",
	"MME_NOMEM",
	"MME_INVALID_TRANSPORT",
	"MME_INVALID_HANDLE",
	"MME_INVALID_ARGUMENT",
	"MME_UNKNOWN_TRANSFORMER",
	"MME_TRANSFORMER_NOT_RESPONDING",
	"MME_HANDLES_STILL_OPEN",
	"MME_COMMAND_STILL_EXECUTING",
	"MME_COMMAND_ABORTED",
	"MME_DATA_UNDERFLOW",
	"MME_DATA_OVERFLOW",
	"MME_TRANSFORM_DEFERRED",
	"MME_SYSTEM_INTERRUPT",
	"MME_EMBX_ERROR",
	"MME_INTERNAL_ERROR",
	"MME_NOT_IMPLEMENTED"
};

MME_Init_func MME_Init = (MME_Init_func) _mme_default_func;
MME_Term_func MME_Term = (MME_Term_func) _mme_default_func;
MME_SendCommand_func  MME_SendCommand = (MME_SendCommand_func) _mme_default_func;
MME_AbortCommand_func MME_AbortCommand = (MME_AbortCommand_func) _mme_default_func;
MME_AllocDataBuffer_func MME_AllocDataBuffer = (MME_AllocDataBuffer_func) _mme_default_func;
MME_FreeDataBuffer_func  MME_FreeDataBuffer  = (MME_FreeDataBuffer_func) _mme_default_func;
MME_InitTransformer_func MME_InitTransformer = (MME_InitTransformer_func) _mme_default_func;
MME_TermTransformer_func MME_TermTransformer = (MME_TermTransformer_func) _mme_default_func;

void *libmme;


MME_ERROR _mme_default_func(void)
{
	return MME_DRIVER_NOT_INITIALIZED;
}

const char *get_mme_event_string(MME_Event_t ev)
{
	return (((unsigned int) ev) < ARRAY_SIZE(mme_event_strings)) ? mme_event_strings[ev] : "* unknown event code *";
}

const char *get_mme_error_string(MME_ERROR e)
{
	return (((unsigned int) e) < ARRAY_SIZE(mme_error_type_strings)) ? mme_error_type_strings[e] : "unknown error code";
}
inline void print_mme_error(MME_ERROR e)
{
	DEBUG_PRINT("Result : %s\n", get_mme_error_string(e));
}

LIBMMEIMG_ERROR mme_loadlib(void)
{
	if (!libmme)
	{
		if ((libmme = dlopen("libmme_host.so", RTLD_LAZY)) != NULL)
		{
			_MME_Init = (MME_Init_func)dlsym(libmme, "MME_Init");
			_MME_Term = (MME_Term_func)dlsym(libmme, "MME_Term");
			_MME_SendCommand  = (MME_SendCommand_func)dlsym(libmme, "MME_SendCommand");
			_MME_AbortCommand = (MME_AbortCommand_func)dlsym(libmme, "MME_AbortCommand");
			_MME_AllocDataBuffer = (MME_AllocDataBuffer_func)dlsym(libmme, "MME_AllocDataBuffer");
			_MME_FreeDataBuffer  = (MME_FreeDataBuffer_func)dlsym(libmme, "MME_FreeDataBuffer");
			_MME_InitTransformer = (MME_InitTransformer_func)dlsym(libmme, "MME_InitTransformer");
			_MME_TermTransformer = (MME_TermTransformer_func)dlsym(libmme, "MME_TermTransformer");

			if (!_MME_Init || !_MME_Term ||
					!_MME_SendCommand || !_MME_AbortCommand ||
					!_MME_AllocDataBuffer || !_MME_FreeDataBuffer ||
					!_MME_InitTransformer || !_MME_TermTransformer)
			{
				_MME_Init = (MME_Init_func) _mme_default_func;
				_MME_Term = (MME_Term_func) _mme_default_func;
				_MME_SendCommand = (MME_SendCommand_func) _mme_default_func;
				_MME_AbortCommand = (MME_AbortCommand_func) _mme_default_func;
				_MME_AllocDataBuffer = (MME_AllocDataBuffer_func) _mme_default_func;
				_MME_FreeDataBuffer  = (MME_FreeDataBuffer_func) _mme_default_func;
				_MME_InitTransformer = (MME_InitTransformer_func) _mme_default_func;
				_MME_TermTransformer = (MME_TermTransformer_func) _mme_default_func;

				dlclose(libmme);
				libmme = NULL;
			}
		}

		if (!libmme)
		{
			DEBUG_PRINT("%s: Couldn't resolve libmme_host.so!\n", __FUNCTION__);
			return LIBMMEIMG_MISC_ERROR;
		}
	}

	return LIBMMEIMG_SUCCESS;
}

static unsigned int init_count = 0;
static sem_t mme_init_sem;
static int mme_init_sem_initialized = 0;

LIBMMEIMG_ERROR mme_init_multi(void)
{
	MME_ERROR res_mme;

	if (!mme_init_sem_initialized)
	{
		sem_init(&mme_init_sem, 0, 1);
		mme_init_sem_initialized = 1;
	}

	sem_wait(&mme_init_sem);
	if (init_count == 0)
	{
		res_mme = MME_Init();
		if (res_mme != MME_SUCCESS && res_mme != MME_DRIVER_ALREADY_INITIALIZED)
		{
			sem_post(&mme_init_sem);
			return LIBMMEIMG_MISC_ERROR;
		}
		else
		{
			init_count++;
			sem_post(&mme_init_sem);
			return LIBMMEIMG_SUCCESS;
		}
	}
	else
	{
		init_count++;
		sem_post(&mme_init_sem);
		return LIBMMEIMG_SUCCESS;
	}


}

LIBMMEIMG_ERROR mme_term_multi(void)
{
	MME_ERROR res_mme;

	sem_wait(&mme_init_sem);
	res_mme = MME_Term();
	if (res_mme != MME_SUCCESS && res_mme != MME_DRIVER_ALREADY_INITIALIZED)
	{
		sem_post(&mme_init_sem);
		return LIBMMEIMG_MISC_ERROR;
	}
	else
	{
		init_count--;
		sem_post(&mme_init_sem);
		return LIBMMEIMG_SUCCESS;
	}
}

LIBMMEIMG_ERROR mme_init_transformer(MMEData *data, const char *transformer_names[], void (*transformer_callback)(MME_Event_t, MME_Command_t *, void *))
{
	unsigned int index = 0;

	MME_ERROR ret;
	MME_TransformerInitParams_t params;

	params.StructSize			= sizeof(params);
	params.Priority				= MME_PRIORITY_BELOW_NORMAL;
	params.Callback				= (void *)transformer_callback;
	params.CallbackUserData			= data;
	params.TransformerInitParamsSize	= 0;
	params.TransformerInitParams_p		= NULL;

	do
	{
		DEBUG_PRINT("initializing transformer %s\n", transformer_names[index]);
		ret = MME_InitTransformer(transformer_names[index], &params, &data->transformer_handle);
	}
	while (ret != MME_SUCCESS && transformer_names[++index] != NULL);

	if (ret != MME_SUCCESS)
	{
		if (ret != MME_DRIVER_NOT_INITIALIZED)
			DEBUG_PRINT("transformer initialisation failed: %s", get_mme_error_string(ret));
		data->transformer_handle = 0;
		return LIBMMEIMG_MISC_ERROR;
	}

	return LIBMMEIMG_SUCCESS;
}

LIBMMEIMG_ERROR mme_deinit_transformer(MMEData *data)
{
	MME_ERROR ret;

	if (!data->transformer_handle)
		return LIBMMEIMG_INVALIDARG;

	DEBUG_PRINT("terminating transformer with handle %d\n", data->transformer_handle);

	ret = MME_TermTransformer(data->transformer_handle);
	if (ret == MME_COMMAND_STILL_EXECUTING)
	{
		mme_abort_transformer(data);
		ret = MME_TermTransformer(data->transformer_handle);
	}

	free_mme_data_buffer(data->transform_command->DataBuffers_p[0]);
	free_mme_command(data->transform_command);

	if (ret)
	{
		DEBUG_PRINT("Couldn't terminate transformer: %s", get_mme_error_string(ret));
		return LIBMMEIMG_MISC_ERROR;
	}

	data->transformer_handle = 0;
	return LIBMMEIMG_SUCCESS;
}

LIBMMEIMG_ERROR create_mme_data_buffer(MME_DataBuffer_t **buf, unsigned int flags, void *dstbuf, unsigned long size)
{
	DEBUG_PRINT("creating databuffer of size = %d, dstbuf = %x\n", size, dstbuf);

	*buf 			     = (MME_DataBuffer_t *)malloc(sizeof(MME_DataBuffer_t));
	if (!*buf)
		return LIBMMEIMG_NOMEM;
	(*buf)->StructSize           = sizeof(MME_DataBuffer_t);
	(*buf)->Flags                = flags;
	(*buf)->StreamNumber         = 0;
	(*buf)->NumberOfScatterPages = 1;
	(*buf)->UserData_p	     = NULL;

	(*buf)->ScatterPages_p  = (MME_ScatterPage_t *)malloc(sizeof(MME_ScatterPage_t));
	if (!(*buf)->ScatterPages_p)
	{
		free(*buf);
		return LIBMMEIMG_NOMEM;
	}
	(*buf)->TotalSize       = size;
	(*buf)->StartOffset     = 0;

	(*buf)->ScatterPages_p[0].Page_p    = dstbuf;
	(*buf)->ScatterPages_p[0].Size      = size;
	(*buf)->ScatterPages_p[0].BytesUsed = 0;
	(*buf)->ScatterPages_p[0].FlagsIn   = 0;
	(*buf)->ScatterPages_p[0].FlagsOut  = 0;
	return LIBMMEIMG_SUCCESS;
}

void free_mme_data_buffer(MME_DataBuffer_t *buf)
{
	if (buf)
	{
		if (buf->ScatterPages_p)
			free(buf->ScatterPages_p);

		free(buf);
	}
}

LIBMMEIMG_ERROR mme_start_transformer(MMEData *data, size_t return_params_size, void *return_params, size_t params_size, void *params, void *out_buffer, unsigned long out_buffer_size)
{
	data->transform_command = (MME_Command_t *) malloc(sizeof(MME_Command_t));
	if (!data->transform_command)
		return LIBMMEIMG_NOMEM;

	data->transform_command->DataBuffers_p = (MME_DataBuffer_t **) malloc(sizeof(MME_DataBuffer_t *));

	data->transform_command->NumberInputBuffers = 0;
	data->transform_command->NumberOutputBuffers = 1;

	if (create_mme_data_buffer(data->transform_command->DataBuffers_p, MME_ALLOCATION_PHYSICAL, out_buffer, out_buffer_size))
	{
		DEBUG_PRINT("error in create_mme_data_buffer");
		return LIBMMEIMG_MISC_ERROR;
	}

	// init the commandstatus
	memset(&(data->transform_command->CmdStatus), 0, sizeof(MME_CommandStatus_t));
	data->transform_command->CmdStatus.AdditionalInfoSize = return_params_size;
	data->transform_command->CmdStatus.AdditionalInfo_p = return_params;

	// set up the command
	data->transform_command->StructSize = sizeof(MME_Command_t);
	data->transform_command->CmdCode    = MME_TRANSFORM;
	data->transform_command->CmdEnd     = MME_COMMAND_END_RETURN_NOTIFY;
	data->transform_command->DueTime    = (MME_Time_t)0;
	data->transform_command->ParamSize  = params_size;
	data->transform_command->Param_p    = params;

	data->transform_command->DataBuffers_p[0]->ScatterPages_p[0].Size = data->transform_command->DataBuffers_p[0]->TotalSize;
	data->transform_command->DataBuffers_p[0]->ScatterPages_p[0].BytesUsed = 0;
	data->transform_command->DataBuffers_p[0]->ScatterPages_p[0].FlagsIn = 0;
	data->transform_command->DataBuffers_p[0]->ScatterPages_p[0].FlagsOut = 0;
	DEBUG_PRINT("start transformer\n");

	/*DEBUG_PRINT("Databuffer dump: StructSize %d, UserData_p 0x%x, Flags %d, StreamNumber %d, NumberOfScatterPages %d, ScatterPages_p 0x%x, ScatterPages_p[0] 0x%x, TotalSize %d, StartOffset %d\n",
	 	data->transform_command->DataBuffers_p[0]->StructSize,
	 	data->transform_command->DataBuffers_p[0]->UserData_p,
	 	data->transform_command->DataBuffers_p[0]->Flags,
	 	data->transform_command->DataBuffers_p[0]->StreamNumber,
	 	data->transform_command->DataBuffers_p[0]->NumberOfScatterPages,
	 	data->transform_command->DataBuffers_p[0]->ScatterPages_p,
	 	data->transform_command->DataBuffers_p[0]->ScatterPages_p[0],
	 	data->transform_command->DataBuffers_p[0]->TotalSize,
	 	data->transform_command->DataBuffers_p[0]->StartOffset);*/

	MME_ERROR mmeRes = MME_SendCommand(data->transformer_handle, data->transform_command);
	if (mmeRes != MME_SUCCESS)
	{
		print_mme_error(mmeRes);
		return LIBMMEIMG_MISC_ERROR;
	}

	return LIBMMEIMG_SUCCESS;
}

LIBMMEIMG_ERROR create_mme_input_command(MME_Command_t **command, MME_DataBuffer_t *data_buffer)
{
	// Allocates a command and populate it
	*command = (MME_Command_t *)malloc(sizeof(MME_Command_t));
	if (!*command)
		return LIBMMEIMG_NOMEM;
	(*command)->NumberInputBuffers  = 1;
	(*command)->NumberOutputBuffers = 0;
	(*command)->StructSize = sizeof(MME_Command_t);
	(*command)->CmdCode = MME_SEND_BUFFERS;
	(*command)->CmdEnd  = MME_COMMAND_END_RETURN_NOTIFY;
	(*command)->DueTime = (MME_Time_t) 0;
	(*command)->DataBuffers_p = (MME_DataBuffer_t **)malloc(sizeof(MME_DataBuffer_t *));
	if (!(*command)->DataBuffers_p)
	{
		free(*command);
		return LIBMMEIMG_NOMEM;
	}

	(*command)->DataBuffers_p[0] = data_buffer;
	(*command)->ParamSize = 0;
	(*command)->Param_p   = NULL;
	memset(&((*command)->CmdStatus), 0, sizeof(MME_CommandStatus_t));
	(*command)->CmdStatus.AdditionalInfoSize = 0;
	(*command)->CmdStatus.AdditionalInfo_p = NULL;
	return LIBMMEIMG_SUCCESS;
}

void free_mme_command(MME_Command_t *command)
{
	if (command)
	{
		if (command->DataBuffers_p)
			free(command->DataBuffers_p);

		free(command);
	}

}

void mme_abort_transformer(MMEData *data)
{
	if (data && data->transform_command)
	{
		DEBUG_PRINT("aborting transform command");
		if (MME_AbortCommand(data->transformer_handle, data->transform_command->CmdStatus.CmdId) != MME_SUCCESS)
			DEBUG_PRINT("abort failed");
	}
}

// data_content needs to be hardware adressable
LIBMMEIMG_ERROR mme_send_data(MMEData *data, char *data_content, unsigned long data_size)
{
	MME_ERROR res = MME_SUCCESS;
	MME_DataBuffer_t *data_buffer;
	MME_Command_t *mme_command;

	sem_init(&data->decode_event, 0, 0);

	DEBUG_PRINT("sending %d bytes of data to coproc", data_size);

	if (create_mme_data_buffer(&data_buffer, MME_ALLOCATION_PHYSICAL, data_content, data_size))
	{
		DEBUG_PRINT("error in create_mme_data_buffer\n");
		return LIBMMEIMG_MISC_ERROR;
	}

	if (create_mme_input_command(&mme_command, data_buffer))
	{
		DEBUG_PRINT("error in create_mme_input_command\n");
		return LIBMMEIMG_MISC_ERROR;
	}


	/*DEBUG_PRINT("Databuffer dump: StructSize %d, UserData_p 0x%x, Flags %d, StreamNumber %d, NumberOfScatterPages %d, ScatterPages_p 0x%x, ScatterPages_p[0] 0x%x, TotalSize %d, StartOffset %d\n",
	 	data_buffer->StructSize,
	 	data_buffer->UserData_p,
	 	data_buffer->Flags,
	 	data_buffer->StreamNumber,
	 	data_buffer->NumberOfScatterPages,
	 	data_buffer->ScatterPages_p,
	 	data_buffer->ScatterPages_p[0],
	 	data_buffer->TotalSize,
	 	data_buffer->StartOffset);*/

	// send the command
	res = MME_SendCommand(data->transformer_handle, mme_command);
	if (res != MME_SUCCESS)
	{
		DEBUG_PRINT("send data command failed : %s, aborting data command", get_mme_error_string(res));
		// abort command
		if (MME_AbortCommand(data->transformer_handle, mme_command->CmdStatus.CmdId) != MME_SUCCESS)
			DEBUG_PRINT("abort failed");
		free_mme_data_buffer(data_buffer);
		free_mme_command(mme_command);
		return MME_INTERNAL_ERROR;
	}

	DEBUG_PRINT("waiting for completion data, \n");

	// wait until the decode is complete
	sem_wait(&data->decode_event);

	free_mme_data_buffer(data_buffer);
	free_mme_command(mme_command);

	return LIBMMEIMG_SUCCESS;
}

// needs modified stmfb to allow blitting on external mem, output is BGR
// memsize needs to be the whole size of srcdestmem
LIBMMEIMG_ERROR blit_decoder_result(char *srcmem, unsigned long srcmem_size, char *destmem, unsigned long destmem_size, unsigned int width, unsigned int height, unsigned int dest_width, unsigned int dest_height, unsigned int removeright, unsigned int removebottom)
{
	int fd;
	STMFBIO_BLT_EXTERN_DATA blt_data;
	int error;

	fd = open("/dev/fb0", O_RDWR);

	DEBUG_PRINT("blitting result");

	if (fd < 0)
	{
		DEBUG_PRINT("cannot access /dev/fb0! err = %d", fd);
		return LIBMMEIMG_NO_DEVACCESS;
	}

	memset(&blt_data, 0, sizeof(STMFBIO_BLT_EXTERN_DATA));
	blt_data.operation  = BLT_OP_COPY;
	blt_data.ulFlags    = 0;
	blt_data.srcOffset  = 0;
	blt_data.srcPitch   = width * 2;
	blt_data.dstOffset  = 0;
	blt_data.dstPitch   = dest_width * 3;
	blt_data.src_top    = 0;
	blt_data.src_left   = 0;
	blt_data.src_right  = width - removeright; // fix macroblock limit
	blt_data.src_bottom = height - removebottom;
	blt_data.dst_left   = 0;
	blt_data.dst_top    = 0;
	blt_data.dst_right  = dest_width;
	blt_data.dst_bottom = dest_height;
	blt_data.srcFormat  = SURF_YCBCR422R;
	blt_data.dstFormat  = SURF_BGR888;
	blt_data.srcMemBase = srcmem;
	blt_data.dstMemBase = destmem;
	blt_data.srcMemSize = srcmem_size;
	blt_data.dstMemSize = destmem_size;

	error = ioctl(fd, STMFBIO_BLT_EXTERN, &blt_data);
	if (error != 0)
	{
		DEBUG_PRINT("blitting failed with error %d", error);
		close(fd);
		return LIBMMEIMG_MISC_ERROR;
	}

	// wait until finished
	ioctl(fd, STMFBIO_SYNC_BLITTER);
	close(fd);

	return LIBMMEIMG_SUCCESS;
}
