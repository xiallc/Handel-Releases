/*
 * Tests for Mercury OEM features
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
static void print_usage(void);
static void start_system(char* ini_file);
static void setup_logging(char* log_name);
static void INThandler(int sig);
static void clean_up();

/* local helper */
static void get_and_print_dspparam(char* parameter_name);
static void check_mercury_oem_features();

/* Test and example usage */
static void test_preamp_gain();
static void test_rc_decay_and_calibration();
static void test_adc_settings();
static void test_baseline_factor();

boolean_t stop = FALSE_;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        exit(1);
    }

    /* Trap SIGINT to ensure clean up */
    signal(SIGINT, INThandler);

    printf("-- Tests for Mercury OEM\n"
           "-- Press CTRL+C to stop\n");

    setup_logging("handel.log");
    start_system(argv[1]);

    check_mercury_oem_features();

    test_baseline_factor();
    test_preamp_gain();
    test_adc_settings();
    test_rc_decay_and_calibration();

    clean_up();
    return 0;
}

static void start_system(char* ini_file) {
    int status;

    printf("Loading the .ini file\n");
    status = xiaInit(ini_file);
    CHECK_ERROR(status);

    /* Boot hardware */
    printf("Starting up the hardware\n");
    status = xiaStartSystem();
    CHECK_ERROR(status);
}

static void setup_logging(char* log_name) {
    printf("Configuring the log file in %s\n", log_name);

    /* log level defined in md_generic.h */
    xiaSetLogLevel(MD_DEBUG);
    xiaSetLogOutput(log_name);
}

/*
 * Check return code for error, stop execution in case of error
 */
static void CHECK_ERROR(int status) {
    /* error codes defined in handel_errors.h */
    if (status != XIA_SUCCESS) {
        printf("Error encountered! Status = %d, %s\n", status, xiaGetErrorText(status));
        clean_up();
        exit(status);
    }
}

static void INThandler(int sig) {
    UNUSED(sig);
    stop = TRUE_;

    clean_up();
    exit(1);
}

/*
 * Clean up and release resources
 *
 */
static void clean_up() {
    printf("\nCleaning up Handel.\n");
    xiaExit();

    printf("Closing the Handel log file.\n");
    xiaCloseLog();
}

