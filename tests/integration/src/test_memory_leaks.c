/*
 * handel memory leak test
 * Use Visual Leak Detector to check for memory leaks in common operations
 * Generate a log file with memory leak information in the current path
 *
 * Copyright (c) 2005-2017 XIA LLC
 * All rights reserved
 *
 */

#ifdef __VLD_MEM_DBG__
#include <vld.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#pragma warning(disable : 4115)

/* For Sleep() */
#include <windows.h>

#include "handel.h"
#include "handel_errors.h"
#include "md_generic.h"

static void CHECK_ERROR(int status);

static void print_usage(void);
static void do_parameter();
static void do_run(int runtime);
static void do_mca(int runtime);
static void do_sca(int runtime);
static void do_mapping(unsigned long nMapPixels);
static void start_system(char* ini_file);

int main(int argc, char* argv[]) {
    int i;

    if (argc < 2) {
        print_usage();
        exit(1);
    }

    /* Setup logging here */
    printf("Configuring the Handel log file.\n");
    xiaSetLogLevel(MD_DEBUG);
    xiaSetLogOutput("handel.log");

    for (i = 1; i < argc; i++) {
        start_system(argv[i]);

        do_parameter();
        do_mca(1000);
        do_sca(1000);
        do_mapping(0xF);

        /* Check that restarting system works without memory failure */
        CHECK_ERROR(xiaExit());
        start_system(argv[i]);

        printf("Cleaning up Handel.\n");
        xiaExit();
    }

    xiaCloseLog();

    return 0;
}

static void start_system(char* ini_file) {
    int status;
    int ignored = 0;

    /* Acquisition Values */
    double pt = 16.0;

    printf("Loading the .ini file.\n");
    status = xiaInit(ini_file);
    CHECK_ERROR(status);

    /* Boot hardware */
    printf("Starting up the hardware.\n");
    status = xiaStartSystem();
    CHECK_ERROR(status);

    /* Configure acquisition values, ignore error for unsupported device */
    printf("Setting the acquisition values.\n");
    status = xiaSetAcquisitionValues(-1, "peaking_time", &pt);

    /* Apply new acquisition values */
    printf("Applying the acquisition values.\n");
    status = xiaBoardOperation(0, "apply", &ignored);
    CHECK_ERROR(status);
}

static void do_parameter() {
    int status;

    unsigned short numParams = 0;
    unsigned short* paramData = NULL;

    printf("Readout DSP paramters.\n");
    status = xiaGetNumParams(0, &numParams);
    CHECK_ERROR(status);

    printf("Allocating memory for the parameter data.\n");
    paramData = malloc(numParams * sizeof(unsigned short));

    if (!paramData) {
        /* Error allocating memory */
        exit(1);
    }

    status = xiaGetParamData(0, "values", paramData);
    CHECK_ERROR(status);

    free(paramData);
}

static void do_run(int runtime) {
    int status;

    /* Start a run w/ the MCA cleared */
    printf("Starting the run.\n");
    status = xiaStartRun(-1, 0);
    CHECK_ERROR(status);

    printf("Waiting %d ms to collect data.\n", runtime);
    Sleep((DWORD) runtime);

    printf("Stopping the run.\n");
    status = xiaStopRun(-1);
    CHECK_ERROR(status);
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
        xiaExit();
        exit(status);
    }
}

/*
 * Do an MCA run then read out MCA data
 */
static void do_mca(int runtime) {
    int status;

    unsigned long mcaLen = 0;
    unsigned long* mca = NULL;

    do_run(runtime);

    /* Prepare to read out MCA spectrum */
    printf("Getting MCA length.\n");
    status = xiaGetRunData(0, "mca_length", &mcaLen);
    CHECK_ERROR(status);

    /* If you don't want to dynamically allocate memory here,
     * then be sure to declare mca as an array of length 8192,
     * since that is the maximum length of the spectrum.
     */
    printf("Allocating memory for the MCA data.\n");
    mca = malloc(mcaLen * sizeof(unsigned long));

    if (!mca) {
        /* Error allocating memory */
        exit(1);
    }

    printf("Reading MCA data.\n");
    status = xiaGetRunData(0, "mca", mca);
    CHECK_ERROR(status);

    /* Display the spectrum, write it to a file, etc... */

    printf("Release MCA memory.\n");
    free(mca);
}

/*
 * A fairly thorough SCA operation which sets a few SCA regions across the
 * entires spectrum, do a run, then read out the SCA data.
 */
