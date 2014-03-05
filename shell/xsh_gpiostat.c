/**
 * @file     xsh_gpiostat.c
 * @provides xsh_gpiostat.
 *
 * $Id: xsh_gpiostat.c 2157 2010-01-19 00:40:07Z brylow $
 */
/* Embedded Xinu, Copyright (C) 2009.  All rights reserved. */

#include <stddef.h>
#include <gpio.h>
#include <stdio.h>
#include <string.h>

/**
 * Shell command (gpiostat) provides information about the current status
 * of the GPIO pins.
 * @param nargs number of arguments in args array
 * @param args  array of arguments
 * @return non-zero value on error
 */
shellcmd xsh_gpiostat(int nargs, char *args[])
{
    struct gpio_csreg *pgcsr;   /* pointer to gpio registers     */
    uchar p;
    ulong offset;

    /* Output help, if '--help' argument was supplied */
    if (nargs == 2 && strncmp(args[1], "--help", 7) == 0)
    {
        printf("Usage: %s\n\n", args[0]);
        printf("Description:\n");
        printf("\tDisplays statitics for general purpose ");
        printf("input/output pins.\n");
        printf("Options:\n");
        printf("\t--help\t display this help and exit\n");
        return 1;
    }

    /* Check for correct number of arguments */
    if (nargs > 1)
    {
        fprintf(stderr, "%s: too many arguments\n", args[0]);
        fprintf(stderr, "Try '%s --help' for more information.\n",
                args[0]);
        return 1;
    }
#ifdef GPIO_BASE
    /* Initialize pointers */
    pgcsr = (struct gpio_csreg *)GPIO_BASE;
#else
    pgcsr = NULL;
#endif                          /* GPIO_BASE */
    if (NULL == pgcsr)
    {
        return -1;
    }

    /* Output GPIO pin status */
    offset = 0;

    printf("PIN MODE   INPUT   OUTPUT  CONTROL\n");
    printf("--- ------ ------- ------- -------\n");
    for (p = 0; p < GPIO_PIN_COUNT; p++)
    {
        offset = 1 << p;
        printf("%2d ", p);
        if (pgcsr->enable & offset)
        {
            printf(" Output ");
        }
        else
        {
            printf(" Input  ");
        }
        printf("   %1d    ", (pgcsr->input & offset) >> p);
        printf("   %1d    ", (pgcsr->output & offset) >> p);
        printf("   %1d    ", (pgcsr->control & offset) >> p);
        printf("\n");
    }

    return 0;
}
