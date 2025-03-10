/*
 * Exercises the xMAP list-mode functionality by repeatedly reading out full
 * buffers.
 *
 * $Id$
 *
 */

#include <windows.h>

#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include "xia_common.h"

#include "handel.h"
#include "handel_errors.h"

#include "md_generic.h"

static void print_usage(void);

#define A 0
#define B 1

#define SWAP_BUFFER(x) ((x) == A ? B : A)

#define MAX_BUFFER_SIZE (1 << 20)

int main(int argc, char* argv[]) {
    char ini[256];
    char data_prefix[256];

    int status;
    int ignore;

    double mode = 3.0;
    double variant = 2.0;
    double n_secs = 0.0;
    double n_hrs = 0.0;

    time_t start;

    char* buffer_str[2] = {"buffer_a", "buffer_b"};

    char* buffer_len_str[2] = {"list_buffer_len_a", "list_buffer_len_b"};

    char* buffer_full_str[2] = {"buffer_full_a", "buffer_full_b"};

    char buffer_done_char[2] = {'a', 'b'};

    int current = A;
    int buffer_number = 0;

    unsigned long* buffer = NULL;

    if (argc < 4) {
        print_usage();
        exit(1);
    }

    sscanf(argv[1], "%s", ini);
    sscanf(argv[2], "%lf", &n_hrs);
    sscanf(argv[3], "%s", data_prefix);

    /* Create the data directory if it does not exist. */
    CreateDirectory("data", NULL);

    xiaSetLogLevel(MD_DEBUG);
    xiaSetLogOutput("handel.log");

    status = xiaInit(ini);

    if (status != XIA_SUCCESS) {
        fprintf(stderr, "Unable to initialize Handel using '%s'.\n", ini);
        exit(1);
    }

    status = xiaStartSystem();

    if (status != XIA_SUCCESS) {
        fprintf(stderr, "Unable to start the system.\n");
        exit(1);
    }

    /* Switch to list-mode. */
    status = xiaSetAcquisitionValues(0, "mapping_mode", &mode);

    if (status != XIA_SUCCESS) {
        xiaExit();

        fprintf(stderr, "Error setting 'mapping_mode' to %.1f.\n", mode);
        exit(1);
    }

    status = xiaSetAcquisitionValues(0, "list_mode_variant", &variant);

    if (status != XIA_SUCCESS) {
        xiaExit();

        fprintf(stderr, "Error setting 'list_mode_variant' to %.1f.\n", variant);
        exit(1);
    }

    status = xiaBoardOperation(0, "apply", &ignore);

    if (status != XIA_SUCCESS) {
        xiaExit();

        fprintf(stderr, "Error applying the list-mode settings.\n");
        exit(1);
    }

    time(&start);

    n_secs = (double) n_hrs * 60.0 * 60.0;

    fprintf(stdout, "Starting the list-mode run.\n");

    status = xiaStartRun(-1, 0);

    if (status != XIA_SUCCESS) {
        xiaExit();

        fprintf(stderr, "Error starting the list-mode run.\n");
        exit(1);
    }

    buffer = malloc(sizeof(unsigned long) * MAX_BUFFER_SIZE);

    if (!buffer) {
        xiaStopRun(-1);
        xiaExit();

        fprintf(stderr, "Unable to allocated %zd bytes for buffer.\n",
                sizeof(unsigned long) * MAX_BUFFER_SIZE);
        exit(1);
    }

    /* The algorithm here is to read the current buffer, let the
     * hardware know we are done with it, write the raw buffer to disk
     * and then read the other buffer, etc.
     */
    while (difftime(time(NULL), start) < n_secs) {
        unsigned long len;
        unsigned long i;

        FILE* fp;

        char name[256];

        unsigned short buffer_full = 0;

        do {
            status = xiaGetRunData(0, buffer_full_str[current], &buffer_full);

            if (status != XIA_SUCCESS) {
                xiaStopRun(-1);
                xiaExit();

                fprintf(stderr, "Error getting the status of buffer '%c'.\n",
                        buffer_done_char[current]);
                exit(1);
            }

            Sleep(1);
        } while (!buffer_full);

        status = xiaGetRunData(0, buffer_str[current], buffer);

        if (status != XIA_SUCCESS) {
            xiaStopRun(-1);
            xiaExit();

            fprintf(stderr, "Error reading '%s'.\n", buffer_str[current]);
            exit(1);
        }

        status = xiaBoardOperation(0, "buffer_done", &buffer_done_char[current]);

        if (status != XIA_SUCCESS) {
            xiaStopRun(-1);
            xiaExit();

            fprintf(stderr, "Error setting buffer '%c' to done.\n",
                    buffer_done_char[current]);
            exit(1);
        }

        status = xiaGetRunData(0, buffer_len_str[current], &len);

        if (status != XIA_SUCCESS) {
            xiaStopRun(-1);
            xiaExit();

            fprintf(stderr, "Error reading '%s'.\n", buffer_len_str[current]);
            exit(1);
        }

        fprintf(stdout, "Preparing to dump buffer '%c'/%d, len = %lu.\n",
                buffer_done_char[current], buffer_number, len);

        sprintf(name, "data\\%s_%d.bin", data_prefix, buffer_number);
        fp = fopen(name, "wb");

        if (!fp) {
            xiaStopRun(-1);
            xiaExit();

            fprintf(stderr, "Unable to open '%s' for writing.\n", name);
            exit(1);
        }

        for (i = 0; i < len; i++) {
            unsigned short w = (unsigned short) buffer[i];

            fwrite(&w, sizeof(unsigned short), 1, fp);
        }

        fclose(fp);

        current = SWAP_BUFFER(current);
        buffer_number++;
    }

    status = xiaStopRun(-1);

    if (status != XIA_SUCCESS) {
        xiaExit();

        fprintf(stderr, "Error stopping the list-mode run.\n");
        exit(1);
    }

    free(buffer);

    xiaExit();

    return 0;
}

static void print_usage(void) {
    fprintf(stdout, "Arguments: [.ini file] [# of hours to run for] "
                    "[data prefix]\n");
    return;
}
