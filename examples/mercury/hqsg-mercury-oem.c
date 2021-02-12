/*
 * Sample code for Mercury OEM
 * Requires Mercury variant Mercury OEM
 *
 * Copyright (c) 2005-2020 XIA LLC
 * All rights reserved
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#if _WIN32
#pragma warning(disable : 4115)

/* For Sleep() */
#include <windows.h>
#else
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif

#include "handel.h"
#include "handel_errors.h"
#include "handel_constants.h"
#include "md_generic.h"

/* generic helper functions */
static void CHECK_ERROR(int status);
static void sleep_seconds(double time_seconds);
static void print_usage(void);
static void start_system(char *ini_file);
static void setup_logging(char *log_name);
static void INThandler(int sig);
static double get_time();
static int get_number_channels();
static void clean_up();

/* local helper */
static void check_mercury_oem_features();

/* Test and example usage */
static void test_preamp_gain();
static void test_rc_decay_and_calibration();

boolean_t stop = FALSE_;

int main(int argc, char *argv[])
{
    if (argc < 2) {
        print_usage();
        exit(1);
    }

    /* Trap SIGINT to ensure clean up */
    signal(SIGINT, INThandler);

    printf("-- Sample code for Mercury OEM\n"
           "-- Press CTRL+C to stop\n");

    setup_logging("handel.log");
    start_system(argv[1]);

    check_mercury_oem_features();

    test_preamp_gain();
    test_rc_decay_and_calibration();

    clean_up();
    return 0;
}

static void start_system(char *ini_file)
{
    int status;

    printf("Loading the .ini file\n");
    status = xiaInit(ini_file);
    CHECK_ERROR(status);

    /* Boot hardware */
    printf("Starting up the hardware\n");
    status = xiaStartSystem();
    CHECK_ERROR(status);
}

static void setup_logging(char *log_name)
{
    printf("Configuring the log file in %s\n", log_name);

    /* log level defined in md_generic.h */
    xiaSetLogLevel(MD_DEBUG);
    xiaSetLogOutput(log_name);
}

/*
 * Check return code for error, stop execution in case of error
 */
static void CHECK_ERROR(int status)
{
    /* error codes defined in handel_errors.h */
    if (status != XIA_SUCCESS) {
        printf("Error encountered! Status = %d, %s\n", status,
            xiaGetErrorText(status));
        clean_up();
        exit(status);
    }
}

static void INThandler(int sig)
{
    UNUSED(sig);
    stop = TRUE_;

    clean_up();
    exit(1);
}

static void sleep_seconds(double time_seconds)
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

double get_time()
{
#if _WIN32
    LARGE_INTEGER t, f;
    QueryPerformanceCounter(&t);
    QueryPerformanceFrequency(&f);
    return (double)t.QuadPart/(double)f.QuadPart;
#else
    struct timeval t;
    struct timezone tzp;
    gettimeofday(&t, &tzp);
    return t.tv_sec + t.tv_usec*1e-6;
#endif
}


/*
 * Clean up and release resources
 *
 */
static void clean_up()
{
    printf("\nCleaning up Handel.\n");
    xiaExit();

    printf("Closing the Handel log file.\n");
    xiaCloseLog();
}


static void print_usage(void)
{
    fprintf(stdout, "\n");
    fprintf(stdout, "* argument: [.ini file]\n");
    fprintf(stdout, "\n");
    return;
}


/*
 * Check to see if connected Mercury supports Mercury-OEM features
 * print a warning if not -- but continue with the operations with
 * possible error results.
 */
static void check_mercury_oem_features()
{
    int status;
    unsigned long features;

    char moduleType[200];

    status = xiaGetModuleItem("module1", "module_type", (void *)moduleType);
    CHECK_ERROR(status);

    printf("Checking %s features\n", moduleType);

    /* Only applicable to Mercury */
    if (strcmp(moduleType, "mercury") != 0) return;

    status = xiaBoardOperation(0, "get_board_features", &features);
    CHECK_ERROR(status);

    /* Feature list constants in handel_constants.h */
    printf(" : Support for mercury oem features - [%s]\n",
            (features & 1 << BOARD_SUPPORTS_MERCURYOEM_FEATURES) ? "YES" : "NO");

}

