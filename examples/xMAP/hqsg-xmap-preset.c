/*
 * This code accompanies the XIA Application Note "Handel Quick Start
 * Guide: xMAP". This sample code acquires data for 5 seconds using a
 * preset realtime run that terminates automatically.
 *
 * Copyright (c) 2005-2014 XIA LLC
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, 
 * with or without modification, are permitted provided 
 * that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above 
 *     copyright notice, this list of conditions and the 
 *     following disclaimer.
 *   * Redistributions in binary form must reproduce the 
 *     above copyright notice, this list of conditions and the 
 *     following disclaimer in the documentation and/or other 
 *     materials provided with the distribution.
 *   * Neither the name of XIA LLC 
 *     nor the names of its contributors may be used to endorse 
 *     or promote products derived from this software without 
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND 
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 * IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE 
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR 
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF 
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
 * SUCH DAMAGE.
 *
 *
 */


#include <stdio.h>
#include <stdlib.h>

#pragma warning(disable : 4115)

/* For Sleep() */
#include <windows.h>

#include "handel.h"
#include "handel_constants.h"
#include "handel_errors.h"
#include "md_generic.h"


static int RUN_ACTIVE_BIT = 0x1;

static void print_usage(void);
static void CHECK_ERROR(int status);



int main(int argc, char *argv[])
{
    int status;
    int ignored = 0;
    int i;

    /* Acquisition Values */
    double pt = 16.0;
    double thresh = 1000.0;
    double calib = 5900.0;
    double dr = 47200.0;
    double mappingMode = 0.0;

    double presetRealtime = 5.0;
    double presetType = XIA_PRESET_FIXED_REAL;

    int nChannelsDone;

    unsigned long runActive;

    if (argc < 2) {
        print_usage();
        exit(1);
    }

    /* Setup logging here */
    printf("Configuring the Handel log file.\n");
    xiaSetLogLevel(MD_WARNING);
    xiaSetLogOutput("handel.log");

    printf("Loading the .ini file.\n");
    status = xiaInit(argv[1]);
    CHECK_ERROR(status);

    /* Boot hardware */
    printf("Starting up the hardware.\n");
    status = xiaStartSystem();
    CHECK_ERROR(status);

    /* Configure acquisition values */
    printf("Setting the acquisition values.\n");
    status = xiaSetAcquisitionValues(-1, "peaking_time", &pt);
    CHECK_ERROR(status);
    
    status = xiaSetAcquisitionValues(-1, "trigger_threshold", &thresh);
    CHECK_ERROR(status);

    status = xiaSetAcquisitionValues(-1, "calibration_energy", &calib);
    CHECK_ERROR(status);

    status = xiaSetAcquisitionValues(-1, "dynamic_range", &dr);
    CHECK_ERROR(status);

    status = xiaSetAcquisitionValues(-1, "preset_type", &presetType);
    CHECK_ERROR(status);

    status = xiaSetAcquisitionValues(-1, "preset_value", &presetRealtime);
    CHECK_ERROR(status);

    status = xiaSetAcquisitionValues(-1, "mapping_mode", &mappingMode);
    CHECK_ERROR(status);

    /* Apply new acquisition values */
    printf("Applying the acquisition values.\n");
    status = xiaBoardOperation(0, "apply", &ignored);
    CHECK_ERROR(status);

    /* Start a run w/ the MCA cleared */
    printf("Starting the run.\n");
    status = xiaStartRun(-1, 0);
    CHECK_ERROR(status);

    /* Poll waiting for the preset run to complete. */
    printf("Polling waiting for preset run to complete.\n");
    do {
        for (i = 0, nChannelsDone = 0; i < 4; i++) {
            status = xiaGetRunData(i, "run_active", &runActive);
            CHECK_ERROR(status);
            
            if ((runActive & RUN_ACTIVE_BIT) == 0) {
                nChannelsDone++;
            }
        }

        Sleep(1);
    } while (nChannelsDone != 4);

    printf("Stopping the run.\n");
    status = xiaStopRun(-1);
    CHECK_ERROR(status);

    /* Display the actual run length for each channel */
    for (i = 0; i < 4; i++) {
        double realtime;

        status = xiaGetRunData(i, "realtime", &realtime);
        CHECK_ERROR(status);

        printf("Elapsed run time channel %d = %0.2fs\n", i, realtime);
    }

    printf("Cleaning up Handel.\n");
    status = xiaExit();
    CHECK_ERROR(status);

    return 0;
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
        printf("Error encountered! Status = %d\n", status);
        getchar();
        exit(status);
    }
}



static void print_usage(void)
{
    fprintf(stdout, "Arguments: [.ini file]]\n");
    return;
}
