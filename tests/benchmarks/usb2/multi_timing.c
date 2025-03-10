#include <stdio.h>
#include <stdlib.h>

#include "windows.h"

#include "xia_usb2.h"
#include "xia_usb2_errors.h"
#include "xia_assert.h"
#include "xia_common.h"

#define CHECK(x) ((x) == XIA_USB2_SUCCESS) ? nop() : exit(1)

static double n_secs_elapsed(LARGE_INTEGER start, LARGE_INTEGER stop,
                             LARGE_INTEGER freq);
static void nop(void);

int main(int argc, char* argv[]) {
    int status;
    int i;

    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    LARGE_INTEGER stop;

    HANDLE h;

    byte_t* buf = NULL;

    FILE* fp = NULL;

    QueryPerformanceFrequency(&freq);

    status = xia_usb2_open(0, &h);
    CHECK(status);

    /* Sub-512 byte Data memory read */
    buf = malloc(2);
    ASSERT(buf != NULL);

    fp = fopen("sub_512_data_memory_times.txt", "w");
    ASSERT(fp != NULL);

    fprintf(fp, "Read time\n");

    for (i = 0; i < 1000; i++) {
        QueryPerformanceCounter(&start);
        status = xia_usb2_read(h, 0x4000, 2, buf);
        QueryPerformanceCounter(&stop);
        CHECK(status);

        fprintf(fp, "%0.6f\n", n_secs_elapsed(start, stop, freq));
    }

    fclose(fp);
    free(buf);

    /* 1024 byte Data memory read */
    buf = malloc(1024);
    ASSERT(buf != NULL);

    fp = fopen("1024_data_memory_times.txt", "w");
    ASSERT(fp != NULL);

    fprintf(fp, "Read time\n");

    for (i = 0; i < 1000; i++) {
        QueryPerformanceCounter(&start);
        status = xia_usb2_read(h, 0x4000, 1024, buf);
        QueryPerformanceCounter(&stop);
        CHECK(status);

        fprintf(fp, "%0.6f\n", n_secs_elapsed(start, stop, freq));
    }

    fclose(fp);
    free(buf);

    /* Sub-512 byte Program memory read */
    buf = malloc(4);
    ASSERT(buf != NULL);

    fp = fopen("sub_512_program_memory_times.txt", "w");
    ASSERT(fp != NULL);

    fprintf(fp, "Read time\n");

    for (i = 0; i < 1000; i++) {
        QueryPerformanceCounter(&start);
        status = xia_usb2_read(h, 0x0, 4, buf);
        QueryPerformanceCounter(&stop);
        CHECK(status);

        fprintf(fp, "%0.6f\n", n_secs_elapsed(start, stop, freq));
    }

    fclose(fp);
    free(buf);

    /* 1024 byte Program memory read */
    buf = malloc(1024);
    ASSERT(buf != NULL);

    fp = fopen("1024_program_memory_times.txt", "w");
    ASSERT(fp != NULL);

    fprintf(fp, "Read time\n");

    for (i = 0; i < 1000; i++) {
        QueryPerformanceCounter(&start);
        status = xia_usb2_read(h, 0x0, 1024, buf);
        QueryPerformanceCounter(&stop);
        CHECK(status);

        fprintf(fp, "%0.6f\n", n_secs_elapsed(start, stop, freq));
    }

    fclose(fp);
    free(buf);

    status = xia_usb2_close(h);
    CHECK(status);

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
