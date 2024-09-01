#ifndef PNGDECODE_TRANSFORMER_H
#define PNGDECODE_TRANSFORMER_H

#include "mme.h"

#define RLEDECODE_TRANSFORMER_RELEASE "RLEDECODE Firmware RLEDecode_0.0.1"

MME_ERROR RLEDecode_GetTransformerCapability(MME_TransformerCapability_t *capability);
MME_ERROR RLEDecode_InitTransformer(MME_UINT size, MME_GenericParams_t params, void **handle);
MME_ERROR RLEDecode_ProcessCommand(void *handle, MME_Command_t *commandInfo);
MME_ERROR RLEDecode_Abort(void *handle, MME_CommandId_t CmdId);
MME_ERROR RLEDecode_TermTransformer(void *handle);

#endif

