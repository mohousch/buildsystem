#ifndef _PTI_HAL_H_
#define _PTI_HAL_H_

#include <linux/module.h>
#include <linux/init.h>

extern int pti_hal_descrambler_set(int session_handle, int descrambler_handle, u8 *Data, int parity);
extern int pti_hal_descrambler_set_aes(int session_handle, int descrambler_handle, u8 *Data, int parity, int data_type);
extern int pti_hal_descrambler_link(int session_handle, int descrambler_handle, int slot_handle);
extern int pti_hal_get_new_slot_handle(int session_handle, int dvb_type,
				       int dvb_pes_type, struct dvb_demux *demux,
				       struct StreamContext_s *DemuxStream,
				       struct DeviceContext_s *DeviceContext);
extern int pti_hal_slot_link_buffer(int session_handle, int slot_handle, int bufType);
extern int pti_hal_slot_free(int session_handle, int slot_handle);
extern int pti_hal_slot_set_pid(int session_handle, int slot_handle, u16 pid);
extern int pti_hal_slot_unlink_buffer(int session_handle, int slot_handle);
extern int pti_hal_slot_clear_pid(int session_handle, int slot_handle);
extern void pti_hal_init(struct stpti *pti , struct dvb_demux *demux, void (*_demultiplex_dvb_packets)(struct dvb_demux *demux, const u8 *buf, int count), int num);
extern int pti_hal_get_new_session_handle(int source, struct dvb_demux *demux);
extern int pti_hal_get_new_descrambler(int session_handle);
extern int pti_hal_set_source(int session_handle, const int source);
extern void paceSwtsByPti(void);

#endif //_PTI_HAL_H_
