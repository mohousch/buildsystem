/*
 *   cxa2161.h - CXA2161 audio/video switch driver - Homecast 5101
 *
 *   written by corev - 29. Dec 2009
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*
 * cxa2161 data struct
 *
 */

typedef struct s_cxa2161_data {
  uint8_t audio_gain:2,
          volume_control:5,
          tv_audio_mute:1;
  uint8_t mono_output:1,
          tv_volume_bypass:1,
          tv_mono_switch:3,
          tv_audio_select:2,
          phono_bypass:1;
  uint8_t tv_audio_mute2:1,
          output_limit:1,
          vcr_mono_switch:3,
          vcr_audio_select:2,
          overlay:1;
  uint8_t tv_input_mute:1,
          logic_level:1,
          fnc_level:2,
          fnc_follow:1,
          fnc_dir:1,
          fast_blank:2;
  uint8_t vcr_video_switch:3,
          rgb_gain:2,
          tv_video_switch:3;
  uint8_t vcr_input_mute:1,
          sync_select:2,
          vin5_clamp:1,
          vin7_clamp:1,
          vin3_clamp:1,
          mixer_control:2;
  uint8_t zcd:1,
          bidir_line_control:1,
          enable_vout6:1,
          enable_vout5:1,
          enable_vout4:1,
          enable_vout3:1,
          enable_vout2:1,
          enable_vout1:1;
} s_cxa2161_data;

typedef struct s_cxa2161_status {
  uint8_t reserved1:2,
        zero_cross_status:1,
        pod:1,
        reserved2:1,
        sync_detect:1,
        fnc_vcr:1;
} s_cxa2161_status;

#define CXA2161_DATA_SIZE sizeof(s_cxa2161_data)

#define I2C_ADDRESS_CXA2161					0x48

#define CXA2161_TV_VIDEOSWITCH_DIGITALENCODER_RGB_CVBS		0
#define CXA2161_TV_VIDEOSWITCH_DIGITALENCODER_SVIDEO		1
#define CXA2161_TV_VIDEOSWITCH_VCR_RGB_SVIDEO			2
#define CXA2161_TV_VIDEOSWITCH_TV_CVBS				3
#define CXA2161_TV_VIDEOSWITCH_ENCODER_SVIDEO			4
#define CXA2161_TV_VIDEOSWITCH_ENCODER_RGB_AUX_CVBS		5
#define CXA2161_TV_VIDEOSWITCH_AUX_SVIDEO_CVBS			6
#define CXA2161_TV_VIDEOSWITCH_MUTE 				7

#define CXA2161_VCR_VIDEOSWITCH_DIGITALENCODER_SVIDEO		0
#define CXA2161_VCR_VIDEOSWITCH_DIGITALENCODER_SVIDEO_CVBS	1
#define CXA2161_VCR_VIDEOSWITCH_VCR_SVIDEO			2
#define CXA2161_VCR_VIDEOSWITCH_TV_CVBS				3
#define CXA2161_VCR_VIDEOSWITCH_ENCODER_SVIDEO			4
#define CXA2161_VCR_VIDEOSWITCH_AUX_CVBS			5
#define CXA2161_VCR_VIDEOSWITCH_AUX_SVIDEO_CVBS			6
#define CXA2161_VCR_VIDEOSWITCH_MUTE				7

#define CXA2161_RGBGAIN_0					0
#define CXA2161_RGBGAIN_1					1
#define CXA2161_RGBGAIN_2					2
#define CXA2161_RGBGAIN_3					3

#define CXA2161_MIXERCONTROL_NOMIX_74				0
#define CXA2161_MIXERCONTROL_MIX_43				1
#define CXA2161_MIXERCONTROL_NOMIX_78				2
#define CXA2161_MIXERCONTROL_DEFAULT				3

#define CXA2161_CLAMP_DEFAULT					0
#define CXA2161_CLAMP_OTHER					1

