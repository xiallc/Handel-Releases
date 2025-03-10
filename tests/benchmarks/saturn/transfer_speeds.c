#include <stdio.h>
#include <stdlib.h>

#include "windows.h"

#include "handel.h"
#include "handel_errors.h"

#include "xia_assert.h"
#include "xia_common.h"

#define CHECK(x) ((x) == XIA_SUCCESS) ? nop() : exit(1)

static double n_secs_elapsed(LARGE_INTEGER start, LARGE_INTEGER stop,
                             LARGE_INTEGER freq);
static void nop(void);

int main(int argc, char* argv[]) {
    int status;
    int i;
    int j;

    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    LARGE_INTEGER stop;

    double read_time;

    unsigned long* mca = NULL;

    double mca_lens[] = {256.0, 512.0, 1024.0, 2048.0, 4096.0, 8192.0};

    if (argc != 2) {
        printf("Expects an argument with a valid Saturn .ini file path.\n");
        exit(1);
    }

    QueryPerformanceFrequency(&freq);

    status = xiaInit(argv[1]);
    CHECK(status);

    status = xiaStartSystem();
    CHECK(status);

    for (i = 0; i < N_ELEMS(mca_lens); i++) {
        status = xiaSetAcquisitionValues(0, "number_mca_channels", &(mca_lens[i]));
        CHECK(status);

        mca = malloc((int) mca_lens[i] * sizeof(unsigned long));
        ASSERT(mca != NULL);

        for (j = 0, read_time = 0.0; j < 1000; j++) {
            QueryPerformanceCounter(&start);
            status = xiaGetRunData(0, "mca", mca);
            QueryPerformanceCounter(&stop);
            CHECK(status);

            read_time += n_secs_elapsed(start, stop, freq);
        }

        printf("Transfer speed (%0.0f bins) = %0.3f MB/s\n", mca_lens[i],
               (((mca_lens[i] * 1000.0 * 4.0) + 7.0) / 1048576.0) / read_time);
        free(mca);
    }

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
