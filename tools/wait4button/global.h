
#ifndef GLOBAL_H_
#define GLOBAL_H_

#ifndef bool
#define bool unsigned char
#define true 1
#define false 0
#endif

#include "map.h"

#define INPUT_PRESS 1
#define INPUT_RELEASE 0

typedef enum {LircdName } eBoxType;
typedef enum {RemoteControl, FrontPanel} eKeyType;

typedef struct Context_s
{
	void * /* RemoteControl_t */  *r; /* instance data */
	int                          fd; /* filedescriptor of fd */

} Context_t;

typedef struct
{
	unsigned int delay;
	unsigned int period;

} tLongKeyPressSupport;

int getInternalCodeLircKeyName(tButton *cButtons, const char cCode[30]);

int printKeyMap(tButton *cButtons);

int checkTuxTxt(const int cCode);

int getEventDevice();

int selectRemote(Context_t  *context, eBoxType type);

#endif
