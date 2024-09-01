
#ifndef __AOTOM_YWDEFS_H__
#define __AOTOM_YWDEFS_H__

#define YW_K                    (1024)
#define YW_M                    (YW_K * YW_K)
#define YW_G                    (YW_K * YW_K * YW_K)
#define YW_ENDIAN_LITTLE

#define YWOS_MODULE_PUBLIC_ID               300
#define YWOS_MODULE_SYSTEMEVT_ID            305
#define YWOS_MODULE_EVT_ID                  310
#define YWOS_MODULE_LIB_ID                  311

#define YWHAL_MODULE_PUBLIC_ID              400
#define YWHAL_MODULE_SYSTEM_ID              410
#define YWHAL_MODULE_PTI_ID                 411
#define YWHAL_MODULE_VIDEO_ID               412
#define YWHAL_MODULE_AUDIO_ID               413
#define YWHAL_MODULE_ENCODER_ID             414
#define YWHAL_MODULE_CANVAS_ID              415
#define YWHAL_MODULE_AVOS_ID                416
#define YWHAL_MODULE_TUNER_ID               417
#define YWHAL_MODULE_FLASH_ID               418
#define YWHAL_MODULE_VFS_ID                 419
#define YWHAL_MODULE_USB_ID                 420
#define YWHAL_MODULE_UART_ID                421
#define YWHAL_MODULE_RTC_ID                 422
#define YWHAL_MODULE_PANEL_ID               423
#define YWHAL_MODULE_PHY_ID                 424
#define YWHAL_MODULE_PCM_ID                 425
#define YWHAL_MODULE_SMART_ID               426
#define YWHAL_MODULE_I2C_ID                 427
#define YWHAL_MODULE_GPIO_ID                428
#define YWHAL_MODULE_HDD_ID                 429
#define YWHAL_MODULE_INJECT_ID              430
#define YWHAL_MODULE_PVR_ID                 431
#define YWHAL_MODULE_TCPIP_ID               432
#define YWHAL_MODULE_PLUGINSWP_ID           433
#define YWHAL_MODULE_CA_ID                  434

/*yw mid */
#define YWMID_MODULE_PUBLIC_ID              500
#define YWMID_MODULE_DBASE_ID               501
#define YWMID_MODULE_EPG_ID                 504
#define YWMID_MODULE_TTX_ID                 505
#define YWMID_MODULE_SUBT_ID                506
#define YWMID_MODULE_TIMER_ID               509
#define YWMID_MODULE_NIM_ID                 511
#define YWMID_MODULE_NVM_ID                 514
#define YWMID_MODULE_CI_ID                  527
#define YWMID_MODULE_TCPIP_ID               528

#define YWMID_MODULE_PRM_ID                 529
#define YWMID_MODULE_CHANNEL_ID             530
#define YWMID_MODULE_UARTMON_ID             531
#define YWMID_MODULE_OTA_ID                 532
#define YWMID_MODULE_CA_ID                  533

#define YWMID_MODULE_FTPC_ID                534
#define YWMID_MODULE_STILL_ID               535
#define YWMID_MODULE_HTTPD_ID               536
#define YWMID_MODULE_ENCODE_ID              537
#define YWMID_MODULE_GUI_ID                 538
#define YWMID_MODULE_HTTPC_ID               539
#define YWMID_MODULE_IMG_ID                 540
#define YWMID_MODULE_ABM_ID                 541
#define YWMID_MODULE_SHOUTCAST_ID           542
#define YWMID_MODULE_YOUTUBE_ID             543
#define YWMID_MODULE_AVCTL_ID               544

/*yw app */
#define YWAPP_MODULE_PUBLIC_ID              600
#define YWAPP_MODULE_INSTALL_ID             611
#define YWAPP_MODULE_SERVICE_ID             612

#define YW_MODULE_SET_ID(a)     (a << 16)
#define YW_MODULE_GET_ID(a)     ((a >> 16) & 0xFFFF)

#define YW_INVALID_HANDLE       0xffffffff

/*Error type*/
#define YW_NO_ERROR             0
#define YW_ERROR_CODE(a,b)      ((b==0)?0:(YW_MODULE_SET_ID(a) +b))

/*Event Type*/
#define YW_EVENT_CODE(a,b)      ((b==0)?0:(YW_MODULE_SET_ID(a) +b))

#ifndef TRUE
#define TRUE (1 == 1)
#endif
#ifndef FALSE
#define FALSE (!TRUE)
#endif

