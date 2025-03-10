/*
 * Generic test to do cycle of preset run and read out MCA data
 *
 * Copyright (c) 2005-2018 XIA LLC
 * All rights reserved
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#if _WIN32
#pragma warning(disable : 4115)
#include <windows.h> /* For Sleep() */
#include <conio.h> /* For key press detection _kbhit */
#else
#include <time.h>
#endif

#include "handel.h"
#include "handel_errors.h"
#include "handel_generic.h"
#include "md_generic.h"

static int RUN_ACTIVE_BIT = 0x1;

static void CHECK_ERROR(int status);
static int MS_SLEEP(float* time);
static void print_usage(void);
static void do_preset_run(int channel);
static void start_system(char* ini_file);
static void setup_logging(char* log_name);
static void clean_up();
static void INThandler(int sig);
static int get_number_channels();

unsigned long* mca = NULL;
FILE* logfp;
boolean_t stop = FALSE_;

int main(int argc, char* argv[]) {
    char* ini;

    int status;
    int ignore;

    unsigned int i;
    int channel;
    int number_channels = 0;

    unsigned long mca_length;
    unsigned long events;
    unsigned long mca_total;

    double mca_channels;
    double preset_type = 1.0;
    double preset_value = 2.0;

    if (argc < 2) {
        print_usage();
        exit(1);
    }

    ini = argv[1];

    if (argc > 2) {
        preset_value = atof(argv[2]);
    }

    signal(SIGINT, INThandler);

    setup_logging("handel.log");
    start_system(ini);

    printf("Setting up parameters.\n");

    number_channels = get_number_channels();

    status = xiaGetRunData(0, "mca_length", &mca_length);
    CHECK_ERROR(status);

    mca_channels = (double) mca_length;
    status = xiaSetAcquisitionValues(-1, "number_mca_channels", &mca_channels);

    mca = malloc(mca_length * sizeof(unsigned long));
    if (!mca) {
        printf("Failed allocating %lu words for MCA.\n", mca_length);
        clean_up();
        exit(2);
    }

    logfp = fopen("data.log", "wb");

    if (!logfp) {
        printf("Failed opening data.log.\n");
        clean_up();
        exit(3);
    }

    fprintf(logfp, "channel,total_output_events,sum of mca data\r\n");
    printf("Preset run length %.1f. \r\n", preset_value);

#if _WIN32
    printf("Press CTRL+C or q to stop.\n");
#else
    printf("Press CTRL+C to stop.\n");
#endif

    while (!stop) {
        printf(".");
        fflush(stdout);

        status = xiaSetAcquisitionValues(-1, "preset_type", &preset_type);
        CHECK_ERROR(status);

        status = xiaSetAcquisitionValues(-1, "preset_value", &preset_value);
        CHECK_ERROR(status);

        for (channel = 0; channel < number_channels; channel++) {
            status = xiaBoardOperation(channel, "apply", &ignore);
            CHECK_ERROR(status);
        }

        do_preset_run(number_channels);
        if (stop)
            break;

        for (channel = 0; channel < number_channels; channel++) {
            status = xiaGetRunData(channel, "mca", mca);
            CHECK_ERROR(status);

            status = xiaGetRunData(channel, "total_output_events", &events);
            CHECK_ERROR(status);

            mca_total = 0;
            for (i = 0; i < mca_length; i++)
                mca_total += mca[i];

            fprintf(logfp, "%u, %lu,%lu\r\n", channel, events, mca_total);
        }
        fflush(logfp);

#if _WIN32
        if (_kbhit())
            stop = (_getch() == 'q');
#endif
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
    printf("Configuring the log file.\n");
    xiaSetLogLevel(MD_DEBUG);
    xiaSetLogOutput(log_name);
}

static void do_preset_run(int number_channels) {
    int status;
    int channel;

    unsigned long runActive;
    float sleep = 1;
    boolean_t runstopped;

    /* start a run and wait for run to stop */
    status = xiaStartRun(-1, 0);
    CHECK_ERROR(status);

    while (!stop) {
        runstopped = TRUE_;
        for (channel = 0; channel < number_channels; channel++) {
            status = xiaGetRunData(channel, "run_active", &runActive);
            runstopped = runstopped && ((runActive & RUN_ACTIVE_BIT) == 0);
            CHECK_ERROR(status);
        }

        if (runstopped)
            break;

        MS_SLEEP(&sleep);
    };

    status = xiaStopRun(-1);
    CHECK_ERROR(status);
}

static void clean_up() {
    printf("\nCleaning up Handel.\n");
    xiaExit();

    printf("Closing the Handel log file.\n");
    xiaCloseLog();

    if (logfp)
        fclose(logfp);

    if (mca)
        free(mca);
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
    fprintf(stdout, "Usage: preset_run_data INI_FILE [PRESET_LEN]\n");
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

static int get_number_channels() {
    int status;
    unsigned int m;

    char module[MAXALIAS_LEN];
    int channel_per_module = 0;

    unsigned int number_modules = 0;
    unsigned int number_channels = 0;

    status = xiaGetNumModules(&number_modules);
    CHECK_ERROR(status);

    for (m = 0; m < number_modules; m++) {
        status = xiaGetModules_VB(m, module);
        CHECK_ERROR(status);

        status = xiaGetModuleItem(module, "number_of_channels", &channel_per_module);
        CHECK_ERROR(status);

        number_channels += channel_per_module;
    }

    return number_channels;
}
