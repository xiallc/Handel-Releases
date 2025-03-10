#include "windows.h"

#include <stdio.h>
#include <stdlib.h>

#include "xia_common.h"

#include "handel.h"
#include "handel_errors.h"
#include "handel_constants.h"

#include "md_generic.h"

#pragma warning(disable : 4127)
#pragma warning(disable : 4127)

#define CHECK(x)                                                                       \
    do {                                                                               \
        int status = x;                                                                \
        if (status != XIA_SUCCESS) {                                                   \
            fprintf(stderr, "Handel call failed with status = %d.\n", status);         \
            exit(status);                                                              \
        }                                                                              \
    } while (0)

static void print_usage(void);

static void config_austinai_mapping_mode(int total_chans);
static void config_austinai_mapping_mode_sca(int total_chans);
static void config_safe_mapping_mode(int total_chans);
static double n_secs_elapsed(LARGE_INTEGER start, LARGE_INTEGER stop,
                             LARGE_INTEGER freq);

int main(int argc, char* argv[]) {
    LARGE_INTEGER freq;
    LARGE_INTEGER start;
    LARGE_INTEGER stop;

    unsigned long buf_len;

    unsigned long* buf;

    double* read_times;
    double* done_times;

    int n_iters;
    int i = 0;
    int n_chans = 0;

    FILE* fp;

    unsigned int n_mods = 0;

    if (argc < 2) {
        print_usage();
        exit(1);
    }

    QueryPerformanceFrequency(&freq);

    CHECK(xiaSetLogOutput("handel.log"));
    CHECK(xiaSetLogLevel(MD_DEBUG));

    CHECK(xiaInit(argv[1]));
    CHECK(xiaStartSystem());

    CHECK(xiaGetNumModules(&n_mods));
    n_chans = n_mods * 4;

    config_austinai_mapping_mode_sca(n_chans);

    CHECK(xiaGetRunData(0, "buffer_len", &buf_len));

    fprintf(stdout, "Buffer len = %lu\n", buf_len);

    buf = malloc(buf_len * sizeof(unsigned long));

    if (!buf) {
        fprintf(stderr, "Unable to allocate %zd bytes for buffer.\n",
                buf_len * sizeof(unsigned long));
        exit(-1);
    }

    for (i = 0; i < n_chans; i += 4) {
        double size = (double) buf_len;
        CHECK(xiaSetAcquisitionValues(i, "buffer_clear_size", &size));
        fprintf(stdout, "buffer_clear_size set to %0.1f\n", size);
    }

    sscanf(argv[2], "%d", &n_iters);
    fprintf(stdout, "Running for %d iters worth of buffer reads.\n", n_iters);

    read_times = malloc(n_iters * sizeof(double));

    if (!read_times) {
        fprintf(stderr, "Unable to allocate %zd bytes for read_times.\n",
                n_iters * sizeof(double));
        exit(-1);
    }

    done_times = malloc(n_iters * sizeof(double));

    if (!done_times) {
        fprintf(stderr, "Unable to allocate %zd bytes for done_times.\n",
                n_iters * sizeof(double));
        exit(-1);
    }

    i = 0;

    CHECK(xiaStartRun(-1, 0));

    while (i < n_iters) {
        unsigned short is_full = 0;
        char b = 'a';
        unsigned int j;

        fprintf(stdout, "Iter %d\n", i);

        /* Wait for A to signal complete. */
        do {
            unsigned short f;

            is_full = 0;
            Sleep(1);

            for (j = 0; j < n_mods; j++) {
                CHECK(xiaGetRunData(j * 4, "buffer_full_a", &f));
                is_full += f;
            }
        } while (is_full != n_mods);

        QueryPerformanceCounter(&start);
        for (j = 0; j < n_mods; j++) {
            CHECK(xiaGetRunData(j * 4, "buffer_a", buf));
        }
        QueryPerformanceCounter(&stop);

        read_times[i] = n_secs_elapsed(start, stop, freq);

        QueryPerformanceCounter(&start);
        for (j = 0; j < n_mods; j++) {
            CHECK(xiaBoardOperation(j * 4, "buffer_done", &b));
        }
        QueryPerformanceCounter(&stop);

        done_times[i] = n_secs_elapsed(start, stop, freq);
        i++;

        is_full = 0;
        b = 'b';

        /* Wait for B to signal complete. */
        do {
            unsigned short f;

            is_full = 0;
            Sleep(1);

            for (j = 0; j < n_mods; j++) {
                CHECK(xiaGetRunData(j * 4, "buffer_full_b", &f));
                is_full += f;
            }
        } while (is_full != n_mods);

        QueryPerformanceCounter(&start);
        for (j = 0; j < n_mods; j++) {
            CHECK(xiaGetRunData(j * 4, "buffer_b", buf));
        }
        QueryPerformanceCounter(&stop);

        read_times[i] = n_secs_elapsed(start, stop, freq);

        QueryPerformanceCounter(&start);
        for (j = 0; j < n_mods; j++) {
            CHECK(xiaBoardOperation(j * 4, "buffer_done", &b));
        }
        QueryPerformanceCounter(&stop);

        done_times[i] = n_secs_elapsed(start, stop, freq);
        i++;
    }

    CHECK(xiaStopRun(-1));

    fp = fopen("read_times.txt", "w");

    if (!fp) {
        fprintf(stderr, "Unable to open read_times.txt.\n");
        exit(-1);
    }

    for (i = 0; i < n_iters; i++) {
        fprintf(fp, "%0.12f\n", read_times[i]);
    }

    fclose(fp);

    fp = fopen("done_times.txt", "w");

    if (!fp) {
        fprintf(stderr, "Unable to open done_times.txt.\n");
        exit(-1);
    }

    for (i = 0; i < n_iters; i++) {
        fprintf(fp, "%0.12f\n", done_times[i]);
    }

    fclose(fp);

    free(buf);
    free(read_times);
    free(done_times);

    xiaExit();

    return 0;
}

