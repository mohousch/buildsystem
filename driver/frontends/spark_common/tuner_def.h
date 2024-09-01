

#ifndef __TUNER_DEF_H__
#define __TUNER_DEF_H__

/* STAPI Driver ID */
#define TUNER_DRIVER_ID 6

/* TUNER specific Error code base value */
#define TUNER_DRIVER_BASE (TUNER_DRIVER_ID << 16)

typedef char TUNER_DeviceName_t[16];
typedef U32 TUNER_CallBackEvent_T;

//#define SEM_LOCK(n) YWOS_SemaphoreWait( n ,YWOS_WAIT_INFINITY)//semaphore_wait(n)
//#define SEM_UNLOCK(n) YWOS_SemaphoreSend( n,YWOS_WAIT_INFINITY )//semaphore_signal(n)
//#define YWOS_TaskSleep(msec) YWOS_TaskSleep(msec)

YWTUNER_Handle_T YWTUNERi_GetTunerHandle(U32 index);

S32 TUNER_Util_PowOf2(S32 number);
S32 TUNER_Util_LongSqrt(S32 Value);
S32 TUNER_Util_BinaryFloatDiv(S32 n1, S32 n2, S32 precision);

BOOL TUNER_WaitTime(U32 Index, S32 TimeOut);
void *TUNER_MemAlloc(S32 size);
void TUNER_MemFree(void *p);

#endif