/*
 * preamp_gain tests
 */
static void test_preamp_gain()
{
    int status;
    int ignored = 0;
    int i;

    /* All possible input_attenuation values */
    const int nbr_input_attenuation = 3;
    double input_attenuation[3] = {2., 1., 0.};

    double preamp_gain[3] = {1.0, 2.5, 5.0};
    double dynamic_range[3] = {47200., 20000., 40000.};
    double mca_bin_width[3] = {20., 15., 10.};

    printf("\nMercury OEM switched gain setting\n");
    printf(""
           "input_attenuation, "
           "preamp_gain, "
           "dynamic_range, "
           "mca_bin_width, "
           "\n");

    /* Cycle through all possible input_attenuation values, check gain settings
     * Note that gain settings changes are internal, and the acquisition value
     * "preamp_gain" is still used in the same way to set and get gain.
     */
    for (i = 0; i < nbr_input_attenuation; i++) {
        status = xiaSetAcquisitionValues(0, "input_attenuation", &input_attenuation[i]);
        CHECK_ERROR(status);

        status = xiaSetAcquisitionValues(0, "preamp_gain", &preamp_gain[i]);
        CHECK_ERROR(status);

        status = xiaSetAcquisitionValues(0, "dynamic_range", &dynamic_range[i]);
        CHECK_ERROR(status);

        status = xiaSetAcquisitionValues(0, "mca_bin_width", &mca_bin_width[i]);
        CHECK_ERROR(status);

        status = xiaBoardOperation(0, "apply", &ignored);
        CHECK_ERROR(status);

        printf("%17.0f, %11.3f, %13.3f, %13.3f,",
            input_attenuation[i], preamp_gain[i], dynamic_range[i], mca_bin_width[i]);

        printf("\r\n");
    }

}

/*
 * rc_time settings and calibration
 */
static void test_rc_decay_and_calibration()
{
    int status;
    int ignored = 0;
    int i;

    /* Possible rc_time_constant */
    const int nbr_rc_time_constant = 7;

    double rc_time_constant = 0.0;
    double rc_time = 0.0;
    double peaking_time = 0.0;

    printf("\nRC decay setting\n");
    printf(
           "rc_time_constant, "
           "rc_time, "
           "\n");

    /* Cycle through all possible rc_time_constant values */
    for (i = 0; i < nbr_rc_time_constant; i++) {
        rc_time_constant  = (double)i;

        /* Setting rc_time_constant should set rc_time to a nominal value */
        status = xiaSetAcquisitionValues(0, "rc_time_constant", &rc_time_constant);
        CHECK_ERROR(status);

        status = xiaBoardOperation(0, "apply", &ignored);
        CHECK_ERROR(status);

        status = xiaGetAcquisitionValues(0, "rc_time", &rc_time);
        CHECK_ERROR(status);
        printf("%16.0f, %7.3f,",
                rc_time_constant, rc_time);

        printf("\n");
    }

    status = xiaGetAcquisitionValues(0, "peaking_time", &peaking_time);

    printf("\nCheck rc_time after calibrate_rc_time, peaking_time = %.2f\n", peaking_time);
    printf(
           "rc_time_constant, "
           "rc_time, "
           "\n");

    /* Cycle through all possible rc_time_constant values, check calibration */
    for (i = 0; i < nbr_rc_time_constant; i++) {
        rc_time_constant  = (double)i;

        /* Setting rc_time_constant should set rc_time to a nominal value */
        status = xiaSetAcquisitionValues(0, "rc_time_constant", &rc_time_constant);
        CHECK_ERROR(status);

        status = xiaBoardOperation(0, "apply", &ignored);
        CHECK_ERROR(status);

        printf("%16.0f, ", rc_time_constant);

        /* Run the calibrate RC special run to calibrate rc_time */
        status = xiaDoSpecialRun(0, "calibrate_rc_time", NULL);
        CHECK_ERROR(status);

        /* Read out the calibrated rc_time */
        status = xiaGetAcquisitionValues(0, "rc_time", &rc_time);
        CHECK_ERROR(status);

        printf("%7.3f, ", rc_time);
        printf("\n");
    }

}

