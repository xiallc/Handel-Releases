/*
 * Sample code for microDXP USDA Vega replacement
 * Requires Vega variant microDXP
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


static void CHECK_ERROR(int status);
static void SLEEP(double time_seconds);
static void print_usage(void);
static void start_system(char *ini_file);
static void setup_logging(char *log_name);
static void clean_up();
static void INThandler(int sig);
static double get_time();
static void check_microdxp_vega_features();

unsigned long *mca = NULL;
unsigned long *mca_gated = NULL;

boolean_t stop = FALSE_;

int main(int argc, char *argv[])
{
    int status;
    unsigned int i, j;
    unsigned long mca_total, mca_gated_total;

    double sleep = 0.5;
    double high_voltage = 1.5;
    double readback = 0;

    double statistics[NUMBER_STATS], statistics_gated[NUMBER_STATS];

    double test_time;
    double mca_length;

    unsigned long mca_lengths[3] = {1024, 2048, 4096};

    if (argc < 2) {
        print_usage();
        exit(1);
    }

    signal(SIGINT, INThandler);

    printf("-- Sample code for microDXP USDA Vega replacement\n"
           "-- Requires microDXP Vega variant\n"
           "-- Press CTRL+C to stop\n");

    setup_logging("handel.log");
    start_system(argv[1]);

    check_microdxp_vega_features();

    status = xiaSetAcquisitionValues(-1, "high_voltage", &high_voltage);
    CHECK_ERROR(status);

    status = xiaGetAcquisitionValues(0, "high_voltage", &readback);
    CHECK_ERROR(status);

    printf("Set high_voltage to %.3fV readback %.3fV\n", high_voltage, readback);

    printf("\n"
           "    mca_length,"
           "     gate high,"
           "     test time,"
           " total realtime");

    for (i = 0; i < 3; i++)
    {
        mca_length = mca_lengths[i];
        status = xiaSetAcquisitionValues(-1, "number_mca_channels", &mca_length);
        CHECK_ERROR(status);

        if (stop) break;

        if (mca) free(mca);
        mca = malloc(mca_lengths[i] * sizeof(unsigned long));
        if (!mca) clean_up();

        if (mca_gated) free(mca_gated);
        mca_gated = malloc(mca_lengths[i] * sizeof(unsigned long));
        if (!mca_gated) clean_up();

        status = xiaStartRun(-1, 0);
        CHECK_ERROR(status);

        test_time = get_time();
        SLEEP(sleep);

        status = xiaStopRun(-1);
        CHECK_ERROR(status);

        test_time = get_time() - test_time;

        status = xiaGetRunData(0, "mca", mca);
        CHECK_ERROR(status);

        status = xiaGetRunData(0, "mca_gated", mca_gated);
        CHECK_ERROR(status);

        /* Statistics index are defined as RunStatistics in handel_constants.h */
        status = xiaGetRunData(0, "module_statistics_2", statistics);
        CHECK_ERROR(status);

        status = xiaGetRunData(0, "module_statistics_gated", statistics_gated);
        CHECK_ERROR(status);

        /* Do a quick check on statistics */
        mca_total = mca_gated_total = 0;
        for (j = 0; j < mca_length; j++) {
            mca_total += mca[j];
            mca_gated_total += mca_gated[j];
        }

        /* mca_length*/
        printf("\n%14.0f", mca_length);

        /* gate high (percentage estimated from total events ratio) */
        printf("%14.0f%%", mca_gated_total * 100.0 / (mca_total + mca_gated_total));

        /* test time, total realtime */
        printf("%14.3f %14.3f", test_time, statistics[Realtime] + statistics_gated[Realtime]);
    }

    clean_up();
    return 0;
}

static void INThandler(int sig)
{
    UNUSED(sig);
    stop = TRUE_;
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
    xiaSetLogLevel(MD_DEBUG);
    xiaSetLogOutput(log_name);
}

static void clean_up()
{
    printf("\nCleaning up Handel.\n");
    xiaExit();

    printf("Closing the Handel log file.\n");
    xiaCloseLog();

    if (mca) free(mca);
    if (mca_gated) free(mca_gated);
}

/*
 * This is just an example of how to handle error values.  A program
 * of any reasonable size should implement a more robust error
 * handling mechanism.
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

static void print_usage(void)
{
    fprintf(stdout, "\n");
    fprintf(stdout, "* argument: [.ini file]\n");
    fprintf(stdout, "\n");
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
 * microDxp specific operation:
 *
 * Check to see if connected microDxp supports Vega features
 * print a warning if not -- but continue with the operations with
 * possible error results.
 */
static void check_microdxp_vega_features()
{
    int status;
    unsigned long features;

    char moduleType[200];

    status = xiaGetModuleItem("module1", "module_type", (void *)moduleType);
    CHECK_ERROR(status);

    printf("Checking %s features\n", moduleType);

    /* Only applicable to microDxp */
    if (strcmp(moduleType, "udxp") != 0) return;

    status = xiaBoardOperation(0, "get_board_features", &features);
    CHECK_ERROR(status);

    /* Feature list constants in handel_constants.h */
    printf(" : Support for vega features - [%s]\n",
            (features & 1 << BOARD_SUPPORTS_VEGA_FEATURES) ? "YES" : "NO");

}
