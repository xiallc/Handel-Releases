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
    int n_iters;
    int i;

    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    LARGE_INTEGER stop;

    unsigned long csr;
    unsigned long mca_length;

    double* read_times = NULL;

    FILE* out = NULL;

    if (argc != 2) {
        fprintf(stderr, "The program expects the first argument to be the "
                        "number of iterations.\n");
        exit(1);
    }

    sscanf(argv[1], "%d", &n_iters);

    read_times = (double*) malloc(n_iters * sizeof(double));
    ASSERT(read_times != NULL);

    QueryPerformanceFrequency(&freq);

    status = xiaInit("xmap.ini");
    CHECK(status);

    status = xiaStartSystem();
    CHECK(status);

    for (i = 0; i < n_iters; i++) {
        QueryPerformanceCounter(&start);
        status = xiaBoardOperation(0, "get_csr", &csr);
        QueryPerformanceCounter(&stop);

        CHECK(status);

        read_times[i] = n_secs_elapsed(start, stop, freq);
    }

    out = fopen("read_times_register.csv", "w");
    ASSERT(out != NULL);

    fprintf(out, "Iteration, Read Time\n");

    for (i = 0; i < n_iters; i++) {
        fprintf(out, "%d,%0.6f\n", i, read_times[i]);
    }

    fclose(out);

    for (i = 0; i < n_iters; i++) {
        QueryPerformanceCounter(&start);
        status = xiaGetRunData(0, "mca_length", &mca_length);
        QueryPerformanceCounter(&stop);

        CHECK(status);

        read_times[i] = n_secs_elapsed(start, stop, freq);
    }

    out = fopen("read_times_mca_length.csv", "w");
    ASSERT(out != NULL);

    fprintf(out, "Iteration, Read Time\n");

    for (i = 0; i < n_iters; i++) {
        fprintf(out, "%d,%0.6f\n", i, read_times[i]);
    }

    fclose(out);

    free(read_times);

    xiaExit();

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