#define CXA2161_SYNCSELECT_VIN8					0
#define CXA2161_SYNCSELECT_VIN9					1
#define CXA2161_SYNCSELECT_VIN10				2
#define CXA2161_SYNCSELECT_VIN12				3

#define CXA2161_OUTPUT_DISABLED					0
#define CXA2161_OUTPUT_ENABLED					1

#define CXA2161_BIDIRLINE_ACTIVE				0
#define CXA2161_BIDIRLINE_OFF					1

#define CXA2161_AUDIOSELECT_LRIN1				0
#define CXA2161_AUDIOSELECT_LRIN2				1
#define CXA2161_AUDIOSELECT_LRIN3				2
#define CXA2161_AUDIOSELECT_LRIN4				3

#define CXA2161_MONOSWITCH_NORMAL				0
#define CXA2161_MONOSWITCH_MONOMIX				1
#define CXA2161_MONOSWITCH_SWAP					2
#define CXA2161_MONOSWITCH_RIGHTONLY				3
#define CXA2161_MONOSWITCH_LEFTONLY				4
#define CXA2161_MONOSWITCH_NORMAL2				5
#define CXA2161_MONOSWITCH_NORMAL3				6
#define CXA2161_MONOSWITCH_DEFAULT				7

#define CXA2161_VOLUMEBYPASS_OFF				0
#define CXA2161_VOLUMEBYPASS_ON					1

#define CXA2161_MONOOUTPUT_TV					0
#define CXA2161_MONOOUTPUT_LRIN1				1

#define CXA2161_VOLUMECONTROL_DEFAULT				3

#define CXA2161_AUDIOGAIN_N6DB					0
#define CXA2161_AUDIOGAIN_N3DB					1
#define CXA2161_AUDIOGAIN_P0DB					2
#define CXA2161_AUDIOGAIN_P3DB					3
#define CXA2161_AUDIOGAIN_DEFAULT				CXA2161_AUDIOGAIN_N6DB

#define CXA2161_OVERLAY_OFF					0
#define CXA2161_OVERLAY_ON					1

#define CXA2161_AUDIO_MUTED					0
#define CXA2161_AUDIO_ACTIVE					1

#define CXA2161_ZEROCROSS_OFF					0
#define CXA2161_ZEROCROSS_ON					1

#define CXA2161_INPUT_MUTE					0
#define CXA2161_INPUT_ACTIVE					1

#define CXA2161_OUTPUTLIMIT_OFF					0
#define CXA2161_OUTPUTLIMIT_ON					1

#define CXA2161_FASTBLANK_OFF					0
#define CXA2161_FASTBLANK_EXTERNAL1				1
#define CXA2161_FASTBLANK_EXTERNAL2				2
#define CXA2161_FASTBLANK_ON					3

#define CXA2161_FNCFOLLOW_OFF					0
#define CXA2161_FNCFOLLOW_ON					1

#define CXA2161_FNCDIR_INOUT					0
#define CXA2161_FNCDIR_OUT					1

#define CXA2161_FNCLEVEL_INTERNAL				0
#define CXA2161_FNCLEVEL_WSS_16_9				1
#define CXA2161_FNCLEVEL_INTERNAL2				2
#define CXA2161_FNCLEVEL_WSS_4_3				3

#define CXA2161_LOGICLEVEL_CURRENTSINK				0
#define CXA2161_LOGICLEVEL_OPENCOLLECTOR			1

#define CXA2161_FNCVCR_INTERNAL					0
#define CXA2161_FNCVCR_WSS_16_9					1
#define CXA2161_FNCVCR_WSS_4_3					3

#ifdef __KERNEL__
int cxa2161_init(struct i2c_client *client);
int cxa2161_command(struct i2c_client *client, unsigned int cmd, void *arg);
int cxa2161_command_kernel(struct i2c_client *client, unsigned int cmd, void *arg);
#endif

