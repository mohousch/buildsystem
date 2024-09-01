#ifndef _TFFP_CONFIG_H_
#define _TFFP_CONFIG_H_

typedef struct
{
	u32 crc: 16;		// bytes 0+1
	u32 version: 8;	// byte 2
	u32 brightness: 3;	// byte 3
	u32 irFilter1: 1;
	u32 irFilter2: 1;
	u32 irFilter3: 1;
	u32 irFilter4: 1;
	u32 allCaps: 1;
	u32 typematicDelay: 8;	// byte 4
	u32 typematicRate: 8;	// byte 5
	u32 scrollMode: 8;	// byte 6
	u32 scrollPause: 8;	// byte 7
	u32 scrollDelay: 8;	// byte 8
	s32 gmtOffset: 16;	// bytes 9+10
	//--------------- for future enhancements: settings version 1 ends here
} tTffpConfig;

int readTffpConfig(tTffpConfig *pCfg);

int writeTffpConfig(tTffpConfig *pCfg);

#endif
