/*
 * This sample code shows how to acquire list mode data with the MPX-32D. No
 * buffer parsing is shown in this sample; refer to the document "PMT List-Mode
 * Specification" for details of the buffer format.
 *
 * XIA Application Note "Handel Quick Start Guide: xMAP"
 * (http://www.xia.com/Software/docs/handel/quickstartguide/xmap/mapping.html#pixel-advance-modes-label)
 * provides background on Handel's architecture and conventions.
 *
 * This sample uses detChan parameter -1 (the first argument to most Handel
 * routines--see the quick start guide for further explanation of detChans)
 * where possible to set values and start/stop runs on all channels. However,
 * this sample currently only checks the buffer status and reads data from
 * detChan 0. In a real run situation, you would need to check buffer status
 * and read data from the first channel in each module. This requires knowing
 * the number of modules and number of channels per module. These values can
 * be obtained via knowledge of the system or, more robustly, using the Handel
 * APIs xiaGetNumModules, xiaGetModules (to get module aliases), and
 * xiaGetModuleItem(modAlias, "number_of_channels", &n).
 * 
 * Usage:
 *   hqsg-stj-list INI
 */

/*
 * Copyright (c) 2014-2015 XIA LLC
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
 */

#include <stdio.h>
#include <stdlib.h>

#pragma warning(disable : 4115)
#include <windows.h>

#include "handel.h"
#include "handel_errors.h"
#include "handel_constants.h"
#include "md_generic.h"
#include "hqsg-stj-buffers.h"

static void print_usage(void);

static void ASSERT(int ok, const char* key);
static void CHECK_ERROR(int status);

static int WaitAndReadBuffer(char buf);
static int WaitForBuffer(char buf);
static int ReadBuffer(char buf);
static int WriteBuffer(char buf, unsigned long* buffer, size_t buffer_len);
static int ParseBuffer(unsigned long* buffer, size_t buffer_len);
static void ConvertBuffer(unsigned long n_bytes, void* data, void* parsed);

int main(int argc, char* argv[]) {
    char ini[256];

    int status;
    int ignored = 0;

    double mappingMode = 3.0; /* List mode */
    double listModeVar = 16.0; /* PMT variant */

    /*
     * Pixel acquisition values control the number of events to be read before
     * the hardware switches a/b buffers or ends the run. Here we set the
     * total number of events to a very small number to force a short run and
     * the events per buffer to half that to force a buffer switch.
     *
     * To continue the run indefinitely set num_map_pixels = 0. Note:
     * continuous runs with constant switching are not recommended with
     * current PMT firwmare capabilities.
     * 
     * To stop the run after one buffer is filled, set num_map_pixels_per_buffer=num_map_pixels.
     * 
     * To use the largest buffer size allowed by SRAM, set
     * num_map_pixels_per_buffer = -1 and then call xiaGetAcquisitionValues to
     * find out the actual value for sizing your array.
     */
    double num_map_pixels = 10.0; /* Total number of events to read
                                               in the run, set for
                                               1*num_map_pixels_per_buffer or
                                               2*num_map_pixels_per_buffer. */
    double num_map_pixels_per_buffer = 5.0; /* Number of events before
                                               switching a/b buffers. */

    unsigned long runActive;

    if (argc < 2) {
        print_usage();
        exit(1);
    }

    sscanf(argv[1], "%s", ini);

    printf("Configuring the Handel log file.\n");

    xiaSetLogLevel(MD_WARNING);
    xiaSetLogOutput("handel.log");

    printf("Loading the .ini file.\n");
    status = xiaInit(ini);
    CHECK_ERROR(status);

    printf("Starting up the hardware.\n");
    status = xiaStartSystem();
    CHECK_ERROR(status);

    printf("Setting the acquisition values.\n");
    CHECK_ERROR(xiaSetAcquisitionValues(-1, "mapping_mode", &mappingMode));
    CHECK_ERROR(xiaSetAcquisitionValues(-1, "list_mode_variant", &listModeVar));
    CHECK_ERROR(xiaSetAcquisitionValues(-1, "num_map_pixels", &num_map_pixels));
    CHECK_ERROR(xiaSetAcquisitionValues(-1, "num_map_pixels_per_buffer",
                                        &num_map_pixels_per_buffer));

    printf("Applying the list mode acquisition values.\n");
    CHECK_ERROR(xiaBoardOperation(0, "apply", (void*) &ignored));

    printf("Starting the mapping run.\n");
    CHECK_ERROR(xiaStartRun(-1, 0));

    printf("Starting main list mode loop.\n");

    CHECK_ERROR(WaitAndReadBuffer('a'));
    CHECK_ERROR(WaitAndReadBuffer('b'));

    /* Check if the hardware stopped the run based on number of events criteria. */
    CHECK_ERROR(xiaGetRunData(0, "run_active", &runActive));

    if (runActive) {
        printf("Run still active, stopping the run.\n");
        CHECK_ERROR(xiaStopRun(-1));
    } else {
        printf("Hardware stopped the run.\n");
    }

    printf("Cleaning up Handel.\n");
    CHECK_ERROR(xiaExit());

    return 0;
}

