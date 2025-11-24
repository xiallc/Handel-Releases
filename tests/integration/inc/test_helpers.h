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

/** @file test_utils.h
 * @brief Helpers for unit tests.
 */

#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <handel.h>
#include <handel_errors.h>
#include <handel_generic.h>
#include <xerxes_errors.h>

#include <util/xia_sleep.h>

#include <acutest.h>

#define MSGLEN (512)
char errmsg[MSGLEN];

void create_det(char* alias, char* type, char* polarity, int num_chans, double tval,
                double gain) {
    TEST_CHECK(xiaNewDetector(alias) == XIA_SUCCESS);
    TEST_CHECK(xiaAddDetectorItem(alias, "number_of_channels", &num_chans) ==
               XIA_SUCCESS);
    TEST_CHECK(xiaAddDetectorItem(alias, "type", type) == XIA_SUCCESS);
    TEST_CHECK(xiaAddDetectorItem(alias, "type_value", &tval) == XIA_SUCCESS);

    for (int i = 0; i < num_chans; i++) {

        TEST_CHECK(xiaAddDetectorItem(alias, "channel0_polarity", polarity) ==
                   XIA_SUCCESS);
        TEST_CHECK(xiaAddDetectorItem(alias, "channel0_gain", &gain) == XIA_SUCCESS);
    }
}

void create_fw(char* alias) {
    TEST_CHECK(xiaNewFirmware(alias) == XIA_SUCCESS);
}

void create_mod(char* alias, char* type, char* iface) {
    TEST_CHECK(xiaNewModule(alias) == XIA_SUCCESS);
    TEST_CHECK(xiaAddModuleItem(alias, "module_type", type) == XIA_SUCCESS);
    TEST_CHECK(xiaAddModuleItem(alias, "interface", iface) == XIA_SUCCESS);
}

static char* tst_msg(char* msg, const unsigned int len, const int code_a,
                     const int code_b) {
    memset(msg, 0, len);

    char txt_a[255] = {0};
    strncpy(txt_a, xiaGetErrorText(code_a), 40);
    char txt_b[255] = {0};
    strncpy(txt_b, xiaGetErrorText(code_b), 40);

    snprintf(msg, len, "%i: %s != %i: %s", code_a, txt_a, code_b, txt_b);
    return msg;
}

/*
 * Helper functions for Handel related tasks
 */

void cleanup() {
    TEST_ASSERT_(xiaExit() == XIA_SUCCESS, "xiaExit");
    TEST_ASSERT_(xiaCloseLog() == XIA_SUCCESS, "xiaCloseLog");
}

unsigned short dsp(const char* dspname) {
    unsigned short paramvalue = 0;

    int retval = xiaGetParameter(0, dspname, (void*) &paramvalue);
    TEST_CHECK_(retval == XIA_SUCCESS, "xiaGetParameter | %s 0x%hx", dspname,
                paramvalue);
    TEST_MSG("xiaGetParameter | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

    return paramvalue;
}

double acq(char* acqname) {
    int retval;
    double acqval;

    retval = xiaGetAcquisitionValues(0, acqname, (void*) &acqval);
    TEST_CHECK_(retval == XIA_SUCCESS, "xiaGetAcquisitionValues | %s %.2f", acqname,
                acqval);
    TEST_MSG("xiaGetAcquisitionValues | %s",
             tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

    return acqval;
}

void acqset(char* acqname, double acqval) {
    int retval;
    int ignore = 0;

    retval = xiaSetAcquisitionValues(0, acqname, (void*) &acqval);
    TEST_CHECK_(retval == XIA_SUCCESS, "xiaGetAcquisitionValues | %s %.2f", acqname,
                acqval);
    TEST_MSG("xiaSetAcquisitionValues | %s",
             tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
    TEST_CHECK_(xiaBoardOperation(0, "apply", &ignore) == XIA_SUCCESS,
                "xiaBoardOperation | apply");
}

#endif
