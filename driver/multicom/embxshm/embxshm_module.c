/**************************************************************/
/* Copyright STMicroelectronics 2002, all rights reserved     */
/*                                                            */
/* File: embxshm_module.c                                     */
/*                                                            */
/* Description:                                               */
/*         EMBX shared memory transport                       */
/*         Linux kernel module functionality                  */
/*                                                            */
/**************************************************************/

#include "embx.h"
#include "embxshm.h"
#include "embxshmP.h"

#include "embxmailbox.h"

#if defined(__LINUX__) && defined(__KERNEL__) && defined(MODULE)

/* module information */

MODULE_DESCRIPTION("Extended EMBX Shared Memory Transport");
MODULE_AUTHOR("STMicroelectronics Ltd.");
#ifdef MULTICOM_GPL
MODULE_LICENSE("GPL");
#else
MODULE_LICENSE("Copyright (C) STMicroelectronics Limited 2002-2003. All rights reserved.");
#endif /* MULTICOM_GPL */

#ifdef EMBXSHM_CACHED_HEAP
EXPORT_SYMBOL( EMBXSHMC_mailbox_factory );
EXPORT_SYMBOL( EMBXSHMC_empi_mailbox_factory );
#else
EXPORT_SYMBOL( EMBXSHM_mailbox_factory );
EXPORT_SYMBOL( EMBXSHM_empi_mailbox_factory );
#endif

/* configurable parameters */

static char *     mailbox0 = NULL;
module_param     (mailbox0, charp, S_IRUGO);
MODULE_PARM_DESC (mailbox0, "Configuration string of the form "
                            "name:id:participants:warp:maxports:maxobjects:freelist:shared:size[:warp:size][:warp2:size2]");

static char *     mailbox1 = NULL;
module_param     (mailbox1, charp, S_IRUGO);
MODULE_PARM_DESC (mailbox1, "Configuration string of the form "
                            "name:id:participants:warp:maxports:maxobjects:freelist:shared:size[:warp:size][:warp2:size2]");

static char *     mailbox2 = NULL;
module_param     (mailbox2, charp, S_IRUGO);
MODULE_PARM_DESC (mailbox2, "Configuration string of the form "
                            "name:id:participants:warp:maxports:maxobjects:freelist:shared:size[:warp:size][:warp2:size2]");

static char *     mailbox3 = NULL;
module_param     (mailbox3, charp, S_IRUGO);
MODULE_PARM_DESC (mailbox3, "Configuration string of the form "
                            "name:id:participants:warp:maxports:maxobjects:freelist:shared:size[:warp:size][:warp2:size2]");

static char *     embxmailbox0 = NULL;
module_param     (embxmailbox0, charp, S_IRUGO);
MODULE_PARM_DESC (embxmailbox0, "Configuration string of the form "
                            "name:id:participants:warp:maxports:maxobjects:freelist:shared:size[:warp:size][:warp2:size2]");

static char *     embxmailbox1 = NULL;
module_param     (embxmailbox1, charp, S_IRUGO);
MODULE_PARM_DESC (embxmailbox1, "Configuration string of the form "
                            "name:id:participants:warp:maxports:maxobjects:freelist:shared:size[:warp:size][:warp2:size2]");

static char *     embxmailbox2 = NULL;
module_param     (embxmailbox2, charp, S_IRUGO);
MODULE_PARM_DESC (embxmailbox2, "Configuration string of the form "
                            "name:id:participants:warp:maxports:maxobjects:freelist:shared:size[:warp:size][:warp2:size2]");

static char *     embxmailbox3 = NULL;
module_param     (embxmailbox3, charp, S_IRUGO);
MODULE_PARM_DESC (embxmailbox3, "Configuration string of the form "
                            "name:id:participants:warp:maxports:maxobjects:freelist:shared:size[:warp:size][:warp2:size2]");

static char *     empi0 = NULL;
module_param     (empi0, charp, S_IRUGO);
MODULE_PARM_DESC (empi0, "Configuration string of the form "
                         "empiaddr:mailboxaddr:sysconfaddr:sysconfset:sysconfclear");

