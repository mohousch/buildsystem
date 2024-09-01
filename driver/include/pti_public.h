#ifndef pti_public_
#define pti_public_

/* public pti header */

#if defined(UFS910) || defined(ADB_BOX)
	/*ufs910 has a memory problem causing artefacts while watching HDTV channels
	so we need to reduce the number of descramblers,
	maybe other boxes have the same problems*/
	#define NUMBER_OF_DESCRAMBLERS 4
#else
	/*quack: 6 is the max for HDBOX maybe other values have to be chosen for other boxes*/
	#define NUMBER_OF_DESCRAMBLERS 6
#endif

struct PtiSession
{
	short int	pidtable[32];
	short int	descramblerForPid[8192];
	short int	references[32];

	short int	type[32];
	short int	pes_type[32];
	short int	num_pids;

	int		slots[32];

	int		session;
	int		descrambler;
	int		descramblers[NUMBER_OF_DESCRAMBLERS];
	int		descramblerindex[32];
	int		source;
};

typedef enum
{
	VID_BUFFER = 222,
	AUD_BUFFER,
	MISC_BUFFER
} BUFFER_TYPE;

/* source */
typedef enum
{
	TSIN0 = 0,
	TSIN1,
	TSIN2,
#if defined(SAGEMCOM88) || defined(SPARK7162)
// 4-TS and 3-SWTS in STi7105
	TSIN3,
	SWTS0,
	SWTS1,
	SWTS2,
#else
	SWTS0,
#endif
	TS_NOTAGS = 0x80
} tInputSource;

struct stpti
{
	size_t		InterruptDMABufferSize;
	dma_addr_t	InterruptDMABufferInfo;
	void		*InterruptBufferStart_p;

	/* spinlock for the interrupt handler */
	spinlock_t	irq_lock;

	/* gemappter speicherbereich; TCDevice_t* */
	unsigned int	pti_io;

	struct dvb_device* ca_device;

	/* work queue for polling the DMA (if configured) */
	wait_queue_head_t queue;
};



#endif
