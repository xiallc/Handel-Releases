#include <stdio.h>
#include <stdlib.h>

#include "windows.h"

#include "handel.h"
#include "handel_errors.h"

#include "xia_assert.h"

#define CHECK(x) ((x) == XIA_SUCCESS) ? nop() : exit(1)

static double n_secs_elapsed(LARGE_INTEGER start, LARGE_INTEGER stop,
                             LARGE_INTEGER freq);
static void nop(void);

int main(int argc, char* argv[]) {
    int status;
    int n_reads;
    int mca_len;
    int i;

    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    LARGE_INTEGER stop;

    double acq_val;

    double* read_times = NULL;

    unsigned long* mca = NULL;

    FILE* out = NULL;

    if (argc != 3) {
        fprintf(stderr, "The program expects the first argument to be the "
                        "number of times the MCA should be read and the second "
                        "argument to be the spectrum size.\n");
        exit(1);
    }

    sscanf(argv[1], "%d", &n_reads);
    sscanf(argv[2], "%d", &mca_len);

    read_times = (double*) malloc(n_reads * sizeof(double));
    ASSERT(read_times != NULL);

    QueryPerformanceFrequency(&freq);

    status = xiaInit("xmap.ini");
    CHECK(status);

    status = xiaStartSystem();
    CHECK(status);

    acq_val = (double) mca_len;
    status = xiaSetAcquisitionValues(-1, "number_mca_channels", &acq_val);
    CHECK(status);

    mca = (unsigned long*) malloc(mca_len * sizeof(unsigned long));
    ASSERT(mca != NULL);

    /* Simulate reading all 4 channels, which is what any new functionality
     * would aim to replace.
     */
    for (i = 0; i < n_reads; i++) {
        QueryPerformanceCounter(&start);
        status = xiaGetRunData(0, "mca", mca);
        CHECK(status);
        status = xiaGetRunData(1, "mca", mca);
        CHECK(status);
        status = xiaGetRunData(2, "mca", mca);
        CHECK(status);
        status = xiaGetRunData(3, "mca", mca);
        CHECK(status);
        QueryPerformanceCounter(&stop);

        read_times[i] = n_secs_elapsed(start, stop, freq);
    }

    free(mca);

    out = fopen("read_times.csv", "w");
    ASSERT(out != NULL);

    fprintf(out, "Iteration, Read Time\n");

    for (i = 0; i < n_reads; i++) {
        fprintf(out, "%d,%0.6f\n", i, read_times[i]);
    }

    xiaExit();

    fclose(out);
    free(read_times);

    return 0;
}

/*
 * Converts two values returned by QueryPerformanceCounter() into seconds.
 */
static double n_secs_elapsed(LARGE_INTEGER start, LARGE_INTEGER stop,
                             LARGE_INTEGER freq) {
    return ((double) (stop.QuadPart - start.QuadPart) / (double) freq.QuadPart);
}

static void nop(void) {
    return;
}
