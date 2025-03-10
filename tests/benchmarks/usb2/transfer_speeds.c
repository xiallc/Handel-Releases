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
    int j;

    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    LARGE_INTEGER stop;

    HANDLE h;

    unsigned long read_lens[] = {256, 512, 1024, 2048, 4096, 8192};

    byte_t* buf = NULL;

    double read_time;

    QueryPerformanceFrequency(&freq);

    status = xia_usb2_open(0, &h);
    CHECK(status);

    for (i = 0; i < N_ELEMS(read_lens); i++) {
        buf = malloc(read_lens[i] * 2);
        ASSERT(buf != NULL);

        for (j = 0, read_time = 0.0; j < 1000; j++) {
            QueryPerformanceCounter(&start);
            status = xia_usb2_read(h, 0x2000, read_lens[i] * 2, buf);
            QueryPerformanceCounter(&stop);
            CHECK(status);

            read_time += n_secs_elapsed(start, stop, freq);
        }

        free(buf);

        printf("Transfer speed (%lu words) = %0.3f MB/s\n", read_lens[i],
               (((double) read_lens[i] * 2.0 * 1000.0) / 1045876.0) / read_time);
    }

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