#ifndef NULL
#define NULL 0
#endif

/*mid public error*/
enum
{
	YWMID_ERROR_BAD_PARAMETER  = YW_MODULE_SET_ID(YWMID_MODULE_PUBLIC_ID),    /* Bad parameter passed       */
	YWMID_ERROR_NO_MEMORY,                      /* Memory allocation failed   */
	YWMID_ERROR_ALREADY_INITIALIZED,            /* Device already initialized */
	YWMID_ERROR_NO_INITIALIZED,                 /* Device has not been initialized*/
	YWMID_ERROR_NO_FREE_HANDLES,                /* Cannot open device again   */
	YWMID_ERROR_OPEN_HANDLE,                    /* At least one open handle   */
	YWMID_ERROR_INVALID_HANDLE,                 /* Handle is not valid        */
	YWMID_ERROR_FEATURE_NOT_SUPPORTED           /* Feature unavailable        */
};

/*Hal public error*/
enum
{
	YWHAL_ERROR_BAD_PARAMETER  = YW_MODULE_SET_ID(YWHAL_MODULE_PUBLIC_ID),    /* Bad parameter passed  */
	YWHAL_ERROR_NO_MEMORY,                          /* Memory allocation failed   */
	YWHAL_ERROR_UNKNOWN_DEVICE,                 /* Unknown device */
	YWHAL_ERROR_ALREADY_INITIALIZED,            /* Device already initialized */
	YWHAL_ERROR_NO_INITIALIZED,                 /* Device has not been initialized*/
	YWHAL_ERROR_NO_FREE_HANDLES,                /* Cannot open device again   */
	YWHAL_ERROR_OPEN_HANDLE,                    /* At least one open handle   */
	YWHAL_ERROR_INVALID_HANDLE,                 /* Handle is not valid        */
	YWHAL_ERROR_FEATURE_NOT_SUPPORTED,          /* Feature unavailable        */
	YWHAL_ERROR_TIMEOUT,                        /* Timeout occured            */
	YWHAL_ERROR_DEVICE_BUSY,                    /* Device is currently busy   */
	YWHAL_ERROR_NOT_OPEN,                       /*Device is not open*/
	YWHAL_ERROR_NOT_ENOUGH_DEVICE               /*Device is all opened*/
};

/*variable type*******************************************************/
typedef signed char         S8;
typedef unsigned char       U8;
typedef signed short        S16;
typedef unsigned short      U16;
typedef signed int          S32;
typedef unsigned int        U32;

typedef U32                 BOOL;
typedef U32                 YW_ErrorType_T;
typedef U32                 YW_EventType_T;

#ifdef ARCHITECTURE_ST40
typedef struct U64_s
{
	unsigned int LSW;
	unsigned int MSW;
} U64;

typedef U64 S64;

/*Value=A+B, where A & B is U64 type*/
#define YWI64_Add(A,B,Value)      { register long long T1,T2,Val; \
		T1  = (long long)(A).MSW << 32 | (A).LSW; \
		T2  = (long long)(B).MSW << 32 | (B).LSW; \
		Val = T1 + T2; \
		(Value).MSW = Val >> 32; \
		(Value).LSW = (U32)Val; \
	}

/*Value=A+B, where A is U64 type & B is 32-bit atmost*/
#define YWI64_AddLit(A,B,Value)   { register long long T1,Val; \
		T1 = (long long)(A).MSW << 32 | (A).LSW; \
		Val=T1+(B); \
		(Value).MSW = Val >> 32; \
		(Value).LSW = (U32)Val; \
	}

/*A==B, A & B are U64 type*/
#define YWI64_IsEqual(A,B)                (((A).LSW == (B).LSW) && ((A).MSW == (B).MSW))

#define YWI64_GetValue(Value,Lower,Upper) ((Lower) = (Value).LSW, (Upper) = (Value).MSW)

/*A>=B, A & B are U64 type*/
#define YWI64_IsGreaterOrEqual(A,B)       ( ((A).MSW >  (B).MSW) || \
					    (((A).MSW == (B).MSW) && ((A).LSW >= (B).LSW)))

/*A>B, A & B are U64 type*/
#define YWI64_IsGreaterThan(A,B)          ( ((A).MSW >  (B).MSW) || \
					    (((A).MSW == (B).MSW) && ((A).LSW > (B).LSW)))

