#ifndef _HWINTERFACE_H
#define _HWINTERFACE_H
#include "libmmeimg_error.h"
#include <mme.h>
#include <stdio.h>
#include <semaphore.h>
#include <dlfcn.h>
#ifdef STM22		// STM22 compatibility
#include <JPEG_TransformerTypes.h>
#else
#include <JPEG_VideoTransformerTypes.h>
#define JPEGD_TransformParams_t JPEGDEC_TransformParams_t
#define JPEGD_TransformReturnParams_t JPEGDEC_TransformReturnParams_t
#endif

typedef struct
{
	unsigned int		original_width;
	unsigned int		original_height;

	unsigned int		decode_width;
	unsigned int		decode_height;
	MME_TransformerHandle_t	transformer_handle;

	// provide input/output buffer and request transform
	MME_Command_t		*transform_command;

	// wait for coproc to finish decode
	sem_t			decode_event;

	int			decode_success;
} MMEData;

#define ARRAY_SIZE(array) ((int)(sizeof(array) / sizeof((array)[0])))

typedef MME_ERROR(*MME_Init_func)(void);
typedef MME_ERROR(*MME_Term_func)(void);
typedef MME_ERROR(*MME_SendCommand_func)(MME_TransformerHandle_t  Handle,
					 MME_Command_t           *CmdInfo_p);
typedef MME_ERROR(*MME_AbortCommand_func)(MME_TransformerHandle_t Handle,
					  MME_CommandId_t         CmdId);
typedef MME_ERROR(*MME_AllocDataBuffer_func)(MME_TransformerHandle_t  handle,
					     MME_UINT                 size,
					     MME_AllocationFlags_t    flags,
					     MME_DataBuffer_t        **dataBuffer_p);
typedef MME_ERROR(*MME_FreeDataBuffer_func)(MME_DataBuffer_t         *DataBuffer_p);
typedef MME_ERROR(*MME_InitTransformer_func)(const char *Name,
					     MME_TransformerInitParams_t *Params_p,
					     MME_TransformerHandle_t     *Handle_p);
typedef MME_ERROR(*MME_TermTransformer_func)(MME_TransformerHandle_t handle);

MME_ERROR _mme_default_func(void);

extern MME_Init_func _MME_Init;
extern MME_Term_func _MME_Term;
extern MME_SendCommand_func  _MME_SendCommand;
extern MME_AbortCommand_func _MME_AbortCommand;
extern MME_AllocDataBuffer_func _MME_AllocDataBuffer;
extern MME_FreeDataBuffer_func  _MME_FreeDataBuffer;
extern MME_InitTransformer_func _MME_InitTransformer;
extern MME_TermTransformer_func _MME_TermTransformer;
#define MME_Init _MME_Init
#define MME_Term _MME_Term
#define MME_SendCommand  _MME_SendCommand
#define MME_AbortCommand _MME_AbortCommand
#define MME_AllocDataBuffer _MME_AllocDataBuffer
#define MME_FreeDataBuffer _MME_FreeDataBuffer
#define MME_InitTransformer _MME_InitTransformer
#define MME_TermTransformer _MME_TermTransformer

const char *get_mme_event_string(MME_Event_t ev);

const char *get_mme_error_string(MME_ERROR e);

inline void print_mme_error(MME_ERROR e);

// non debug functions

LIBMMEIMG_ERROR mme_init_multi();
LIBMMEIMG_ERROR mme_term_multi();

LIBMMEIMG_ERROR mme_loadlib();

LIBMMEIMG_ERROR mme_init_transformer(MMEData *data, const char *transformer_names[], void (*transformer_callback)(MME_Event_t, MME_Command_t *, void *));

LIBMMEIMG_ERROR mme_deinit_transformer(MMEData *data);

LIBMMEIMG_ERROR create_mme_data_buffer(MME_DataBuffer_t **buf, unsigned int flags, void *dstbuf, unsigned long size);

void free_mme_data_buffer(MME_DataBuffer_t *buf);

LIBMMEIMG_ERROR mme_start_transformer(MMEData *data, size_t return_params_size, void *return_params, size_t params_size, void *params, void *out_buffer, unsigned long out_buffer_size);

LIBMMEIMG_ERROR create_mme_input_command(MME_Command_t **command, MME_DataBuffer_t *data_buffer);

void free_mme_command(MME_Command_t *command);

void mme_abort_transformer(MMEData *data);

LIBMMEIMG_ERROR mme_send_data(MMEData *data, char *data_content, unsigned long data_size);

// needs modified stmfb to allow blitting on external mem, output is BGR
// memsize needs to be the whole size of srcdestmem
LIBMMEIMG_ERROR blit_decoder_result(char *srcmem, unsigned long srcmem_size, char *destmem, unsigned long destmem_size, unsigned int width, unsigned int height, unsigned int dest_width, unsigned int dest_height, unsigned int removeright, unsigned int removebottom);


#endif
