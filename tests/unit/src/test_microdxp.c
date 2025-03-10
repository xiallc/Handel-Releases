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
#include <stdio.h>
#include <stdlib.h>

#include "test_utils.h"

#ifdef _MSC_VER
#pragma warning(push)
/* Disable warning "conditional expression is constant"
     * building TEST_ASSERT with VS100 */
#pragma warning(disable : 4127)
#endif

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

void init_udxp() {
    int retval;
    char module_type[256];
    char* ini = TEST_INI;

    TEST_ASSERT_(xiaSetLogLevel(4) == XIA_SUCCESS, "xiaSetLogLevel");
    TEST_ASSERT_(xiaSetLogOutput("unit_test.log") == XIA_SUCCESS, "xiaSetLogOutput");

    retval = xiaInit(ini);
    TEST_ASSERT_(retval == XIA_SUCCESS, "xiaInit");
    TEST_MSG("xiaInit | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

    retval = xiaStartSystem();
    TEST_ASSERT_(retval == XIA_SUCCESS, "xiaStartSystem");
    TEST_MSG("xiaStartSystem | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

    /* Check that module is udxp and exit test if not */
    retval = xiaGetModuleItem("module1", "module_type", (void*) module_type);
    TEST_ASSERT_(retval == XIA_SUCCESS, "xiaGetModuleItem | module_type %s",
                 module_type);
    TEST_ASSERT_(strcmp(module_type, "udxp") == 0, "module_type is udxp");
}

/*
 * TEST_LIST begins here
 */
void udxp_filter_params(void) {
    double clock_speed, decimation_ratio;
    double peaksam, peakint, acq_val;

    unsigned short peaksam_calculated, peakint_calculated;
    unsigned short PEAKSAM, PEAKINT;

    init_udxp();

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

        /* Set to 4 decimation 0 ticks */
        peaksam = xia_round(decimation_ratio * 0.080) * decimation_ratio;
        acqset("peaksam_offset", peaksam);

        PEAKSAM = dsp("PEAKSAM");
        peaksam_calculated =
            dsp("SLOWLEN") + dsp("SLOWGAP") - (unsigned short) (peaksam * clock_speed);
        TEST_CHECK(peaksam_calculated == PEAKSAM);
        TEST_MSG("peaksam_calculated, PEAKSAM | %d,  %d", peaksam_calculated, PEAKSAM);

        /* default setting */
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

        /* Set to 4 decimation 0 ticks */
        peakint = xia_round(decimation_ratio * 0.080) * decimation_ratio;
        acqset("peakint_offset", peakint);
        PEAKINT = dsp("PEAKINT");
        peakint_calculated =
            dsp("SLOWLEN") + dsp("SLOWGAP") + (unsigned short) (peakint * clock_speed);
        TEST_CHECK(peakint_calculated == PEAKINT);

        /* default setting */
        peakint = 0.;
        acqset("peakint_offset", peakint);
        PEAKINT = dsp("PEAKINT");
        peakint_calculated = dsp("SLOWLEN") + dsp("SLOWGAP");
        TEST_CHECK(peakint_calculated == PEAKINT);
    }

    cleanup();
}

void udxp_detector_polarity(void) {
    unsigned short POL;
    double polarity;

    init_udxp();

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

void udxp_board_operations(void) {
    unsigned long version;
    unsigned long features;

    char* acq_names[] = {"baseline_threshold", "energy_threshold", "trigger_threshold",
                         "mca_bin_width"};
    double acq_vals[] = {50., 60., 40., 10.};
    double acq_value;

    int acq_number = sizeof(acq_vals) / sizeof(double);
    int i;

    init_udxp();

    TEST_CASE("BoardOperation apply");
    {
        for (i = 0; i < acq_number; i++) {
            acqset(acq_names[i], acq_vals[i]);
            acq_value = acq(acq_names[i]);
            TEST_CHECK(fabs(acq_value - acq_vals[i]) < acq_vals[i] * 0.1);
        }
    }

    TEST_CASE("cpld versions");
    {
        TEST_CHECK(xiaBoardOperation(0, "get_board_features", (void*) &features) ==
                   XIA_SUCCESS);

        if ((features & 0x05) != 0) {
            TEST_CHECK(xiaBoardOperation(0, "get_udxp_cpld_version",
                                         (void*) &version) == XIA_SUCCESS);
            TEST_CHECK(version > 1L);
            TEST_CHECK(xiaBoardOperation(0, "get_udxp_cpld_variant",
                                         (void*) &version) == XIA_SUCCESS);
            TEST_CHECK(version > 1L);
        }
    }

    TEST_CASE("usb version");
    {
        if (is_usb() && (features & 0x05) != 0) {
            TEST_CHECK(xiaBoardOperation(0, "get_usb_version", (void*) &version) ==
                       XIA_SUCCESS);
            TEST_CHECK(version > 1L);
        }
    }
    cleanup();
}

void udxp_board_info(void) {
    unsigned char board_info[26];
    unsigned short nfippi;
    double gain_man, gain_exp;

    init_udxp();

    TEST_CASE("board info");
    {
        TEST_CHECK(xiaBoardOperation(0, "get_board_info", (void*) &board_info) ==
                   XIA_SUCCESS);

        nfippi = board_info[8];
        TEST_CHECK(nfippi > 0);

        gain_man = (double) (board_info[11] << 8) + board_info[10];
        gain_exp = (double) board_info[12];

        TEST_CHECK((gain_man / 32768.) * pow(2, gain_exp) > 0);
    }
    cleanup();
}

void udxp_fippis(void) {
    int retval;
    unsigned short nfippi, pt_per_fippi;
    double fippi, acq_val;
    double* pt_ranges;
    double* current_pts;
    double* peaking_times;

    init_udxp();

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

void udxp_preset_run(void) {
    double preset[2];
    unsigned short busy;

    init_udxp();

    TEST_CASE("fixed realtime run");
    {
        preset[0] = 1.;
        preset[1] = 0.5;
        TEST_CHECK(xiaBoardOperation(0, "set_preset", (void*) &preset) == XIA_SUCCESS);
        TEST_CHECK(xiaStartRun(0, 0) == XIA_SUCCESS);

        xia_sleep(preset[1] + 0.5);
        busy = dsp("BUSY");
        TEST_CHECK(busy == 0);
        TEST_CHECK(xiaStopRun(0) == XIA_SUCCESS);
    }

    TEST_CASE("fixed livetime run");
    {
        preset[0] = 2.;
        preset[1] = 0.5;
        TEST_CHECK(xiaBoardOperation(0, "set_preset", (void*) &preset) == XIA_SUCCESS);
        TEST_CHECK(xiaStartRun(0, 0) == XIA_SUCCESS);

        xia_sleep(preset[1] + 0.5);
        busy = dsp("BUSY");
        TEST_CHECK(busy == 0);
        TEST_CHECK(xiaStopRun(0) == XIA_SUCCESS);
    }

    TEST_CASE("indefinite run");
    {
        preset[0] = 0.;
        preset[1] = 1.;
        TEST_CHECK(xiaBoardOperation(0, "set_preset", (void*) &preset) == XIA_SUCCESS);
        TEST_CHECK(xiaStartRun(0, 0) == XIA_SUCCESS);
        busy = dsp("BUSY");
        TEST_CHECK(busy != 0);

        TEST_CHECK(xiaStopRun(0) == XIA_SUCCESS);
        busy = dsp("BUSY");
        TEST_CHECK(busy == 0);
    }

    cleanup();
}

void udxp_parameters(void) {
    unsigned short param, new_param, old_param;

    init_udxp();

    TEST_CASE("microdxp specific paramter");
    {
        old_param = dsp("TRACEWAIT");

        param = old_param + 1;
        TEST_CHECK(xiaSetParameter(0, "TRACEWAIT", param) == XIA_SUCCESS);
        new_param = dsp("TRACEWAIT");

        TEST_CHECK(new_param == param);
        TEST_CHECK(xiaSetParameter(0, "TRACEWAIT", old_param) == XIA_SUCCESS);
    }
    cleanup();
}

void udxp_thresholds(void) {
    int retval;
    double threshold, acq_val;

    int i;
    char* threshold_types[] = {"trigger_threshold", "baseline_threshold",
                               "energy_threshold"};

    init_udxp();

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

void udxp_statistics(void) {
    unsigned long i;

    double tick = 500E-9;
    double test_time = 0.5;

    double livetime_dsp, realtime_dsp;
    double realtime, runtime, trigger_livetime;
    double energy_livetime, icr, ocr, fastpeaks_dsp;

    unsigned long triggers, mca_events, summed_events, mca_length;
    unsigned long* mca;

    double mod_stats[9];

    double underflows, overflows, eventsinrun, run_data;

    init_udxp();

    TEST_CASE("realtime");
    {
        run(test_time);

        TEST_CHECK(xiaGetRunData(0, "realtime", (void*) &realtime) == XIA_SUCCESS);
        TEST_CHECK(xiaGetRunData(0, "runtime", (void*) &runtime) == XIA_SUCCESS);

        /* Allow ~60ms of slop since this is not a preset run */
        TEST_CHECK(fabs(runtime - realtime) < 0.06);
        realtime_dsp =
            tick * (((long long) dsp("REALTIMEHI") << 32) +
                    ((unsigned long) dsp("REALTIMEMID") << 16) + dsp("REALTIMELO"));
        TEST_CHECK(realtime == realtime_dsp);
    }

    TEST_CASE("trigger_livetime");
    {
        run(test_time);

        TEST_CHECK(xiaGetRunData(0, "trigger_livetime", (void*) &trigger_livetime) ==
                   XIA_SUCCESS);

        TEST_CHECK(trigger_livetime < (test_time + 0.06));
        TEST_CHECK(trigger_livetime > 0.);

        livetime_dsp =
            tick * (((long long) dsp("LIVETIMEHI") << 32) +
                    ((unsigned long) dsp("LIVETIMEMID") << 16) + dsp("LIVETIMELO"));
        TEST_CHECK(trigger_livetime == livetime_dsp);
    }

    TEST_CASE("energy_livetime");
    {
        run(test_time);

        TEST_CHECK(xiaGetRunData(0, "energy_livetime", (void*) &energy_livetime) ==
                   XIA_SUCCESS);
        TEST_CHECK(energy_livetime < (test_time + 0.06));
        TEST_MSG("energy_livetime | %.2f", energy_livetime);

        /* Need a signal input to get meaningful data for testing */
        if (energy_livetime > 0.) {
            TEST_CHECK(xiaGetRunData(0, "realtime", (void*) &realtime) == XIA_SUCCESS);
            TEST_CHECK(xiaGetRunData(0, "input_count_rate", (void*) &icr) ==
                       XIA_SUCCESS);
            TEST_CHECK(xiaGetRunData(0, "output_count_rate", (void*) &ocr) ==
                       XIA_SUCCESS);

            TEST_CHECK(energy_livetime == (realtime * ocr / icr));
            TEST_MSG("rt * ocr / icr | %.2f, %.2f, %.2f", realtime, ocr, icr);
        }
    }

    TEST_CASE("triggers");
    {
        run(test_time);

        TEST_CHECK(xiaGetRunData(0, "triggers", (void*) &triggers) == XIA_SUCCESS);
        fastpeaks_dsp =
            (double) ((unsigned long) dsp("FASTPEAKSHI") << 16) + dsp("FASTPEAKSLO");
        TEST_CHECK_(triggers == fastpeaks_dsp, "triggers | %lu", triggers);
    }

    TEST_CASE("mca_events");
    {
        run(test_time);

        TEST_CHECK(xiaGetRunData(0, "mca_events", (void*) &mca_events) == XIA_SUCCESS);
        eventsinrun =
            (double) ((unsigned long) dsp("EVTSINRUNHI") << 16) + dsp("EVTSINRUNLO");
        TEST_CHECK_(mca_events == eventsinrun, "mca_events | %lu", mca_events);

        TEST_CHECK(xiaGetRunData(0, "mca_length", (void*) &mca_length) == XIA_SUCCESS);

        mca = malloc(mca_length * sizeof(unsigned long));
        TEST_CHECK(xiaGetRunData(0, "mca", mca) == XIA_SUCCESS);

        summed_events = 0;
        for (i = 0; i < mca_length; i++) {
            summed_events += mca[i];
        }
        free(mca);

        TEST_CHECK(summed_events == mca_events);
    }

    TEST_CASE("underflows");
    {
        run(test_time);

        TEST_CHECK(xiaGetRunData(0, "module_statistics_2", (void*) &mod_stats) ==
                   XIA_SUCCESS);
        underflows =
            (double) ((unsigned long) dsp("UNDRFLOWSHI") << 16) + dsp("UNDRFLOWSLO");
        TEST_CHECK(mod_stats[7] == underflows);
    }

    TEST_CASE("overflows");
    {
        run(test_time);

        TEST_CHECK(xiaGetRunData(0, "module_statistics_2", (void*) &mod_stats) ==
                   XIA_SUCCESS);
        overflows =
            (double) ((unsigned long) dsp("OVERFLOWSHI") << 16) + dsp("OVERFLOWSLO");
        TEST_CHECK(mod_stats[8] == overflows);
    }

    TEST_CASE("module_statistics_2");
    {
        run(test_time);

        TEST_CHECK(xiaGetRunData(0, "module_statistics_2", (void*) &mod_stats) ==
                   XIA_SUCCESS);

        TEST_CHECK(xiaGetRunData(0, "realtime", (void*) &run_data) == XIA_SUCCESS);
        TEST_CHECK(mod_stats[0] == run_data);

        TEST_CHECK(xiaGetRunData(0, "trigger_livetime", (void*) &run_data) ==
                   XIA_SUCCESS);
        TEST_CHECK(mod_stats[1] == run_data);

        TEST_CHECK(xiaGetRunData(0, "energy_livetime", (void*) &run_data) ==
                   XIA_SUCCESS);
        TEST_CHECK(mod_stats[2] == run_data);

        TEST_CHECK(xiaGetRunData(0, "triggers", (void*) &run_data) == XIA_SUCCESS);
        TEST_CHECK(mod_stats[3] == run_data);

        TEST_CHECK(xiaGetRunData(0, "mca_events", (void*) &run_data) == XIA_SUCCESS);
        TEST_CHECK(mod_stats[4] == run_data);

        TEST_CHECK(xiaGetRunData(0, "input_count_rate", (void*) &run_data) ==
                   XIA_SUCCESS);
        TEST_CHECK(mod_stats[5] == run_data);

        TEST_CHECK(xiaGetRunData(0, "output_count_rate", (void*) &run_data) ==
                   XIA_SUCCESS);
        TEST_CHECK(mod_stats[6] == run_data);
    }

    cleanup();
}

void udxp_gain(void) {
    int retval;
    double gain;

    double acqval;
    unsigned short gain_mode;
    unsigned short dspval;

    unsigned short AV_MEM_PARSET = 0x4;
    unsigned short AV_MEM_GENSET = 0x8;

    init_udxp();

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

void udxp_gain_calibrate(void) {
    double original_gain, scaled_gain;
    double gain_scale = 1.5;

    init_udxp();

    TEST_CASE("xiaGainCalibrate");
    {
        original_gain = acq("gain");
        TEST_CHECK(xiaGainCalibrate(0, gain_scale) == XIA_SUCCESS);
        scaled_gain = acq("gain");

        /* Test that we are within 1% */
        TEST_CHECK(fabs((original_gain * gain_scale) - scaled_gain) <
                   (scaled_gain * 0.01));
        TEST_MSG("original_gain, scaled_gain | %.2f, %.2f", original_gain, scaled_gain);
        acqset("gain", original_gain);
    }

    TEST_CASE("xiaGainOperation");
    {
        original_gain = acq("gain");
        TEST_CHECK(xiaGainOperation(0, "calibrate", (void*) &gain_scale) ==
                   XIA_SUCCESS);
        scaled_gain = acq("gain");

        /* Test that we are within 1% */
        TEST_CHECK(fabs((original_gain * gain_scale) - scaled_gain) <
                   (scaled_gain * 0.01));
        TEST_MSG("original_gain, scaled_gain | %.2f, %.2f", original_gain, scaled_gain);
        acqset("gain", original_gain);
    }

    TEST_CASE("calibrate_gain_trim");
    {
        gain_scale = 1.1;
        original_gain = acq("gain_trim");
        TEST_CHECK(xiaGainOperation(0, "calibrate_gain_trim", (void*) &gain_scale) ==
                   XIA_SUCCESS);
        scaled_gain = acq("gain_trim");

        /* Test that we are within 1% */
        TEST_CHECK(fabs((original_gain * gain_scale) - scaled_gain) <
                   (scaled_gain * 0.01));
        TEST_MSG("original_gain, scaled_gain | %.2f, %.2f", original_gain, scaled_gain);

        acqset("gain", original_gain);
    }
    cleanup();
}

void udxp_trace_read(void) {
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

    init_udxp();

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
        fill_ulong_ary(adc_trace, adc_trace_length, 0xDEADBEEF);
        fill_ulong_ary(adc_trace_2, adc_trace_length, 0xDEADBEEF);

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
                TEST_CHECK(compare_pct(adc_trace[k], adc_trace_2[k], 0.05));
                TEST_MSG("trace data mismatch | %lu != %lu", adc_trace[k],
                         adc_trace_2[k]);
            }
        }
    }
    free(adc_trace);
    free(adc_trace_2);
    cleanup();
}

TEST_LIST = {
    {"udxp_board_info", udxp_board_info},
    {"udxp_board_operations", udxp_board_operations},
    {"udxp_detector_polarity", udxp_detector_polarity},
    {"udxp_filter_params", udxp_filter_params},
    {"udxp_fippis", udxp_fippis},
    {"udxp_gain", udxp_gain},
    {"udxp_gain_calibrate", udxp_gain_calibrate},
    {"udxp_parameters", udxp_parameters},
    {"udxp_preset_run", udxp_preset_run},
    {"udxp_thresholds", udxp_thresholds},
    {"udxp_trace_read", udxp_trace_read},
    {"udxp_statistics", udxp_statistics},
    {NULL, NULL} /* zeroed record marking the end of the list */
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif
