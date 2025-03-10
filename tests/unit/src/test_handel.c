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

/** @file test_handel.c
 * @brief Tests for detector agnostic Handel API functionality.
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

/*
 * TEST_LIST begins here
 */
void handel_errors(void) {
    const int comp_len = 16;
    char retval[17];

    const char* expected_2048 = "Unknown error code";
    const char* expected_XIA_BAD_PSL_ARGS = "Bad call arguments to PSL function";
    const char* expected_DXP_LOG_LEVEL = "Log level invalid";

    TEST_CASE("Non exiting error");
    {
        strncpy(retval, xiaGetErrorText(2048), comp_len);
        TEST_CHECK(strncmp(retval, expected_2048, comp_len) == 0);
        TEST_MSG("xiaGetErrorText | %s != %s", retval, expected_2048);
    }

    TEST_CASE("Handel error");
    {
        strncpy(retval, xiaGetErrorText(XIA_BAD_PSL_ARGS), comp_len);
        TEST_CHECK(strncmp(retval, expected_XIA_BAD_PSL_ARGS, comp_len) == 0);
        TEST_MSG("xiaGetErrorText | %s != %s", retval, expected_XIA_BAD_PSL_ARGS);
    }

    TEST_CASE("Xerxes error");
    {
        strncpy(retval, xiaGetErrorText(DXP_LOG_LEVEL), comp_len);
        TEST_CHECK(strncmp(retval, expected_DXP_LOG_LEVEL, comp_len) == 0);
        TEST_MSG("xiaGetErrorText | %s != %s", retval, expected_DXP_LOG_LEVEL);
    }
}