/*A<B, A & B are U64 type*/
#define YWI64_IsLessThan(A,B)             ( ((A).MSW <  (B).MSW) || \
					    (((A).MSW == (B).MSW) && ((A).LSW < (B).LSW)))

/*A<=B, A & B are U64 type*/
#define YWI64_IsLessOrEqual(A,B)          ( ((A).MSW <  (B).MSW) || \
					    (((A).MSW == (B).MSW) && ((A).LSW <= (B).LSW)))

#define YWI64_IsNegative(A)               ((A).MSW & 0X80000000)

/*A==0, A is U64 type*/
#define YWI64_IsZero(A)                   (((A).LSW == 0) && ((A).MSW == 0))

/*A!=B, A & B are U64 type*/
#define YWI64_AreNotEqual(A,B)            (((A).LSW != (B).LSW) || ((A).MSW != (B).MSW))

#define YWI64_SetValue(Lower,Upper,Value) ((Value).LSW = (Lower), (Value).MSW = (Upper))

/*Value=A-B, where A & B are U64 type*/
#define YWI64_Sub(A,B,Value)              ((Value).MSW  = (A).MSW - (B).MSW - (((A).LSW < (B).LSW)?1:0), \
					   (Value).LSW  = (A).LSW - (B).LSW)

/*Value=A-B, where A is U64 type & B is 32-bit atmost*/
#define YWI64_SubLit(A,B,Value)           ((Value).MSW  = (A).MSW - (((A).LSW < (B))?1:0), \
					   (Value).LSW  = (A).LSW - (B))
/*Value=A/B, where A ,B is U64 type */
#define YWI64_Div(A,B,Value)           { register long long T1,T2, Val;\
		T1 = (long long)(A).MSW << 32 | (A).LSW;\
		T2 = (long long)(B).MSW << 32 | (B).LSW;\
		Val = T1/T2;\
		(Value).MSW = Val >> 32;\
		(Value).LSW = (U32)Val;\
	}

/*Value=A/B, where A is U64 type & B is 32-bit atmost*/
#define YWI64_DivLit(A,B,Value)           { register long long T1, Val;\
		T1 = (long long)(A).MSW << 32 | (A).LSW;\
		Val = T1/(B);\
		(Value).MSW = Val >> 32;\
		(Value).LSW = (U32)Val;\
	}

/*Value=A%B, where A is U64 type & B is 32-bit atmost*/
#define YWI64_ModLit(A,B,Value)           { register long long T1, Val;\
		T1 = (long long)(A).MSW << 32 | (A).LSW;\
		Val=T1%(B);\
		(Value).MSW = Val >> 32;\
		(Value).LSW = (U32)Val;\
	}

/*Value=A*B, where A & B are U64 type*/
#define YWI64_Mul(A,B,Value)              { register long long T1, T2, Val; \
		T1 = (long long)(A).MSW << 32 | (A).LSW; \
		T2 = (long long)(B).MSW << 32 | (B).LSW; \
		Val=T1*T2; \
		(Value).MSW = Val >> 32; \
		(Value).LSW = (U32)Val; \
	}

/*Value=A*B, where A is U64 type & B is 32-bit atmost*/
#define I64_MulLit(A,B,Value)           { register long long T1,Val; \
		T1 = (long long)(A).MSW << 32 | (A).LSW; \
		Val=T1*(B); \
		(Value).MSW = Val >> 32; \
		(Value).LSW = (U32)Val; \
	}

/*Value=Value<<Shift, where Value is U64 type*/
#define YWI64_ShiftLeft(Shift,Value)      { register long long T1, T2, Val; \
		T1 = (long long)(Value).MSW << 32 | (Value).LSW; \
		Val=T1 << Shift; \
		(Value).MSW = Val >> 32; \
		(Value).LSW = (U32)Val; \
	}

/*Value=Value>>Shift, where Value is U64 type*/
#define YWI64_ShiftRight(Shift,Value)     { register long long T1, T2, Val; \
		T1 = (long long)(Value).MSW << 32 | (Value).LSW; \
		Val=T1 >> Shift; \
		(Value).MSW = Val >> 32; \
		(Value).LSW = (U32)Val; \
	}
#endif /*#ifdef ARCHITECTURE_ST40*/

#define YW_HandleValid(Table, Max, Handle) ((Handle) >= (U32)&(Table)[0] \

#endif /* __AOTOM_YWDEFS_H__ */