/**
 * This is just an example of how to handle error values.
 * A program of any reasonable size should
 * implement a more robust error handling mechanism.
 */
static void CHECK_ERROR(int status) {
    /* XIA_SUCCESS is defined in handel_errors.h */
    if (status != XIA_SUCCESS) {
        printf("Error encountered! Status = %d\n", status);
        getchar();
        exit(status);
    }
}

/**
 * Waits for the specified buffer to fill, then reads it out.
 */
static int WaitAndReadBuffer(char buf) {
    int status;

    printf("Waiting for buffer '%c' to fill.\n", buf);
    status = WaitForBuffer(buf);
    if (status != XIA_SUCCESS) {
        return status;
    }

    printf("Reading buffer '%c'.\n", buf);
    status = ReadBuffer(buf);
    if (status != XIA_SUCCESS) {
        return status;
    }

    return XIA_SUCCESS;
}

/**
 * Waits for the specified buffer to fill.
 */
static int WaitForBuffer(char buf) {
    int status;

    char bufString[15];

    unsigned short isFull = FALSE_;
    unsigned long pixel = 0;

    printf("\tWaiting for buffer '%c'.\n", buf);

    sprintf(bufString, "buffer_full_%c", buf);

    while (!isFull) {
        status = xiaGetRunData(0, bufString, (void*) &isFull);

        if (status != XIA_SUCCESS) {
            return status;
        }

        status = xiaGetRunData(0, "current_pixel", (void*) &pixel);

        if (status != XIA_SUCCESS) {
            return status;
        }

        printf("\r\tCurrent pixel = %lu", pixel);

        Sleep(1);
    }

    printf("\n");

    return XIA_SUCCESS;
}

/**
 * Reads the requested buffer.
 */
static int ReadBuffer(char buf) {
    int status;

    char bufString[100];

    unsigned long bufferLen = 0;
    unsigned long* buffer = NULL;

    /* Prepare the buffer we will use to read back the data from the board. */
    sprintf(bufString, "list_buffer_len_%c", buf);
    status = xiaGetRunData(0, bufString, (void*) &bufferLen);
    CHECK_ERROR(status);

    printf("Allocating list mode buffer, length = %lu.\n", bufferLen);
    buffer = (unsigned long*) malloc(bufferLen * sizeof(unsigned long));

    if (!buffer) {
        /* Error allocating memory */
        exit(1);
    }

    printf("\tReading buffer '%c'.\n", buf);

    sprintf(bufString, "buffer_%c", buf);

    xiaGetRunData(0, bufString, (void*) buffer);

    /*
     * This is where you would ordinarily do something with the data:
     * write it to a file, post-process it, etc.
     */

    /* Write the raw data like ProSpect's binary files */
    CHECK_ERROR(WriteBuffer(buf, buffer, bufferLen));

    /* Also try parsing the records */
    CHECK_ERROR(ParseBuffer(buffer, bufferLen));

    printf("Release mapping buffer memory.\n");
    free(buffer);

    return status;
}

/**
 * Parses the STJ PMT list mode buffer specification.
 */
