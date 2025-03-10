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

    double t;

    unsigned short param;

    double mca_lens[] = {256.0, 512.0, 1024.0, 2048.0, 4096.0, 8192.0};

    char mca_file_name[128];

    unsigned long* mca = NULL;

    FILE* fp;

    if (argc != 2) {
        printf("Expects an argument with a valid Saturn .ini file path.\n");
        exit(1);
    }

    QueryPerformanceFrequency(&freq);

    status = xiaInit(argv[1]);
    CHECK(status);

    QueryPerformanceCounter(&start);
    status = xiaStartSystem();
    QueryPerformanceCounter(&stop);
    CHECK(status);

    t = n_secs_elapsed(start, stop, freq);

    printf("Start system time = %0.6f seconds\n", t);

    printf("START THE LOGIC ANALYZER NOW!!\n");

    Sleep(2000);

    fp = fopen("dsp_parameter_read_times.txt", "w");
    ASSERT(fp != NULL);

    fprintf(fp, "Read Time\n");

    for (i = 0, t = 0.0; i < 10; i++) {
        QueryPerformanceCounter(&start);
        status = xiaGetParameter(0, "DECIMATION", &param);
        QueryPerformanceCounter(&stop);
        CHECK(status);

        fprintf(fp, "%0.6f\n", n_secs_elapsed(start, stop, freq));

        t += n_secs_elapsed(start, stop, freq);
    }

    fclose(fp);

    printf("Mean DSP parameter read time = %0.6f seconds\n", t / 10.0);

    for (i = 0, t = 0.0; i < N_ELEMS(mca_lens); i++) {
        sprintf(mca_file_name, "%d_mca_read_times.txt", (int) mca_lens[i]);
        fp = fopen(mca_file_name, "w");
        ASSERT(fp != NULL);

        fprintf(fp, "Read Time\n");

        status = xiaSetAcquisitionValues(0, "number_mca_channels", &(mca_lens[i]));
        CHECK(status);

        mca = malloc((int) mca_lens[i] * sizeof(unsigned long));
        ASSERT(mca != NULL);

        for (j = 0; j < 10; j++) {
            QueryPerformanceCounter(&start);
            status = xiaGetRunData(0, "mca", mca);
            QueryPerformanceCounter(&stop);
            CHECK(status);

            fprintf(fp, "%0.6f\n", n_secs_elapsed(start, stop, freq));

            t += n_secs_elapsed(start, stop, freq);
        }

        fclose(fp);
        free(mca);

        printf("Mean MCA (%d bins) read time = %0.6f\n", (int) mca_lens[i], t / 10.0);
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
