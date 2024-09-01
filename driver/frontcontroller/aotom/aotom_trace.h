#ifndef __AOTOM_YWTRACE_H
#define __AOTOM_YWTRACE_H

#define TRACE_TIMESTAMP			0x10000000
#define TRACE_MODLUE_MASK		0x00FFFFFF

enum ywtrace_level_e
{
	TRACE_FATAL	= 0x01,
	TRACE_ERROR	= 0x02,
	TRACE_INFO	= 0x04,
	TRACE_OK	= 0x08,
	TRACE_MASK	= 0x0F
};

int YWTRACE_Init(void);
int YWTRACE_Print(const unsigned int level, const char *format, ...);

#ifdef __TRACE__
#define	ywtrace_init		YWTRACE_Init
#define	ywtrace_print		YWTRACE_Print
#else
#define	ywtrace_init(x...)	do{} while(0)
#define	ywtrace_print(x...)	do{} while(0)
#endif

#define YW_PANEL_DEBUG

#ifdef YW_PANEL_DEBUG
#define PANEL_DEBUG(x)	ywtrace_print(TRACE_ERROR,"%s() error at line: %d in file:%s ^!^\n",__FUNCTION__, __LINE__, __FILE__)
#define PANEL_PRINT(x)	ywtrace_print x
#else
#define PANEL_DEBUG(x)
#define PANEL_PRINT(x)
#endif

#endif
