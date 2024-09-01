#ifndef CEC_DEV_H_
#define CEC_DEV_H_

#define CEC_MAJOR			149

#define CEC_FLUSH          0x0
#define CEC_GET_ADDRESS    0x1

typedef struct
{
	unsigned char logical;
	unsigned char physical[2];
	unsigned char type;
} tCECAddressinfo;

typedef struct
{
	unsigned char address;
	unsigned char length;
	unsigned char data[256];
} tCECMessage;

void AddMessageToBuffer(unsigned char *rawmsg, unsigned int len);

int init_dev(void);
int cleanup_dev(void);

#endif
