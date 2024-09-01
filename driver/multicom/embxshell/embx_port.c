/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: embx_port.c                                               */
/*                                                                 */
/* Description:                                                    */
/*         EMBX Port API entrypoints                               */
/*                                                                 */
/*******************************************************************/

#include "embx.h"
#include "embxP.h"
#include "embx_osinterface.h"

#include "debug_ctrl.h"
#include "embxshell.h"

#ifdef EMBX_VERBOSE
static char *_driver_uninit_error = "Failed 'EMBX Driver is not initialized'\n";
#endif /* EMBX_VERBOSE */


static EMBX_ERROR createport_validate_args(EMBX_TRANSPORT htp, const EMBX_CHAR *portName, EMBX_PORT *port)
{
    if(!_embx_dvr_ctx.isLockInitialized)
    {
        EMBX_DebugMessage((_driver_uninit_error));
        return EMBX_INVALID_TRANSPORT;
    }

    if(port == 0 || portName == 0)
    {
        return EMBX_INVALID_ARGUMENT;
    }

    if(!EMBX_HANDLE_ISTYPEOF(htp, EMBX_HANDLE_TYPE_TRANSPORT))
    {
        EMBX_DebugMessage(("Failed 'transport handle parameter is not a transport handle'\n"));
        return EMBX_INVALID_TRANSPORT;
    }

    if(strlen(portName) > EMBX_MAX_PORT_NAME)
    {
        return EMBX_INVALID_ARGUMENT;
    }

    return EMBX_SUCCESS;
}



static EMBX_ERROR createport_allocate_resources(EMBX_LocalPortList_t **node, EMBX_PORT *port)
{
    /*
     * Try and allocate a port list node
     */
    *node = (EMBX_LocalPortList_t *)EMBX_OS_MemAlloc(sizeof(EMBX_LocalPortList_t));
    if(*node != 0)
    {
        /*
         * Try and get a new public HANDLE.
         */
        *port = EMBX_HANDLE_CREATE(&_embx_handle_manager, 0, EMBX_HANDLE_TYPE_LOCALPORT);
        if(*port != EMBX_INVALID_HANDLE_VALUE)
        {
            return EMBX_SUCCESS;
        }
        else
        {
            EMBX_DebugMessage(("Failed 'no more free handles'\n"));

            EMBX_OS_MemFree(*node);
            *node = 0;
        }
    }

    return EMBX_NOMEM;
}



#ifdef EMBX_RECEIVE_CALLBACK
static EMBX_ERROR do_createport(EMBX_TRANSPORT htp, const EMBX_CHAR *portName, EMBX_PORT *port,
				EMBX_Callback_t callback, EMBX_HANDLE handle)
#else
static EMBX_ERROR do_createport(EMBX_TRANSPORT htp, const EMBX_CHAR *portName, EMBX_PORT *port)
#endif /* EMBX_RECEIVE_CALLBACK */
{
EMBX_ERROR res = EMBX_INVALID_TRANSPORT;
EMBX_TransportHandle_t *tph;

    tph = (EMBX_TransportHandle_t *)EMBX_HANDLE_GETOBJ(&_embx_handle_manager, htp);
    if( (tph != 0) && (tph->state == EMBX_HANDLE_VALID) )
    {
    EMBX_LocalPortHandle_t *ph;
    EMBX_LocalPortList_t   *node;

        if(tph->transport->transportInfo.maxPorts > 0 &&
           tph->transport->transportInfo.nrPortsInUse == tph->transport->transportInfo.maxPorts)
        {
            EMBX_DebugMessage(("Failed 'reached maximum port count'\n"));
            return EMBX_NOMEM;
        }

        res = createport_allocate_resources(&node, port);
        if(res != EMBX_SUCCESS)
        {
            return res;
        }

#ifdef EMBX_RECEIVE_CALLBACK
        res = tph->methods->create_port(tph, portName, &ph, callback, handle);
#else
        res = tph->methods->create_port(tph, portName, &ph);
#endif /* EMBX_RECEIVE_CALLBACK */
        if(res != EMBX_SUCCESS)
        {
            EMBX_OS_MemFree(node);
            EMBX_HANDLE_FREE(&_embx_handle_manager, *port);
            *port = EMBX_INVALID_HANDLE_VALUE;
        }
        else
        {
            /* Attach the handle structure to the public HANDLE */
            EMBX_HANDLE_ATTACHOBJ(&_embx_handle_manager, *port, ph);

            /* Add to the transport handle's local port list */
            node->port = ph;
            node->prev = 0;
            node->next = tph->localPorts;

            if(node->next != 0)
                node->next->prev = node;

            tph->localPorts = node;

            tph->transport->transportInfo.nrPortsInUse++;

            /* Finally fill in name and transport handle then
             * mark the handle structure state as valid
             */
            strcpy(ph->portName, portName);
            ph->transportHandle = tph;
            ph->state = EMBX_HANDLE_VALID;

            EMBX_Info(EMBX_INFO_PORT, ("Created port, structure address = 0x%08lx HANDLE = 0x%08lx\n",(unsigned long)ph,(unsigned long)*port));
        }
    }

    return res;
}



