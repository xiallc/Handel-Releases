/*
 * This code accompanies the XIA Application Note
 * "Handel Quick Start Guide: Saturn".
 *
 * Copyright (c) 2002-2004, X-ray Instrumentation Associates
 *               2005-2009, XIA LLC
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

static void print_usage(void);
static void CHECK_ERROR(int status);
static void SLEEP(double time_seconds);

int main(int argc, char* argv[]) {
    int status;
    int i;

    /* Acquisition Values */
    double pt = 16.0;
    double thresh = 1000.0;
    double calib = 5900.0;

    /* SCA Settings */
    double nSCAs = 2.0;

    double scaLowLimits[] = {10.0, 500.0};
    double scaHighLimits[] = {20.0, 700.0};

    unsigned long SCAs[2];

    char scaStr[80];

    double runtime = 1.0;
    unsigned long mcaLen = 0;
    unsigned long* mca = NULL;

    if (argc < 2) {
        print_usage();
        exit(1);
    }

    printf("-- Initializing Handel\n");
    status = xiaInit(argv[1]);
    CHECK_ERROR(status);

    /* Setup logging here */
    xiaSetLogLevel(MD_DEBUG);
    xiaSetLogOutput("errors.log");

    printf("-- Starting The System\n");
    status = xiaStartSystem();
    CHECK_ERROR(status);

    printf("-- Setting Acquisition Values\n");

    status = xiaSetAcquisitionValues(0, "peaking_time", (void*) &pt);
    CHECK_ERROR(status);

    status = xiaSetAcquisitionValues(0, "trigger_threshold", (void*) &thresh);
    CHECK_ERROR(status);

    status = xiaSetAcquisitionValues(0, "calibration_energy", (void*) &calib);
    CHECK_ERROR(status);

    /* Set the number of SCAs */
    printf("-- Set SCAs\n");
    status = xiaSetAcquisitionValues(0, "number_of_scas", (void*) &nSCAs);
    CHECK_ERROR(status);

    /* Set the individual SCA limits */
    for (i = 0; i < (unsigned short) nSCAs; i++) {
        sprintf(scaStr, "sca%d_lo", i);
        status = xiaSetAcquisitionValues(0, scaStr, (void*) &(scaLowLimits[i]));
        CHECK_ERROR(status);

        sprintf(scaStr, "sca%d_hi", i);
        status = xiaSetAcquisitionValues(0, scaStr, (void*) &(scaHighLimits[i]));
        CHECK_ERROR(status);
    }

    /* Start a run w/ the MCA cleared */
    printf("-- Startinga Run\n");
    status = xiaStartRun(0, 0);
    CHECK_ERROR(status);

    printf("-- Waiting %0.2f\n", runtime);
    SLEEP(runtime);

    printf("-- Stopping a run\n");
    status = xiaStopRun(0);
    CHECK_ERROR(status);

    /* Prepare to read out MCA spectrum */
    status = xiaGetRunData(0, "mca_length", (void*) &mcaLen);
    CHECK_ERROR(status);
    printf("-- Read out MCA spectrum length: %lu\n", mcaLen);

    /*
     * If you don't want to dynamically allocate memory here,
     * then be sure to declare mca as an array of length 8192,
     * since that is the maximum length of the spectrum.
     */
    mca = (unsigned long*) malloc(mcaLen * sizeof(unsigned long));

    if (mca == NULL) {
        /* Error allocating memory */
        exit(1);
    }

    printf("-- Read out the MCA Spectrum\n");
    status = xiaGetRunData(0, "mca", (void*) mca);
    CHECK_ERROR(status);

    /* Display the spectrum, write it to a file, etc... */

    free(mca);

    /* Read out the SCAs from the data buffer */
    status = xiaGetRunData(0, "sca", (void*) SCAs);
    CHECK_ERROR(status);

    for (i = 0; i < (unsigned short) nSCAs; i++) {
        printf("-- SCA%d = %lu\n", i, SCAs[i]);
    }

    xiaExit();

    return 0;
}

/**********
 * This is just an example of how to handle error values.
 * A program of any reasonable size should
 * implement a more robust error handling mechanism.
 **********/
static void CHECK_ERROR(int status) {
    /* XIA_SUCCESS is defined in handel_errors.h */
    if (status != XIA_SUCCESS) {
        printf("-- Error encountered! Status = %d\n", status);
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