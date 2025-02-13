/*
 * This code accompanies the XIA Application Note
 * "Handel Quick Start Guide: microDXP".
 *
 * Copyright (c) 2004-2015 XIA LLC
 * All rights reserved
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#pragma warning(disable : 4115)

/* For Sleep() */
#include <windows.h>
#else
#include <time.h>
#endif

#include "handel.h"
#include "handel_errors.h"
#include "md_generic.h"

/**
 * @brief Cleanly exits the system when an error is encountered.
 */
static void clean_exit(int exit_code) {
    xiaExit();
    xiaCloseLog();
    exit(exit_code);
}

/**
 * This is an example of how to handle error values. In your program
 * it is likely that you will want to do something more robust than
 * just exit the program.
 */
static void CHECK_ERROR(int status) {
    /* XIA_SUCCESS is defined in handel_errors.h */
    if (status != XIA_SUCCESS) {
        fprintf(stderr, "Error encountered! Status = %d\n", status);
        clean_exit(status);
    }
}

static void CHECK_MEM(void* mem) {
    if (mem == NULL) {
        printf("Memory allocation failed\n");
        clean_exit(EXIT_FAILURE);
    }
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

static void get_serial_number() {
    int i;
    unsigned char serial_number[17];
    int status;

    status = xiaBoardOperation(0, "get_serial_number", serial_number);
    CHECK_ERROR(status);
    printf("hardware:\n");
    printf("    variant: %c%c\n", serial_number[3], serial_number[4]);
    printf("    revision: %c%c\n", serial_number[5], serial_number[6]);
    printf("    batch:\n"
           "        week: %c%c\n"
           "        year: %c%c\n",
           serial_number[7], serial_number[8], serial_number[9], serial_number[10]);
    printf("    sn: \'");
    for (i = 11; i < 17; i++) {
        printf("%c", serial_number[i]);
    }
    printf("\'\n");
}

static void get_usb_version() {
    unsigned long usb_version = 0;
    short rev;
    short major;
    short minor;
    short patch;
    int status;

    status = xiaBoardOperation(0, "get_usb_version", &usb_version);
    CHECK_ERROR(status);
    rev = usb_version & 0xFF;
    patch = (usb_version >> 8) & 0xFF;
    minor = (usb_version >> 16) & 0xFF;
    major = (usb_version >> 24) & 0xFF;
    printf("usb:\n    version: %u.%u.%u.%u\n", major, minor, patch, rev);
}

static void get_fippi_variant() {
    unsigned short fippi_var;
    CHECK_ERROR(xiaGetParameter(0, "FIPPIVAR", &fippi_var));
    printf("    variant: %hhu\n", fippi_var);
}

static void get_fippi_version() {
    unsigned short fippi_rev;
    short major;
    short minor;
    short patch;
    CHECK_ERROR(xiaGetParameter(0, "FIPPIREV", &fippi_rev));
    major = (fippi_rev >> 12) & 0xF;
    minor = (fippi_rev >> 8) & 0x0F;
    patch = fippi_rev & 0xFF;
    printf("    version: %u.%u.%u\n", major, minor, patch);
}

static void get_dsp_version() {
    unsigned short dsp_rev;
    short major;
    short minor;
    short patch;

    CHECK_ERROR(xiaGetParameter(0, "CODEREV", &dsp_rev));
    major = (dsp_rev >> 12) & 0xF;
    minor = (dsp_rev >> 8) & 0x0F;
    patch = dsp_rev & 0xFF;
    printf("    coderev: %u.%u.%u\n", major, minor, patch);
}

static void generate_system_report() {
    int status;
    unsigned char board_info[26];
    unsigned short gain_base;
    double gain;

    get_serial_number();
    get_usb_version();

    status = xiaBoardOperation(0, "get_board_info", &board_info);
    CHECK_ERROR(status);
    printf("pic:\n"
           "    version: %hhu.%hhu.%hhu\n",
           board_info[0], board_info[1], board_info[2]);

    printf("dsp:\n"
           "    version: %hhu.%hhu.%hhu\n"
           "    clock_speed_mhz: %hhu\n",
           board_info[3], board_info[4], board_info[5], board_info[6]);
    get_dsp_version();

    gain_base = (board_info[11] << 16) | board_info[10];
    gain = (double) (gain_base / 32768.) * pow(2, (double) board_info[12]);
    printf("afe:\n"
           "    clock_enable: %hhu\n"
           "    gain:\n"
           "        mode: %hhu\n"
           "        value: %f\n",
           board_info[7], board_info[9], gain);
    printf("    nyquist_filter: %hhu\n"
           "    power_supply: %hhu\n"
           "    adc_speed_grade: %hhu\n",
           board_info[13], board_info[16], board_info[14]);
    printf("fpga:\n    speed: %hhu\n", board_info[15]);
    printf("fippi:\n    count: %hhu\n", board_info[8]);
    get_fippi_version();
    get_fippi_variant();
    printf("    fippi_0:\n"
           "        decimation: %hhu\n"
           "        variant: %hhu\n"
           "        decimation: %hhu\n",
           board_info[17], board_info[18], board_info[19]);
    printf("    fippi_1:\n"
           "        decimation: %hhu\n"
           "        variant: %hhu\n"
           "        decimation: %hhu\n",
           board_info[20], board_info[21], board_info[22]);
    printf("    fippi_2:\n"
           "        decimation: %hhu\n"
           "        variant: %hhu\n"
           "        decimation: %hhu\n",
           board_info[23], board_info[24], board_info[25]);
}

int main(int argc, char* argv[]) {
    int status = XIA_SUCCESS;
    int i;

    double nMCA = 4096.0;
    double thresh = 48.0;
    double polarity = 0.0;
    double gain = 4.077;

    unsigned short ignored = 0;
    double currentGENSET;
    double currentPARSET;
    unsigned short GENSET;
    unsigned short PARSET;

    unsigned short numberPeakingTimes = 0;
    double* peakingTimes = NULL;

    unsigned short numberFippis = 0;
    unsigned long mcaLen = 0;
    unsigned long* mca = NULL;

    if (argc != 2) {
        fprintf(stdout, "Arguments: [.ini file]]\n");
        EXIT_FAILURE;
    }

    printf("Configuring the Handel log file.\n");
    xiaSetLogLevel(MD_DEBUG);
    xiaSetLogOutput("handel.log");

    printf("Loading the .ini file.\n");
    status = xiaInit(argv[1]);
    CHECK_ERROR(status);

    xiaSetIOPriority(MD_IO_PRI_HIGH);

    status = xiaStartSystem();
    CHECK_ERROR(status);

    /* Print system information to the terminal */
    printf("******** Begin System Report ************\n");
    generate_system_report();
    printf("******** End System Report ***********\n");

    /* Modify some acquisition values */
    status = xiaSetAcquisitionValues(0, "number_mca_channels", &nMCA);
    CHECK_ERROR(status);

    status = xiaSetAcquisitionValues(0, "trigger_threshold", &thresh);
    CHECK_ERROR(status);

    status = xiaSetAcquisitionValues(0, "polarity", &polarity);
    CHECK_ERROR(status);

    status = xiaSetAcquisitionValues(0, "gain", &gain);
    CHECK_ERROR(status);

    /* Apply changes to parameters */
    status = xiaBoardOperation(0, "apply", (void*) &ignored);

    /* Save the settings to the current GENSET and PARSET */
    status = xiaGetAcquisitionValues(0, "genset", &currentGENSET);
    CHECK_ERROR(status);

    status = xiaGetAcquisitionValues(0, "parset", &currentPARSET);
    CHECK_ERROR(status);

    GENSET = (unsigned short) currentGENSET;
    status = xiaBoardOperation(0, "save_genset", &GENSET);
    CHECK_ERROR(status);

    PARSET = (unsigned short) currentPARSET;
    status = xiaBoardOperation(0, "save_parset", &PARSET);
    CHECK_ERROR(status);

    /* Read out number of peaking times to pre-allocate peaking time array */
    status = xiaBoardOperation(0, "get_number_pt_per_fippi", &numberPeakingTimes);
    CHECK_ERROR(status);

    peakingTimes = (double*) malloc(numberPeakingTimes * sizeof(double));
    CHECK_MEM(peakingTimes);

    status = xiaBoardOperation(0, "get_current_peaking_times", peakingTimes);
    CHECK_ERROR(status);

    /* Print out the current peaking times */
    for (i = 0; i < numberPeakingTimes; i++) {
        printf("peaking time %d = %lf\n", i, peakingTimes[i]);
    }

    free(peakingTimes);

    /* Read out number of fippis to pre-allocate peaking time array */
    status = xiaBoardOperation(0, "get_number_of_fippis", &numberFippis);
    CHECK_ERROR(status);

    peakingTimes = (double*) malloc(numberPeakingTimes * numberFippis * sizeof(double));
    CHECK_MEM(peakingTimes);

    status = xiaBoardOperation(0, "get_peaking_times", peakingTimes);
    CHECK_ERROR(status);

    /* Print out the current peaking times */
    for (i = 0; i < numberPeakingTimes * numberFippis; i++) {
        printf("peaking time %d = %lf\n", i, peakingTimes[i]);
    }

    free(peakingTimes);

    /* Start a run w/ the MCA cleared */
    status = xiaStartRun(0, 0);
    CHECK_ERROR(status);

    printf("Started run. Sleeping...\n");
    SLEEP(1);

    status = xiaStopRun(0);
    CHECK_ERROR(status);

    /* Prepare to read out MCA spectrum */
    status = xiaGetRunData(0, "mca_length", &mcaLen);
    CHECK_ERROR(status);

    if (mcaLen > 0) {
        printf("Got run data\n");
    }

    /**
     * If you don't want to dynamically allocate memory here,
     * then be sure to declare mca as an array of length 8192,
     * since that is the maximum length of the spectrum.
     */
    mca = (unsigned long*) malloc(mcaLen * sizeof(unsigned long));
    CHECK_MEM(mca);

    status = xiaGetRunData(0, "mca", (void*) mca);
    CHECK_ERROR(status);

    /* Display the spectrum, write it to a file, etc... */

    xiaSetIOPriority(MD_IO_PRI_NORMAL);

    free(mca);

    status = xiaExit();
    CHECK_ERROR(status);
    xiaCloseLog();

    return EXIT_SUCCESS;
}