static void print_usage(void) {
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
static void check_mercury_oem_features() {
    int status;
    unsigned long features;

    char moduleType[200];

    status = xiaGetModuleItem("module1", "module_type", (void*) moduleType);
    CHECK_ERROR(status);

    printf("Checking %s features\n", moduleType);

    /* Only applicable to Mercury */
    if (strcmp(moduleType, "mercury") != 0)
        return;

    status = xiaBoardOperation(0, "get_board_features", &features);
    CHECK_ERROR(status);

    /* Feature list constants in handel_constants.h */
    printf(" : Support for mercury oem features - [%s]\n",
           (features & 1 << BOARD_SUPPORTS_MERCURYOEM_FEATURES) ? "YES" : "NO");
}

/*
 * Read a DSP parameter and print a formatted number to stdout
 * only needed for internal diagnostic and testing
 */
static void get_and_print_dspparam(char* parameter_name) {
    int status;
    char formatstring[24];
    parameter_t PARAMETER;

    status = xiaGetParameter(0, parameter_name, &PARAMETER);
    CHECK_ERROR(status);

    sprintf(formatstring, "%%%zi, ", strlen(parameter_name));
    printf(formatstring, PARAMETER);
}

/*
 * Baseline factor setting
 * only needed for internal diagnostic and testing
 */
static void test_baseline_factor() {
    int status;
    int i, j;
    int ignored = 0;

    /* All possible baseline_factor values */
    const int nbr_baseline_factor = 2;
    double baseline_factor[2] = {0., 1.};

    /* Random list of peaking_peaking times */
    const int nbr_peaking_time = 3;
    double peaking_times[3] = {0.1, 0.3, 3.1};

    double peaking_time;
    double baseline_factor_readout;

    printf("\nMercury OEM modifiy peaking_time and check baseline_factor.\n");
    printf(""
           "peaking_time, "
           "acutal pt, "
           "baseline_factor, "
           "SLOWLEN, "
           "SLOWGAP, "
           "PEAKINT, "
           "BFACTOR "
           "\n");

    /* Cycle through a few peaking_time and check baseline_factor values */
    for (i = 0; i < nbr_peaking_time; i++) {
        peaking_time = peaking_times[i];
        printf("%12.3f, ", peaking_time);

        status = xiaSetAcquisitionValues(0, "peaking_time", &peaking_time);
        CHECK_ERROR(status);

        status = xiaBoardOperation(0, "apply", &ignored);
        CHECK_ERROR(status);

        status =
            xiaGetAcquisitionValues(0, "baseline_factor", &baseline_factor_readout);
        CHECK_ERROR(status);

        printf("%9.3f, %15.3f, ", peaking_time, baseline_factor_readout);

        /* Check corresponding DSP parameter to verify */
        get_and_print_dspparam("SLOWLEN");
        get_and_print_dspparam("SLOWGAP");
        get_and_print_dspparam("PEAKINT");
        get_and_print_dspparam("BFACTOR");

        printf("\n");
    }

    printf("\nMercury OEM modifiy baseline_factor and check filter parameter.\n");
    printf(""
           "peaking_time, "
           "acutal pt, "
           "baseline_factor, "
           "SLOWLEN, "
           "SLOWGAP, "
           "PEAKINT, "
           "BFACTOR "
           "\n");

    for (i = 0; i < nbr_peaking_time; i++) {
        peaking_time = peaking_times[i];

        status = xiaSetAcquisitionValues(0, "peaking_time", &peaking_time);
        CHECK_ERROR(status);

        status = xiaBoardOperation(0, "apply", &ignored);
        CHECK_ERROR(status);

        for (j = 0; j < nbr_baseline_factor; j++) {
            printf("%12.3f, ", peaking_times[i]);

            status = xiaSetAcquisitionValues(0, "baseline_factor", &baseline_factor[j]);
            CHECK_ERROR(status);

            status = xiaBoardOperation(0, "apply", &ignored);
            CHECK_ERROR(status);

            status = xiaGetAcquisitionValues(0, "peaking_time", &peaking_time);
            CHECK_ERROR(status);

            printf("%9.3f, %15.3f, ", peaking_time, baseline_factor[j]);

            /* Check corresponding DSP parameter to verify */
            get_and_print_dspparam("SLOWLEN");
            get_and_print_dspparam("SLOWGAP");
            get_and_print_dspparam("PEAKINT");
            get_and_print_dspparam("BFACTOR");

            printf("\n");
        }
    }
}

/*
 * preamp_gain implementation
 */
static void test_preamp_gain() {
    int status;
    int ignored = 0;
    int i;

    /* All possible input_attenuation values */
    const int nbr_input_attenuation = 3;
    double input_attenuation[3] = {2., 1., 0.};

    double preamp_gain[3] = {1.0, 2.5, 5.0};
    double dynamic_range[3] = {47200., 20000., 40000.};
    double mca_bin_width[3] = {20., 15., 10.};

    signed short MCAGAINEXP;

    printf("\nMercury OEM switched gain setting\n");
    printf(""
           "input_attenuation, "
           "preamp_gain, "
           "dynamic_range, "
           "mca_bin_width, "
           "SWGAIN, "
           "MCAGAIN, "
           "MCAGAINEXP"
           "\n");

    /* Cycle through all possible input_attenuation values, check gain settings */
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

        printf("%17.0f, %11.3f, %13.3f, %13.3f,", input_attenuation[i], preamp_gain[i],
               dynamic_range[i], mca_bin_width[i]);

        /* Check corresponding DSP parameter to verify */
        get_and_print_dspparam("SWGAIN");
        get_and_print_dspparam("MCAGAIN");

        status = xiaGetParameter(0, "MCAGAINEXP", (unsigned short*) &MCAGAINEXP);
        CHECK_ERROR(status);

        printf("%10hi\r\n", MCAGAINEXP);
    }
}

/*
 * rc_time settings and calibration
 */
