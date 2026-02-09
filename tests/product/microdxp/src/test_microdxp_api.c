/* SPDX-License-Identifier: Apache-2.0 */

/*
 * Copyright 2024 XIA LLC, All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/** @file test_microdxp.c
 * @brief Tests for MicroDXP specific Handel API functionality.
 */
#include <math.h>
#include <stdlib.h>

#include <util/xia_ary_manip.h>
#include <util/xia_compare.h>
#include <util/xia_sleep.h>

#include <test_helpers.h>

static char CONFIG_FILE[] = "configs/udxp_usb2.ini";

int is_usb() {
    int retval;
    char module_interface[16];

    retval = xiaGetModuleItem("module1", "interface", (void*) module_interface);
    TEST_ASSERT_(retval == XIA_SUCCESS, "xiaGetModuleItem | interface");
    TEST_MSG("xiaGetModuleItem | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

    return (strcmp(module_interface, "usb2") == 0);
}

int is_supermicro() {
    int retval;
    unsigned char board_info[32];

    retval = xiaBoardOperation(0, "get_board_info", (void*) board_info);
    TEST_ASSERT_(retval == XIA_SUCCESS, "xiaBoardOperation | get_board_info");
    TEST_MSG("xiaBoardOperation | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

    return (board_info[1] >= 3);
}

void init(void) {
    int retval;
    xiaSuppressLogOutput();

    retval = xiaInit(CONFIG_FILE);
    TEST_ASSERT_(retval == XIA_SUCCESS, "xiaInit | init fail w/ code %i", retval);

    retval = xiaStartSystem();
    TEST_ASSERT_(retval == XIA_SUCCESS,
                 "xiaStartSystem | system start failed w/ code %i", retval);

    char module_type[256] = {0};
    retval = xiaGetModuleItem("module1", "module_type", (void*) module_type);
    TEST_ASSERT_(retval == XIA_SUCCESS, "xiaGetModuleItem | get mod type w/ code %i",
                 retval);

    TEST_ASSERT_(strcmp(module_type, "udxp") == 0, "%s != udxp", module_type);
}

void board_info(void) {
    int retval;
    unsigned char board_info[26];

    init();

    retval = xiaBoardOperation(0, "get_board_info", (void*) &board_info);
    TEST_CHECK(retval == XIA_SUCCESS);
    TEST_MSG("xiaBoardOperation | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

    TEST_CHECK(board_info[6] != 0);
    TEST_MSG("clock_speed cannot be 0");

    TEST_CHECK(board_info[8] > 0);
    TEST_MSG("nfippi cannot be 0");

    double gain = (((double) (board_info[11] << 8) + board_info[10]) / 32768.) *
                  pow(2, (double) board_info[12]);
    TEST_CHECK(gain > 0);
    TEST_MSG("gain cannot be 0");

    cleanup();
}

void board_features(void) {
    int retval;
    unsigned long features;
    unsigned long version;

    init();

    TEST_CASE("cpld versions");
    {
        retval = xiaBoardOperation(0, "get_board_features", (void*) &features);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaBoardOperation | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

        TEST_CHECK(features != 0);
        TEST_MSG("%lu = 0", features);

        if ((features & 0x05) != 0) {
            TEST_CHECK(xiaBoardOperation(0, "get_udxp_cpld_version",
                                         (void*) &version) == XIA_SUCCESS);
            TEST_CHECK(version > 1L);
            TEST_CHECK(xiaBoardOperation(0, "get_udxp_cpld_variant",
                                         (void*) &version) == XIA_SUCCESS);
            TEST_CHECK(version > 1L);
        }
    }

    cleanup();
}

void board_operations(void) {
    char* acq_names[] = {"baseline_threshold", "energy_threshold", "trigger_threshold",
                         "mca_bin_width"};
    double acq_vals[] = {50., 60., 40., 10.};
    double acq_value;

    int acq_number = sizeof(acq_vals) / sizeof(double);
    int i;

    init();

    TEST_CASE("BoardOperation apply");
    {
        for (i = 0; i < acq_number; i++) {
            acqset(acq_names[i], acq_vals[i]);
            acq_value = acq(acq_names[i]);
            TEST_CHECK(fabs(acq_value - acq_vals[i]) < acq_vals[i] * 0.1);
        }
    }

    cleanup();
}

void detector_polarity(void) {
    unsigned short POL;
    double polarity;

    init();

    TEST_CASE("initial start-up values");
    {
        polarity = acq("polarity");
        POL = dsp("POLARITY");
        TEST_CHECK(polarity == POL);
    }

    TEST_CASE("setting acq polarity");
    {
        polarity = 1.0;
        TEST_CHECK(xiaSetAcquisitionValues(0, "polarity", (void*) &polarity) ==
                   XIA_SUCCESS);
        POL = dsp("POLARITY");
        TEST_CHECK(polarity == POL);
    }

    cleanup();
}

void filter_params(void) {
    double clock_speed, decimation_ratio;
    double peaksam, peakint, acq_val;

    unsigned short peaksam_calculated, peakint_calculated;
    unsigned short PEAKSAM, PEAKINT;

    init();

    TEST_CASE("peaksam_offset");
    {
        clock_speed = acq("clock_speed");
        decimation_ratio = 1. / clock_speed;
        peaksam = xia_round(decimation_ratio * 0.040) * decimation_ratio;

        acqset("peaksam_offset", peaksam);
        acq_val = acq("peaksam_offset");
        TEST_CHECK_(acq_val == peaksam, "peaksam_offset Set, Get | %.2f,  %.2f",
                    peaksam, acq_val);
    }

    TEST_CASE("peaksam offset DSP");
    {
        clock_speed = acq("clock_speed");
        decimation_ratio = 1. / clock_speed;

        peaksam = xia_round(decimation_ratio * 0.080) * decimation_ratio;
        acqset("peaksam_offset", peaksam);

        PEAKSAM = dsp("PEAKSAM");
        peaksam_calculated =
            dsp("SLOWLEN") + dsp("SLOWGAP") - (unsigned short) (peaksam * clock_speed);
        TEST_CHECK(peaksam_calculated == PEAKSAM);
        TEST_MSG("peaksam_calculated, PEAKSAM | %d,  %d", peaksam_calculated, PEAKSAM);

        peaksam = 0.;
        acqset("peaksam_offset", peaksam);

        PEAKSAM = dsp("PEAKSAM");
        peaksam_calculated = dsp("SLOWLEN") + dsp("SLOWGAP");
        TEST_CHECK(peaksam_calculated == PEAKSAM);
        TEST_MSG("peaksam_calculated, PEAKSAM | %d,  %d", peaksam_calculated, PEAKSAM);
    }

    TEST_CASE("peakint_offset");
    {
        clock_speed = acq("clock_speed");
        decimation_ratio = 1. / clock_speed;
        peakint = xia_round(decimation_ratio * 0.040) * decimation_ratio;

        acqset("peakint_offset", peakint);
        acq_val = acq("peakint_offset");
        TEST_CHECK_(acq_val == peakint, "peakint_offset Set, Get | %.2f,  %.2f",
                    peakint, acq_val);
    }

    TEST_CASE("peakint offset DSP");
    {
        clock_speed = acq("clock_speed");
        decimation_ratio = 1. / clock_speed;

        peakint = xia_round(decimation_ratio * 0.080) * decimation_ratio;
        acqset("peakint_offset", peakint);
        PEAKINT = dsp("PEAKINT");
        peakint_calculated =
            dsp("SLOWLEN") + dsp("SLOWGAP") + (unsigned short) (peakint * clock_speed);
        TEST_CHECK(peakint_calculated == PEAKINT);

        peakint = 0.;
        acqset("peakint_offset", peakint);
        PEAKINT = dsp("PEAKINT");
        peakint_calculated = dsp("SLOWLEN") + dsp("SLOWGAP");
        TEST_CHECK(peakint_calculated == PEAKINT);
    }

    cleanup();
}

void fippis(void) {
    int retval;
    unsigned short nfippi, pt_per_fippi;
    double fippi, acq_val;
    double* pt_ranges;
    double* current_pts;
    double* peaking_times;

    init();

    TEST_CASE("fippi and peaking time info");
    {
        TEST_CHECK(xiaBoardOperation(0, "get_number_of_fippis", (void*) &nfippi) ==
                   XIA_SUCCESS);
        TEST_CHECK(nfippi >= 0);
        TEST_CHECK(nfippi <= 3);

        TEST_CHECK(xiaBoardOperation(0, "get_number_pt_per_fippi",
                                     (void*) &pt_per_fippi) == XIA_SUCCESS);
        TEST_CHECK(pt_per_fippi >= 5);
        TEST_CHECK(pt_per_fippi <= 24);

        pt_ranges = malloc(nfippi * 2 * sizeof(double));
        TEST_CHECK(xiaBoardOperation(0, "get_peaking_time_ranges", (void*) pt_ranges) ==
                   XIA_SUCCESS);
        free(pt_ranges);

        current_pts = malloc(pt_per_fippi * sizeof(double));
        TEST_CHECK(xiaBoardOperation(0, "get_current_peaking_times",
                                     (void*) current_pts) == XIA_SUCCESS);
        free(current_pts);

        peaking_times = malloc(nfippi * pt_per_fippi * sizeof(double));
        TEST_CHECK(xiaBoardOperation(0, "get_peaking_times", (void*) peaking_times) ==
                   XIA_SUCCESS);
        free(peaking_times);
    }

    TEST_CASE("fippi switching");
    {
        retval = xiaBoardOperation(0, "get_number_of_fippis", (void*) &nfippi);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaBoardOperation | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

        if (nfippi == 1) {
            fippi = 0.;
            retval = xiaSetAcquisitionValues(0, "fippi", (void*) &fippi);
            TEST_CHECK_(retval == DXP_STATUS_ERROR, "xiaGetAcquisitionValues | %s %.2f",
                        "fippi", fippi);
            TEST_MSG("xiaSetAcquisitionValues | %s",
                     tst_msg(errmsg, MSGLEN, retval, DXP_STATUS_ERROR));
            acq_val = acq("fippi");
            TEST_CHECK(fippi == acq_val);
        } else if (nfippi > 1) {
            fippi = 1.;
            acqset("fippi", fippi);
            acq_val = acq("fippi");
            TEST_CHECK(fippi == acq_val);
        }

        fippi = nfippi;
        retval = xiaSetAcquisitionValues(0, "fippi", (void*) &fippi);
        TEST_CHECK(retval == XIA_FIP_OOR);
    }
    cleanup();
}

void gain(void) {
    int retval;
    double gain;

    double acqval;
    unsigned short gain_mode;
    unsigned short dspval;

    unsigned short AV_MEM_PARSET = 0x4;
    unsigned short AV_MEM_GENSET = 0x8;

    init();

    TEST_CASE("get_gain_mode");
    {
        retval = xiaBoardOperation(0, "get_gain_mode", (void*) &gain_mode);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaBoardOperation | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

        TEST_CHECK_(gain_mode >= 0 && gain_mode <= 4, "get_gain_mode | %hu", gain_mode);
    }

    TEST_CASE("Switched gain mode");
    {
        TEST_CHECK(xiaBoardOperation(0, "get_gain_mode", (void*) &gain_mode) ==
                   XIA_SUCCESS);

        if (gain_mode == 3) {
            gain = 11.;
            TEST_CHECK(xiaSetAcquisitionValues(0, "gain", (void*) &gain) ==
                       XIA_SUCCESS);
            TEST_CHECK(xiaBoardOperation(0, "apply", (void*) &AV_MEM_GENSET) ==
                       XIA_SUCCESS);

            dspval = dsp("SWGAIN");
            TEST_CHECK_(dspval == 5, "SWGAIN | %hu", dspval);

            dspval = dsp("DGAINBASEEXP");
            TEST_CHECK_(dspval == 0, "DGAINBASEEXP | %hu", dspval);

            dspval = dsp("DGAINBASE");
            TEST_CHECK_(dspval == 35332, "DGAINBASE | %hu", dspval);

            gain = 14.109;
            TEST_CHECK(xiaSetAcquisitionValues(0, "gain", (void*) &gain) ==
                       XIA_SUCCESS);
            TEST_CHECK(xiaBoardOperation(0, "apply", (void*) &AV_MEM_GENSET) ==
                       XIA_SUCCESS);

            dspval = dsp("SWGAIN");
            TEST_CHECK_(dspval == 7, "SWGAIN | %hu", dspval);

            dspval = dsp("DGAINBASEEXP");
            TEST_CHECK_(dspval == 65535, "DGAINBASEEXP | %hu", dspval);

            dspval = dsp("DGAINBASE");
            TEST_CHECK_(fabs(dspval - 61209) < 20., "DGAINBASE | %hu", dspval);
        }
    }

    TEST_CASE("High-low gain mode");
    {
        TEST_CHECK(xiaBoardOperation(0, "get_gain_mode", (void*) &gain_mode) ==
                   XIA_SUCCESS);

        if (gain_mode == 4) {
            gain = 1.238;
            TEST_CHECK(xiaSetAcquisitionValues(0, "gain", (void*) &gain) ==
                       XIA_SUCCESS);
            TEST_CHECK(xiaBoardOperation(0, "apply", (void*) &AV_MEM_GENSET) ==
                       XIA_SUCCESS);

            dspval = dsp("SWGAIN");
            TEST_CHECK_(dspval == 1, "SWGAIN | %hu", dspval);

            dspval = dsp("DGAINBASEEXP");
            TEST_CHECK_(dspval == 0, "DGAINBASEEXP | %hu", dspval);

            dspval = dsp("DGAINBASE");
            TEST_CHECK_(dspval == 33481, "DGAINBASE | %hu", dspval);

            gain = 3.;
            TEST_CHECK(xiaSetAcquisitionValues(0, "gain", (void*) &gain) ==
                       XIA_SUCCESS);
            TEST_CHECK(xiaBoardOperation(0, "apply", (void*) &AV_MEM_GENSET) ==
                       XIA_SUCCESS);

            dspval = dsp("SWGAIN");
            TEST_CHECK_(dspval == 0, "SWGAIN | %hu", dspval);

            dspval = dsp("DGAINBASEEXP");
            TEST_CHECK_(dspval == 0, "DGAINBASEEXP | %hu", dspval);

            dspval = dsp("DGAINBASE");
            TEST_CHECK_(dspval == 40567, "DGAINBASE | %hu", dspval);
        }
    }

    TEST_CASE("peak_mode");
    {
        if (is_supermicro()) {
            acqval = 0.;
            TEST_CHECK(xiaSetAcquisitionValues(0, "peak_mode", (void*) &acqval) ==
                       XIA_SUCCESS);
            TEST_CHECK(xiaBoardOperation(0, "apply", (void*) &AV_MEM_PARSET) ==
                       XIA_SUCCESS);

            dspval = dsp("PEAKMODE");
            TEST_CHECK_(dspval == 0, "PEAKMODE | %hu", dspval);

            acqval = 1.;
            TEST_CHECK(xiaSetAcquisitionValues(0, "peak_mode", (void*) &acqval) ==
                       XIA_SUCCESS);
            TEST_CHECK(xiaBoardOperation(0, "apply", (void*) &AV_MEM_PARSET) ==
                       XIA_SUCCESS);

            dspval = dsp("PEAKMODE");
            TEST_CHECK_(dspval == 1, "PEAKMODE | %hu", dspval);
        }
    }

    TEST_CASE("baseline_factor");
    {
        if (is_supermicro()) {
            acqval = acq("baseline_factor");
            TEST_CHECK_(acqval >= 0. && acqval <= 1., "baseline_factor | %.2f", acqval);

            dspval = dsp("BFACTOR");
            TEST_CHECK_(dspval == acqval, "BFACTOR | %hu", dspval);
        }
    }
    cleanup();
}

void gain_calibrate(void) {
    double original_gain, scaled_gain;
    double gain_scale = 1.5;

    init();

    TEST_CASE("xiaGainCalibrate");
    {
        original_gain = acq("gain");
        TEST_CHECK(xiaGainCalibrate(0, gain_scale) == XIA_SUCCESS);
        scaled_gain = acq("gain");

        TEST_CHECK(xia_pct_diff(original_gain, scaled_gain / gain_scale, 1));
        TEST_MSG("%.2f not within 2%% of %.2f", original_gain, scaled_gain);
        acqset("gain", original_gain);
    }

    TEST_CASE("xiaGainOperation");
    {
        original_gain = acq("gain");
        TEST_CHECK(xiaGainOperation(0, "calibrate", (void*) &gain_scale) ==
                   XIA_SUCCESS);
        scaled_gain = acq("gain");

        TEST_CHECK(xia_pct_diff(original_gain, scaled_gain / gain_scale, 1));
        TEST_MSG("%.2f not within 2%% of %.2f", original_gain, scaled_gain);
        acqset("gain", original_gain);
    }
    cleanup();
}

void preset_run(void) {
    int retval;
    double run_length_s = 0.5;
    unsigned int run_length_ms = (unsigned int) xia_round(run_length_s * 1000);
    unsigned short busy;

    init();

    TEST_CASE("Indefinite length");
    {
        acqset("preset_type", 0.);

        retval = xiaStartRun(0, 0);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaStartRun | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

        busy = dsp("BUSY");
        TEST_CHECK(busy != 0);
        TEST_MSG("%u = %u", busy, 0);

        TEST_CHECK(xiaStopRun(0) == XIA_SUCCESS);
        TEST_MSG("xiaStopRun | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

        busy = dsp("BUSY");
        TEST_CHECK(busy == 0);
        TEST_MSG("%u != %u", busy, 0);
    }

    TEST_CASE("Real time");
    {
        acqset("preset_type", 1.);
        acqset("preset_value", run_length_s);

        TEST_CHECK(xiaStartRun(0, 0) == XIA_SUCCESS);
        TEST_MSG("xiaStartRun | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

        xia_sleep(run_length_ms);

        busy = dsp("BUSY");
        TEST_CHECK(busy == 0);
        TEST_MSG("%u != %u", busy, 0);

        double realtime = 0.;
        retval = xiaGetRunData(0, "realtime", &realtime);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaGetRunData | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

        TEST_CHECK(xia_approx_dbl(realtime, run_length_s, 0.06));
        TEST_MSG("%f != %f", realtime, run_length_s);
    }

    TEST_CASE("Live time");
    {
        acqset("preset_type", 2.);
        acqset("preset_value", run_length_s);

        TEST_CHECK(xiaStartRun(0, 0) == XIA_SUCCESS);
        TEST_MSG("xiaStartRun | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

        xia_sleep(run_length_ms);

        busy = dsp("BUSY");
        TEST_CHECK(busy == 0);
        TEST_MSG("%u != %u", busy, 0);

        double livetime = 0.;
        retval = xiaGetRunData(0, "realtime", &livetime);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaGetRunData | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

        TEST_CHECK(xia_approx_dbl(livetime, run_length_s, 0.06));
        TEST_MSG("%f != %f", livetime, run_length_s);
    }

    cleanup();
}

void parameters(void) {
    int retval;
    unsigned short param, new_param, old_param;

    init();

    TEST_CASE("TRACEWAIT");
    {
        old_param = dsp("TRACEWAIT");

        param = old_param + 1;

        retval = xiaSetParameter(0, "TRACEWAIT", param);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaSetParameter | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
        new_param = dsp("TRACEWAIT");

        TEST_CHECK(new_param == param);
        TEST_MSG("%u != %u", new_param, param);

        retval = xiaSetParameter(0, "TRACEWAIT", old_param);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaSetParameter | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
    }
    cleanup();
}

void statistics(void) {
    int retval;

    double ns_per_tick = 500E-9;
    unsigned int test_time_ms = 500;
    double test_time_s = test_time_ms / 1000.;

    init();

    TEST_CASE("Execute data run");
    {
        retval = xiaStartRun(0, 0);
        TEST_ASSERT(retval == XIA_SUCCESS);
        TEST_MSG("xiaStartRun | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

        xia_sleep(test_time_ms);

        retval = xiaStopRun(0);
        TEST_ASSERT(retval == XIA_SUCCESS);
        TEST_MSG("xiaStopRun | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
    }

    TEST_CASE("Get run statistics");
    double mod_stats[9];
    {
        retval = xiaGetRunData(0, "module_statistics_2", &mod_stats);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaGetRunData | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
    }

    TEST_CASE("Run length");
    double runtime = 0;
    {
        retval = xiaGetRunData(0, "runtime", &runtime);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaGetRunData | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

        TEST_CHECK(xia_approx_dbl(runtime, test_time_s, 0.06));
        TEST_MSG("%f != %f", runtime, test_time_s);
    }

    TEST_CASE("Real time");
    double realtime = 0;
    {
        retval = xiaGetRunData(0, "realtime", &realtime);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaGetRunData | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

        double realtime_dsp =
            ns_per_tick *
            (((long long) dsp("REALTIMEHI") << 32) +
             ((unsigned long) dsp("REALTIMEMID") << 16) + dsp("REALTIMELO"));

        TEST_CHECK(xia_approx_dbl(realtime, test_time_s, 0.06));
        TEST_MSG("%f != %f", realtime, test_time_s);

        TEST_CHECK(xia_approx_dbl(realtime_dsp, test_time_s, 0.06));
        TEST_MSG("%f != %f", realtime, test_time_s);

        TEST_CHECK(xia_approx_dbl(mod_stats[0], test_time_s, 0.06));
        TEST_MSG("%f != %f", mod_stats[0], test_time_s);
    }

    TEST_CASE("Trigger live time");
    double trigger_livetime = 0;
    {
        retval = xiaGetRunData(0, "trigger_livetime", &trigger_livetime);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaGetRunData | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

        double livetime_dsp =
            ns_per_tick *
            (((long long) dsp("LIVETIMEHI") << 32) +
             ((unsigned long) dsp("LIVETIMEMID") << 16) + dsp("LIVETIMELO"));

        TEST_CHECK(xia_approx_dbl(trigger_livetime, test_time_s, 0.1));
        TEST_MSG("%f != %f", trigger_livetime, test_time_s);

        TEST_CHECK(xia_approx_dbl(livetime_dsp, test_time_s, 0.1));
        TEST_MSG("%f != %f", livetime_dsp, test_time_s);

        TEST_CHECK(xia_approx_dbl(mod_stats[1], test_time_s, 0.1));
        TEST_MSG("%f != %f", mod_stats[1], test_time_s);
    }

    TEST_CASE("Input Count Rate");
    double icr = 0;
    {
        TEST_CHECK(xiaGetRunData(0, "input_count_rate", &icr) == XIA_SUCCESS);
        TEST_MSG("xiaGetRunData | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
        TEST_CHECK(xia_approx_dbl(icr, mod_stats[5], 0.001));
        TEST_MSG("%f != %f", icr, mod_stats[5]);
    }

    TEST_CASE("Output Count Rate");
    double ocr = 0;
    {
        TEST_CHECK(xiaGetRunData(0, "output_count_rate", &ocr) == XIA_SUCCESS);
        TEST_MSG("xiaGetRunData | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
        TEST_CHECK(xia_approx_dbl(ocr, mod_stats[6], 0.001));
        TEST_MSG("%f != %f", ocr, mod_stats[6]);
    }

    TEST_CASE("energy_livetime");
    double energy_livetime = 0;
    {
        TEST_CHECK(xiaGetRunData(0, "energy_livetime", &energy_livetime) ==
                   XIA_SUCCESS);
        TEST_MSG("xiaGetRunData | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

        TEST_CHECK(xia_approx_dbl(energy_livetime, test_time_s, 0.1));
        TEST_MSG("%f != %f", energy_livetime, test_time_s);

        double elt_calc = realtime * ocr / icr;
        TEST_CHECK(xia_approx_dbl(energy_livetime, elt_calc, 0.001));
        TEST_MSG("%f != %f", energy_livetime, elt_calc);
    }

    TEST_CASE("triggers");
    {
        unsigned long triggers = 0;
        TEST_CHECK(xiaGetRunData(0, "triggers", &triggers) == XIA_SUCCESS);
        double fastpeaks_dsp =
            (double) ((unsigned long) dsp("FASTPEAKSHI") << 16) + dsp("FASTPEAKSLO");
        TEST_CHECK(xia_approx_dbl(triggers, fastpeaks_dsp, 0));
        TEST_MSG("%lu != %f", triggers, fastpeaks_dsp);
    }

    TEST_CASE("mca_events");
    unsigned long hist_evts = 0;
    {
        unsigned long mca_events;
        TEST_CHECK(xiaGetRunData(0, "mca_events", &mca_events) == XIA_SUCCESS);

        unsigned long eventsinrun =
            ((unsigned long) dsp("EVTSINRUNHI") << 16) + dsp("EVTSINRUNLO");

        unsigned long mca_length = 0;
        TEST_CHECK(xiaGetRunData(0, "mca_length", &mca_length) == XIA_SUCCESS);

        unsigned long* mca = malloc(mca_length * sizeof(unsigned long));
        TEST_CHECK(xiaGetRunData(0, "mca", mca) == XIA_SUCCESS);

        hist_evts = 0;
        for (unsigned long i = 0; i < mca_length; i++) {
            hist_evts += mca[i];
        }
        free(mca);

        TEST_CHECK(hist_evts == mca_events);
        TEST_MSG("%lu != %lu", hist_evts, mca_events);

        TEST_CHECK(mca_events == eventsinrun);
        TEST_MSG("%lu != %lu", mca_events, eventsinrun);
    }

    TEST_CASE("underflows");
    {
        unsigned long underflows =
            ((unsigned long) dsp("UNDRFLOWSHI") << 16) + dsp("UNDRFLOWSLO");
        TEST_CHECK(xia_approx_dbl(mod_stats[7], underflows, 0));
        TEST_MSG("%f != %lu", mod_stats[7], underflows);
    }

    TEST_CASE("overflows");
    {
        unsigned long overflows =
            ((unsigned long) dsp("OVERFLOWSHI") << 16) + dsp("OVERFLOWSLO");
        TEST_CHECK(mod_stats[8] == overflows);
        TEST_MSG("%f != %lu", mod_stats[8], overflows);
    }

    cleanup();
}

void thresholds(void) {
    int retval;
    double threshold, acq_val;

    int i;
    char* threshold_types[] = {"trigger_threshold", "baseline_threshold",
                               "energy_threshold"};

    init();

    for (i = 0; i < sizeof(threshold_types) / sizeof(threshold_types[0]); i++) {
        TEST_CASE(threshold_types[i]);
        {
            threshold = 2.;
            acqset(threshold_types[i], threshold);
            acq_val = acq(threshold_types[i]);
            TEST_CHECK(acq_val == threshold);

            threshold = 255.;
            acqset(threshold_types[i], threshold);
            acq_val = acq(threshold_types[i]);
            TEST_CHECK(acq_val == threshold);

            threshold = 4096.;
            retval = xiaSetAcquisitionValues(0, threshold_types[i], (void*) &threshold);
            TEST_CHECK(retval == XIA_THRESH_OOR);

            threshold = 255.;
            acq_val = acq(threshold_types[i]);
            TEST_CHECK(acq_val == threshold);
        }
    }
    cleanup();
}

void trace_read(void) {
    int retval;
    double trace_info[2] = {0., 25.};

    int num_trace_types = 10;
    char* trace_types[] = {"adc_trace",
                           "adc_average",
                           "fast_filter",
                           "raw_intermediate_filter",
                           "baseline_samples",
                           "baseline_average",
                           "raw_intermediate_filter",
                           "raw_slow_filter",
                           "scaled_slow_filter",
                           "debug"};

    init();

    unsigned long adc_trace_length;
    TEST_CASE("Get ADC Trace Length");
    {
        retval = xiaGetSpecialRunData(0, "adc_trace_length", (void*) &adc_trace_length);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaGetSpecialRunData | %i != %i", retval, XIA_SUCCESS);

        TEST_CHECK(adc_trace_length != 0);
        TEST_MSG("adc_trace_length = 0");
    }

    unsigned long* adc_trace = malloc(adc_trace_length * sizeof(unsigned long));
    unsigned long* adc_trace_2 = malloc(adc_trace_length * sizeof(unsigned long));

    for (int j = 0; j < num_trace_types; j++) {
        xia_fill_ulong_ary(adc_trace, adc_trace_length, 0xDEADBEEF);
        xia_fill_ulong_ary(adc_trace_2, adc_trace_length, 0xDEADBEEF);

        TEST_CASE(trace_types[j]);
        {
            retval = xiaDoSpecialRun(0, trace_types[j], &trace_info);
            TEST_CHECK(retval == XIA_SUCCESS);
            TEST_MSG("xiaDoSpecialRun | %i != %i", retval, XIA_SUCCESS);

            retval = xiaGetSpecialRunData(0, "adc_trace", adc_trace);
            TEST_CHECK(retval == XIA_SUCCESS);
            TEST_MSG("xiaGetSpecialRunData | %i != %i", retval, XIA_SUCCESS);

            /* Do a second run for comparison */
            retval = xiaDoSpecialRun(0, trace_types[j], &trace_info);
            TEST_CHECK(retval == XIA_SUCCESS);
            TEST_MSG("xiaDoSpecialRun | %i != %i", retval, XIA_SUCCESS);

            retval = xiaGetSpecialRunData(0, "adc_trace", adc_trace_2);
            TEST_CHECK(retval == XIA_SUCCESS);
            TEST_MSG("xiaGetSpecialRunData | %i != %i", retval, XIA_SUCCESS);

            for (unsigned long k = 0; k < adc_trace_length; k++) {
                TEST_CHECK(xia_pct_diff(adc_trace[k], adc_trace_2[k], 5));
                TEST_MSG("trace data mismatch | %lu != %lu", adc_trace[k],
                         adc_trace_2[k]);
            }
        }
    }
    free(adc_trace);
    free(adc_trace_2);
    cleanup();
}

void usb_info(void) {
    int retval;
    unsigned long version = 0;

    init();

    TEST_CASE("usb version");
    {
        if (is_usb()) {
            retval = xiaBoardOperation(0, "get_usb_version", &version);
            TEST_CHECK(retval == XIA_SUCCESS);
            TEST_CHECK(version > 1L);
        }
    }
    cleanup();
}

TEST_LIST = {
    {"Board Information", board_info},
    {"Board features", board_features},
    {"Board Operations", board_operations},
    {"Detector Polarity", detector_polarity},
    {"Filter Parameters", filter_params},
    {"FIPPIs", fippis},
    {"Gain", gain},
    {"Gain calibration", gain_calibrate},
    {"Parameters", parameters},
    {"Preset Runs", preset_run},
    {"Statistics", statistics},
    {"Thresholds", thresholds},
    {"Traces", trace_read},
    {"USB info", usb_info},
    {NULL, NULL} /* zeroed record marking the end of the list */
};
