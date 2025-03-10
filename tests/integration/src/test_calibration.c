/*
 * Generic test to iterate through typical Handel operations
 *
 * Copyright (c) 2005-2015 XIA LLC
 * All rights reserved
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

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
static void setup_logging(char* log_name);
static void clean_up();
static void INThandler(int sig);

int main(int argc, char* argv[]) {
    int status;
    int ignore;

    double new_gain = 0.0;
    double original_gain = 0.0;
    double scale;

    if (argc < 2) {
        print_usage();
        exit(1);
    }

    signal(SIGINT, INThandler);

    setup_logging("handel.log");
    start_system(argv[1]);

    status = xiaGetAcquisitionValues(0, "gain", &original_gain);
    CHECK_ERROR(status);

    do_run(1000.0);

    printf("Calibrating gain. Press CTRL+C to stop.\n");

    for (;;) {
        printf(".");

        new_gain = (rand() % 98) + 1.0;
        scale = new_gain / original_gain;

        status = xiaGainOperation(0, "calibrate", &scale);
        CHECK_ERROR(status);
        status = xiaBoardOperation(0, "apply", &ignore);
        CHECK_ERROR(status);

        status = xiaGetAcquisitionValues(0, "gain", &original_gain);
        CHECK_ERROR(status);

        if ((original_gain - new_gain) > 0.1) {
            printf("Gain read out value %0.3f does not match %0.3f\n", original_gain,
                   new_gain);
            break;
        }
    }

    clean_up();
    return 0;
}

static void INThandler(int sig) {
    (void) sig;
    printf("\n");
    clean_up();
    exit(1);
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

static void setup_logging(char* log_name) {
    printf("Configuring the log file.\n");
    xiaSetLogLevel(MD_DEBUG);
    xiaSetLogOutput(log_name);
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

static void clean_up() {
    printf("Cleaning up Handel.\n");
    xiaExit();

    printf("Closing the Handel log file.\n");
    xiaCloseLog();
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
        clean_up();
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