static char *     empi1 = NULL;
module_param     (empi1, charp, S_IRUGO);
MODULE_PARM_DESC (empi1, "Configuration string of the form "
                         "empiaddr:mailboxaddr:sysconfaddr:sysconfset:sysconfclear");

static char *     empi2 = NULL;
module_param     (empi2, charp, S_IRUGO);
MODULE_PARM_DESC (empi2, "Configuration string of the form "
                         "empiaddr:mailboxaddr:sysconfaddr:sysconfset:sysconfclear");

static char *     empi3 = NULL;
module_param     (empi3, charp, S_IRUGO);
MODULE_PARM_DESC (empi3, "Configuration string of the form "
                         "empiaddr:mailboxaddr:sysconfaddr:sysconfset:sysconfclear");

/* list of work required to deinitialize the transport */
#define MAX_FACTORIES 4
static EMBX_UINT numRegisteredFactories = 0;
static EMBX_FACTORY registeredFactories[MAX_FACTORIES];

static int splitString(char *str, char c, int n, const char **split)
{
	int i;
	char *p;

	EMBX_Info(EMBX_INFO_FACTORY, (">>>splitString(\"%s\", '%c', %d, ...)\n", str, c, n));
	EMBX_Assert(n > 0);

	split[0] = str;
	for (p = strchr(str, c), i=1; p && i<n; p = strchr(p, c), i++) 
	{
		*p++ = '\0';
		split[i] = p;
	}

	EMBX_Info(EMBX_INFO_FACTORY, ("<<<splitString() = %d\n", i));
	return i;
}

static int parseNumber(const char *str, EMBX_UINT *num)
{
	int   base;
	char *endp;

	EMBX_Info(EMBX_INFO_FACTORY, (">>>parseNumber(\"%s\", ...)\n", str));

	/* lazy evaulation will prevent us illegally accessing str[1] */
	base = (str[0] == '0' && str[1] == 'x' ? str += 2, 16 : 10);
	*num = simple_strtoul(str, &endp, base);

	EMBX_Info(EMBX_INFO_FACTORY, ("<<<parseNumber(..., %d) = %s\n", *num,
	                             ('\0' != *str && '\0' == *endp ? "0" : "-EINVAL")));
	return ('\0' != *str && '\0' == *endp ? 0 : -EINVAL);
}