static int ParseBuffer(unsigned long* buffer, size_t buffer_len) {
    struct pmt_buffer* pmt;
    struct header* header;
    unsigned long buffer_number;

    union event_record* event;
    struct event_record_base* stamp;
    unsigned long long event_number, event_time;
    unsigned long i, j;

    /*
     * If the structs are defined right, we can just cast the buffer and
     * access named fields.
     */
    pmt = (struct pmt_buffer*) buffer;
    header = &pmt->header;

    /* access to header fields */
    ASSERT(header->tag0 == 0x55AA, "tag0");
    ASSERT(header->tag1 == 0xAA55, "tag1");
    ASSERT(header->header_size == 256, "header size");
    ASSERT(sizeof(struct header) == 256 * sizeof(word), "header struct size");
    ASSERT(header->list_mode_variant >= AnodeVariant &&
               header->list_mode_variant <= PMTAllVariant,
           "header list mode variant");
    ASSERT(header->words_per_event == 272, "words per event");
    ASSERT(buffer_len == MAKE_WORD32(header->total_words) + header->header_size,
           "buffer_len/total_words");
    ASSERT(buffer_len - header->header_size == header->words_per_event * header->events,
           "buffer_len/words_per_event*events");

    buffer_number = MAKE_WORD32(header->buffer_number);

    /* If the header looks good, proceed to loop through the event records. */
    for (i = 0; i < header->events; i++) {
        event = &pmt->events[i];

        /*
         * Since both event record types start with a common base, we can cast to that
         * to get the time stamp and event ID.
         */
        stamp = (struct event_record_base*) event;

        event_number = MAKE_WORD64(stamp->event_id);

        /* 64-bit value in 320ns units */
        event_time = MAKE_WORD64(stamp->time);

        ASSERT(stamp->tag == 0xEEEE, "event tag");

        /*
         * Access specific event record types once we have parsed the base fields
         * and know the type. Depending on the firmware version, the variant may
         * be specifically 10 or 11, or erroneously 16 for all events.
         */
        if (stamp->list_mode_variant == DynodeVariant) {
            /* access to event->dynode.multiplicity, mask1, mask2 */

            for (j = 0;
                 j < sizeof(event->dynode.energy) / sizeof(event->dynode.energy[0]);
                 j++) {
                /* 32 dynode energy values in event->dynode.energy[j] */
            }
        } else if (stamp->list_mode_variant == AnodeVariant ||
                   stamp->list_mode_variant == PMTAllVariant) {
            for (j = 0;
                 j < sizeof(event->anode.energy) / sizeof(event->anode.energy[0]);
                 j++) {
                /* 256 anode energy values in event->anode.energy[j] */
            }
        } else {
            ASSERT(0, "event record list mode variant");
        }
    }

    return XIA_SUCCESS;
}

/**
 * Writes the buffer to a binary file in the same format as ProSpect (16-bit words).
 */
static int WriteBuffer(char buf, unsigned long* buffer, size_t buffer_len) {
    unsigned short* parsed;
    size_t source_bytes = buffer_len * sizeof(unsigned long);
    size_t parsed_bytes = buffer_len * sizeof(unsigned short);
    FILE* f;
    char file_string[100];

    parsed = malloc(parsed_bytes);
    if (!parsed) {
        exit(1);
    }

    ConvertBuffer((unsigned long) source_bytes, buffer, parsed);

    /*
     * This sample overwrites buffer_a.bin and buffer_b.bin. The user may
     * restructure to use a shared file handle to stitch a single file for all
     * buffers in the run.
     */
    sprintf(file_string, "buffer_%c.bin", buf);

    f = fopen(file_string, "wb");

    if (!f) {
        exit(2);
    }

    fwrite(parsed, parsed_bytes, 1, f);
    fclose(f);

    return XIA_SUCCESS;
}

/**
 * Converts a list mode buffer to unsigned short as ProSpect does for writing
 * binary files. The mapping data is expected as unsigned longs, of which we
 * will pick off only the lower 16-bits.
 */
static void ConvertBuffer(unsigned long n_bytes, unsigned long* data,
                          unsigned short* parsed) {
    unsigned long i;
    unsigned long j;

    unsigned short* us_data = NULL;

    us_data = (unsigned short*) data;

    for (i = 0, j = 0; i < (n_bytes / 2); i += 2, j++) {
        parsed[j] = us_data[i];
    }
}

static void print_usage(void) {
    fprintf(stdout, "Arguments: [.ini file]]\n");
    return;
}

static void ASSERT(int ok, const char* key) {
    if (!ok) {
        printf("failed check: %s\n", key);
        exit(3);
    }
}
