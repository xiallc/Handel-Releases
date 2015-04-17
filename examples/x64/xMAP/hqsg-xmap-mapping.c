/*
 * This code accompanies the XIA Application Note "Handel Quick Start Guide:
 * xMAP". This sample code shows how to acquire MCA mapping mode data and
 * save it to a file for later processing.
 *
 * To simulate pixel advance in the absence of a GATE or SYNC signal, this
 * application uses a thread to tell Handel to manually advance the pixel.
 * This technique should not be used in production code.
 *
 * Copyright (c) 2005-2014, XIA LLC
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
#pragma warning(disable : 4127)
#include <windows.h>
#include <process.h>

#include "handel.h"
#include "handel_errors.h"
#include "handel_constants.h"
#include "md_generic.h"


static CRITICAL_SECTION LOCK;

/* Acquires the global lock before making the Handel call. */
#define SYNC(x) \
    do { \
    EnterCriticalSection(&LOCK); \
    status = (x); \
    LeaveCriticalSection(&LOCK); \
    } while (0)


static void print_usage(void);
static void CHECK_ERROR(int status);

static int WaitForBuffer(char buf);
static int ReadBuffer(char buf, unsigned long *data);
static int SwitchBuffer(char *buf);
static int GetCurrentPixel(unsigned long *pixel);

static unsigned _stdcall PixelAdvanceStart(LPVOID *params);


int main(int argc, char *argv[])
{
    int status;
    int ignored = 0;

    char curBuffer = 'a';

    unsigned long curPixel = 0;
    unsigned long bufferLen = 0;

    double nBins = 2048.0;
    double nMapPixels = 200.0;
    double nMapPixelsPerBuffer = -1.0;
    double mappingMode = 1.0;
    double pixControl = XIA_MAPPING_CTL_GATE;

    unsigned long *buffer = NULL;

    HANDLE pixAdvThreadHandle;

    unsigned pixAdvThreadId;

    if (argc < 2) {
        print_usage();
        exit(1);
    }

    printf("Configuring the Handel log file.\n");
    
    /* Setup logging here */
    xiaSetLogLevel(MD_WARNING);
    xiaSetLogOutput("handel.log");

    printf("Loading the .ini file.\n");
    status = xiaInit(argv[1]);
    CHECK_ERROR(status);

    /* Boot hardware */
    printf("Starting up the hardware.\n");
    status = xiaStartSystem();
    CHECK_ERROR(status);

    /* Set mapping parameters. */
    printf("Setting the acquisition values.\n");
    status = xiaSetAcquisitionValues(-1, "number_mca_channels", (void *)&nBins);
    CHECK_ERROR(status);

    status = xiaSetAcquisitionValues(-1, "num_map_pixels", (void *)&nMapPixels);
    CHECK_ERROR(status);

    status = xiaSetAcquisitionValues(-1, "num_map_pixels_per_buffer",
                                     (void *)&nMapPixelsPerBuffer);
    CHECK_ERROR(status);

    status = xiaSetAcquisitionValues(-1, "pixel_advance_mode",
                                     (void *)&pixControl);
    CHECK_ERROR(status);

    status = xiaSetAcquisitionValues(-1, "mapping_mode", (void *)&mappingMode);
    CHECK_ERROR(status);

    /* Apply the mapping parameters. */
    printf("Applying the acquisition values.\n");
    status = xiaBoardOperation(0, "apply", (void *)&ignored);
    CHECK_ERROR(status);

    /* Prepare the buffer we will use to read back the data from the board. */
    status = xiaGetRunData(0, "buffer_len", (void *)&bufferLen);
    CHECK_ERROR(status);

    printf("Mapping buffer length = %lu.\n", bufferLen);
    printf("Allocating memory for mapping buffer.\n");
    buffer = (unsigned long *)malloc(bufferLen * sizeof(unsigned long));
    
    if (!buffer) {
        /* Error allocating memory */
        exit(1);
    }

    InitializeCriticalSection(&LOCK);

    /* Start the mapping run. */
    printf("Starting the mapping run.\n");
    status = xiaStartRun(-1, 0);
    CHECK_ERROR(status);

    pixAdvThreadHandle = (HANDLE)_beginthreadex(NULL, 0, PixelAdvanceStart,
                                                NULL, 0, &pixAdvThreadId);

    if (!pixAdvThreadHandle) {
        printf("Error creating and starting pixel advance thread.\n");
        DeleteCriticalSection(&LOCK);
        xiaExit();
        exit(1);
    }

    printf("Starting main mapping loop.\n");
    /* The main loop that is described in the Quick Start Guide. */
    do {
        printf("Waiting for buffer '%c' to fill.\n", curBuffer);
        status = WaitForBuffer(curBuffer);
        CHECK_ERROR(status);

        printf("Reading buffer '%c'.\n", curBuffer);
        status = ReadBuffer(curBuffer, buffer);
        CHECK_ERROR(status);

        /* This is where you would ordinarily do something with the data:
         * write it to a file, post-process it, etc.
         */

        printf("Switching buffers.\n");
        status = SwitchBuffer(&curBuffer);
        CHECK_ERROR(status);

        status = GetCurrentPixel(&curPixel);
        CHECK_ERROR(status);

    } while (curPixel < (unsigned long)nMapPixels);

    /* Cleanup related to the pixel advance thread. */
    TerminateThread(pixAdvThreadHandle, 0);
    DeleteCriticalSection(&LOCK);
    CloseHandle(pixAdvThreadHandle);

    printf("Release mapping buffer memory.\n");
    free(buffer);

    /* Stop the mapping run. */
    printf("Stopping the run.\n");
    status = xiaStopRun(-1);
    CHECK_ERROR(status);

    printf("Cleaning up Handel.\n");
    status = xiaExit();
    CHECK_ERROR(status);

    return 0;
}


