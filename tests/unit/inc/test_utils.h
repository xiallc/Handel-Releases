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

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "handel.h"
#include "handel_errors.h"
#include "handel_generic.h"
#include "xerxes_errors.h"

#include "acutest.h"

/* Variables used in multiple test cases */
#define MSGLEN (256)
char errmsg[MSGLEN];

#if _WIN32
/* For Sleep() */
#include <time.h>
#include <windows.h>
#endif

#ifdef _MSC_VER
#pragma warning(push)
/* Disable warning "conditional expression is constant"
     * building TEST_ASSERT with VS100 */
#pragma warning(disable : 4127)
#endif

static char* TEST_INI = "helpers/microdxp_usb.ini";

static bool approx_dbl(const double lhs, const double rhs, const double epsilon) {
    if (fabs(lhs - rhs) > epsilon) {
        return false;
    }
    return true;
}

static bool approx_int(const int lhs, const int rhs, const double epsilon) {
    if (abs(lhs - rhs) > epsilon) {
        return false;
    }
    return true;
}

static bool compare_pct(const double lhs, const double rhs, const double epsilon) {
    return fabs(lhs - rhs) < (rhs * epsilon);
}

static bool compare_dbl_ary(const double* lhs, const double* rhs,
                            const unsigned int len, const double epsilon) {
    if (lhs == NULL || rhs == NULL) {
        return false;
    }
    for (unsigned int i = 0; i < len; i++) {
        if (!approx_dbl(lhs[i], rhs[i], epsilon)) {
            return false;
        }
    }
    return true;
}

static bool compare_uint_ary(const unsigned int* lhs, const unsigned int* rhs,
                             const unsigned int len) {
    if (lhs == NULL || rhs == NULL) {
        return false;
    }
    for (unsigned int i = 0; i < len; i++) {
        if (lhs[i] != rhs[i]) {
            return false;
        }
    }
    return true;
}

static bool compare_ulong_ary(const unsigned long* lhs, const unsigned long* rhs,
                              const unsigned long len) {
    if (lhs == NULL || rhs == NULL) {
        return false;
    }
    for (unsigned long i = 0; i < len; i++) {
        if (lhs[i] != rhs[i]) {
            return false;
        }
    }
    return true;
}

static bool compare_ushort_ary(const unsigned short* lhs, const unsigned short* rhs,
                               const unsigned int len) {
    if (lhs == NULL || rhs == NULL) {
        return false;
    }
    for (unsigned int i = 0; i < len; i++) {
        if (lhs[i] != rhs[i]) {
            return false;
        }
    }
    return true;
}

static char* concat(const char* s1, const char* s2) {
    char* result = malloc(strlen(s1) + strlen(s2) + 1);
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

static void fill_dbl_ary(double* ary, const unsigned int len, const double val) {
    if (ary != NULL) {
        for (unsigned int i = 0; i < len; i++) {
            ary[i] = val;
        }
    }
}

static void fill_uint_ary(unsigned int* ary, const unsigned int len,
                          const unsigned int val) {
    if (ary != NULL) {
        for (unsigned int i = 0; i < len; i++) {
            ary[i] = val;
        }
    }
}

static void fill_ushort_ary(unsigned short* ary, const unsigned int len,
                            const unsigned short val) {
    if (ary != NULL) {
        for (unsigned int i = 0; i < len; i++) {
            ary[i] = val;
        }
    }
}

static void fill_ulong_ary(unsigned long* ary, const unsigned long len,
                           const unsigned long val) {
    if (ary != NULL) {
        for (unsigned long i = 0; i < len; i++) {
            ary[i] = val;
        }
    }
}

/* A temporary round function until we move to C99 */
static double xia_round(double x) {
    return (x < 0.) ? ceil(x - 0.5) : floor(x + 0.5);
}

/* Unify the platform specific sleep functions */
static void xia_sleep(double time_seconds) {
#if _WIN32
    DWORD wait = (DWORD) (1000.0 * time_seconds);
    Sleep(wait);
#else
    unsigned long secs = (unsigned long) time_seconds;
    struct timespec req = {.tv_sec = secs,
                           .tv_nsec = ((time_seconds - secs) * 1000000000.0)};
    struct timespec rem = {.tv_sec = 0, .tv_nsec = 0};
    while (TRUE_) {
        if (nanosleep(&req, &rem) == 0)
            break;
        req = rem;
    }
#endif
}

static char* tst_msg(char* msg, const unsigned int len, const int code_a,
                     const int code_b) {
    memset(msg, 0, len);

    char txt_a[40] = {0};
    strncpy(txt_a, xiaGetErrorText(code_a), 40);
    char txt_b[40] = {0};
    strncpy(txt_b, xiaGetErrorText(code_b), 40);

    snprintf(msg, len, "%i: %s != %i: %s", code_a, txt_a, code_b, txt_b);
    return msg;
}

/*
 * Helper functions for Handel related tasks
 */
void init_logging() {
    TEST_ASSERT_(xiaSetLogLevel(4) == XIA_SUCCESS, "xiaSetLogLevel");
    TEST_ASSERT_(xiaSetLogOutput("unit_test.log") == XIA_SUCCESS, "xiaSetLogOutput");
}

void init() {
    int retval;
    char* ini = TEST_INI;

    init_logging();

    retval = xiaInit(ini);
    TEST_ASSERT_(xiaInit(ini) == XIA_SUCCESS, "xiaInit");
    TEST_MSG("xiaInit | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

    retval = xiaStartSystem();
    TEST_ASSERT_(retval == XIA_SUCCESS, "xiaStartSystem");
    TEST_MSG("xiaStartSystem | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
}

void cleanup() {
    TEST_ASSERT_(xiaExit() == XIA_SUCCESS, "xiaExit");
    TEST_ASSERT_(xiaCloseLog() == XIA_SUCCESS, "xiaCloseLog");
}

void run(double seconds) {
    int retval;

    retval = xiaStartRun(0, 0);
    TEST_ASSERT_(retval == XIA_SUCCESS, "xiaStartRun");
    TEST_MSG("xiaStartRun | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

    xia_sleep(seconds);

    retval = xiaStopRun(0);
    TEST_ASSERT_(retval == XIA_SUCCESS, "xiaStopRun");
    TEST_MSG("xiaStopRun | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
}

unsigned short dsp(const char* dspname) {
    int retval;
    unsigned short paramvalue;

    retval = xiaGetParameter(0, dspname, (void*) &paramvalue);
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

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
