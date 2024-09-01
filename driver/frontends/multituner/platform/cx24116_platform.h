#ifndef cx24116_platform_123
#define cx24116_platform_123

struct cx24116_private_data_s
{
	u32  useUnknown; /* use unknwon commands from new fw */
	u32  usedLNB;
	char *fw_name;
	u32  fastDelay;
};
#endif
// vim:ts=4