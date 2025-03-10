/*
 * Performs low-level benchmarks of the raw burst read speed at a variety of
 * block sizes. This test assumes that the hardware is already running the proper
 * firmware.
 *
 * $Id$
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <windows.h>

#include "xia_common.h"

#include "plxlib.h"

static void check_status(int status);
static double n_secs_elapsed(LARGE_INTEGER start, LARGE_INTEGER stop,
                             LARGE_INTEGER freq);

typedef struct _Benchmark_Pair {
    unsigned long block_size;
    unsigned long n_iters;
} Benchmark_Pair_t;

int main(int argc, char* argv[]) {
    int status;

    unsigned long i;
    unsigned long j;

    byte_t bus = 0;
    byte_t slot = 0;

    double elapsed = 0.0;
    double total_time = 0.0;

    unsigned long* data = NULL;

    HANDLE h;

    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    LARGE_INTEGER stop;

    Benchmark_Pair_t tests[] = {
        {1, 100000},   {2, 100000},   {10, 100000},  {512, 10000},
        {1024, 10000}, {8192, 10000}, {32768, 1000},
    };

    sscanf(argv[1], "%uc", &bus);
    sscanf(argv[2], "%uc", &slot);

    QueryPerformanceFrequency(&freq);

    status = plx_open_slot((unsigned short) -1, bus, slot, &h);
    check_status(status);

    data = (unsigned long*) malloc(32768 * sizeof(unsigned long));
    assert(data != NULL);

    for (i = 0; i < N_ELEMS(tests); i++) {
        total_time = 0.0;

        for (j = 0; j < tests[i].n_iters; j++) {
            QueryPerformanceCounter(&start);
            status = plx_read_block(h, 0x3000000, tests[i].block_size, 2, data);
            QueryPerformanceCounter(&stop);
            check_status(status);

            elapsed = n_secs_elapsed(start, stop, freq);
            total_time += elapsed;
        }

        printf("Block size = %lu, avg. time = %0.6fs (@ %lu iterations)\n",
               tests[i].block_size, total_time / (double) tests[i].n_iters,
               tests[i].n_iters);
    }

    free(data);

    status = plx_close_slot(h);
    check_status(status);

    return 0;
}

/*
 * Basic error handling.
 */
static void check_status(int status) {
    if (status != 0) {
        printf("Status = %d, exiting...\n", status);
        exit(status);
    }
}

/*
 * Converts two values returned by QueryPerformanceCounter() into seconds.
 */
static double n_secs_elapsed(LARGE_INTEGER start, LARGE_INTEGER stop,
                             LARGE_INTEGER freq) {
    return ((double) (stop.QuadPart - start.QuadPart) / (double) freq.QuadPart);
}
