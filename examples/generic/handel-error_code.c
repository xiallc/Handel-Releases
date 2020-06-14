/*
* Example code to demonstrate setting preset run parameters
*
* Supported devices are xMap, Saturn, STJ, Mercury / Mercury4, microDXP
*
* Copyright (c) 2008-2020, XIA LLC
* All rights reserved
*
*
*/

#include <stdio.h>
#include <stdlib.h>

/* For Sleep() */
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

#include "handel.h"
#include "handel_errors.h"
#include "xerxes_errors.h"
#include "md_generic.h"
#include "handel_constants.h"


static void print_usage(void);
static void CHECK_ERROR(int status);
static void SLEEP(double time_seconds);


int main()
{

    printf("--- Checking Handel error code translationg.\n");

    printf("DXP_MEMORY_LENGTH       -- %s\n", xiaGetErrorText(DXP_MEMORY_LENGTH));
    printf("DXP_MALFORMED_FILE      -- %s\n", xiaGetErrorText(DXP_MALFORMED_FILE));
    printf("DXP_SUCCESS             -- %s\n", xiaGetErrorText(DXP_SUCCESS));

    printf("XIA_SIZE_MISMATCH       -- %s\n", xiaGetErrorText(XIA_SIZE_MISMATCH));
    printf("XIA_PRESET_VALUE_OOR    -- %s\n", xiaGetErrorText(XIA_PRESET_VALUE_OOR));
    printf("XIA_LIVEOUTPUT_OOR      -- %s\n", xiaGetErrorText(XIA_LIVEOUTPUT_OOR));

    printf("None existing error     -- %s\n", xiaGetErrorText(2048));

    return 0;
}

/*
* This is just an example of how to handle error values.
* A program of any reasonable size should
* implement a more robust error handling mechanism.
*/
static void CHECK_ERROR(int status)
{
    /* XIA_SUCCESS is defined in handel_errors.h */
    if (status != XIA_SUCCESS) {
        printf("Error encountered! Status = %d\n", status);
        xiaExit();
        xiaCloseLog();
        exit(status);
    }
}

static void print_usage(void)
{
    fprintf(stdout, "Arguments: [.ini file]]\n");
    return;
}

static void SLEEP(double time_seconds)
{
#if _WIN32
    DWORD wait = (DWORD)(1000.0 * time_seconds);
    Sleep(wait);
#else
    unsigned long secs = (unsigned long)time_seconds;
    struct timespec req = {
        .tv_sec = secs,
        .tv_nsec = ((time_seconds - secs) * 1000000000.0)
    };
    struct timespec rem = {
        .tv_sec = 0,
        .tv_nsec = 0
    };
    while (TRUE_) {
        if (nanosleep(&req, &rem) == 0)
        break;
        req = rem;
    }
#endif
}