/* This setup is specific to Austin AI's low-latency application. */
static void config_austinai_mapping_mode(int total_chans) {
    double vals[] = {1.0, 2048.0, 0.0, 20.0, 0.0};

    char* names[] = {"mapping_mode", "number_mca_channels", "num_map_pixels",
                     "num_map_pixels_per_buffer", "synchronous_run"};

    double master = 1.0;
    double pix_mode = XIA_MAPPING_CTL_GATE;

    int i;
    int j;

    for (i = 0; i < (sizeof(vals) / sizeof(vals[0])); i++) {
        for (j = 0; j < total_chans; j++) {
            CHECK(xiaSetAcquisitionValues(j, names[i], &(vals[i])));
        }
    }

    /* Special acq vals that shouldn't be applied to all channels. */
    CHECK(xiaSetAcquisitionValues(0, "gate_master", &master));
    CHECK(xiaSetAcquisitionValues(0, "pixel_advance_mode", &pix_mode));

    for (i = 0; i < total_chans; i += 4) {
        CHECK(xiaBoardOperation(i, "apply", &i));
    }
}

static void config_safe_mapping_mode(int total_chans) {
    double vals[] = {1.0, 2048.0, 0.0, -1.0, 0.0};

    char* names[] = {"mapping_mode", "number_mca_channels", "num_map_pixels",
                     "num_map_pixels_per_buffer", "synchronous_run"};

    double master = 1.0;
    double pix_mode = XIA_MAPPING_CTL_GATE;

    int i;
    int j;

    for (i = 0; i < (sizeof(vals) / sizeof(vals[0])); i++) {
        for (j = 0; j < total_chans; j++) {
            CHECK(xiaSetAcquisitionValues(j, names[i], &(vals[i])));
        }
    }

    /* Special acq vals that shouldn't be applied to all channels. */
    CHECK(xiaSetAcquisitionValues(0, "gate_master", &master));
    CHECK(xiaSetAcquisitionValues(0, "pixel_advance_mode", &pix_mode));

    for (i = 0; i < total_chans; i += 4) {
        CHECK(xiaBoardOperation(i, "apply", &i));
    }
}

/*
 * Converts two values returned by QueryPerformanceCounter() into seconds.
 */
static double n_secs_elapsed(LARGE_INTEGER start, LARGE_INTEGER stop,
                             LARGE_INTEGER freq) {
    return ((double) (stop.QuadPart - start.QuadPart) / (double) freq.QuadPart);
}

static void config_austinai_mapping_mode_sca(int total_chans) {
    char* names[] = {"mapping_mode",   "num_map_pixels", "num_map_pixels_per_buffer",
                     "number_of_scas", "sca0_lo",        "sca0_hi",
                     "sca1_lo",        "sca1_hi",        "sca2_lo",
                     "sca2_hi",        "sca3_lo",        "sca3_hi",
                     "sca4_lo",        "sca4_hi"};

    double vals[] = {2.0,  0.0,  2.0,  5.0,  1.0,  10.0,  20.0,
                     50.0, 60.0, 65.0, 70.0, 80.0, 100.0, 1000.0};

    double master = 1.0;
    double pix_mode = XIA_MAPPING_CTL_GATE;

    int i;
    int j;

    for (i = 0; i < (sizeof(vals) / sizeof(vals[0])); i++) {
        for (j = 0; j < total_chans; j++) {
            CHECK(xiaSetAcquisitionValues(j, names[i], &(vals[i])));
        }
    }

    /* Special acq vals that shouldn't be applied to all channels. */
    CHECK(xiaSetAcquisitionValues(0, "gate_master", &master));
    CHECK(xiaSetAcquisitionValues(0, "pixel_advance_mode", &pix_mode));

    for (i = 0; i < total_chans; i += 4) {
        CHECK(xiaBoardOperation(i, "apply", &i));
    }
}

static void print_usage(void) {
    fprintf(stdout, "Arguments: [.ini file]]\n");
    return;
}