static int parseMailboxString(char *str, EMBXSHM_MailboxConfig_t *mailbox)
{
	const char zero[] = "0";
	const char *field[14];
	int err;
	int i;
	EMBX_UINT participants;

	EMBX_Info(EMBX_INFO_FACTORY, (">>>parseMailboxString(\"%s\", ...)\n", str));
	EMBX_Assert(str && mailbox);

	i = splitString(str, ':', ARRAY_SIZE(field), field);
	if (9 == i)
	{
		/* No Primary Warp range */
		field[9] = zero;
		field[10] = zero;

		/* MULTICOM_32BIT_SUPPORT: No Secondary Warp range */
		field[11] = zero;
		field[12] = zero;
	}
	else if (11 == i)
	{
		/* MULTICOM_32BIT_SUPPORT: No Secondary Warp range */
	  	field[11] = zero;
		field[12] = zero;
	}
	else if (13 != i) 
	{
		EMBX_Info(EMBX_INFO_FACTORY, ("<<<parseMailboxString() = -EINVAL:1\n"));
		return -EINVAL;
	}

	strncpy(mailbox->name, field[0], sizeof(mailbox->name));
	err  = parseNumber(field[ 1], &(mailbox->cpuID));
	err += parseNumber(field[ 2], &participants);
	err += parseNumber(field[ 3], &(mailbox->pointerWarp));
	err += parseNumber(field[ 4], &(mailbox->maxPorts));
	err += parseNumber(field[ 5], &(mailbox->maxObjects));
	err += parseNumber(field[ 6], &(mailbox->freeListSize));
	err += parseNumber(field[ 7], (EMBX_UINT *) &(mailbox->sharedAddr));
	err += parseNumber(field[ 8], &(mailbox->sharedSize));
	err += parseNumber(field[ 9], (EMBX_UINT *) &(mailbox->warpRangeAddr));
	err += parseNumber(field[10], &(mailbox->warpRangeSize));

	/* MULTICOM_32BIT_SUPPORT: Secondary Warp range support */
	err += parseNumber(field[11], (EMBX_UINT *) &(mailbox->warpRangeAddr2));
	err += parseNumber(field[12], &(mailbox->warpRangeSize2));

	if (0 != err) 
	{
		EMBX_Info(EMBX_INFO_FACTORY, ("<<<parseMailboxString() = -EINVAL:2\n"));
		return -EINVAL;
	}

	/* we have successfully parsed the string at this point all that
	 * remains is to sanity check the participants map
	 */
	for (i=0; i<EMBXSHM_MAX_CPUS; i++) {
		mailbox->participants[i] = ((1 << i) & participants ? 1 : 0);
	}

	/* convert the shared size from bytes to kilobytes */
	mailbox->sharedSize *= 1024;

	EMBX_Info(EMBX_INFO_FACTORY, ("<<<parseMailboxString shared addr 0x%08x\n", (unsigned)mailbox->sharedAddr));
	EMBX_Info(EMBX_INFO_FACTORY, ("<<<parseMailboxString shared size 0x%08x\n", mailbox->sharedSize));
	EMBX_Info(EMBX_INFO_FACTORY, ("<<<parseMailboxString warp addr 0x%08x\n", (unsigned)mailbox->warpRangeAddr));
	EMBX_Info(EMBX_INFO_FACTORY, ("<<<parseMailboxString warp size 0x%08x\n", mailbox->warpRangeSize));
	EMBX_Info(EMBX_INFO_FACTORY, ("<<<parseMailboxString warp addr 2 0x%08x\n", (unsigned)mailbox->warpRangeAddr2));
	EMBX_Info(EMBX_INFO_FACTORY, ("<<<parseMailboxString warp size 2 0x%08x\n", mailbox->warpRangeSize2));


	EMBX_Info(EMBX_INFO_FACTORY, ("<<<parseMailboxString() = 0\n"));
	return 0;
}

static int parseEmpiString(char *str, EMBXSHM_EMPIMailboxConfig_t *empi)
{
	int err;
	const char *field[6];

	EMBX_Info(EMBX_INFO_FACTORY, (">>>parseEmpiString(\"%s\", ...)\n", str));
	EMBX_Assert(str && empi);

	if (5 != splitString(str, ':', 6, field))
	{
		EMBX_Info(EMBX_INFO_FACTORY, ("<<<parseEmpiString() = -EINVAL:1\n"));
		return -EINVAL;
	}

	err  = parseNumber(field[0], (EMBX_UINT *) &(empi->empiAddr));
	err += parseNumber(field[1], (EMBX_UINT *) &(empi->mailboxAddr));
	err += parseNumber(field[2], (EMBX_UINT *) &(empi->sysconfAddr));
	err += parseNumber(field[3], &(empi->sysconfMaskSet));
	err += parseNumber(field[4], &(empi->sysconfMaskClear));

	if (0 != err) 
	{
		EMBX_Info(EMBX_INFO_FACTORY, ("<<<parseEmpiString() = -EINVAL:2\n"));
		return -EINVAL;
	}

	EMBX_Info(EMBX_INFO_FACTORY, ("<<<parseEmpiString() = 0\n"));
	return 0;
}

int __init embxshm_init( void )
{
    int err;
    EMBX_ERROR res;
    EMBXSHM_EMPIMailboxConfig_t config[4];

    /* Allow for alternative embxmailbox parameter naming */
    mailbox0 = embxmailbox0 ? embxmailbox0 : mailbox0;
    mailbox1 = embxmailbox1 ? embxmailbox1 : mailbox1;
    mailbox2 = embxmailbox2 ? embxmailbox2 : mailbox2;
    mailbox3 = embxmailbox3 ? embxmailbox3 : mailbox3;

    /* check for mismatched structures */
    if ((empi0 && !mailbox0) || (empi1 && !mailbox1) ||
	(empi2 && !mailbox2) || (empi3 && !mailbox3))
    {
	return -EINVAL;
    }

    memset(config, 0, sizeof(config));

    err  = (NULL == mailbox0 ? 0 : parseMailboxString(mailbox0, &(config[0].mailbox)));
    err += (NULL == mailbox1 ? 0 : parseMailboxString(mailbox1, &(config[1].mailbox)));
    err += (NULL == mailbox2 ? 0 : parseMailboxString(mailbox2, &(config[2].mailbox)));
    err += (NULL == mailbox3 ? 0 : parseMailboxString(mailbox3, &(config[3].mailbox)));
    err += (NULL == empi0    ? 0 : parseEmpiString(empi0, config+0));
    err += (NULL == empi1    ? 0 : parseEmpiString(empi1, config+1));
    err += (NULL == empi2    ? 0 : parseEmpiString(empi2, config+2));
    err += (NULL == empi3    ? 0 : parseEmpiString(empi3, config+3));

    if (0 != err)
    {
	return -EINVAL;
    }

    if (mailbox0) 
    {
#ifdef EMBXSHM_CACHED_HEAP
	res = EMBX_RegisterTransport(empi0 ? EMBXSHMC_empi_mailbox_factory : EMBXSHMC_mailbox_factory,
				     config+0, sizeof(config[0]),
				     &(registeredFactories[numRegisteredFactories]));
#else
	res = EMBX_RegisterTransport(empi0 ? EMBXSHM_empi_mailbox_factory : EMBXSHM_mailbox_factory,
				     config+0, sizeof(config[0]),
				     &(registeredFactories[numRegisteredFactories]));
#endif
	numRegisteredFactories += (EMBX_SUCCESS == res ? 1 : 0);
    }

    if (mailbox1)
    {
#ifdef EMBXSHM_CACHED_HEAP
	res = EMBX_RegisterTransport(empi1 ? EMBXSHMC_empi_mailbox_factory : EMBXSHMC_mailbox_factory,
				     config+1, sizeof(config[1]),
				     &(registeredFactories[numRegisteredFactories]));
#else
	res = EMBX_RegisterTransport(empi1 ? EMBXSHM_empi_mailbox_factory : EMBXSHM_mailbox_factory,
				     config+1, sizeof(config[1]),
				     &(registeredFactories[numRegisteredFactories]));
#endif
	numRegisteredFactories += (EMBX_SUCCESS == res ? 1 : 0);
    }

    if (mailbox2)
    {
#ifdef EMBXSHM_CACHED_HEAP
	res = EMBX_RegisterTransport(empi2 ? EMBXSHMC_empi_mailbox_factory : EMBXSHMC_mailbox_factory,
				     config+2, sizeof(config[2]),
				     &(registeredFactories[numRegisteredFactories]));
#else
	res = EMBX_RegisterTransport(empi2 ? EMBXSHM_empi_mailbox_factory : EMBXSHM_mailbox_factory,
				     config+2, sizeof(config[2]),
				     &(registeredFactories[numRegisteredFactories]));
#endif
	numRegisteredFactories += (EMBX_SUCCESS == res ? 1 : 0);
    }

    if (mailbox3)
    {
#ifdef EMBXSHM_CACHED_HEAP
	res = EMBX_RegisterTransport(empi3 ? EMBXSHMC_empi_mailbox_factory : EMBXSHMC_mailbox_factory,
				     config+3, sizeof(config[3]),
				     &(registeredFactories[numRegisteredFactories]));
#else
	res = EMBX_RegisterTransport(empi3 ? EMBXSHM_empi_mailbox_factory : EMBXSHM_mailbox_factory,
				     config+3, sizeof(config[3]),
				     &(registeredFactories[numRegisteredFactories]));
#endif
	numRegisteredFactories += (EMBX_SUCCESS == res ? 1 : 0);
    }

    return 0;
}

void __exit embxshm_deinit( void )
{
    while (0 != numRegisteredFactories)
    {
	(void) EMBX_UnregisterTransport(registeredFactories[--numRegisteredFactories]);
    }
    return;
}

module_init(embxshm_init);
module_exit(embxshm_deinit);

#endif /* __LINUX__ */

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 */