EMBX_ERROR EMBX_CreatePort(EMBX_TRANSPORT htp, const EMBX_CHAR *portName, EMBX_PORT *port)
{
EMBX_ERROR res;

    EMBX_Info(EMBX_INFO_PORT, (">>>>EMBX_CreatePort(htp=0x%lx,%s)\n",(unsigned long)htp,(portName==0?"(NULL)":portName)));
    EMBX_DebugOn(EMBX_INFO_PORT);

    res = createport_validate_args(htp, portName, port);
    if(res != EMBX_SUCCESS)
    {
        goto exit;
    }


    EMBX_OS_MUTEX_TAKE(&_embx_dvr_ctx.lock);

    if(_embx_dvr_ctx.state != EMBX_DVR_INITIALIZED)
    {
        EMBX_DebugMessage((_driver_uninit_error));
        res = EMBX_INVALID_TRANSPORT;
    }
    else
    {
#ifdef EMBX_RECEIVE_CALLBACK
	res = do_createport(htp, portName, port, NULL, 0);
#else
	res = do_createport(htp, portName, port);
#endif /* EMBX_RECEIVE_CALLBACK */
    }

    EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);


exit:
    EMBX_DebugOff(EMBX_INFO_PORT);
    EMBX_Info(EMBX_INFO_PORT, ("<<<<EMBX_CreatePort(htp=0x%lx,%s) = %d\n",(unsigned long)htp,(portName==0?"(NULL)":portName), res));

    return res;
}


#ifdef EMBX_RECEIVE_CALLBACK
EMBX_ERROR EMBX_CreatePortCallback(EMBX_TRANSPORT htp, const EMBX_CHAR *portName, EMBX_PORT *port,
				   EMBX_Callback_t callback, EMBX_HANDLE handle)
{
EMBX_ERROR res;

    EMBX_Info(EMBX_INFO_PORT, (">>>>EMBX_CreatePortCallback(htp=0x%lx,%s)\n",(unsigned long)htp,(portName==0?"(NULL)":portName)));
    EMBX_DebugOn(EMBX_INFO_PORT);

    res = createport_validate_args(htp, portName, port);
    if(res != EMBX_SUCCESS)
    {
        goto exit;
    }

    EMBX_OS_MUTEX_TAKE(&_embx_dvr_ctx.lock);

    if(_embx_dvr_ctx.state != EMBX_DVR_INITIALIZED)
    {
        EMBX_DebugMessage((_driver_uninit_error));
        res = EMBX_INVALID_TRANSPORT;
    }
    else
    {
      res = do_createport(htp, portName, port, callback, handle);
    }

    EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);


exit:
    EMBX_DebugOff(EMBX_INFO_PORT);
    EMBX_Info(EMBX_INFO_PORT, ("<<<<EMBX_CreatePortCallback(htp=0x%lx,%s) = %d\n",(unsigned long)htp,(portName==0?"(NULL)":portName),res));

    return res;
}
#endif /* EMBX_RECEIVE_CALLBACK */

/****************************************************************************/

static EMBX_ERROR do_local_closeport(EMBX_PORT port)
{
EMBX_ERROR res = EMBX_INVALID_PORT;
EMBX_LocalPortHandle_t *ph;

    ph = (EMBX_LocalPortHandle_t *)EMBX_HANDLE_GETOBJ(&_embx_handle_manager, port);
    /* We can close local handles which are both valid and invalid (but not already
     * closed of course)
     */

    if(ph != 0)
    {
    EMBX_TransportHandle_t *th       = ph->transportHandle;
    EMBX_LocalPortList_t   *portList = th->localPorts;
       
        /* Try and find the port handle on the transport handle
         * local port list.
         */
        while(portList)
        {
            if(portList->port == ph)
                break;

            portList = portList->next;
        }

#if defined(EMBX_VERBOSE)
        if(portList == 0)
        {
            EMBX_DebugMessage(("Unable to find port (0x%08lx) on transport handle (0x%08lx) port list!\n",(unsigned long)ph,(unsigned long)th));
        }
#endif /* EMBX_VERBOSE */

        res = ph->methods->close_port(ph);
        if(res == EMBX_SUCCESS)
        {
            /* we must use a saved version of the transport pointer
             * here, to update the ports in use counter, as ph has just
             * been successfully deleted.
             */
            th->transport->transportInfo.nrPortsInUse--;
            EMBX_HANDLE_FREE(&_embx_handle_manager,port);

            /* Unhook the port list entry */
            if(portList != 0)
            {
                if(portList->prev != 0)
                    portList->prev->next = portList->next;
                else
                    th->localPorts = portList->next;

                if(portList->next != 0)
                    portList->next->prev = portList->prev;
                
                EMBX_OS_MemFree(portList);    
            }
        }
    }

    return res;
}



