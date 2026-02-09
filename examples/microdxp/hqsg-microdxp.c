/* SPDX-License-Identifier: Apache-2.0 */

/*
 * Copyright Monday, February 9, 2026 XIA LLC, All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/** @file hqsg-microdxp.c
 * @brief Provides example functionality for the microDXP device.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <handel.h>
#include <handel_errors.h>
#include <md_generic.h>
#include "util/xia_sleep.h"

static void xia_cleanup(const int status) {
    xiaExit();
    xiaCloseLog();
    exit(status);
}

/**
 * This is just an example of how to handle error values.  A program
 * of any reasonable size should implement a more robust error
 * handling mechanism.
 */
static void xia_error_check(const int status) {
    if (status != XIA_SUCCESS) {
        const char* msg = xiaGetErrorText(status);
        printf("[ERROR] %d: %s \n", status, msg);
        xia_cleanup(status);
    }
}

static void CHECK_MEM(void* mem) {
    if (mem == NULL) {
        printf("Memory allocation failed\n");
        xia_cleanup(EXIT_FAILURE);
    }
}

static void get_serial_number(const int* mod_num) {
    unsigned char serial_number[17];
    int status = xiaBoardOperation(*mod_num, "get_serial_number", serial_number);
    xia_error_check(status);
    printf("hardware:\n");
    printf("    variant: %c%c\n", serial_number[3], serial_number[4]);
    printf("    revision: %c%c\n", serial_number[5], serial_number[6]);
    printf("    batch:\n"
           "        week: %c%c\n"
           "        year: %c%c\n",
           serial_number[7], serial_number[8], serial_number[9], serial_number[10]);
    printf("    sn: \'");
    for (unsigned int i = 11; i < 16; i++) {
        printf("%c", serial_number[i]);
    }
    printf("\'\n");
}

static void get_usb_version(const int* mod_num) {
    unsigned long usb_version = 0;
    short rev;
    short major;
    short minor;
    short patch;
    int status;

    status = xiaBoardOperation(*mod_num, "get_usb_version", &usb_version);
    xia_error_check(status);
    rev = usb_version & 0xFF;
    patch = (usb_version >> 8) & 0xFF;
    minor = (usb_version >> 16) & 0xFF;
    major = (usb_version >> 24) & 0xFF;
    printf("usb:\n    version: %u.%u.%u.%u\n", major, minor, patch, rev);
}

static void get_fippi_variant() {
    unsigned short fippi_var;
    xia_error_check(xiaGetParameter(0, "FIPPIVAR", &fippi_var));
    printf("    variant: %hhu\n", fippi_var);
}

static void get_fippi_version() {
    unsigned short fippi_rev;
    short major;
    short minor;
    short patch;
    xia_error_check(xiaGetParameter(0, "FIPPIREV", &fippi_rev));
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

    xia_error_check(xiaGetParameter(0, "CODEREV", &dsp_rev));
    major = (dsp_rev >> 12) & 0xF;
    minor = (dsp_rev >> 8) & 0x0F;
    patch = dsp_rev & 0xFF;
    printf("    coderev: %u.%u.%u\n", major, minor, patch);
}

static void generate_system_report(const int* mod_num) {
    int status;
    unsigned char board_info[26];
    unsigned short gain_base;
    double gain;

    get_serial_number(mod_num);
    get_usb_version(mod_num);

    status = xiaBoardOperation(*mod_num, "get_board_info", &board_info);
    xia_error_check(status);
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
    xia_error_check(status);

    xiaSetIOPriority(MD_IO_PRI_HIGH);

    status = xiaStartSystem();
    xia_error_check(status);

    unsigned int num_mods = 0;
    xiaGetNumModules(&num_mods);

    for (int mod_num = 0; mod_num < num_mods; mod_num++) {
        /* Print system information to the terminal */
        printf("******** Begin Device %i Report ************\n", mod_num);
        generate_system_report(&mod_num);
        printf("******** End Device %i Report ***********\n", mod_num);

        /* Modify some acquisition values */
        status = xiaSetAcquisitionValues(mod_num, "number_mca_channels", &nMCA);
        xia_error_check(status);

        status = xiaSetAcquisitionValues(mod_num, "trigger_threshold", &thresh);
        xia_error_check(status);

        status = xiaSetAcquisitionValues(mod_num, "polarity", &polarity);
        xia_error_check(status);

        status = xiaSetAcquisitionValues(mod_num, "gain", &gain);
        xia_error_check(status);

        /* Apply changes to parameters */
        status = xiaBoardOperation(mod_num, "apply", (void*) &ignored);

        /* Save the settings to the current GENSET and PARSET */
        status = xiaGetAcquisitionValues(mod_num, "genset", &currentGENSET);
        xia_error_check(status);

        status = xiaGetAcquisitionValues(mod_num, "parset", &currentPARSET);
        xia_error_check(status);

        GENSET = (unsigned short) currentGENSET;
        status = xiaBoardOperation(mod_num, "save_genset", &GENSET);
        xia_error_check(status);

        PARSET = (unsigned short) currentPARSET;
        status = xiaBoardOperation(mod_num, "save_parset", &PARSET);
        xia_error_check(status);

        /* Read out number of peaking times to pre-allocate peaking time array */
        status =
            xiaBoardOperation(mod_num, "get_number_pt_per_fippi", &numberPeakingTimes);
        xia_error_check(status);

        peakingTimes = (double*) malloc(numberPeakingTimes * sizeof(double));
        CHECK_MEM(peakingTimes);

        status = xiaBoardOperation(mod_num, "get_current_peaking_times", peakingTimes);
        xia_error_check(status);
        free(peakingTimes);

        /* Read out number of fippis to pre-allocate peaking time array */
        status = xiaBoardOperation(mod_num, "get_number_of_fippis", &numberFippis);
        xia_error_check(status);

        peakingTimes =
            (double*) malloc(numberPeakingTimes * numberFippis * sizeof(double));
        CHECK_MEM(peakingTimes);

        status = xiaBoardOperation(mod_num, "get_peaking_times", peakingTimes);
        xia_error_check(status);
        free(peakingTimes);

        /* Start a run w/ the MCA cleared */
        status = xiaStartRun(mod_num, 0);
        xia_error_check(status);

        printf("Started run. Sleeping...\n");
        xia_sleep(1000);

        status = xiaStopRun(mod_num);
        xia_error_check(status);

        /* Prepare to read out MCA spectrum */
        status = xiaGetRunData(mod_num, "mca_length", &mcaLen);
        xia_error_check(status);

        if (mcaLen > 0) {
            printf("Got run data\n");
        }

        /*
         * If you don't want to dynamically allocate memory here,
         * then be sure to declare mca as an array of length 8192,
         * since that is the maximum length of the spectrum.
         */
        mca = (unsigned long*) malloc(mcaLen * sizeof(unsigned long));
        CHECK_MEM(mca);

        status = xiaGetRunData(mod_num, "mca", (void*) mca);
        xia_error_check(status);

        /* Display the spectrum, write it to a file, etc... */

        xiaSetIOPriority(MD_IO_PRI_NORMAL);

        free(mca);
    }

    status = xiaExit();
    xia_error_check(status);
    xiaCloseLog();

    return EXIT_SUCCESS;
}
