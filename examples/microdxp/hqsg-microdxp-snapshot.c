/*
 * Snapshot data and statistics readout with benchmark utility for microDxp
 *
 * Copyright (c) 2005-2017 XIA LLC
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
static void start_system(char* ini_file);
static void setup_logging(char* log_name);
static void clean_up();
static void INThandler(int sig);
static double get_time();
static void check_microdxp_sca_features();

unsigned long* mca = NULL;
double* sca = NULL;

boolean_t stop = FALSE_;

int main(int argc, char* argv[]) {
    int status;
    unsigned int i, j;
    unsigned long mca_total;

    double sleep = 0.5;
    double clearspectrum[1] = {0.};
    double statistics[9];

    double test_time;
    double mca_length;

    unsigned long snapshot_sca_length;
    unsigned long mca_lengths[3] = {1024, 2048, 4096};

    double sca_length = 2;
    double sca_limit = 0;

    if (argc < 2) {
        print_usage();
        exit(1);
    }

    signal(SIGINT, INThandler);

    setup_logging("handel.log");
    start_system(argv[1]);

    printf("Test started. Press CTRL+C to stop.\n");

    check_microdxp_sca_features();

    status = xiaSetAcquisitionValues(-1, "number_of_scas", &sca_length);
    CHECK_ERROR(status);

    status = xiaGetSpecialRunData(0, "snapshot_sca_length", &snapshot_sca_length);
    CHECK_ERROR(status);

    printf("\r\nsnapshot_sca_length = %lu", snapshot_sca_length);

    sca = malloc((int) snapshot_sca_length * sizeof(double));
    if (!sca)
        clean_up();

    for (i = 0; i < 3; i++) {
        mca_length = mca_lengths[i];
        status = xiaSetAcquisitionValues(-1, "number_mca_channels", &mca_length);
        CHECK_ERROR(status);

        printf("\r\nmca_length = %.0f\r\n", mca_length);

        if (stop)
            break;

        if (mca)
            free(mca);
        mca = malloc(mca_lengths[i] * sizeof(unsigned long));
        if (!mca)
            clean_up();

        /* Set two SCA regions splitting the entire spectrum */
        sca_limit = 0;
        status = xiaSetAcquisitionValues(-1, "sca0_lo", &sca_limit);
        CHECK_ERROR(status);

        sca_limit = (int) (mca_length / 2);
        status = xiaSetAcquisitionValues(-1, "sca0_hi", &sca_limit);
        CHECK_ERROR(status);

        sca_limit++;
        status = xiaSetAcquisitionValues(-1, "sca1_lo", &sca_limit);
        CHECK_ERROR(status);

        status = xiaSetAcquisitionValues(-1, "sca1_hi", &mca_length);
        CHECK_ERROR(status);

        /* start a run and take snapshots */
        status = xiaStartRun(-1, 0);
        CHECK_ERROR(status);

        SLEEP(sleep);

        test_time = get_time();
        status = xiaDoSpecialRun(0, "snapshot", &clearspectrum);
        CHECK_ERROR(status);

        printf("take snapshot elapsed %.6fs\r\n", get_time() - test_time);
        test_time = get_time();

        status = xiaGetSpecialRunData(0, "snapshot_mca", mca);
        CHECK_ERROR(status);

        printf("read snapshot mca elapsed %.6fs\r\n", get_time() - test_time);
        test_time = get_time();

        status = xiaGetSpecialRunData(0, "snapshot_statistics", statistics);
        CHECK_ERROR(status);

        printf("read snapshot statistics elapsed %.6fs\r\n", get_time() - test_time);
        test_time = get_time();

        status = xiaGetSpecialRunData(0, "snapshot_sca", sca);
        printf("read snapshot sca elapsed %.6fs\r\n", get_time() - test_time);

        if (status != XIA_NOSUPPORT_VALUE) {
            CHECK_ERROR(status);
            printf("sca[0] = %.0f\r\n", sca[0]);
            printf("sca[1] = %.0f\r\n", sca[1]);
        }

        mca_total = 0;
        for (j = 0; j < mca_length; j++) {
            mca_total += mca[j];
        }

        printf("events = %.0f mca_total = %lu\r\n", statistics[4], mca_total);

        /*
         * Writes a JSON object, so make sure first and last have the open/close brace!
         */
        printf("Run Statistics:\n");
        printf("{\"run_time\": %.4f,", statistics[0]);
        printf("\"trigger_livetime\": %.4f,", statistics[1]);
        printf("\"energy_livetime\": %.4f,", statistics[2]);
        printf("\"triggers\": %.4f,", statistics[3]);
        printf("\"events\": %.4f,", statistics[4]);
        printf("\"icr\": %.4f,", statistics[5]);
        printf("\"ocr\": %.4f,", statistics[6]);
        printf("\"underflows\": %.4f,", statistics[7]);
        printf("\"overflows\": %.4f}\n", statistics[8]);

        status = xiaStopRun(-1);
        CHECK_ERROR(status);
    }

    clean_up();
    return 0;
}

static void INThandler(int sig) {
    UNUSED(sig);
    stop = TRUE_;
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
    printf("Configuring the log file in %s.\n", log_name);
    xiaSetLogLevel(MD_DEBUG);
    xiaSetLogOutput(log_name);
}

static void clean_up() {
    printf("\nCleaning up Handel.\n");
    xiaExit();

    printf("Closing the Handel log file.\n");
    xiaCloseLog();

    if (mca)
        free(mca);
    if (sca)
        free(sca);
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

double get_time() {
#if _WIN32
    LARGE_INTEGER t, f;
    QueryPerformanceCounter(&t);
    QueryPerformanceFrequency(&f);
    return (double) t.QuadPart / (double) f.QuadPart;
#else
    struct timeval t;
    struct timezone tzp;
    gettimeofday(&t, &tzp);
    return t.tv_sec + t.tv_usec * 1e-6;
#endif
}

/*
 * microDxp specific operation:
 *
 * Check to see if connected microDxp supports the snapshot features
 * print a warning if not -- but continue with the operations with
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
    printf(" Support for snapshot special run - [%s]\n",
           (features & 1 << BOARD_SUPPORTS_SNAPSHOT) ? "YES" : "NO");

    printf(" Support for snapshot SCA data readout, run data 'snapshot_sca' - [%s]\n",
           (features & 1 << BOARD_SUPPORTS_SNAPSHOTSCA) ? "YES" : "NO");
}