static EMBX_ERROR do_remote_closeport(EMBX_PORT port)
{
EMBX_ERROR res = EMBX_INVALID_PORT;
EMBX_RemotePortHandle_t *ph;

    ph = (EMBX_RemotePortHandle_t *)EMBX_HANDLE_GETOBJ(&_embx_handle_manager, port);
    /* Although we cannot explicitly invalidate a remote port, the closing of
     * the other side will cause the driver to invalidate the handle, which an
     * application still has to close to clean things up. So both valid and 
     * invalid handles can be closed.
     */
    if(ph != 0)
    {
    EMBX_TransportHandle_t *th       = ph->transportHandle;
    EMBX_RemotePortList_t  *portList = th->remotePorts;
       
        /* Try and find the port handle on the transport handle
         * local port list.
         */
        while(portList)
        {
            if(portList->port == ph)
                break;

            portList = portList->next;
        }

#if defined(EMBX_VERBOSE)
        if(portList == 0)
        {
            EMBX_DebugMessage(("Unable to find port (0x%08lx) on transport handle (0x%08lx) port list!\n",(unsigned long)ph,(unsigned long)th));
        }
#endif /* EMBX_VERBOSE */

        res = ph->methods->close_port(ph);
        if(res == EMBX_SUCCESS)
        {
            EMBX_HANDLE_FREE(&_embx_handle_manager,port);

            /* Unhook the port list entry */
            if(portList != 0)
            {
                if(portList->prev != 0)
                    portList->prev->next = portList->next;
                else
                    th->remotePorts = portList->next;

                if(portList->next != 0)
                    portList->next->prev = portList->prev;
                
                EMBX_OS_MemFree(portList);    
            }
        }
    }

    return res;
}



EMBX_ERROR EMBX_ClosePort(EMBX_PORT port)
{
EMBX_ERROR res = EMBX_INVALID_PORT;

    EMBX_Info(EMBX_INFO_PORT, (">>>>EMBX_ClosePort(port=0x%08lx)\n",(unsigned long)port));
    EMBX_DebugOn(EMBX_INFO_PORT);

    if(!_embx_dvr_ctx.isLockInitialized)
    {
        EMBX_DebugMessage((_driver_uninit_error));
        goto exit;
    }


    EMBX_OS_MUTEX_TAKE(&_embx_dvr_ctx.lock);

    if(_embx_dvr_ctx.state != EMBX_DVR_INITIALIZED &&
       _embx_dvr_ctx.state != EMBX_DVR_IN_SHUTDOWN)
    {
        EMBX_DebugMessage((_driver_uninit_error));
    }
    else
    {
        if(EMBX_HANDLE_ISTYPEOF(port, EMBX_HANDLE_TYPE_LOCALPORT))
        {
            res = do_local_closeport(port);
        }
        else if(EMBX_HANDLE_ISTYPEOF(port, EMBX_HANDLE_TYPE_REMOTEPORT))
        {
            res = do_remote_closeport(port);
        }
    }

    EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);


exit:
    EMBX_DebugOff(EMBX_INFO_PORT);
    EMBX_Info(EMBX_INFO_PORT, ("<<<<EMBX_ClosePort(port=0x%08lx) = %d\n",(unsigned long)port, res));

    return res;
}


/****************************************************************************/

EMBX_ERROR EMBX_InvalidatePort(EMBX_PORT port)
{
EMBX_ERROR res = EMBX_INVALID_PORT;

    EMBX_Info(EMBX_INFO_PORT, (">>>>EMBX_InvalidatePort(port=0x%08lx)\n",(unsigned long)port));
    EMBX_DebugOn(EMBX_INFO_PORT);

    if(!_embx_dvr_ctx.isLockInitialized)
    {
        EMBX_DebugMessage((_driver_uninit_error));
        goto exit;
    }


    EMBX_OS_MUTEX_TAKE(&_embx_dvr_ctx.lock);

    if(_embx_dvr_ctx.state != EMBX_DVR_INITIALIZED &&
       _embx_dvr_ctx.state != EMBX_DVR_IN_SHUTDOWN)
    {
        EMBX_DebugMessage((_driver_uninit_error));
    }
    else
    {
        /* Invalidate is only an operation on locally created ports */
        if(EMBX_HANDLE_ISTYPEOF(port, EMBX_HANDLE_TYPE_LOCALPORT))
        {
        EMBX_LocalPortHandle_t *ph;

            ph = (EMBX_LocalPortHandle_t *)EMBX_HANDLE_GETOBJ(&_embx_handle_manager, port);
            if( (ph != 0) && (ph->state == EMBX_HANDLE_VALID) )
            {
                res = ph->methods->invalidate_port(ph);
            }
        }
    }

    EMBX_OS_MUTEX_RELEASE(&_embx_dvr_ctx.lock);


exit:
    EMBX_DebugOff(EMBX_INFO_PORT);
    EMBX_Info(EMBX_INFO_PORT, ("<<<<EMBX_InvalidatePort(port=0x%08lx) = %d\n",(unsigned long)port, res));

    return res;
}
