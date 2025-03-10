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

    double peaking_time;
    double mca_bin_width;
    double trig_thresh;
    double energy_thresh;
    double calib_ev;
    double adc_rule;
    double preset;

    parameter_t BUSY;

    unsigned long mca[4096];

    for (i = 0; i < 10; i++) {
        /*
         * This is the exact sequence that shows the leak in SIINT's test application.
         */
        status = xiaInit(argv[1]);
        CHECK(status);
        status = xiaStartSystem();
        CHECK(status);

        status = xiaSetParameter(0, "MCALIMHI", 4096);
        CHECK(status);

        mca_bin_width = 10.0;
        status = xiaSetAcquisitionValues(0, "mca_bin_width", &mca_bin_width);
        CHECK(status);

        peaking_time = 1.0;
        status = xiaSetAcquisitionValues(0, "peaking_time", &peaking_time);
        CHECK(status);

        trig_thresh = 650.0;
        status = xiaSetAcquisitionValues(0, "trigger_threshold", &trig_thresh);
        CHECK(status);

        energy_thresh = 0.0;
        status = xiaSetAcquisitionValues(0, "energy_threshold", &energy_thresh);
        CHECK(status);

        calib_ev = 5900.0;
        status = xiaSetAcquisitionValues(0, "calibration_energy", &calib_ev);
        CHECK(status);

        status = xiaSetParameter(0, "GAINDAC", 34000);
        CHECK(status);

        adc_rule = 5.0;
        status = xiaSetAcquisitionValues(0, "adc_percent_rule", &adc_rule);
        CHECK(status);

        status = xiaInit(argv[1]);
        CHECK(status);
        status = xiaStartSystem();
        CHECK(status);

        status = xiaSetParameter(0, "MCALIMHI", 4096);
        CHECK(status);

        mca_bin_width = 10.0;
        status = xiaSetAcquisitionValues(0, "mca_bin_width", &mca_bin_width);
        CHECK(status);

        peaking_time = 1.0;
        status = xiaSetAcquisitionValues(0, "peaking_time", &peaking_time);
        CHECK(status);

        trig_thresh = 650.0;
        status = xiaSetAcquisitionValues(0, "trigger_threshold", &trig_thresh);
        CHECK(status);

        energy_thresh = 0.0;
        status = xiaSetAcquisitionValues(0, "energy_threshold", &energy_thresh);
        CHECK(status);

        calib_ev = 5900.0;
        status = xiaSetAcquisitionValues(0, "calibration_energy", &calib_ev);
        CHECK(status);

        status = xiaSetParameter(0, "GAINDAC", 34000);
        CHECK(status);

        adc_rule = 5.0;
        status = xiaSetAcquisitionValues(0, "adc_percent_rule", &adc_rule);
        CHECK(status);

        preset = 10.0;
        status = xiaSetAcquisitionValues(0, "preset_livetime", &preset);
        CHECK(status);

        status = xiaStartRun(0, 0);
        CHECK(status);

        do {
            status = xiaGetParameter(0, "BUSY", &BUSY);
            CHECK(status);
            Sleep(1);
        } while (BUSY != 0);

        status = xiaStopRun(0);
        CHECK(status);

        status = xiaGetRunData(0, "mca", &mca);
        CHECK(status);
    }

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