static void test_rc_decay_and_calibration() {
    int status;
    int ignored = 0;
    int i;

    /* Possible rc_time_constant */
    const int nbr_rc_time_constant = 7;

    double rc_time_constant = 0.0;
    double rc_time = 0.0;
    double peaking_time = 0.0;

    printf("\nRC decay setting\n");
    printf("rc_time_constant, "
           "rc_time, "
           "TAUCTRL, "
           "RCTAU, "
           "RCTAUFRAC, "
           "\n");

    /* Cycle through all possible rc_time_constant values */
    for (i = 0; i < nbr_rc_time_constant; i++) {
        rc_time_constant = (double) i;

        /* Setting rc_time_constant should set rc_time to a nominal value */
        status = xiaSetAcquisitionValues(0, "rc_time_constant", &rc_time_constant);
        CHECK_ERROR(status);

        status = xiaBoardOperation(0, "apply", &ignored);
        CHECK_ERROR(status);

        status = xiaGetAcquisitionValues(0, "rc_time", &rc_time);
        CHECK_ERROR(status);
        printf("%16.0f, %7.3f,", rc_time_constant, rc_time);

        /* Check corresponding DSP parameter to verify */
        get_and_print_dspparam("TAUCTRL");
        get_and_print_dspparam("RCTAU");
        get_and_print_dspparam("RCTAUFRAC");
        printf("\n");
    }

    status = xiaGetAcquisitionValues(0, "peaking_time", &peaking_time);

    printf("\nCheck rc_time after calibrate_rc_time, peaking_time = %.2f\n",
           peaking_time);
    printf("rc_time_constant, "
           "rc_time, "
           "TAUCTRL, "
           "RCTAU, "
           "RCTAUFRAC, "
           "SETRCTAU, "
           "SETRCTAUFRAC, "
           "\n");

    /* Cycle through all possible rc_time_constant values, check calibration */
    for (i = 0; i < nbr_rc_time_constant; i++) {
        rc_time_constant = (double) i;

        /* Setting rc_time_constant should set rc_time to a nominal value */
        status = xiaSetAcquisitionValues(0, "rc_time_constant", &rc_time_constant);
        CHECK_ERROR(status);

        status = xiaBoardOperation(0, "apply", &ignored);
        CHECK_ERROR(status);

        printf("%16.0f, ", rc_time_constant);

        /* Run the calibrate RC special run to check calibrate rc_time */
        status = xiaDoSpecialRun(0, "calibrate_rc_time", NULL);
        CHECK_ERROR(status);

        status = xiaGetAcquisitionValues(0, "rc_time", &rc_time);
        CHECK_ERROR(status);

        printf("%7.3f, ", rc_time);

        /* Check corresponding DSP parameter to verify */
        get_and_print_dspparam("TAUCTRL");
        get_and_print_dspparam("RCTAU");
        get_and_print_dspparam("RCTAUFRAC");
        get_and_print_dspparam("SETRCTAU");
        get_and_print_dspparam("SETRCTAUFRAC");
        printf("\n");
    }
}

/*
 * New ADC trace features, only needed for internal diagnostic and testing
 */
static void test_adc_settings() {
    int i;
    int status;

    double trace_trigger_position = 128.0;
    double trace_trigger_type = 2.0;

    double adc_offset, offset_dac;

    /* A random list of trace wait */
    const int nbr_trace_wait = 3;
    double info[3] = {234.0, 1024.0, 2055.0};

    double traceinfo[2] = {0, 16384.};

    /* All possible trace types */
    const int nbr_trace_type = 5;
    char* tracetypes[5] = {"adc_trace", "adc_average", "debug", "fast_filter",
                           "raw_intermediate_filter"};

    printf("\nADC settings\n");

    status = xiaSetAcquisitionValues(0, "trace_trigger_type", &trace_trigger_type);
    CHECK_ERROR(status);

    status =
        xiaSetAcquisitionValues(0, "trace_trigger_position", &trace_trigger_position);
    CHECK_ERROR(status);

    printf("adjust_offsets, "
           "adc_offset, "
           "offset_dac"
           "SETOFFADC, "
           "SETODAC, "
           "\n");

    /* Run the adjust_offsets special run */
    for (i = 0; i < nbr_trace_wait; i++) {
        status = xiaDoSpecialRun(0, "adjust_offsets", &info[i]);
        CHECK_ERROR(status);

        status = xiaGetAcquisitionValues(0, "adc_offset", &adc_offset);
        CHECK_ERROR(status);

        status = xiaGetAcquisitionValues(0, "offset_dac", &offset_dac);
        CHECK_ERROR(status);

        printf("%14.0f, %10.0f, %10.0f", info[i], adc_offset, offset_dac);

        /* Check corresponding DSP parameter to verify */
        get_and_print_dspparam("SETOFFADC");
        get_and_print_dspparam("SETODAC");

        printf("\n");
    }

    printf("\nCheck trace special runs\n");
    printf("%20s%10s, %9s, %9s\n", "Trace type", "", "TRACETYPE", "TRACEWAIT");

    /* Do trace special runs */
    for (i = 0; i < nbr_trace_type; i++) {
        status = xiaDoSpecialRun(0, tracetypes[i], traceinfo);
        CHECK_ERROR(status);

        printf("%30s, ", tracetypes[i]);

        /* Check corresponding DSP parameter to verify */
        get_and_print_dspparam("TRACETYPE");
        get_and_print_dspparam("TRACEWAIT");

        printf("\n");

        /* TODO this scaling will be removed in future versions of Handel */
        traceinfo[1] /= 1.0e-9;
    }
}
