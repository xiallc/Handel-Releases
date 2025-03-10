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

    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    LARGE_INTEGER stop;

    double pt;
    double elapsed;
    double t;

    FILE* fp;

    if (argc != 2) {
        printf("Expects an argument with a valid Saturn .ini file path.\n");
        exit(1);
    }

    QueryPerformanceFrequency(&freq);

    status = xiaInit(argv[1]);
    CHECK(status);
    status = xiaStartSystem();
    CHECK(status);

    fp = fopen("decimation_0_2.txt", "w");
    ASSERT(fp != NULL);
    fprintf(fp, "Read Time\n");

    for (i = 0, elapsed = 0.0; i < 100; i++) {
        printf("Iteration %d\n", i);

        /* DECIMATION 0 -> 2 */
        pt = 0.5;
        status = xiaSetAcquisitionValues(0, "peaking_time", &pt);
        CHECK(status);

        pt = 2.0;
        QueryPerformanceCounter(&start);
        status = xiaSetAcquisitionValues(0, "peaking_time", &pt);
        QueryPerformanceCounter(&stop);

        t = n_secs_elapsed(start, stop, freq);
        fprintf(fp, "%0.6f\n", t);
        elapsed += t;
    }

    fclose(fp);

    printf("Mean peaking time change (s) = %0.6f\n", elapsed / 100.0);

    fp = fopen("same_decimation.txt", "w");
    ASSERT(fp != NULL);
    fprintf(fp, "Read Time\n");

    pt = 2.0;
    status = xiaSetAcquisitionValues(0, "peaking_time", &pt);
    CHECK(status);

    for (i = 0, elapsed = 0.0; i < 100; i++) {
        printf("Iteration %d\n", i);

        pt = 2.0;
        status = xiaSetAcquisitionValues(0, "peaking_time", &pt);
        CHECK(status);

        pt = 3.0;
        QueryPerformanceCounter(&start);
        status = xiaSetAcquisitionValues(0, "peaking_time", &pt);
        QueryPerformanceCounter(&stop);

        t = n_secs_elapsed(start, stop, freq);
        fprintf(fp, "%0.6f\n", t);
        elapsed += t;
    }

    fclose(fp);

    printf("Mean peaking time change (s) = %0.6f\n", elapsed / 100.0);

    fp = fopen("same_peaking_time.txt", "w");
    ASSERT(fp != NULL);
    fprintf(fp, "Read Time\n");

    pt = 2.0;
    status = xiaSetAcquisitionValues(0, "peaking_time", &pt);
    CHECK(status);

    for (i = 0, elapsed = 0.0; i < 100; i++) {
        printf("Iteration %d\n", i);

        pt = 2.0;
        QueryPerformanceCounter(&start);
        status = xiaSetAcquisitionValues(0, "peaking_time", &pt);
        QueryPerformanceCounter(&stop);

        t = n_secs_elapsed(start, stop, freq);
        fprintf(fp, "%0.6f\n", t);
        elapsed += t;
    }

    printf("Mean peaking time change (s) = %0.6f\n", elapsed / 100.0);

    fclose(fp);

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