void handel_exit(void) {
    int retval;
    TEST_CASE("xiaExit always succeed");
    {
        retval = xiaExit();
        TEST_CHECK_(retval == XIA_SUCCESS, "xiaExit | %d", retval);
        TEST_MSG("xiaExit | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
    }

    cleanup();
}

void handel_init(void) {
    int retval;
    char* ini = TEST_INI;

    init_logging();
    TEST_CASE("Basic initialization");
    {
        retval = xiaInitHandel();
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaInitHandel | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

        retval = xiaExit();
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaExit | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
    }

    TEST_CASE("Basic initialization with ini file");
    {
        retval = xiaInit(ini);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaInit | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

        retval = xiaExit();
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaExit | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
    }

    TEST_CASE("Basic initialization with bad ini file");
    {
        retval = xiaInit("bad.ini");
        TEST_CHECK(retval != XIA_BAD_NAME);
        TEST_MSG("xiaInit | %s", tst_msg(errmsg, MSGLEN, retval, XIA_BAD_NAME));
    }

    TEST_CASE("Null ini input");
    {
        retval = xiaInit((char*) NULL);
        TEST_CHECK(retval == XIA_BAD_NAME);
        TEST_MSG("xiaInit | %s", tst_msg(errmsg, MSGLEN, retval, XIA_BAD_NAME));
    }

    cleanup();
}

void handel_no_init(void) {
    int retval;
    unsigned short numparam, param;

    init_logging();

    TEST_CASE("xiaStartSystem without xiaInit returns error");
    {
        retval = xiaStartSystem();
        TEST_CHECK_(retval != XIA_SUCCESS, "xiaStartSystem | before init %d", retval);
        TEST_CHECK_(xiaExit() == XIA_SUCCESS, "xiaExit | after failure");
    }

    TEST_CASE("xiaBoardOperation before init retuns error");
    {
        retval = xiaBoardOperation(0, "apply", NULL);
        TEST_CHECK_(retval != XIA_SUCCESS, "xiaBoardOperation | %d", retval);
    }

    TEST_CASE("xiaStatRun before init returns error");
    {
        retval = xiaStartRun(0, 0);
        TEST_CHECK_(retval != XIA_SUCCESS, "xiaStatRun | %d", retval);
    }

    TEST_CASE("xiaGetRunData before init");
    {
        retval = xiaGetRunData(0, "mca_length", NULL);
        TEST_CHECK_(retval != XIA_SUCCESS, "xiaGetRunData | %d", retval);
        ;
    }

    TEST_CASE("xiaGetNumParams before init returns error");
    {
        retval = xiaGetNumParams(0, &numparam);
        TEST_CHECK_(retval != XIA_SUCCESS, "xiaGetNumParams | %d", retval);
    }

    TEST_CASE("xiaGetParameter before init returns error");
    {
        retval = xiaGetParameter(0, "SLOWLEN", (void*) &param);
        TEST_CHECK_(retval != XIA_SUCCESS, "xiaGetParameter | %d", retval);

        param = 32;
        retval = xiaSetParameter(0, "SLOWLEN", param);
        TEST_CHECK_(retval != XIA_SUCCESS, "xiaSetParameter | %d", retval);
    }

    cleanup();
}

void handel_save_system(void) {
    int retval;

    unsigned int num = 1;
    const char* detector_type = "reset";
    const char* polarity = "-";

    init_logging();
    TEST_CASE("Saving empty configurations");
    {
        retval = xiaSaveSystem("handel_ini", "unit_test_save_system.ini");
        TEST_CHECK_(retval == XIA_SUCCESS, "xiaSaveSystem");
        TEST_MSG("xiaSaveSystem | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
    }

    TEST_CASE("Invalid input for xiaSaveSystem");
    {
        retval = xiaSaveSystem("handel_ini", NULL);
        TEST_CHECK_(retval != XIA_SUCCESS, "xiaSaveSystem | null file name");

        retval = xiaSaveSystem("non-exiting-input", "unit_test_save_system.ini");
        TEST_CHECK_(retval != XIA_SUCCESS, "xiaSaveSystem | invalid input");

        retval = xiaSaveSystem(NULL, "unit_test_save_system.ini");
        TEST_CHECK_(retval != XIA_SUCCESS, "xiaSaveSystem | null input");
    }

    TEST_CASE("Creating dynamic configuration");
    {
        TEST_CHECK(xiaNewDetector("detector1") == XIA_SUCCESS);
        TEST_CHECK(xiaNewDetector("detector2") == XIA_SUCCESS);

        TEST_CHECK(xiaGetNumDetectors(&num) == XIA_SUCCESS);
        TEST_CHECK(num == 2);

        TEST_CHECK(xiaAddDetectorItem("detector1", "number_of_channels",
                                      (void*) &num) == XIA_SUCCESS);
        TEST_CHECK(xiaAddDetectorItem("detector1", "type", (void*) detector_type) ==
                   XIA_SUCCESS);
        TEST_CHECK(xiaAddDetectorItem("detector1", "channel0_polarity",
                                      (void*) polarity) == XIA_SUCCESS);

        TEST_CHECK(xiaNewModule("module1") == XIA_SUCCESS);
        TEST_CHECK(xiaNewModule("module2") == XIA_SUCCESS);

        TEST_CHECK(xiaGetNumModules(&num) == XIA_SUCCESS);
        TEST_CHECK(num == 2);
    }

    cleanup();
}

void handel_ini_file(void) {
    int retval;
    char module_type[256];

    init_logging();
    TEST_CASE("Unix-style EOLs");
    {
        retval = xiaInit("helpers/unix.ini");
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaInit | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
    }

    TEST_CASE("Parsing a common ini file");
    {
        /* xiaInit should return an error if xmap support is not built
         * Otherwise it should correctly parse the file
         */
        retval = xiaInit("helpers/ini_test.ini");
        TEST_CHECK_(retval == XIA_SUCCESS || retval == XIA_UNKNOWN_BOARD, "xiaInit");
        TEST_MSG("xiaInit | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

        if (retval == XIA_SUCCESS) {
            TEST_CHECK_(xiaGetModuleItem("module1", "module_type",
                                         (void*) module_type) == XIA_SUCCESS,
                        "xiaGetModuleItem | module_type %s", module_type);
            TEST_CHECK(strcmp(module_type, "xmap") == 0);
        }
    }

    cleanup();
}

/*
 * Tests below will need a connected device
 */
void handel_start_system(void) {
    int retval;
    int i;
    int times = 3;
    char* ini = TEST_INI;

    init_logging();

    TEST_CASE("Start system");
    {
        retval = xiaInit(ini);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaInit | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

        retval = xiaStartSystem();
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaStartSystem | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
    }

    TEST_CASE("Multiple restarts");
    {
        for (i = 0; i < times; i++) {
            TEST_CHECK(xiaInit(ini) == XIA_SUCCESS);
            TEST_CHECK(xiaStartSystem() == XIA_SUCCESS);
            TEST_CHECK(xiaExit() == XIA_SUCCESS);
        }
    }

    TEST_CASE("Multiple restarts without xiaExit");
    {
        for (i = 0; i < times; i++) {
            TEST_CHECK(xiaInit(ini) == XIA_SUCCESS);
            TEST_CHECK(xiaStartSystem() == XIA_SUCCESS);
        }
    }

    cleanup();
}

void handel_parameters(void) {
    int retval;
    unsigned short i;
    char paramname[MAXITEM_LEN];
    unsigned short numparam, n_different_param;
    unsigned short param, new_param, old_param;
    unsigned short* paramData = NULL;

    init();
    TEST_CASE("xiaGetNumParams");
    {
        retval = xiaGetNumParams(0, &numparam);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaGetNumParams | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
        TEST_CHECK_(numparam > 0, "%hu > 0", numparam);
    }

    TEST_CASE("xiaGetParameter");
    {
        retval = xiaGetParameter(0, "THRESHOLD", (void*) &param);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaGetParameter | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
    }

    TEST_CASE("xiaSetParameter");
    {
        param = 32;
        TEST_CHECK(xiaGetParameter(0, "THRESHOLD", (void*) &old_param) == XIA_SUCCESS);
        TEST_CHECK(xiaSetParameter(0, "THRESHOLD", param) == XIA_SUCCESS);
        TEST_CHECK(xiaSetParameter(0, "THRESHOLD", old_param) == XIA_SUCCESS);
    }

    TEST_CASE("RW Parameter");
    {
        param = 23;
        TEST_CHECK(xiaGetParameter(0, "THRESHOLD", (void*) &old_param) == XIA_SUCCESS);
        TEST_CHECK(xiaSetParameter(0, "THRESHOLD", param) == XIA_SUCCESS);
        TEST_CHECK(xiaGetParameter(0, "THRESHOLD", (void*) &new_param) == XIA_SUCCESS);
        TEST_CHECK(new_param == param);
        TEST_CHECK(xiaSetParameter(0, "THRESHOLD", old_param) == XIA_SUCCESS);
    }

    TEST_CASE("get values via xiaGetParamData");
    {
        TEST_CHECK(xiaGetNumParams(0, &numparam) == XIA_SUCCESS);

        paramData = malloc(numparam * sizeof(unsigned short));
        TEST_CHECK(xiaGetParamData(0, "values", paramData) == XIA_SUCCESS);
        TEST_MSG("xiaGetParamData | numparam %d", numparam);

        n_different_param = 0;
        for (i = 0; i < numparam; i++) {
            retval = xiaGetParamName(0, i, paramname);
            TEST_CHECK_(retval == XIA_SUCCESS, "xiaGetParamName | %s", paramname);
            param = dsp(paramname);
            if (paramData[i] != param)
                n_different_param++;
        }

        free(paramData);
        TEST_CHECK_(n_different_param < 6, "n_different_param | %d", n_different_param);
    }

    TEST_CASE("xiaGetParameter Invalid param name returns error");
    {
        retval = xiaGetParameter(0, "INVALIDPARAM", (void*) &param);
        TEST_CHECK(retval == DXP_NOSYMBOL);
        TEST_MSG("xiaGetParameter | %s", tst_msg(errmsg, MSGLEN, retval, DXP_NOSYMBOL));

        retval = xiaSetParameter(0, "INVALIDPARAM", param);
        TEST_CHECK(retval == DXP_NOSYMBOL);
        TEST_MSG("xiaSetParameter | %s", tst_msg(errmsg, MSGLEN, retval, DXP_NOSYMBOL));

        retval = xiaGetParameter(0, NULL, (void*) &param);
        TEST_CHECK(retval == XIA_NULL_VALUE);
        TEST_MSG("xiaGetParameter | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_VALUE));
    }

    TEST_CASE("xiaGetParamData invalid input check");
    {
        TEST_CHECK(xiaGetNumParams(0, &numparam) == XIA_SUCCESS);

        retval = xiaGetParamName(0, numparam, paramname);
        TEST_CHECK_(retval != XIA_SUCCESS, "xiaGetParamName | out of range index %d",
                    retval);

        retval = xiaGetParamData(0, "values", NULL);
        TEST_CHECK_(retval == XIA_NULL_VALUE, "xiaGetParamData | null input %d",
                    retval);

        retval = xiaGetParamData(0, NULL, NULL);
        TEST_CHECK_(retval == XIA_NULL_VALUE, "xiaGetParamData | null input %d",
                    retval);
    }

    cleanup();
}

void handel_run_control(void) {
    unsigned long run_active;
    time_t start_s = 0;
    time_t stop_s = 0;

    init();
    TEST_CASE("Stop run before start");
    {
        TEST_CHECK(xiaStopRun(0) == XIA_SUCCESS);
    }

    TEST_CASE("Repeated start runs");
    {
        TEST_CHECK(xiaStartRun(-1, 0) == XIA_SUCCESS);
        TEST_CHECK(xiaStartRun(-1, 0) == XIA_SUCCESS);
        TEST_CHECK(xiaStopRun(0) == XIA_SUCCESS);
    }

    TEST_CASE("Stop is fast");
    {
        TEST_CHECK(xiaStartRun(-1, 0) == XIA_SUCCESS);
        time(&start_s);
        TEST_CHECK(xiaStopRun(0) == XIA_SUCCESS);
        time(&stop_s);
        TEST_CHECK(fabs((double) (stop_s - start_s)) < 1.);
    }

    TEST_CASE("Run active");
    {
        TEST_CHECK(xiaStartRun(-1, 0) == XIA_SUCCESS);
        TEST_CHECK(xiaGetRunData(0, "run_active", &run_active) == XIA_SUCCESS);
        TEST_CHECK((run_active & 0x1) > 0);
        TEST_CHECK(xiaStopRun(0) == XIA_SUCCESS);
    }

    cleanup();
}

void handel_run_data(void) {
    int retval;
    unsigned long mca_length;
    unsigned long* mca;

    unsigned long baseline_length;
    unsigned long* baseline;

    init();
    TEST_CASE("xiaGetRunData bad inputs");
    {
        retval = xiaGetRunData(0, "mca_length", NULL);
        TEST_CHECK(retval == XIA_NULL_VALUE);
        TEST_MSG("xiaGetRunData | %s", tst_msg(errmsg, MSGLEN, retval, XIA_NULL_VALUE));

        retval = xiaGetRunData(0, "non_existing_run_data", (void*) &mca_length);
        TEST_CHECK(retval != XIA_SUCCESS);
        TEST_MSG("xiaGetRunData | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

        retval = xiaGetRunData(0, NULL, (void*) &mca_length);
        TEST_CHECK(retval == XIA_NULL_VALUE);
        TEST_MSG("xiaGetRunData | %s", tst_msg(errmsg, MSGLEN, retval, XIA_NULL_VALUE));
    }

    TEST_CASE("mca");
    {
        TEST_CHECK(xiaGetRunData(0, "mca_length", (void*) &mca_length) == XIA_SUCCESS);

        mca = malloc(mca_length * sizeof(unsigned long));
        TEST_CHECK(xiaGetRunData(0, "mca", mca) == XIA_SUCCESS);
        free(mca);
    }

    TEST_CASE("baseline");
    {
        TEST_CHECK(xiaGetRunData(0, "baseline_length", (void*) &baseline_length) ==
                   XIA_SUCCESS);

        baseline = malloc(baseline_length * sizeof(unsigned long));
        TEST_CHECK(xiaGetRunData(0, "baseline", baseline) == XIA_SUCCESS);
        free(baseline);
    }

    cleanup();
}

TEST_LIST = {
    {"handel_errors", handel_errors},
    {"handel_exit", handel_exit},
    {"handel_ini_file", handel_ini_file},
    {"handel_init", handel_init},
    {"handel_no_init", handel_no_init},
    {"handel_save_system", handel_save_system},
    {"handel_start_system", handel_start_system},
    {"handel_parameters", handel_parameters},
    {"handel_run_control", handel_run_control},
    {"handel_run_data", handel_run_data},
    {NULL, NULL} /* zeroed record marking the end of the list */
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif
