/* @file snoopOpen.c
 * @provides snoopOpen
 *
 * $Id: snoopOpen.c 2070 2009-09-18 22:36:02Z brylow $
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <device.h>
#include <interrupt.h>
#include <network.h>
#include <snoop.h>

/**
 * Opens a capture from a network device.
 * @param cap pointer to capture structure
 * @param name of underlying device, ALL for all network devices
 * @return OK if open was successful, otherwise SYSERR
 * @pre-condition filter settings should already be setup in cap
 */
int snoopOpen(struct snoop *cap, char *devname)
{
    int i;
    int count = 0;
    int devnum;
    irqmask im;

    i = 0;
    /* Error check pointers */
    if ((NULL == cap) || (NULL == devname))
    {
        return SYSERR;
    }

    SNOOP_TRACE("Opening capture on %s", devname);

    /* Reset statistics */
    cap->ncap = 0;
    cap->nmatch = 0;
    cap->novrn = 0;

    /* Allocated mailbox for queue packets */
    cap->queue = mailboxAlloc(SNOOP_QLEN);
    if (SYSERR == (int)cap->queue)
    {
        SNOOP_TRACE("Failed to allocate mailbox");
        return SYSERR;
    }

    /* Attach capture to all running network interfaces for devname "ALL" */
    if (0 == strncmp(devname, "ALL", 4))
    {
        im = disable();
#if NNETIF
        for (i = 0; i < NNETIF; i++)
        {
            if (NET_ALLOC == netiftab[i].state)
            {
                netiftab[i].capture = cap;
                count++;
                SNOOP_TRACE("Attached capture to interface %d", i);
            }
        }
#endif
        restore(im);
        if (0 == count)
        {
            SNOOP_TRACE("Capture not attached to any interface");
            return SYSERR;
        }
        return OK;
    }

    /* Determine network interface to attach capture to */
    devnum = getdev(devname);
    if (SYSERR == devnum)
    {
        SNOOP_TRACE("Invalid device");
        return SYSERR;
    }
    im = disable();
#if NNETIF
    for (i = 0; i < NNETIF; i++)
    {
        if ((NET_ALLOC == netiftab[i].state)
            && (netiftab[i].dev == devnum))
        {
            netiftab[i].capture = cap;
            restore(im);
            SNOOP_TRACE("Attached capture to interface %d", i);
            return OK;
        }
    }
#endif

    /* No network interface found */
    restore(im);
    SNOOP_TRACE("No network interface found");
    return SYSERR;
}
