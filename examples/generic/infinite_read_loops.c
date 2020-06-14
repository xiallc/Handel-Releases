/*
 * Generic test to loop on data read
 *
 * Copyright (c) 2005-2019 XIA LLC
 * All rights reserved
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#if _WIN32
#pragma warning(disable : 4115)
#include <windows.h> /* For Sleep() */
#else
#include <time.h>
#endif

#include "handel.h"
#include "handel_errors.h"
#include "handel_generic.h"
#include "md_generic.h"

static void CHECK_ERROR(int status);
static int SLEEP(float seconds);
static void INThandler(int sig);
static void print_usage(void);
static void start_system(char *ini_file);
static void setup_logging(char *log_name);
static void clean_up();
static int get_number_channels();

unsigned long *mca = NULL;
unsigned short *params = NULL;

boolean_t stop = FALSE_;

int main(int argc, char *argv[])
{
    char *ini;
    int status;
    float runtime = 0.02f;

    int channel;
    int number_channels = 0;
    unsigned long loops = 0;

    unsigned long mca_length;
    double mca_channels = 8192;
    double statistics[9];

    if (argc < 2) {
        print_usage();
        exit(1);
    }

    ini = argv[1];

    if (argc > 2) {
        runtime = (float)atof(argv[2]);
    }

    signal(SIGINT, INThandler);

    printf("\n");
    setup_logging("handel.log");
    start_system(ini);

    number_channels = get_number_channels();
    status = xiaSetAcquisitionValues(-1, "number_mca_channels", &mca_channels);

    status = xiaGetRunData(0, "mca_length", &mca_length);
    CHECK_ERROR(status);

    mca = malloc(mca_length * sizeof(unsigned long));
    if (!mca) {
        printf("Failed allocating %lu words for MCA\n", mca_length);
        clean_up();
        exit(2);
    }

    printf("Starting run loop wait time %.4fs\n", runtime);
    printf("Press CTRL+C to stop the program\n");

    fflush(stdout);

    status = xiaStartRun(-1, 0);
    CHECK_ERROR(status);

    while(!stop)
    {
        for (channel = 0; channel < number_channels; channel++) {
            /*status = xiaGetRunData(channel, "mca", mca);
            CHECK_ERROR(status);*/

            status = xiaGetRunData(channel, "module_statistics_2", statistics);
            CHECK_ERROR(status);
            if (statistics[0] <= 0)
                stop = TRUE_;
        }

        loops++;
        printf("\rLoop -- %lu", loops);
        fflush(stdout);
        SLEEP(runtime);
    }

    printf("\nStopping run");

    status = xiaStopRun(-1);
    CHECK_ERROR(status);

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

    printf("Loading ini file %s\n", ini_file);
    status = xiaInit(ini_file);
    CHECK_ERROR(status);

    /* Boot hardware */
    status = xiaStartSystem();
    CHECK_ERROR(status);
}

static void setup_logging(char *log_name)
{
    printf("Configuring Handel log file %s\n", log_name);
    xiaSetLogLevel(MD_DEBUG);
    xiaSetLogOutput(log_name);
}

static void clean_up()
{
    printf("\nCleaning up Handel\n");
    xiaExit();

    printf("Closing Handel log file\n");
    xiaCloseLog();

    if (mca)
        free(mca);
}


/*
 * This is just an example of how to handle error values.  A program
 * of any reasonable size should implement a more robust error
 * handling mechanism.
 */
static void CHECK_ERROR(int status)
{
    /* XIA_SUCCESS is defined in handel_errors.h */
    if (status != XIA_SUCCESS) {
        printf("\nError encountered! Status = %d %s\n", status, xiaGetErrorText(status));
        clean_up();
        exit(status);
    }
}

static void print_usage(void)
{
    fprintf(stdout, "\n");
    fprintf(stdout, "Usage: infinite_read_loops INI_FILE [LOOP_WAIT_SECOND]\n");
    fprintf(stdout, "\n");
    return;
}

static int SLEEP(float seconds)
{
#if _WIN32
    DWORD wait = (DWORD)(1000.0 * seconds);
    Sleep(wait);
#else
    unsigned long secs = (unsigned long) seconds;
    struct timespec req = {
        .tv_sec = secs,
        .tv_nsec = ((seconds - secs) * 1000000000.0)
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
    return XIA_SUCCESS;
}

static int get_number_channels()
{
    int status;
    unsigned int m;

    char module[MAXALIAS_LEN];
    int channel_per_module = 0;

    unsigned int number_modules =  0;
    unsigned int number_channels  = 0;

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

