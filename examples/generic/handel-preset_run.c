/*
* Example code to demonstrate setting preset run parameters
*
* Supported devices are xMap, Saturn, STJ, Mercury / Mercury4, microDXP
*
* Copyright (c) 2008-2017, XIA LLC
* All rights reserved
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
#include "md_generic.h"
#include "handel_constants.h"

static void print_usage(void);
static void CHECK_ERROR(int status);
static void SLEEP(double time_seconds);

static int RUN_ACTIVE_BIT = 0x1;

int main(int argc, char* argv[]) {
    int status;
    int ignored = 0;

    /* preset settings */
    double presetRealtime = 5.0;

    /* preset types from handel_constants.h */
    double presetType = XIA_PRESET_FIXED_REAL;
    double realtime = 0.0;

    unsigned long runActive;

    if (argc < 2) {
        print_usage();
        exit(1);
    }

    printf("-- Initializing Handel.\n");
    status = xiaInit(argv[1]);
    CHECK_ERROR(status);

    xiaSetLogLevel(MD_DEBUG);
    xiaSetLogOutput("handel.log");

    printf("-- Starting the system\n");
    status = xiaStartSystem();
    CHECK_ERROR(status);

    printf("-- Setting Acquisition Values\n");

    status = xiaSetAcquisitionValues(-1, "preset_type", &presetType);
    CHECK_ERROR(status);

    status = xiaSetAcquisitionValues(-1, "preset_value", &presetRealtime);
    CHECK_ERROR(status);

    /* Apply new acquisition values */
    status = xiaBoardOperation(0, "apply", &ignored);
    CHECK_ERROR(status);

    /* Start a run w/ the MCA cleared */
    printf("-- Starting a Run\n");
    status = xiaStartRun(0, 0);
    CHECK_ERROR(status);

    /*
     * Poll waiting for the preset run to complete, for simplicity
     * Only the first channel in the system is used here
     */
    printf("-- Polling waiting for preset run of %0.2fs realtime to complete.",
           presetRealtime);

    for (;;) {
        printf(".");
        status = xiaGetRunData(0, "run_active", &runActive);
        CHECK_ERROR(status);

        if ((runActive & RUN_ACTIVE_BIT) == 0) {
            printf("\n");
            break;
        }

        SLEEP(0.5);
    }

    printf("-- Stopping a run.\n");
    status = xiaStopRun(0);
    CHECK_ERROR(status);

    status = xiaGetRunData(0, "realtime", &realtime);
    CHECK_ERROR(status);

    printf("-- Elapsed run time channel %d = %0.2fs\n", 0, realtime);

    printf("-- Cleaning up Handel.\n");
    xiaExit();
    xiaCloseLog();

    return 0;
}

/*
 * This is just an example of how to handle error values.
 * A program of any reasonable size should
 * implement a more robust error handling mechanism.
 */
static void CHECK_ERROR(int status) {
    /* XIA_SUCCESS is defined in handel_errors.h */
    if (status != XIA_SUCCESS) {
        printf("Error encountered! Status = %d\n", status);
        xiaExit();
        xiaCloseLog();
        exit(status);
    }
}

static void print_usage(void) {
    fprintf(stdout, "Arguments: [.ini file]]\n");
    return;
}

static void SLEEP(double time_seconds) {
#if _WIN32
    DWORD wait = (DWORD) (1000.0 * time_seconds);
    Sleep(wait);
#else
    unsigned long secs = (unsigned long) time_seconds;
    struct timespec req = {.tv_sec = secs,
                           .tv_nsec = ((time_seconds - secs) * 1000000000.0)};
    struct timespec rem = {.tv_sec = 0, .tv_nsec = 0};
    while (TRUE_) {
        if (nanosleep(&req, &rem) == 0)
            break;
        req = rem;
    }
#endif
}
