/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: embx_support.c                                            */
/*                                                                 */
/* Description:                                                    */
/*         EMBX support routines                                   */
/*                                                                 */
/*******************************************************************/

#include "embx_typesP.h"
#include "embxP.h"
#include "embx_osinterface.h"
#include "embxshell.h"

/*
 * Management of event lists, used to track and signal blocked tasks
 * waiting on OpenTransport, ConnectBlock and ReceiveBlock
 */
EMBX_VOID EMBX_EventListAdd(EMBX_EventList_t **head, EMBX_EventList_t *node)
{
    node->prev = 0;
    node->next = *head;

    if(*head != 0)
    {
        (*head)->prev = node;
    }

    *head = node;
}

EMBX_VOID EMBX_EventListRemove(EMBX_EventList_t **head, EMBX_EventList_t *node)
{
    if(node->prev != 0)
    {
        node->prev->next = node->next;
    }

    if(node->next != 0)
    {
        node->next->prev = node->prev;
    }

    if(node == *head)
    {
        *head = node->next;
    }
}

EMBX_EventList_t * EMBX_EventListFront(EMBX_EventList_t **head)
{
EMBX_EventList_t *node = *head;

    if (node != 0)
    {
	*head = (*head)->next;

	if (*head != 0) 
	{
	    (*head)->prev = 0;
	}
    }

    return node;
}

EMBX_VOID EMBX_EventListSignal(EMBX_EventList_t **head, EMBX_ERROR result)
{
EMBX_EventList_t *l = *head;

    while(l)
    {
        l->result = result;
        EMBX_OS_EVENT_POST(&(l->event));
        l = l->next;
    }
}



EMBX_TransportList_t *EMBX_find_transport_entry(const EMBX_CHAR *name)
{
EMBX_TransportList_t *tl = _embx_dvr_ctx.transports;

    while(tl)
    {
        if(!strcmp(name, tl->transport->transportInfo.name))
        {
            return tl;
        }

        tl = tl->next;
    }

    return 0;
}

