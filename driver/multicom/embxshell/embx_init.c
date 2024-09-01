/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: embx_init.c                                               */
/*                                                                 */
/* Description:                                                    */
/*         EMBX Initialization API entrypoints                     */
/*                                                                 */
/*******************************************************************/

#include "embx.h"
#include "embxP.h"
#include "embx_osinterface.h"

#include "embxshell.h"
#include "debug_ctrl.h"

EMBX_Context_t _embx_dvr_ctx = {EMBX_DVR_UNINITIALIZED,EMBX_FALSE};
EMBX_HandleManager_t _embx_handle_manager = {EMBX_HANDLE_DEFAULT_TABLE_SIZE};

/*
 * Helper function to initialize the driver lock in a thread safe
 * manner, where that is possible.On Linux the lock will already be
 * set up by the module intialization, which is guaranteed to be
 * thread safe anyway. Currently WinCE has a small race condition.
 */
static EMBX_BOOL _initialize_lock(void)
{
    /* initialize the driver lock, in a thread safe manner
       where possible. 
     */
    if(!_embx_dvr_ctx.isLockInitialized)
    {
#if defined(__OS21__)
        EMBX_MUTEX local_lock;
        EMBX_BOOL  success;

        task_lock();

        success = EMBX_OS_MUTEX_INIT(&local_lock);

        if(success)
        {
            if(!_embx_dvr_ctx.isLockInitialized)
            {
                _embx_dvr_ctx.isLockInitialized = success;
                _embx_dvr_ctx.lock              = local_lock;
            }
            else
            {
                EMBX_OS_MUTEX_DESTROY(&local_lock);
            }
        }

        task_unlock();

#else /* !__OS21__ */

#if defined(__OS20__)
        task_lock();
#endif

        _embx_dvr_ctx.isLockInitialized = EMBX_OS_MUTEX_INIT(&_embx_dvr_ctx.lock);

#endif

#if defined(__OS20__)
        task_unlock();
#endif

        if(!_embx_dvr_ctx.isLockInitialized)
        {
            EMBX_DebugMessage(("_initialize_lock 'Failed to initialize mutex'\n"));
            return EMBX_FALSE;
        }
    }

    return EMBX_TRUE;
}


/*
 * Helper function for EMBX_Init and EMBX_RegisterTransport to 
 * create a transport instance and update the transport list. 
 */
static EMBX_BOOL _create_transport(EMBX_TransportFactory_t *pFactory)
{
EMBX_Transport_t     *pTransport;
EMBX_TransportList_t *pTransportList;

    pTransportList = (EMBX_TransportList_t *)EMBX_OS_MemAlloc(sizeof(EMBX_TransportList_t));
    if(pTransportList == 0)
    {
        /* Don't try a debug message here, in case that needs heap
         * to work, the caller will try and free some memory before
         * giving a debug message.
         */
        return EMBX_FALSE;
    }

    pTransport = pFactory->create_fn(pFactory->argument);

    if( pTransport != 0)
    {
        pTransport->parent        = pFactory;
        pTransportList->transport = pTransport;
        pTransportList->next      = _embx_dvr_ctx.transports;

        _embx_dvr_ctx.transports  = pTransportList;
    }
    else
    {
        /* A transport create function returning 0 is NOT an error,
         * it just means that this transport/argument combination
         * is not available on the system, e.g. the hardware isn't
         * there or is unavailable for use.
         */
        EMBX_OS_MemFree(pTransportList);
    }

    return EMBX_TRUE;
}


/*
 * Helper functions allowing transports to take the global driver lock
 * to solve course thread safety problems. These functions should be used
 * with care since on some systems the global driver lock could be heavily
 * contented.
 */
void _EMBX_DriverMutexTake(void)
{
    EMBX_Assert(_embx_dvr_ctx.isLockInitialized);
    EMBX_OS_MUTEX_TAKE(&_embx_dvr_ctx.lock);
}

/*
 * Partner function to the above.
 */
void _EMBX_DriverMutexRelease(void)
{
    EMBX_Assert(_embx_dvr_ctx.isLockInitialized);
    EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);
}


/*
 * Initialise the driver, creating instances of transports from
 * registered transport factories.
 */
EMBX_ERROR EMBX_Init(void)
{
EMBX_ERROR res = EMBX_SYSTEM_ERROR;
EMBX_TransportFactory_t *pCurrentFactory;

    EMBX_Info(EMBX_INFO_INIT, (">>>>EMBX_Init\n"));
    EMBX_DebugOn(EMBX_INFO_INIT);

    if(!_initialize_lock())
    {
        goto exit;
    }

    EMBX_OS_MUTEX_TAKE(&_embx_dvr_ctx.lock);

    if(_embx_dvr_ctx.state != EMBX_DVR_UNINITIALIZED)
    {
        if(_embx_dvr_ctx.state != EMBX_DVR_FAILED)
        {
            res = EMBX_ALREADY_INITIALIZED;
        }
        else
        {
            EMBX_DebugMessage(("The EMBX driver is in a failed state due to a previous error,\nit may not be initialized again in a safe manner\n"));
        }
        goto release_and_exit;
    }

    if(EMBX_HandleManager_Init(&_embx_handle_manager) == EMBX_NOMEM)
    {
        goto release_and_exit;
    }

    pCurrentFactory = _embx_dvr_ctx.transportFactories;

    while(pCurrentFactory)
    {
        if(!_create_transport(pCurrentFactory))
        {
            /* This only happens when the shell cannot allocate its
             * list resources, this doesn't happen if the transport
             * factory was unable to create a transport.
             */
            _embx_dvr_ctx.state = EMBX_DVR_FAILED;
            EMBX_DebugMessage(("EMBX_Init '_create_transport failed due to insufficient memory'\n"));

            goto release_and_exit;
        }

        pCurrentFactory = pCurrentFactory->next;
    }

    _embx_dvr_ctx.state = EMBX_DVR_INITIALIZED;
    res = EMBX_SUCCESS;

release_and_exit:
    EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);

exit:
    EMBX_DebugOff(EMBX_INFO_INIT);
    EMBX_Info(EMBX_INFO_INIT, ("<<<<EMBX_Init = %d\n", res));

    return res;
}



EMBX_ERROR EMBX_RegisterTransport(EMBX_TransportFactory_fn *fn, EMBX_VOID *arg, EMBX_UINT arg_size, EMBX_FACTORY *hFactory)
{
EMBX_ERROR res;
EMBX_TransportFactory_t *pFactory;
unsigned char *pArgCopy;

    EMBX_Info(EMBX_INFO_INIT, (">>>>EMBX_RegisterTransport\n"));
    EMBX_DebugOn(EMBX_INFO_INIT);

    if(!_initialize_lock())
    {
        res = EMBX_SYSTEM_ERROR;
        goto exit;
    }

    if(fn == 0 || arg == 0 || arg_size == 0 || hFactory == 0)
    {
        res = EMBX_INVALID_ARGUMENT;
        goto exit;
    }

    EMBX_OS_MUTEX_TAKE(&_embx_dvr_ctx.lock);


    if(EMBX_HandleManager_Init(&_embx_handle_manager) == EMBX_NOMEM)
    {
        res = EMBX_NOMEM;
        goto release_and_exit;
    }


    pFactory = (EMBX_TransportFactory_t *)EMBX_OS_MemAlloc(sizeof(EMBX_TransportFactory_t));
    if(pFactory == 0)
    {
        /* Try a debug message, but the low memory condition may cause this
         * to fail as well. Unlike in EMBX_Init there is nothing we can sensibly
         * free to help out.
         */ 
        EMBX_DebugMessage(("EMBX_RegisterTransport 'Memory allocation failed'\n"));

        res = EMBX_NOMEM;
        goto release_and_exit;
    }

    pArgCopy = (unsigned char *)EMBX_OS_MemAlloc(arg_size);
    if(pArgCopy == 0)
    {
        EMBX_OS_MemFree(pFactory);

        EMBX_DebugMessage(("EMBX_RegisterTransport 'Memory allocation failed'\n"));

        res = EMBX_NOMEM;
        goto release_and_exit;
    }

    memcpy(pArgCopy, (unsigned char *)arg, arg_size);

    *hFactory = EMBX_HANDLE_CREATE(&_embx_handle_manager, 0, EMBX_HANDLE_TYPE_FACTORY);
    if(*hFactory == EMBX_INVALID_HANDLE_VALUE)
    {
        EMBX_OS_MemFree(pFactory);
        EMBX_OS_MemFree(pArgCopy);

        EMBX_DebugMessage(("Failed 'no more free handles'\n"));

        res = EMBX_NOMEM;
        goto release_and_exit;
    }

    pFactory->create_fn = fn;
    pFactory->argument  = (void *)pArgCopy;
    pFactory->prev = 0;
    pFactory->next = _embx_dvr_ctx.transportFactories;

    if (pFactory->next)
      pFactory->next->prev = pFactory;

    _embx_dvr_ctx.transportFactories = pFactory;

    EMBX_HANDLE_ATTACHOBJ(&_embx_handle_manager, *hFactory, pFactory);

    /* If the driver is already initialized then try and create
     * the transport immediately. This allows Linux based modules,
     * that implement transports, to be loaded after the driver
     * has been loaded and initialized.
     */
    if(EMBX_DVR_INITIALIZED == _embx_dvr_ctx.state)
    {
        if(!_create_transport(pFactory))
        {
            /* This can only happen in a low memory condition where
             * we could not create the transport list entry. There
             * isn't much we can do about it and the factory has been
             * registered at this point so just continue, but try
             * and get a debug message out if we can.
             */
            EMBX_DebugMessage(("EMBX_RegisterTransport '_create_transport failed due to insufficient memory'\n"));
        }
    }

    EMBX_OS_MODULE_REF();

    res = EMBX_SUCCESS;

release_and_exit:
    EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);

exit:
    EMBX_DebugOff(EMBX_INFO_INIT);
    EMBX_Info(EMBX_INFO_INIT, ("<<<<EMBX_RegisterTransport = %d\n", res));

    return res;
}

