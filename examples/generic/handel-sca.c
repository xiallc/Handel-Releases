/*
 * Example code to demonstrate setting of sca regions and reading out sca values
 *
 * Supported devices are xMap, Saturn, Mercury / Mercury4, 
 * microDxp (limited support depending on firmware version) 
 *
 * Copyright (c) 2008-2016, XIA LLC
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
#include "handel_constants.h"
#include "md_generic.h"

static void print_usage(void);
static void CHECK_ERROR(int status);
static void do_run(double runtime_ms);
static void SLEEP(double time_seconds);

static void check_microdxp_sca_features();

int main(int argc, char* argv[]) {
    int status;
    unsigned short i;
    int ignored = 0;

    /* SCA Settings. sca_values must match number_scas */
    double number_scas = 4.0;
    double sca_values[4];

    double runtime = 1.0;

    double sca_bound = 0.0;
    double sca_size = 0.0;
    double number_mca_channels = 0.0;

    char scaStr[9];

    if (argc < 2) {
        print_usage();
        exit(1);
    }

    printf("-- Initializing Handel\n");
    status = xiaInit(argv[1]);
    CHECK_ERROR(status);

    xiaSetLogLevel(MD_DEBUG);
    xiaSetLogOutput("handel.log");

    printf("-- Starting the system\n");
    status = xiaStartSystem();
    CHECK_ERROR(status);

    check_microdxp_sca_features();

    printf("-- Setting Acquisition Values\n");

    /* Set the number of SCAs */
    printf("-- Set SCAs\n");
    status = xiaSetAcquisitionValues(0, "number_of_scas", (void*) &number_scas);
    CHECK_ERROR(status);

    printf("-- Number of SCAs %0.0f\n", number_scas);

    /* Divide the entire spectrum region into equal number of SCAs */
    status =
        xiaGetAcquisitionValues(0, "number_mca_channels", (void*) &number_mca_channels);
    CHECK_ERROR(status);

    sca_bound = 0.0;
    sca_size = (int) (number_mca_channels / number_scas);

    /* Set the individual SCA limits */
    for (i = 0; i < (unsigned short) number_scas; i++) {
        sprintf(scaStr, "sca%hhu_lo", (byte_t) i);

        status = xiaSetAcquisitionValues(0, scaStr, (void*) &sca_bound);
        CHECK_ERROR(status);

        printf("  %0.0f,", sca_bound);
        sca_bound += sca_size;

        sprintf(scaStr, "sca%hhu_hi", (byte_t) i);
        status = xiaSetAcquisitionValues(0, scaStr, (void*) &sca_bound);
        CHECK_ERROR(status);

        printf("%0.0f\n", sca_bound);
    }

    /* Apply new acquisition values */
    status = xiaBoardOperation(0, "apply", &ignored);
    CHECK_ERROR(status);

    do_run(runtime);

    printf("-- Read out the SCA values\n");

    /* Read out the SCAs from the data buffer */
    status = xiaGetRunData(0, "sca", (void*) sca_values);
    CHECK_ERROR(status);

    for (i = 0; i < (int) number_scas; i++) {
        printf(" SCA%d = %0f\n", i, sca_values[i]);
    }

    printf("-- Cleaning up Handel.\n");
    xiaExit();
    xiaCloseLog();

    return 0;
}

/*
 * microDxp specific operation:
 *
 * Check to see if connected microDxp supports the latest SCA features
 * print a warning if not -- but continue with the SCA operations with
 * possible error results.
 */
static void check_microdxp_sca_features() {
    int status;
    unsigned long features;

    char moduleType[200];

    status = xiaGetModuleItem("module1", "module_type", (void*) moduleType);
    CHECK_ERROR(status);

    printf("-- Checking %s SCA features.\n", moduleType);

    /* Only applicable to microDxp */
    if (strcmp(moduleType, "udxp") != 0)
        return;

    status = xiaBoardOperation(0, "get_board_features", &features);
    CHECK_ERROR(status);

    /* feature list constants in handel_constants.h */
    printf(" Support for SCA region settings - [%s]\n",
           (features & 1 << BOARD_SUPPORTS_SCA) ? "YES" : "NO");

    printf(" Support for SCA data readout, run data 'sca' - [%s]\n",
           (features & 1 << BOARD_SUPPORTS_UPDATED_SCA) ? "YES" : "NO");
}

/*
 * This is just an example of how to handle error values. A program of any reasonable
 * size should implement a more robust error handling mechanism.
 */
static void CHECK_ERROR(int status) {
    /* XIA_SUCCESS is defined in handel_errors.h */
    if (status != XIA_SUCCESS) {
        printf("-- Error encountered! Status = %d, please check handel.log.\n", status);
        exit(status);
    }
}

static void print_usage(void) {
    fprintf(stdout, "Arguments: [.ini file]\n");
}

static void do_run(double runtime) {
    int status;

    /* Start a run w/ the MCA cleared */
    printf("-- Starting run\n");
    status = xiaStartRun(0, 0);
    CHECK_ERROR(status);

    printf("-- Waiting %0.2f\n", runtime);
    SLEEP(runtime);

    printf("-- Stopping run\n");
    status = xiaStopRun(0);
    CHECK_ERROR(status);
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