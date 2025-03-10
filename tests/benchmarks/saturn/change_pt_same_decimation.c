#define XIA_ENABLE_TIMING_HELPERS
#include "xia_timing_helpers.h"

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

    double pt;

    INIT_TIMER_VARS;

    INIT_TIMER_FREQ;
    INIT_TIMER_LOG("xiaSetAcquisitionValues.log");

    if (argc != 2) {
        printf("Expects an argument with a valid Saturn .ini file path.\n");
        exit(1);
    }

    status = xiaInit(argv[1]);
    CHECK(status);
    status = xiaStartSystem();
    CHECK(status);

    pt = 2.0;
    status = xiaSetAcquisitionValues(0, "peaking_time", &pt);
    CHECK(status);

    for (i = 0; i < 100; i++) {
        pt = 2.0;
        status = xiaSetAcquisitionValues(0, "peaking_time", &pt);
        CHECK(status);

        pt = 2.1;
        START_TIMER;
        status = xiaSetAcquisitionValues(0, "peaking_time", &pt);
        STOP_TIMER;
        CHECK(status);
        LOG_TIME("xiaSetAcquisitionValues_Time", CALCULATE_TIME);
    }

    DESTROY_TIMER;

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