static void do_sca(int runtime) {
    int i;
    int status;
    int ignored = 0;

    /* SCA Settings. sca_values must match number_scas */
    double number_scas = 4.0;
    double sca_values[4];

    double sca_bound = 0.0;
    double sca_size = 0.0;
    double number_mca_channels = 0.0;

    char scaStr[8];

    /* Set the number of SCAs */
    printf("Set SCAs\n");
    status = xiaSetAcquisitionValues(0, "number_of_scas", (void*) &number_scas);
    CHECK_ERROR(status);

    printf("Number of SCAs %0.0f\n", number_scas);

    /* Divide the entire spectrum region into equal number of SCAs */
    status =
        xiaGetAcquisitionValues(0, "number_mca_channels", (void*) &number_mca_channels);
    CHECK_ERROR(status);

    sca_bound = 0.0;
    sca_size = (int) (number_mca_channels / number_scas);

    /* Set the individual SCA limits */
    for (i = 0; i < (unsigned short) number_scas; i++) {
        sprintf(scaStr, "sca%d_lo", i);

        status = xiaSetAcquisitionValues(0, scaStr, (void*) &sca_bound);
        CHECK_ERROR(status);

        printf("  %0.0f,", sca_bound);
        sca_bound += sca_size;

        sprintf(scaStr, "sca%d_hi", i);
        status = xiaSetAcquisitionValues(0, scaStr, (void*) &sca_bound);
        CHECK_ERROR(status);

        printf("%0.0f\n", sca_bound);
    }

    /* Apply new acquisition values */
    status = xiaBoardOperation(0, "apply", &ignored);
    CHECK_ERROR(status);

    do_run(runtime);

    printf("Read out the SCA values\n");

    /* Read out the SCAs from the data buffer */
    status = xiaGetRunData(0, "sca", (void*) sca_values);
    if (status != XIA_SUCCESS) {
        return;
    }

    for (i = 0; i < (int) number_scas; i++) {
        printf(" SCA%d = %0f\n", i, sca_values[i]);
    }
}
static void do_mapping(unsigned long nMapPixels) {
    int status;
    double mappingmode = 1.0;

    double pixPerBuffer = 2.0;
    double mcachannels = 1024.0;
    double pixelAdvanceMode = 1.0;

    unsigned short isFull = 0;
    int ignored = 0;

    unsigned long curPixel = 0;
    unsigned long bufLen = 0;
    unsigned long* databuffer = NULL;

    char curBuffer = 'a';

    char bufferfull[15];
    char buffername[9];

    /* Do mapping loop if supported */
    status = xiaSetAcquisitionValues(0, "mapping_mode", &mappingmode);
    if (status != XIA_SUCCESS) {
        return;
    }

    CHECK_ERROR(xiaBoardOperation(0, "apply", &ignored));
    CHECK_ERROR(xiaSetAcquisitionValues(-1, "pixel_advance_mode", &pixelAdvanceMode));
    CHECK_ERROR(xiaSetAcquisitionValues(-1, "number_mca_channels", &mcachannels));
    CHECK_ERROR(
        xiaSetAcquisitionValues(-1, "num_map_pixels_per_buffer", &pixPerBuffer));
    CHECK_ERROR(xiaBoardOperation(0, "apply", &ignored));

    CHECK_ERROR(xiaGetRunData(0, "buffer_len", (void*) &bufLen));

    databuffer = (unsigned long*) malloc(bufLen * sizeof(unsigned long));

    printf("Starting mapping loop buffer length %lu.\n", bufLen);
    CHECK_ERROR(xiaStartRun(-1, 0));

    /* Simulate pixel advance by using mapping_pixel_next at every loop. */
    do {
        sprintf(bufferfull, "buffer_full_%c", curBuffer);
        sprintf(buffername, "buffer_%c", curBuffer);

        isFull = 0;
        while (!isFull) {
            CHECK_ERROR(xiaBoardOperation(0, "mapping_pixel_next", &ignored));
            CHECK_ERROR(xiaGetRunData(0, bufferfull, &isFull));
        }

        CHECK_ERROR(xiaGetRunData(0, buffername, (void*) databuffer));
        CHECK_ERROR(xiaBoardOperation(0, "buffer_done", &curBuffer));
        CHECK_ERROR(xiaGetRunData(0, "current_pixel", (void*) &curPixel));

        if (curBuffer == 'a')
            curBuffer = 'b';
        else
            curBuffer = 'a';
    } while (curPixel < nMapPixels);

    CHECK_ERROR(xiaStopRun(-1));
    free(databuffer);
}

static void print_usage(void) {
    fprintf(stdout, "\n");
    fprintf(stdout, "**********************************************************\n");
    fprintf(stdout, "* Memory leak detection test program for Handel library. *\n");
    fprintf(stdout, "* Run from staging folder with argument: [.ini file]     *\n");
    fprintf(stdout, "**********************************************************\n");
    fprintf(stdout, "\n");
    return;
}