/********** 
 * This is just an example of how to handle error values.
 * A program of any reasonable size should
 * implement a more robust error handling mechanism.
 **********/
static void CHECK_ERROR(int status)
{
    /* XIA_SUCCESS is defined in handel_errors.h */
    if (status != XIA_SUCCESS) {
        printf("Error encountered! Status = %d\n", status);
        getchar();
        exit(status);
    }
}


/**********
 * Waits for the specified buffer to fill.
 **********/
static int WaitForBuffer(char buf)
{
    int status;

    char bufString[15];

    unsigned short isFull = FALSE_;


    printf("\tWaiting for buffer '%c'.\n", buf);

    sprintf(bufString, "buffer_full_%c", buf);

    while (!isFull) {
        SYNC(xiaGetRunData(0, bufString, (void *)&isFull));
    
        if (status != XIA_SUCCESS) {
            return status;
        }

        Sleep(1);
    }

    return XIA_SUCCESS;
}


/**********
 * Reads the requested buffer.
 **********/
static int ReadBuffer(char buf, unsigned long *data)
{
    int status;

    char bufString[9];


    printf("\tReading buffer '%c'.\n", buf);

    sprintf(bufString, "buffer_%c", buf);
  
    SYNC(xiaGetRunData(0, bufString, (void *)data));
  
    return status;
}


/**********
 * Clears the current buffer and switches to the next buffer.
 **********/
static int SwitchBuffer(char *buf)
{
    int status;

  
    printf("\tSwitching from buffer '%c'...", *buf);

    SYNC(xiaBoardOperation(0, "buffer_done", (void *)buf));

    if (status == XIA_SUCCESS) {
        *buf = (char)((*buf == 'a') ? 'b' : 'a');
    }

    printf("...to buffer '%c'.\n", *buf);

    return status;
}


/**********
 * Get the current mapping pixel.
 **********/
static int GetCurrentPixel(unsigned long *pixel)
{
    int status;


    SYNC(xiaGetRunData(0, "current_pixel", (void *)pixel));
  
    printf("Current pixel = %lu.\n", *pixel);

    return status;
}


/* Manually advances the mapping pixel every 10 ms. In real
 * applications, use a GATE or SYNC signal to advance the pixel.
 */
static unsigned _stdcall PixelAdvanceStart(LPVOID *params)
{
    int ignored;
    int status;

    UNUSED(params);


    while (TRUE_) {
        Sleep(10);
        SYNC(xiaBoardOperation(0, "mapping_pixel_next", &ignored));
        CHECK_ERROR(status);
    }

    return 0;
}


static void print_usage(void)
{
    fprintf(stdout, "Arguments: [.ini file]]\n");
    return;
}
