/*
 * Generic test to iterate through typical Handel operations
 *
 * Copyright (c) 2005-2015 XIA LLC
 * All rights reserved
 *
 */

#include <stdio.h>
#include <stdlib.h>

#if _WIN32
#pragma warning(disable : 4115)

/* For Sleep() */
#include <windows.h>
#else
#include <time.h>
#endif

#include "handel.h"
#include "handel_errors.h"
#include "md_generic.h"

static void CHECK_ERROR(int status);
static int MS_SLEEP(float* time);
static void print_usage(void);
static void do_run(float ms);
static void start_system(char* ini_file);

int main(int argc, char* argv[]) {
    int status;

    unsigned long mcaLen = 0;
    unsigned long* mca = NULL;

    if (argc < 2) {
        print_usage();
        exit(1);
    }

    /* Setup logging */
    printf("Configuring the Handel log file.\n");
    xiaSetLogLevel(MD_WARNING);
    xiaSetLogOutput("handel.log");

    /* Test that redundant call to xiaExit won't cause crashes */
    xiaExit();

    start_system(argv[1]);

    /* Test that after MCA readout PLX event handle is cleaned up at disconnect */
    do_run(100);

    /* Prepare to read out MCA spectrum */
    printf("Getting the MCA length.\n");
    status = xiaGetRunData(0, "mca_length", &mcaLen);
    CHECK_ERROR(status);

    printf("Allocating memory for the MCA data.\n");
    mca = malloc(mcaLen * sizeof(unsigned long));

    if (!mca) {
        /* Error allocating memory */
        exit(1);
    }

    printf("Reading the MCA.\n");
    status = xiaGetRunData(0, "mca", mca);
    CHECK_ERROR(status);

    printf("Release MCA memory.\n");
    free(mca);

    /* Test closing and reopening log */
    printf("Closing log in the middle of the application.\n");
    xiaCloseLog();

    do_run(100);

    xiaSetLogLevel(MD_DEBUG);
    xiaSetLogOutput("handel.log");

    printf("Cleaning up Handel.\n");
    status = xiaExit();
    CHECK_ERROR(status);

    /* Test that restarting system works without memory failure */
    printf("Restarting Handel.\n");
    xiaStartSystem();

    printf("Cleaning up Handel.\n");
    status = xiaExit();
    CHECK_ERROR(status);

    printf("Closing the Handel log file.\n");
    xiaCloseLog();

    return 0;
}

static void start_system(char* ini_file) {
    int status;

    printf("Loading the .ini file.\n");
    status = xiaInit(ini_file);
    CHECK_ERROR(status);

    /* Boot hardware */
    printf("Starting up the hardware.\n");
    status = xiaStartSystem();
    CHECK_ERROR(status);
}

static void do_run(float ms) {
    int status;

    /* Start a run w/ the MCA cleared */
    printf("Starting the run.\n");
    status = xiaStartRun(-1, 0);
    CHECK_ERROR(status);

    printf("Waiting %f ms to collect data.\n", ms);
    MS_SLEEP(&ms);

    printf("Stopping the run.\n");
    status = xiaStopRun(-1);
    CHECK_ERROR(status);
}

/*
 * This is just an example of how to handle error values.  A program
 * of any reasonable size should implement a more robust error
 * handling mechanism.
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
    fprintf(stdout, "\n");
    fprintf(stdout, "* argument: [.ini file]\n");
    fprintf(stdout, "\n");
    return;
}

static int MS_SLEEP(float* time) {
#if _WIN32
    DWORD wait = (DWORD) (1000.0 * (*time));
    Sleep(wait);
#else
    unsigned long secs = (unsigned long) *time;
    struct timespec req = {.tv_sec = secs, .tv_nsec = ((*time - secs) * 1000000000.0)};
    struct timespec rem = {.tv_sec = 0, .tv_nsec = 0};
    while (TRUE_) {
        if (nanosleep(&req, &rem) == 0)
            break;
        req = rem;
    }
#endif
    return XIA_SUCCESS;
}
