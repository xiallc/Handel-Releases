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

#include <util/xia_ary_manip.h>
#include <util/xia_compare.h>
#include <util/xia_crc.h>
#include <util/xia_str_manip.h>

#include <test_helpers.h>

static const unsigned int NUM_ALIASES = 3;
static char* aliases[] = {"jabberwocky", "tweedle_dee", "tweedle_dum"};
static char shared_alias[] = "apl";
static char shared_name[] = "alice";
static char shared_type[] = "human";
static int shared_value = 4714;
static double shared_values[3] = {4512, 4613, 4714};
static double shared_info[2] = {0., 0.};

void add_detector_item(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null Alias");
    {
        retval = xiaAddDetectorItem(NULL, NULL, NULL);
        TEST_CHECK(retval == XIA_NULL_ALIAS);
        TEST_MSG("xiaAddDetectorItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_ALIAS));
    }

    TEST_CASE("Null Name");
    {
        retval = xiaAddDetectorItem(shared_alias, NULL, NULL);
        TEST_CHECK(retval == XIA_NULL_NAME);
        TEST_MSG("xiaAddDetectorItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_NAME));
    }

    TEST_CASE("Null Value");
    {
        retval = xiaAddDetectorItem(shared_alias, shared_name, NULL);
        TEST_CHECK(retval == XIA_NULL_VALUE);
        TEST_MSG("xiaAddDetectorItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_VALUE));
    }

    TEST_CASE("Undefined alias");
    {
        retval = xiaAddDetectorItem(shared_alias, shared_name, &shared_value);
        TEST_CHECK(retval == XIA_NO_ALIAS);
        TEST_MSG("xiaAddDetectorItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NO_ALIAS));
    }

    TEST_CASE("No channels");
    TEST_CHECK(xiaNewDetector(shared_alias) == XIA_SUCCESS);
    {
        retval = xiaAddDetectorItem(shared_alias, shared_name, &shared_value);
        TEST_CHECK(retval == XIA_NO_CHANNELS);
        TEST_MSG("xiaAddDetectorItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NO_CHANNELS));
    }

    TEST_CASE("One channel");
    int num_chans = 1;
    {
        retval = xiaAddDetectorItem(shared_alias, "number_of_channels", &num_chans);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaAddDetectorItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
        int chk = 0;
        TEST_CHECK(xiaGetDetectorItem(shared_alias, "number_of_channels", &chk) ==
                   XIA_SUCCESS);
        TEST_CHECK(chk == 1);
    }

    TEST_CASE("Bad name");
    {
        retval = xiaAddDetectorItem(shared_alias, shared_name, &shared_value);
        TEST_CHECK(retval == XIA_BAD_NAME);
        TEST_MSG("xiaAddDetectorItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_BAD_NAME));
    }

    TEST_CASE("Bad type");
    {
        char btype[] = "jabberwoky";
        retval = xiaAddDetectorItem(shared_alias, "type", btype);
        TEST_CHECK(retval == XIA_BAD_VALUE);
        TEST_MSG("xiaAddDetectorItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_BAD_VALUE));
    }

    TEST_CASE("rc_feedback type");
    {
        char btype[] = "rc_feedback";
        retval = xiaAddDetectorItem(shared_alias, "type", btype);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaAddDetectorItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
        char chk[11] = {0};
        TEST_CHECK(xiaGetDetectorItem(shared_alias, "type", &chk) == XIA_SUCCESS);
        TEST_CHECK(strncmp(btype, chk, strlen(btype)) == 0);
        TEST_MSG("%s != %s", chk, btype);
    }

    TEST_CASE("Reset type");
    {
        char btype[] = "reset";
        retval = xiaAddDetectorItem(shared_alias, "type", btype);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaAddDetectorItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
        char chk[5] = {0};
        TEST_CHECK(xiaGetDetectorItem(shared_alias, "type", &chk) == XIA_SUCCESS);
        TEST_CHECK(strncmp(btype, chk, strlen(btype)) == 0);
        TEST_MSG("%s != %s", chk, btype);
    }

    TEST_CASE("type_value");
    {
        retval = xiaAddDetectorItem(shared_alias, "type_value", &shared_values[0]);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaAddDetectorItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
        double chk = 0;
        TEST_CHECK(xiaGetDetectorItem(shared_alias, "type_value", &chk) == XIA_SUCCESS);
        TEST_CHECK(chk == shared_values[0]);
        TEST_MSG("%f != %f", chk, shared_values[0]);
    }

    TEST_CASE("Bad channel number");
    {
        retval = xiaAddDetectorItem(shared_alias, "channel0_hare", &shared_values[0]);
        TEST_CHECK(retval == XIA_BAD_NAME);
        TEST_MSG("xiaAddDetectorItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_BAD_NAME));
    }

    TEST_CASE("Gain w/ bad channel num");
    {
        retval = xiaAddDetectorItem(shared_alias, "channel4_gain", &shared_values[0]);
        TEST_CHECK(retval == XIA_BAD_VALUE);
        TEST_MSG("xiaAddDetectorItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_BAD_VALUE));
    }

    TEST_CASE("Gain happy path");
    {
        retval = xiaAddDetectorItem(shared_alias, "channel0_gain", &shared_values[0]);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaAddDetectorItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
        double chk = 0;
        TEST_CHECK(xiaGetDetectorItem(shared_alias, "channel0_gain", &chk) ==
                   XIA_SUCCESS);
        TEST_CHECK(chk == shared_values[0]);
        TEST_MSG("%f != %f", chk, shared_values[0]);
    }

    TEST_CASE("Polarity w/ bad channel num");
    {
        retval =
            xiaAddDetectorItem(shared_alias, "channel4_polarity", &shared_values[0]);
        TEST_CHECK(retval == XIA_BAD_VALUE);
        TEST_MSG("xiaAddDetectorItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_BAD_VALUE));
    }

    TEST_CASE("Polarity w/ bad val");
    {
        retval =
            xiaAddDetectorItem(shared_alias, "channel0_polarity", &shared_values[0]);
        TEST_CHECK(retval == XIA_BAD_VALUE);
        TEST_MSG("xiaAddDetectorItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_BAD_VALUE));
    }

    char* pos[] = {"+", "pos", "positive"};
    for (unsigned int i = 0; i < 3; i++) {
        char* test_name = xia_concat("Polarity ", pos[i]);
        TEST_CASE(test_name);
        {
            retval = xiaAddDetectorItem(shared_alias, "channel0_polarity", pos[i]);
            TEST_CHECK(retval == XIA_SUCCESS);
            TEST_MSG("xiaAddDetectorItem | %s",
                     tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

            char chk[3] = {0};
            TEST_CHECK(xiaGetDetectorItem(shared_alias, "channel0_polarity", &chk) ==
                       XIA_SUCCESS);
            TEST_CHECK(strncmp(chk, "pos", 3) == 0);
            TEST_MSG("%s != pos", chk);
        }
        free(test_name);
    }

    char* neg[] = {"-", "neg", "negative"};
    for (unsigned int i = 0; i < 3; i++) {
        char* test_name = xia_concat("Polarity ", neg[i]);
        TEST_CASE(test_name);
        {
            retval = xiaAddDetectorItem(shared_alias, "channel0_polarity", neg[i]);
            TEST_CHECK(retval == XIA_SUCCESS);
            TEST_MSG("xiaAddDetectorItem | %s",
                     tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

            char chk[3] = {0};
            TEST_CHECK(xiaGetDetectorItem(shared_alias, "channel0_polarity", &chk) ==
                       XIA_SUCCESS);
            TEST_CHECK(strncmp(chk, "neg", 3) == 0);
            TEST_MSG("%s != neg", chk);
        }
        free(test_name);
    }
    cleanup();
}

void add_firmware_item(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null Alias");
    {
        retval = xiaAddFirmwareItem(NULL, NULL, NULL);
        TEST_CHECK(retval == XIA_NULL_ALIAS);
        TEST_MSG("xiaAddFirmwareItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_ALIAS));
    }

    TEST_CASE("Null Name");
    {
        retval = xiaAddFirmwareItem(shared_alias, NULL, NULL);
        TEST_CHECK(retval == XIA_NULL_NAME);
        TEST_MSG("xiaAddFirmwareItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_NAME));
    }

    TEST_CASE("Null Value");
    {
        retval = xiaAddFirmwareItem(shared_alias, shared_name, NULL);
        TEST_CHECK(retval == XIA_NULL_VALUE);
        TEST_MSG("xiaAddFirmwareItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_NAME));
    }

    TEST_CASE("Undefined alias");
    {
        retval = xiaAddFirmwareItem(shared_alias, shared_name, &shared_value);
        TEST_CHECK(retval == XIA_NO_ALIAS);
        TEST_MSG("xiaAddFirmwareItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NO_ALIAS));
    }

    TEST_CASE("Add filename");
    create_fw(shared_alias);
    {
        retval = xiaAddFirmwareItem(shared_alias, "filename", "red_queen.bin");
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaAddFirmwareItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

        char chk[13] = {0};
        TEST_CHECK(xiaGetFirmwareItem(shared_alias, 0, "filename", chk) == XIA_SUCCESS);
        TEST_CHECK(strncmp(chk, "red_queen.bin", 1) == 0);
        TEST_MSG("%s != red_queen.bin", chk);
    }

    TEST_CASE("Add ptrr");
    {
        unsigned short val = 14;
        retval = xiaAddFirmwareItem(shared_alias, "ptrr", &val);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaAddFirmwareItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
    }

    TEST_CASE("Bad item name");
    {
        retval = xiaAddFirmwareItem(shared_alias, shared_name, &shared_value);
        TEST_CHECK(retval == XIA_BAD_VALUE);
        TEST_MSG("xiaAddFirmwareItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_BAD_VALUE));
    }
    cleanup();
}

void add_module_item(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null Alias");
    {
        retval = xiaAddModuleItem(NULL, NULL, NULL);
        TEST_CHECK(retval == XIA_NULL_ALIAS);
        TEST_MSG("xiaAddModuleItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_ALIAS));
    }

    TEST_CASE("Null Name");
    {
        retval = xiaAddModuleItem(shared_alias, NULL, NULL);
        TEST_CHECK(retval == XIA_NULL_NAME);
        TEST_MSG("xiaAddModuleItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_NAME));
    }

    TEST_CASE("Null Value");
    {
        retval = xiaAddModuleItem(shared_alias, shared_name, NULL);
        TEST_CHECK(retval == XIA_NULL_VALUE);
        TEST_MSG("xiaAddModuleItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_NAME));
    }

    TEST_CASE("Unknown item");
    {
        create_mod(shared_alias, "udxp", "usb2");
        retval = xiaAddModuleItem(shared_alias, shared_name, &shared_value);
        TEST_CHECK(retval == XIA_UNKNOWN_ITEM);
        TEST_MSG("xiaAddModuleItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_UNKNOWN_ITEM));
    }

    TEST_CASE("Known item");
    {
        retval = xiaAddModuleItem(shared_alias, "device_number", &shared_value);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaAddModuleItem | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

        int chk = 0;
        TEST_CHECK(xiaGetModuleItem(shared_alias, "device_number", &chk) ==
                   XIA_SUCCESS);
        TEST_CHECK(chk == shared_value);
        TEST_MSG("%i != %i", chk, shared_value);
    }
    cleanup();
}

void board_operation(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null Name");
    {
        retval = xiaBoardOperation(0, NULL, NULL);
        TEST_CHECK(retval == XIA_NULL_NAME);
        TEST_MSG("xiaBoardOperation | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_NAME));
    }

    TEST_CASE("Null Value");
    {
        retval = xiaBoardOperation(0, shared_name, NULL);
        TEST_CHECK(retval == XIA_NULL_VALUE);
        TEST_MSG("xiaBoardOperation | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_VALUE));
    }

    TEST_CASE("Bad det-chan");
    {
        retval = xiaBoardOperation(0, shared_name, &shared_value);
        TEST_CHECK(retval == XIA_INVALID_DETCHAN);
        TEST_MSG("xiaBoardOperation | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_INVALID_DETCHAN));
    }
    cleanup();
}

void close_log(void) {
    xiaSuppressLogOutput();
    int retval = xiaCloseLog();
    TEST_CHECK(retval == XIA_SUCCESS);
    TEST_MSG("xiaCloseLog | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
    cleanup();
}

void do_special_run(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null Name");
    {
        retval = xiaDoSpecialRun(0, NULL, NULL);
        TEST_CHECK(retval == XIA_NULL_NAME);
        TEST_MSG("xiaDoSpecialRun | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_NAME));
    }

    TEST_CASE("Null Value");
    {
        retval = xiaDoSpecialRun(0, shared_name, NULL);
        TEST_CHECK(retval == XIA_NULL_INFO);
        TEST_MSG("xiaDoSpecialRun | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_INFO));
    }

    TEST_CASE("Bad det-chan");
    {
        retval = xiaDoSpecialRun(0, shared_name, &shared_info);
        TEST_CHECK(retval == XIA_INVALID_DETCHAN);
        TEST_MSG("xiaDoSpecialRun | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_INVALID_DETCHAN));
    }
    cleanup();
}

void download_firmware(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("NULL type");
    {
        retval = xiaDownloadFirmware(0, NULL);
        TEST_CHECK(retval == XIA_NULL_TYPE);
        TEST_MSG("xiaDownloadFirmware | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_TYPE));
    }

    TEST_CASE("Bad det-chan");
    {
        retval = xiaDownloadFirmware(0, shared_type);
        TEST_CHECK(retval == XIA_INVALID_DETCHAN);
        TEST_MSG("xiaDownloadFirmware | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_INVALID_DETCHAN));
    }
    cleanup();
}

void enable_log_output(void) {
    int retval;

    TEST_CASE("Enable once");
    {
        retval = xiaEnableLogOutput();
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaEnableLogOutput | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
    }

    TEST_CASE("Enable twice");
    {
        retval = xiaEnableLogOutput();
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaEnableLogOutput | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
        retval = xiaEnableLogOutput();
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaEnableLogOutput | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
    }
    cleanup();
}

void exit_system(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Exit before start");
    {
        retval = xiaExit();
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaExit | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
    }

    TEST_CASE("Exit multiple times");
    {
        retval = xiaExit();
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaExit | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
        retval = xiaExit();
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaExit | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
    }
    cleanup();
}

void gain_operation(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null Name");
    {
        retval = xiaGainOperation(0, NULL, NULL);
        TEST_CHECK(retval == XIA_NULL_NAME);
        TEST_MSG("xiaGainOperation | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_NAME));
    }

    TEST_CASE("Null Value");
    {
        retval = xiaGainOperation(0, shared_name, NULL);
        TEST_CHECK(retval == XIA_NULL_VALUE);
        TEST_MSG("xiaGainOperation | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_VALUE));
    }

    TEST_CASE("Bad det-chan");
    {
        retval = xiaGainOperation(0, shared_name, &shared_value);
        TEST_CHECK(retval == XIA_INVALID_DETCHAN);
        TEST_MSG("xiaGainOperation | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_INVALID_DETCHAN));
    }
    cleanup();
}

void get_acquisition_values(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null Name");
    {
        retval = xiaGetAcquisitionValues(0, NULL, NULL);
        TEST_CHECK(retval == XIA_NULL_NAME);
        TEST_MSG("xiaGetAcquisitionValues | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_NAME));
    }

    TEST_CASE("Null Value");
    {
        retval = xiaGetAcquisitionValues(0, shared_name, NULL);
        TEST_CHECK(retval == XIA_NULL_VALUE);
        TEST_MSG("xiaGetAcquisitionValues | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_VALUE));
    }

    TEST_CASE("Bad det-chan");
    {
        retval = xiaGetAcquisitionValues(0, shared_name, &shared_value);
        TEST_CHECK(retval == XIA_INVALID_DETCHAN);
        TEST_MSG("xiaGetAcquisitionValues | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_INVALID_DETCHAN));
    }
    cleanup();
}

void get_detector_item(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null Alias");
    {
        retval = xiaGetDetectorItem(NULL, NULL, NULL);
        TEST_CHECK(retval == XIA_NULL_ALIAS);
        TEST_MSG("xiaGetDetectorItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_ALIAS));
    }

    TEST_CASE("Null Name");
    {
        retval = xiaGetDetectorItem(shared_alias, NULL, NULL);
        TEST_CHECK(retval == XIA_NULL_NAME);
        TEST_MSG("xiaGetDetectorItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_NAME));
    }

    TEST_CASE("Null Value");
    {
        retval = xiaGetDetectorItem(shared_alias, shared_name, NULL);
        TEST_CHECK(retval == XIA_NULL_VALUE);
        TEST_MSG("xiaGetDetectorItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_NAME));
    }

    TEST_CASE("No alias");
    {
        retval = xiaGetDetectorItem(shared_alias, shared_name, &shared_value);
        TEST_CHECK(retval == XIA_NO_ALIAS);
        TEST_MSG("xiaGetDetectorItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NO_ALIAS));
    }

    TEST_CASE("Undefined item");
    create_det(shared_alias, "reset", "-", 1, shared_value, shared_value);
    {
        retval = xiaGetDetectorItem(shared_alias, shared_name, &shared_value);
        TEST_CHECK(retval == XIA_BAD_NAME);
        TEST_MSG("xiaGetDetectorItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_BAD_NAME));
    }

    TEST_CASE("Happy path");
    {
        double val = 0;
        retval = xiaGetDetectorItem(shared_alias, "type_value", &val);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaGetDetectorItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
        TEST_CHECK(val == shared_value);
        TEST_MSG("%i != %i", (int)val, shared_value);
    }
    cleanup();
}

void get_detectors(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null list");
    {
        retval = xiaGetDetectors(NULL);
        TEST_CHECK(retval == XIA_NULL_VALUE);
        TEST_MSG("xiaGetDetectors | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_VALUE));
    }

    TEST_CASE("Null list elements");
    {
        for (unsigned int i = 0; i < NUM_ALIASES; i++) {
            retval = xiaNewDetector(aliases[i]);
            TEST_CHECK(retval == XIA_SUCCESS);
            TEST_MSG("xiaNewDetector | %s",
                     tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
        }

        char** det_list = NULL;
        det_list = (char**) malloc(1 * sizeof(char*));
        for (unsigned int i = 0; i < 1; i++) {
            det_list[i] = NULL;
        }

        retval = xiaGetDetectors(det_list);
        TEST_CHECK(retval == XIA_NULL_VALUE);
        TEST_MSG("xiaGetDetectors | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_VALUE));

        for (unsigned int i = 0; i < 1; i++) {
            free(det_list[i]);
        }
        free(det_list);
    }

    TEST_CASE("Validate order");
    {
        unsigned int det_chk = 0;
        xiaGetNumDetectors(&det_chk);
        TEST_CHECK(det_chk == 3);
        TEST_MSG("%i != 3", det_chk);

        char** det_list = NULL;
        det_list = (char**) malloc(det_chk * sizeof(char*));
        for (unsigned int i = 0; i < det_chk; i++) {
            det_list[i] = (char*) calloc(MAXALIAS_LEN, sizeof(char));
        }

        retval = xiaGetDetectors(det_list);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaGetDetectors | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

        for (unsigned int i = 0; i < det_chk; i++) {
            TEST_CHECK(strncmp(det_list[i], aliases[i], strlen(det_list[i])) == 0);
            TEST_MSG("%s != %s", det_list[i], aliases[i]);
            free(det_list[i]);
        }
        free(det_list);
    }
    cleanup();
}

void get_error_text(void) {
    xiaSuppressLogOutput();

    const int comp_len = 16;
    char retval[17];

    TEST_CASE("unknown");
    {
        const char expected_2048[] = "Unknown error code";
        strncpy(retval, xiaGetErrorText(2048), comp_len);
        TEST_CHECK(strncmp(retval, expected_2048, comp_len) == 0);
        TEST_MSG("xiaGetErrorText | %s != %s", retval, expected_2048);
    }

    TEST_CASE("XIA_BAD_PSL_ARGS");
    {
        const char expected_XIA_BAD_PSL_ARGS[] = "Bad call arguments to PSL function";
        strncpy(retval, xiaGetErrorText(XIA_BAD_PSL_ARGS), comp_len);
        TEST_CHECK(strncmp(retval, expected_XIA_BAD_PSL_ARGS, comp_len) == 0);
        TEST_MSG("xiaGetErrorText | %s != %s", retval, expected_XIA_BAD_PSL_ARGS);
    }

    TEST_CASE("DXP_LOG_LEVEL");
    {
        const char expected_DXP_LOG_LEVEL[] = "Log level invalid";
        strncpy(retval, xiaGetErrorText(DXP_LOG_LEVEL), comp_len);
        TEST_CHECK(strncmp(retval, expected_DXP_LOG_LEVEL, comp_len) == 0);
        TEST_MSG("xiaGetErrorText | %s != %s", retval, expected_DXP_LOG_LEVEL);
    }
    cleanup();
}

void get_firmware_item(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null Alias");
    {
        retval = xiaGetFirmwareItem(NULL, 0, NULL, NULL);
        TEST_CHECK(retval == XIA_NULL_ALIAS);
        TEST_MSG("xiaGetFirmwareItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_ALIAS));
    }

    TEST_CASE("Null Name");
    {
        retval = xiaGetFirmwareItem(shared_alias, 0, NULL, NULL);
        TEST_CHECK(retval == XIA_NULL_NAME);
        TEST_MSG("xiaGetFirmwareItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_NAME));
    }

    TEST_CASE("Null Value");
    {
        retval = xiaGetFirmwareItem(shared_alias, 0, shared_name, NULL);
        TEST_CHECK(retval == XIA_NULL_VALUE);
        TEST_MSG("xiaGetFirmwareItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_NAME));
    }

    TEST_CASE("Undefined alias");
    {
        retval = xiaGetFirmwareItem(shared_alias, 0, shared_name, &shared_value);
        TEST_CHECK(retval == XIA_NO_ALIAS);
        TEST_MSG("xiaGetFirmwareItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NO_ALIAS));
    }
    cleanup();
}

void get_firmware_sets(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null list");
    {
        retval = xiaGetFirmwareSets(NULL);
        TEST_CHECK(retval == XIA_NULL_VALUE);
        TEST_MSG("xiaGetFirmwareSets | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_VALUE));
    }

    TEST_CASE("Null list elements");
    {
        for (unsigned int i = 0; i < NUM_ALIASES; i++) {
            retval = xiaNewFirmware(aliases[i]);
            TEST_CHECK(retval == XIA_SUCCESS);
            TEST_MSG("xiaNewFirmware | %s",
                     tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
        }

        char** fw_list = NULL;
        fw_list = (char**) malloc(1 * sizeof(char*));
        for (unsigned int i = 0; i < 1; i++) {
            fw_list[i] = NULL;
        }

        retval = xiaGetFirmwareSets(fw_list);
        TEST_CHECK(retval == XIA_NULL_VALUE);
        TEST_MSG("xiaGetFirmwareSets | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_VALUE));

        for (unsigned int i = 0; i < 1; i++) {
            free(fw_list[i]);
        }
        free(fw_list);
    }

    TEST_CASE("Validate order");
    {
        unsigned int fw_chk = 0;
        xiaGetNumFirmwareSets(&fw_chk);
        TEST_CHECK(fw_chk == 3);
        TEST_MSG("%i != 3", fw_chk);

        char** fw_list = NULL;
        fw_list = (char**) malloc(fw_chk * sizeof(char*));
        for (unsigned int i = 0; i < fw_chk; i++) {
            fw_list[i] = (char*) calloc(MAXALIAS_LEN, sizeof(char));
        }

        retval = xiaGetFirmwareSets(fw_list);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaGetFirmwareSets | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

        for (unsigned int i = 0; i < fw_chk; i++) {
            TEST_CHECK(strncmp(fw_list[i], aliases[i], strlen(fw_list[i])) == 0);
            TEST_MSG("%s != %s", fw_list[i], aliases[i]);
            free(fw_list[i]);
        }
        free(fw_list);
    }
    cleanup();
}

void get_module_item(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null Alias");
    {
        retval = xiaGetModuleItem(NULL, NULL, NULL);
        TEST_CHECK(retval == XIA_NULL_ALIAS);
        TEST_MSG("xiaGetModuleItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_ALIAS));
    }

    TEST_CASE("Null Name");
    {
        retval = xiaGetModuleItem(shared_alias, NULL, NULL);
        TEST_CHECK(retval == XIA_NULL_NAME);
        TEST_MSG("xiaGetModuleItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_NAME));
    }

    TEST_CASE("Null Value");
    {
        retval = xiaGetModuleItem(shared_alias, shared_name, NULL);
        TEST_CHECK(retval == XIA_NULL_VALUE);
        TEST_MSG("xiaGetModuleItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_NAME));
    }
    cleanup();
}

void get_modules(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null list");
    {
        retval = xiaGetModules(NULL);
        TEST_CHECK(retval == XIA_NULL_VALUE);
        TEST_MSG("xiaGetModules | %s", tst_msg(errmsg, MSGLEN, retval, XIA_NULL_VALUE));
    }

    TEST_CASE("Null list elements");
    {
        for (unsigned int i = 0; i < NUM_ALIASES; i++) {
            retval = xiaNewModule(aliases[i]);
        }
        char** mod_list = NULL;
        mod_list = (char**) malloc(1 * sizeof(char*));
        for (unsigned int i = 0; i < 1; i++) {
            mod_list[i] = NULL;
        }

        retval = xiaGetModules(mod_list);
        TEST_CHECK(retval == XIA_NULL_VALUE);
        TEST_MSG("xiaGetModules | %s", tst_msg(errmsg, MSGLEN, retval, XIA_NULL_VALUE));

        for (unsigned int i = 0; i < 1; i++) {
            free(mod_list[i]);
        }
        free(mod_list);
    }

    TEST_CASE("Check order");
    {
        unsigned int mod_chk = 0;
        xiaGetNumModules(&mod_chk);
        TEST_CHECK(mod_chk == 3);
        TEST_MSG("%i != 3", mod_chk);

        char** mod_list = NULL;
        mod_list = (char**) malloc(mod_chk * sizeof(char*));
        for (unsigned int i = 0; i < mod_chk; i++) {
            mod_list[i] = (char*) calloc(MAXALIAS_LEN, sizeof(char));
        }

        retval = xiaGetModules(mod_list);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaGetModules | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

        for (unsigned int i = 0; i < mod_chk; i++) {
            TEST_CHECK(strncmp(mod_list[i], aliases[i], strlen(mod_list[i])) == 0);
            TEST_MSG("%s != %s", mod_list[i], aliases[i]);
            free(mod_list[i]);
        }
        free(mod_list);
    }
    cleanup();
}

void get_num_detectors(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null Value");
    {
        retval = xiaGetNumDetectors(NULL);
        TEST_CHECK(retval == XIA_NULL_VALUE);
        TEST_MSG("xiaGetNumDetectors | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_VALUE));
    }

    TEST_CASE("No detectors");
    {
        unsigned int num_dets = 12;
        retval = xiaGetNumDetectors(&num_dets);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaGetNumDetectors | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
        TEST_CHECK(num_dets == 0);
        TEST_MSG("%i = %i", num_dets, 0);
    }

    TEST_CASE("One detector");
    {
        create_det(shared_alias, "reset", "+", 1, shared_value, shared_value);
        unsigned int num_dets = 12;
        retval = xiaGetNumDetectors(&num_dets);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaGetNumDetectors | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
        TEST_CHECK(num_dets == 1);
        TEST_MSG("%i = %i", num_dets, 1);
    }
    cleanup();
}

void get_num_firmware_sets(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null Value");
    {
        retval = xiaGetNumFirmwareSets(NULL);
        TEST_CHECK(retval == XIA_NULL_VALUE);
        TEST_MSG("xiaGetNumFirmwareSets | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_VALUE));
    }

    TEST_CASE("No firmware");
    {
        unsigned int num_fw = 12;
        retval = xiaGetNumFirmwareSets(&num_fw);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaGetNumFirmwareSets | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
        TEST_CHECK(num_fw == 0);
        TEST_MSG("%i = %i", num_fw, 0);
    }

    TEST_CASE("One firmware");
    {
        create_fw(shared_alias);
        unsigned int num_fw = 12;
        retval = xiaGetNumFirmwareSets(&num_fw);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaGetNumFirmwareSets | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
        TEST_CHECK(num_fw == 1);
        TEST_MSG("%i = %i", num_fw, 1);
    }
    cleanup();
}

void get_num_modules(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null Value");
    {
        retval = xiaGetNumModules(NULL);
        TEST_CHECK(retval == XIA_NULL_VALUE);
        TEST_MSG("xiaGetNumModules | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_VALUE));
    }

    TEST_CASE("No modules");
    {
        unsigned int num_mods = 12;
        retval = xiaGetNumModules(&num_mods);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaGetNumModules | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
        TEST_CHECK(num_mods == 0);
        TEST_MSG("%i != %i", num_mods, 0);
    }

    TEST_CASE("One module");
    {
        create_mod(shared_alias, "udxp", "usb2");
        unsigned int num_mods = 12;
        retval = xiaGetNumModules(&num_mods);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaGetNumModules | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
        TEST_CHECK(num_mods == 1);
        TEST_MSG("%i != %i", num_mods, 1);
    }
    cleanup();
}

void get_num_params(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null Value");
    {
        retval = xiaGetNumParams(0, NULL);
        TEST_CHECK(retval == XIA_NULL_VALUE);
        TEST_MSG("xiaGetNumParams | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_VALUE));
    }

    unsigned short num_param = 0;
    TEST_CASE("Uninitialized");
    {
        retval = xiaGetNumParams(0, &num_param);
        TEST_CHECK(retval == XIA_INVALID_DETCHAN);
        TEST_MSG("xiaGetNumParams | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_INVALID_DETCHAN));
    }
    cleanup();
}

void get_num_ptrrs(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null Alias");
    {
        retval = xiaGetNumPTRRs(NULL, NULL);
        TEST_CHECK(retval == XIA_NULL_ALIAS);
        TEST_MSG("xiaGetNumPTRRs | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_ALIAS));
    }

    TEST_CASE("Null Value");
    {
        retval = xiaGetNumPTRRs(shared_alias, NULL);
        TEST_CHECK(retval == XIA_NULL_VALUE);
        TEST_MSG("xiaGetNumPTRRs | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_VALUE));
    }

    unsigned int num_ptrr = 1337;

    TEST_CASE("No Alias");
    {
        retval = xiaGetNumPTRRs(shared_alias, &num_ptrr);
        TEST_CHECK(retval == XIA_NO_ALIAS);
        TEST_MSG("xiaGetNumPTRRs | %s", tst_msg(errmsg, MSGLEN, retval, XIA_NO_ALIAS));
    }

    xiaNewFirmware(shared_alias);

    TEST_CASE("Happy path");
    {
        retval = xiaGetNumPTRRs(shared_alias, &num_ptrr);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaGetNumPTRRs | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_LOOKING_PTRR));
        TEST_CHECK(num_ptrr == 0);
        TEST_MSG("%i != %i", num_ptrr, 0);
    }

    char filename[] = "file.txt";
    xiaAddFirmwareItem(shared_alias, "filename", filename);
    TEST_CASE("Alias w/ file");
    {
        retval = xiaGetNumPTRRs(shared_alias, &num_ptrr);
        TEST_CHECK(retval == XIA_LOOKING_PTRR);
        TEST_MSG("xiaGetNumPTRRs | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_LOOKING_PTRR));
    }
    cleanup();
}

void get_param_data(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null Name");
    {
        retval = xiaGetParamData(0, NULL, NULL);
        TEST_CHECK(retval == XIA_NULL_NAME);
        TEST_MSG("xiaGetParamData | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_NAME));
    }

    TEST_CASE("Null Value");
    {
        retval = xiaGetParamData(0, shared_name, NULL);
        TEST_CHECK(retval == XIA_NULL_VALUE);
        TEST_MSG("xiaGetParamData | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_VALUE));
    }

    TEST_CASE("Invalid det chan");
    {
        int value = 1337;
        retval = xiaGetParamData(0, shared_name, &value);
        TEST_CHECK(retval == XIA_INVALID_DETCHAN);
        TEST_MSG("xiaGetParamData | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_INVALID_DETCHAN));
    }
    cleanup();
}

void get_param_name(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null Name");
    {
        retval = xiaGetParamName(0, 0, NULL);
        TEST_CHECK(retval == XIA_NULL_NAME);
        TEST_MSG("xiaGetParamName | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_NAME));
    }

    TEST_CASE("Invalid det chan");
    {
        char param_name[] = "no-name";
        retval = xiaGetParamName(0, 0, param_name);
        TEST_CHECK(retval == XIA_INVALID_DETCHAN);
        TEST_MSG("xiaGetParamName | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_INVALID_DETCHAN));
    }
    cleanup();
}

void get_parameter(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null Name");
    {
        retval = xiaGetParameter(0, NULL, NULL);
        TEST_CHECK(retval == XIA_NULL_NAME);
        TEST_MSG("xiaGetParameter | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_NAME));
    }

    TEST_CASE("Null Value");
    {
        retval = xiaGetParameter(0, shared_name, NULL);
        TEST_CHECK(retval == XIA_NULL_VALUE);
        TEST_MSG("xiaGetParameter | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_VALUE));
    }

    unsigned short param = 0;
    TEST_CASE("Uninitialized");
    {
        retval = xiaGetParameter(0, shared_name, &param);
        TEST_CHECK(retval == XIA_INVALID_DETCHAN);
        TEST_MSG("xiaGetParameter | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_INVALID_DETCHAN));
    }
    cleanup();
}

void get_run_data(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null Name");
    {
        retval = xiaGetRunData(0, NULL, NULL);
        TEST_CHECK(retval == XIA_NULL_NAME);
        TEST_MSG("xiaGetRunData | %s", tst_msg(errmsg, MSGLEN, retval, XIA_NULL_NAME));
    }

    TEST_CASE("Null Value");
    {
        retval = xiaGetRunData(0, shared_name, NULL);
        TEST_CHECK(retval == XIA_NULL_VALUE);
        TEST_MSG("xiaGetRunData | %s", tst_msg(errmsg, MSGLEN, retval, XIA_NULL_VALUE));
    }

    unsigned short param = 0;
    TEST_CASE("Uninitialized");
    {
        retval = xiaGetRunData(0, shared_name, &param);
        TEST_CHECK(retval == XIA_INVALID_DETCHAN);
        TEST_MSG("xiaGetRunData | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_INVALID_DETCHAN));
    }
    cleanup();
}

void get_special_run_data(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null Name");
    {
        retval = xiaGetSpecialRunData(0, NULL, NULL);
        TEST_CHECK(retval == XIA_NULL_NAME);
        TEST_MSG("xiaGetSpecialRunData | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_NAME));
    }

    TEST_CASE("Null Value");
    {
        retval = xiaGetSpecialRunData(0, shared_name, NULL);
        TEST_CHECK(retval == XIA_NULL_VALUE);
        TEST_MSG("xiaGetSpecialRunData | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_VALUE));
    }

    unsigned short param = 0;
    TEST_CASE("Uninitialized");
    {
        retval = xiaGetSpecialRunData(0, shared_name, &param);
        TEST_CHECK(retval == XIA_INVALID_DETCHAN);
        TEST_MSG("xiaGetSpecialRunData | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_INVALID_DETCHAN));
    }
    cleanup();
}

void get_version_info(void) {
    xiaSuppressLogOutput();

    int rel = -1337;
    TEST_CASE("Null min");
    {
        xiaGetVersionInfo(&rel, NULL, NULL, NULL);
        TEST_CHECK(rel == -1337);
        TEST_MSG("%i == %i", rel, -1337);
    }

    int min = -1336;
    TEST_CASE("Null maj");
    {
        xiaGetVersionInfo(&rel, &min, NULL, NULL);
        TEST_CHECK(rel == -1337);
        TEST_MSG("%i == %i", rel, -1337);
        TEST_CHECK(min == -1336);
        TEST_MSG("%i == %i", rel, -1336);
    }

    int maj = -1335;
    TEST_CASE("Null pretty");
    {
        xiaGetVersionInfo(&rel, &min, &maj, NULL);
        TEST_CHECK(rel != -1337);
        TEST_MSG("%i == %i", rel, -1337);
        TEST_CHECK(min != -1336);
        TEST_MSG("%i == %i", rel, -1336);
        TEST_CHECK(min != -1335);
        TEST_MSG("%i == %i", rel, -1335);
    }
    cleanup();
}

void init_handel(void) {
    xiaSuppressLogOutput();
    int retval = xiaInitHandel();
    TEST_CHECK(retval == XIA_SUCCESS);
    TEST_MSG("xiaInitHandel | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
    cleanup();
}

void load_system(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null type");
    {
        retval = xiaLoadSystem(NULL, NULL);
        TEST_CHECK(retval == XIA_NULL_TYPE);
        TEST_MSG("xiaLoadSystem | %s", tst_msg(errmsg, MSGLEN, retval, XIA_NULL_TYPE));
    }

    TEST_CASE("Null filename");
    {
        retval = xiaLoadSystem(shared_type, NULL);
        TEST_CHECK(retval == XIA_NO_FILENAME);
        TEST_MSG("xiaLoadSystem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NO_FILENAME));
    }

    TEST_CASE("Bad extension");
    {
        char path[] = "bad_path.txt";
        retval = xiaLoadSystem(shared_type, path);
        TEST_CHECK(retval == XIA_FILE_TYPE);
        TEST_MSG("xiaLoadSystem | %s", tst_msg(errmsg, MSGLEN, retval, XIA_FILE_TYPE));
    }

    TEST_CASE("File missing");
    {
        char ftype[] = "handel_ini";
        char path[] = "bad_path.ini";
        retval = xiaLoadSystem(ftype, path);
        TEST_CHECK(retval == XIA_OPEN_FILE);
        TEST_MSG("xiaLoadSystem | %s", tst_msg(errmsg, MSGLEN, retval, XIA_OPEN_FILE));
    }
    cleanup();
}

void modify_detector_item(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null Alias");
    {
        retval = xiaModifyDetectorItem(NULL, NULL, NULL);
        TEST_CHECK(retval == XIA_NULL_ALIAS);
        TEST_MSG("xiaModifyDetectorItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_ALIAS));
    }

    TEST_CASE("Null Name");
    {
        retval = xiaModifyDetectorItem(shared_alias, NULL, NULL);
        TEST_CHECK(retval == XIA_NULL_NAME);
        TEST_MSG("xiaModifyDetectorItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_NAME));
    }

    TEST_CASE("Null Value");
    {
        retval = xiaModifyDetectorItem(shared_alias, shared_name, NULL);
        TEST_CHECK(retval == XIA_NULL_VALUE);
        TEST_MSG("xiaModifyDetectorItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_VALUE));
    }

    TEST_CASE("Bad name");
    {
        retval = xiaModifyDetectorItem(shared_alias, shared_name, &shared_value);
        TEST_CHECK(retval == XIA_BAD_NAME);
        TEST_MSG("xiaModifyDetectorItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_BAD_NAME));
    }

    TEST_CASE("Missing alias");
    {
        retval = xiaModifyDetectorItem(shared_alias, "channel_gain", &shared_value);
        TEST_CHECK(retval == XIA_NO_ALIAS);
        TEST_MSG("xiaModifyDetectorItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NO_ALIAS));
    }
    cleanup();
}

void modify_firmware_item(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null Alias");
    {
        retval = xiaModifyFirmwareItem(NULL, 0, NULL, NULL);
        TEST_CHECK(retval == XIA_NULL_ALIAS);
        TEST_MSG("xiaModifyFirmwareItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_ALIAS));
    }

    TEST_CASE("Null Name");
    {
        retval = xiaModifyFirmwareItem(shared_alias, 0, NULL, NULL);
        TEST_CHECK(retval == XIA_NULL_NAME);
        TEST_MSG("xiaModifyFirmwareItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_NAME));
    }

    TEST_CASE("Null Value");
    {
        retval = xiaModifyFirmwareItem(shared_alias, 0, shared_name, NULL);
        TEST_CHECK(retval == XIA_NULL_VALUE);
        TEST_MSG("xiaModifyFirmwareItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_VALUE));
    }

    TEST_CASE("Missing alias");
    {
        retval = xiaModifyFirmwareItem(shared_alias, 0, shared_name, &shared_value);
        TEST_CHECK(retval == XIA_NO_ALIAS);
        TEST_MSG("xiaModifyFirmwareItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NO_ALIAS));
    }

    xiaNewFirmware(shared_alias);

    TEST_CASE("PTRR invariant name");
    {
        retval = xiaModifyFirmwareItem(shared_alias, 0, "mmu", &shared_value);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaModifyFirmwareItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
    }

    TEST_CASE("PTRR dependent name");
    {
        retval = xiaModifyFirmwareItem(shared_alias, 0, shared_name, &shared_value);
        TEST_CHECK(retval == XIA_BAD_VALUE);
        TEST_MSG("xiaModifyFirmwareItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_BAD_VALUE));
    }
    cleanup();
}

void modify_module_item(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null Alias");
    {
        retval = xiaModifyModuleItem(NULL, NULL, NULL);
        TEST_CHECK(retval == XIA_NULL_ALIAS);
        TEST_MSG("xiaModifyModuleItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_ALIAS));
    }

    TEST_CASE("Null Name");
    {
        retval = xiaModifyModuleItem(shared_alias, NULL, NULL);
        TEST_CHECK(retval == XIA_NULL_NAME);
        TEST_MSG("xiaModifyModuleItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_NAME));
    }

    TEST_CASE("Null Value");
    {
        retval = xiaModifyModuleItem(shared_alias, shared_name, NULL);
        TEST_CHECK(retval == XIA_NULL_VALUE);
        TEST_MSG("xiaModifyModuleItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_VALUE));
    }

    TEST_CASE("Missing alias");
    {
        retval = xiaModifyModuleItem(shared_alias, shared_name, &shared_value);
        TEST_CHECK(retval == XIA_NO_ALIAS);
        TEST_MSG("xiaModifyModuleItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NO_ALIAS));
    }

    TEST_CASE("Read-only items");
    {
        retval = xiaModifyModuleItem(shared_alias, "module_type", &shared_value);
        TEST_CHECK(retval == XIA_NO_MODIFY);
        TEST_MSG("xiaModifyModuleItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NO_MODIFY));

        retval = xiaModifyModuleItem(shared_alias, "number_of_channels", &shared_value);
        TEST_CHECK(retval == XIA_NO_MODIFY);
        TEST_MSG("xiaModifyModuleItem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NO_MODIFY));
    }
    cleanup();
}

void new_detector(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null Alias");
    {
        retval = xiaNewDetector(NULL);
        TEST_CHECK(retval == XIA_NULL_ALIAS);
        TEST_MSG("xiaNewDetector | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_ALIAS));
    }

    TEST_CASE("Alias too long");
    {
        char long_alias[1024];
        xia_fill_char_ary(long_alias, 1024, 'a');
        retval = xiaNewDetector(long_alias);
        TEST_CHECK(retval == XIA_ALIAS_SIZE);
        TEST_MSG("xiaNewDetector | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_ALIAS_SIZE));
    }

    TEST_CASE("Define");
    {
        retval = xiaNewDetector(aliases[0]);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaNewDetector | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
    }

    TEST_CASE("Define again");
    {
        retval = xiaNewDetector(aliases[0]);
        TEST_CHECK(retval == XIA_ALIAS_EXISTS);
        TEST_MSG("xiaNewDetector | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_ALIAS_EXISTS));
    }
    cleanup();
}

void new_firmware(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null Alias");
    {
        retval = xiaNewFirmware(NULL);
        TEST_CHECK(retval == XIA_NULL_ALIAS);
        TEST_MSG("xiaNewFirmware | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_ALIAS));
    }

    TEST_CASE("Alias too long");
    {
        char long_alias[1024];
        xia_fill_char_ary(long_alias, 1024, 'a');
        retval = xiaNewFirmware(long_alias);
        TEST_CHECK(retval == XIA_ALIAS_SIZE);
        TEST_MSG("xiaNewFirmware | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_ALIAS_SIZE));
    }

    TEST_CASE("Define");
    {
        retval = xiaNewFirmware(aliases[0]);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaNewFirmware | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
    }

    TEST_CASE("Define again");
    {
        retval = xiaNewFirmware(aliases[0]);
        TEST_CHECK(retval == XIA_ALIAS_EXISTS);
        TEST_MSG("xiaNewFirmware | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_ALIAS_EXISTS));
    }
    cleanup();
}

void new_module(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null Alias");
    {
        retval = xiaNewModule(NULL);
        TEST_CHECK(retval == XIA_NULL_ALIAS);
        TEST_MSG("xiaNewModule | %s", tst_msg(errmsg, MSGLEN, retval, XIA_NULL_ALIAS));
    }

    TEST_CASE("Alias too long");
    {
        char long_alias[1024];
        xia_fill_char_ary(long_alias, 1024, 'a');
        retval = xiaNewModule(long_alias);
        TEST_CHECK(retval == XIA_ALIAS_SIZE);
        TEST_MSG("xiaNewModule | %s", tst_msg(errmsg, MSGLEN, retval, XIA_ALIAS_SIZE));
    }

    TEST_CASE("Define");
    {
        retval = xiaNewModule(aliases[0]);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaNewModule | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
    }

    TEST_CASE("Define again");
    {
        retval = xiaNewModule(aliases[0]);
        TEST_CHECK(retval == XIA_ALIAS_EXISTS);
        TEST_MSG("xiaNewModule | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_ALIAS_EXISTS));
    }
    cleanup();
}

void remove_acquisition_values(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null Name");
    {
        retval = xiaRemoveAcquisitionValues(0, NULL);
        TEST_CHECK(retval == XIA_NULL_NAME);
        TEST_MSG("xiaRemoveAcquisitionValues | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_NAME));
    }

    TEST_CASE("Bad det-chan");
    {
        retval = xiaRemoveAcquisitionValues(0, shared_name);
        TEST_CHECK(retval == XIA_INVALID_DETCHAN);
        TEST_MSG("xiaRemoveAcquisitionValues | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_INVALID_DETCHAN));
    }
    cleanup();
}

void remove_detector(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null Alias");
    {
        retval = xiaRemoveDetector(NULL);
        TEST_CHECK(retval == XIA_NULL_ALIAS);
        TEST_MSG("xiaRemoveDetector | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_ALIAS));
    }

    TEST_CASE("List empty");
    {
        retval = xiaRemoveDetector(shared_alias);
        TEST_CHECK(retval == XIA_NO_ALIAS);
        TEST_MSG("xiaRemoveDetector | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NO_ALIAS));
    }

    TEST_CASE("Doesn't exist");
    {
        for (unsigned int i = 0; i < NUM_ALIASES; i++) {
            xiaNewDetector(aliases[i]);
        }
        retval = xiaRemoveDetector(shared_alias);
        TEST_CHECK(retval == XIA_NO_ALIAS);
        TEST_MSG("xiaRemoveDetector | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NO_ALIAS));
    }

    unsigned int det_chk = 0;
    TEST_CASE("Remove Head");
    {
        retval = xiaRemoveDetector(aliases[0]);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaRemoveDetector | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

        xiaGetNumDetectors(&det_chk);
        TEST_CHECK(det_chk == 2);
        TEST_MSG("%i != 2", det_chk);

        char** det_list = NULL;
        det_list = (char**) malloc(det_chk * sizeof(char*));
        for (unsigned int i = 0; i < det_chk; i++) {
            det_list[i] = (char*) calloc(MAXALIAS_LEN, sizeof(char));
        }

        retval = xiaGetDetectors(det_list);

        for (unsigned int i = 0; i < det_chk; i++) {
            TEST_CHECK(strncmp(det_list[i], aliases[i + 1], strlen(det_list[i])) == 0);
            TEST_MSG("%s != %s", det_list[i], aliases[i + 1]);
            free(det_list[i]);
        }
        free(det_list);
    }

    TEST_CASE("Remove middle");
    {
        xiaNewDetector(shared_alias);

        det_chk = 0;
        xiaGetNumDetectors(&det_chk);
        TEST_CHECK(det_chk == 3);
        TEST_MSG("%i != 3", det_chk);

        retval = xiaRemoveDetector(aliases[2]);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaRemoveDetector | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

        det_chk = 0;
        xiaGetNumDetectors(&det_chk);
        TEST_CHECK(det_chk == 2);
        TEST_MSG("%i != 2", det_chk);

        char** det_list = NULL;
        det_list = (char**) malloc(det_chk * sizeof(char*));
        for (unsigned int i = 0; i < det_chk; i++) {
            det_list[i] = (char*) calloc(MAXALIAS_LEN, sizeof(char));
        }

        retval = xiaGetDetectors(det_list);

        char* expected[] = {"tweedle_dee", shared_alias};
        for (unsigned int i = 0; i < det_chk; i++) {
            TEST_CHECK(strncmp(det_list[i], expected[i], strlen(det_list[i])) == 0);
            TEST_MSG("%s != %s", det_list[i], expected[i]);
            free(det_list[i]);
        }
        free(det_list);
    }
    cleanup();
}

void remove_firmware(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null Alias");
    {
        retval = xiaRemoveFirmware(NULL);
        TEST_CHECK(retval == XIA_NULL_ALIAS);
        TEST_MSG("xiaRemoveFirmware | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_ALIAS));
    }

    TEST_CASE("List empty");
    {
        retval = xiaRemoveFirmware(shared_alias);
        TEST_CHECK(retval == XIA_NO_ALIAS);
        TEST_MSG("xiaRemoveFirmware | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NO_ALIAS));
    }

    TEST_CASE("Doesn't exist");
    {
        for (unsigned int i = 0; i < NUM_ALIASES; i++) {
            xiaNewFirmware(aliases[i]);
        }
        retval = xiaRemoveFirmware(shared_alias);
        TEST_CHECK(retval == XIA_NO_ALIAS);
        TEST_MSG("xiaRemoveFirmware | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NO_ALIAS));
    }

    unsigned int fw_chk = 0;
    TEST_CASE("Remove Head");
    {
        retval = xiaRemoveFirmware(aliases[0]);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaRemoveFirmware | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

        xiaGetNumFirmwareSets(&fw_chk);
        TEST_CHECK(fw_chk == 2);
        TEST_MSG("%i != 2", fw_chk);

        char** fw_list = NULL;
        fw_list = (char**) malloc(fw_chk * sizeof(char*));
        for (unsigned int i = 0; i < fw_chk; i++) {
            fw_list[i] = (char*) calloc(MAXALIAS_LEN, sizeof(char));
        }

        retval = xiaGetFirmwareSets(fw_list);

        for (unsigned int i = 0; i < fw_chk; i++) {
            TEST_CHECK(strncmp(fw_list[i], aliases[i + 1], strlen(fw_list[i])) == 0);
            TEST_MSG("%s != %s", fw_list[i], aliases[i + 1]);
            free(fw_list[i]);
        }
        free(fw_list);
    }

    TEST_CASE("Remove middle");
    {
        xiaNewFirmware(shared_alias);

        fw_chk = 0;
        xiaGetNumFirmwareSets(&fw_chk);
        TEST_CHECK(fw_chk == 3);
        TEST_MSG("%i != 3", fw_chk);

        retval = xiaRemoveFirmware(aliases[2]);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaRemoveFirmware | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

        fw_chk = 0;
        xiaGetNumFirmwareSets(&fw_chk);
        TEST_CHECK(fw_chk == 2);
        TEST_MSG("%i != 2", fw_chk);

        char** fw_list = NULL;
        fw_list = (char**) malloc(fw_chk * sizeof(char*));
        for (unsigned int i = 0; i < fw_chk; i++) {
            fw_list[i] = (char*) calloc(MAXALIAS_LEN, sizeof(char));
        }

        retval = xiaGetFirmwareSets(fw_list);

        char* expected[] = {"tweedle_dee", shared_alias};
        for (unsigned int i = 0; i < fw_chk; i++) {
            TEST_CHECK(strncmp(fw_list[i], expected[i], strlen(fw_list[i])) == 0);
            TEST_MSG("%s != %s", fw_list[i], expected[i]);
            free(fw_list[i]);
        }
        free(fw_list);
    }
    cleanup();
}

void remove_module(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null Alias");
    {
        retval = xiaRemoveModule(NULL);
        TEST_CHECK(retval == XIA_NULL_ALIAS);
        TEST_MSG("xiaRemoveModule | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_ALIAS));
    }

    TEST_CASE("List empty");
    {
        retval = xiaRemoveModule(shared_alias);
        TEST_CHECK(retval == XIA_NO_ALIAS);
        TEST_MSG("xiaRemoveModule | %s", tst_msg(errmsg, MSGLEN, retval, XIA_NO_ALIAS));
    }

    TEST_CASE("Doesn't exist");
    {
        for (unsigned int i = 0; i < NUM_ALIASES; i++) {
            xiaNewModule(aliases[i]);
        }
        retval = xiaRemoveModule(shared_alias);
        TEST_CHECK(retval == XIA_NO_ALIAS);
        TEST_MSG("xiaRemoveModule | %s", tst_msg(errmsg, MSGLEN, retval, XIA_NO_ALIAS));
    }

    unsigned int mod_chk = 0;
    TEST_CASE("Remove Head");
    {
        retval = xiaRemoveModule(aliases[0]);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaRemoveModule | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

        xiaGetNumModules(&mod_chk);
        TEST_CHECK(mod_chk == 2);
        TEST_MSG("%i != 2", mod_chk);

        char** mod_list = NULL;
        mod_list = (char**) malloc(mod_chk * sizeof(char*));
        for (unsigned int i = 0; i < mod_chk; i++) {
            mod_list[i] = (char*) calloc(MAXALIAS_LEN, sizeof(char));
        }

        retval = xiaGetModules(mod_list);

        for (unsigned int i = 0; i < mod_chk; i++) {
            TEST_CHECK(strncmp(mod_list[i], aliases[i + 1], strlen(mod_list[i])) == 0);
            TEST_MSG("%s != %s", mod_list[i], aliases[i + 1]);
            free(mod_list[i]);
        }
        free(mod_list);
    }

    TEST_CASE("Remove middle");
    {
        xiaNewModule(shared_alias);

        mod_chk = 0;
        xiaGetNumModules(&mod_chk);
        TEST_CHECK(mod_chk == 3);
        TEST_MSG("%i != 3", mod_chk);

        retval = xiaRemoveModule(aliases[2]);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaRemoveModule | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

        mod_chk = 0;
        xiaGetNumModules(&mod_chk);
        TEST_CHECK(mod_chk == 2);
        TEST_MSG("%i != 2", mod_chk);

        char** mod_list = NULL;
        mod_list = (char**) malloc(mod_chk * sizeof(char*));
        for (unsigned int i = 0; i < mod_chk; i++) {
            mod_list[i] = (char*) calloc(MAXALIAS_LEN, sizeof(char));
        }

        retval = xiaGetModules(mod_list);

        char* expected[] = {"tweedle_dee", shared_alias};
        for (unsigned int i = 0; i < mod_chk; i++) {
            TEST_CHECK(strncmp(mod_list[i], expected[i], strlen(mod_list[i])) == 0);
            TEST_MSG("%s != %s", mod_list[i], expected[i]);
            free(mod_list[i]);
        }
        free(mod_list);
    }
    cleanup();
}

void save_system(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null type");
    {
        retval = xiaSaveSystem(NULL, NULL);
        TEST_CHECK(retval == XIA_NULL_TYPE);
        TEST_MSG("xiaSaveSystem | %s", tst_msg(errmsg, MSGLEN, retval, XIA_NULL_TYPE));
    }

    TEST_CASE("Null Name");
    {
        retval = xiaSaveSystem(shared_type, NULL);
        TEST_CHECK(retval == XIA_NO_FILENAME);
        TEST_MSG("xiaSaveSystem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NO_FILENAME));
    }

    TEST_CASE("Bad type");
    {
        retval = xiaSaveSystem(shared_type, "rabbithole/setting.ini");
        TEST_CHECK(retval == XIA_FILE_TYPE);
        TEST_MSG("xiaSaveSystem | %s", tst_msg(errmsg, MSGLEN, retval, XIA_FILE_TYPE));
    }

    TEST_CASE("Empty file");
    {
        retval = xiaSaveSystem("handel_ini", "");
        TEST_CHECK(retval == XIA_NO_FILENAME);
        TEST_MSG("xiaSaveSystem | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NO_FILENAME));
    }

    TEST_CASE("Bad file");
    {
        retval = xiaSaveSystem("handel_ini", "rabbithole/setting.ini");
        TEST_CHECK(retval == XIA_OPEN_FILE);
        TEST_MSG("xiaSaveSystem | %s", tst_msg(errmsg, MSGLEN, retval, XIA_OPEN_FILE));
    }

    uint32_t expected = 0;
    TEST_CASE("Empty ini");
    {
        char empty_ini[] = "test_api-empty_config.ini";
        retval = xiaSaveSystem("handel_ini", empty_ini);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaSaveSystem | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

#ifdef XIA_HANDEL_WINDOWS
        expected = 0x96C82422;
#else
        expected = 0x5C5099A3;
#endif
        uint32_t crc = xia_crc32_file(empty_ini);
        TEST_CHECK(crc == expected);
        TEST_MSG("invalid crc: %u != %u", crc, expected);

        TEST_CHECK(remove(empty_ini) == 0);
        TEST_MSG("unable to remove %s", empty_ini);
    }

    TEST_CASE("Happy path");
    {
        create_det("detector1", "reset", "-", 1, 1337, 106);
        create_det("detector2", "reset", "-", 1, 1337, 106);

        unsigned int num_dets = 0;
        TEST_CHECK(xiaGetNumDetectors(&num_dets) == XIA_SUCCESS);
        TEST_CHECK(num_dets == 2);

        create_mod("module1", "udxp", "usb2");
        create_mod("module2", "udxp", "usb2");

        unsigned int num_mods = 0;
        TEST_CHECK(xiaGetNumModules(&num_mods) == XIA_SUCCESS);
        TEST_CHECK(num_mods == 2);

        char twodets_ini[] = "test_api-2dets.ini";
        retval = xiaSaveSystem("handel_ini", twodets_ini);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaSaveSystem | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

        retval = xiaLoadSystem("handel_ini", "test_api-2dets.ini");
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaLoadSystem | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));

#ifdef XIA_HANDEL_WINDOWS
        expected = 0x9CDAB366;
#else
        expected = 0x751E4705;
#endif
        uint32_t crc = xia_crc32_file(twodets_ini);
        TEST_CHECK(crc == expected);
        TEST_MSG("invalid crc: %u != %u", crc, expected);

        TEST_CHECK(remove(twodets_ini) == 0);
        TEST_MSG("unable to remove %s", twodets_ini);
    }
    cleanup();
}

void set_acquisition_values(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null Name");
    {
        retval = xiaSetAcquisitionValues(0, NULL, NULL);
        TEST_CHECK(retval == XIA_NULL_NAME);
        TEST_MSG("xiaSetAcquisitionValues | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_NAME));
    }

    TEST_CASE("Null Value");
    {
        retval = xiaSetAcquisitionValues(0, shared_name, NULL);
        TEST_CHECK(retval == XIA_NULL_VALUE);
        TEST_MSG("xiaSetAcquisitionValues | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_VALUE));
    }

    TEST_CASE("Bad det-chan");
    {
        retval = xiaSetAcquisitionValues(0, shared_name, &shared_value);
        TEST_CHECK(retval == XIA_INVALID_DETCHAN);
        TEST_MSG("xiaSetAcquisitionValues | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_INVALID_DETCHAN));
    }
    cleanup();
}

void set_log_level(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Too small");
    {
        retval = xiaSetLogLevel(-100);
        TEST_CHECK(retval == DXP_LOG_LEVEL);
        TEST_MSG("xiaSetLogLevel | %s", tst_msg(errmsg, MSGLEN, retval, DXP_LOG_LEVEL));
    }

    TEST_CASE("Too BIG");
    {
        retval = xiaSetLogLevel(1000);
        TEST_CHECK(retval == DXP_LOG_LEVEL);
        TEST_MSG("xiaSetLogLevel | %s", tst_msg(errmsg, MSGLEN, retval, DXP_LOG_LEVEL));
    }
    cleanup();
}

void set_log_output(void) {
    /**
     * This can only fail if xiaInitHandel fails. We'll test the gnarly inputs, but
     * know that this isn't really testing much. The only feedback on an error will
     * come from stdout, but it won't propagate an error message.
     */
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null Name");
    {
        retval = xiaSetLogOutput(NULL);
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaSetLogOutput | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
    }

    TEST_CASE("Empty name");
    {
        retval = xiaSetLogOutput("");
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaSetLogOutput | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
    }
    cleanup();
}

void set_parameter(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null Name");
    {
        retval = xiaSetParameter(0, NULL, (unsigned short)shared_value);
        TEST_CHECK(retval == XIA_NULL_NAME);
        TEST_MSG("xiaSetParameter | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_NULL_NAME));
    }

    TEST_CASE("Bad det-chan");
    {
        retval = xiaSetParameter(0, shared_name, (unsigned short)shared_value);
        TEST_CHECK(retval == XIA_INVALID_DETCHAN);
        TEST_MSG("xiaSetParameter | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_INVALID_DETCHAN));
    }
    cleanup();
}

void start_run(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Bad det-chan");
    {
        retval = xiaStartRun(0, 0);
        TEST_CHECK(retval == XIA_INVALID_DETCHAN);
        TEST_MSG("xiaStartRun | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_INVALID_DETCHAN));
    }
    cleanup();
}

void stop_run(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Bad det-chan");
    {
        retval = xiaStopRun(0);
        TEST_CHECK(retval == XIA_INVALID_DETCHAN);
        TEST_MSG("xiaStopRun | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_INVALID_DETCHAN));
    }
    cleanup();
}

void suppress_log_output(void) {
    int retval;

    TEST_CASE("Always success");
    {
        retval = xiaSuppressLogOutput();
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaSuppressLogOutput | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
    }
    cleanup();
}

void update_user_params(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Bad det-chan");
    {
        retval = xiaUpdateUserParams(0);
        TEST_CHECK(retval == XIA_INVALID_DETCHAN);
        TEST_MSG("xiaUpdateUserParams | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_INVALID_DETCHAN));
    }

    TEST_CASE("Bad det-chan");
    {
        retval = xiaUpdateUserParams(0);
        TEST_CHECK(retval == XIA_INVALID_DETCHAN);
        TEST_MSG("xiaUpdateUserParams | %s",
                 tst_msg(errmsg, MSGLEN, retval, XIA_INVALID_DETCHAN));
    }
}

void xia_init(void) {
    int retval;
    xiaSuppressLogOutput();

    TEST_CASE("Null argument");
    {
        retval = xiaInit(NULL);
        TEST_CHECK(retval == XIA_BAD_NAME);
        TEST_MSG("xiaInit | %s", tst_msg(errmsg, MSGLEN, retval, XIA_BAD_NAME));
    }

    TEST_CASE("File doesn't exist");
    {
        retval = xiaInit("bad.ini");
        TEST_CHECK(retval != XIA_BAD_NAME);
        TEST_MSG("xiaInit | %s", tst_msg(errmsg, MSGLEN, retval, XIA_BAD_NAME));
    }

    TEST_CASE("Empty File");
    {
        retval = xiaInit("configs/empty.ini");
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaInit | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
    }

    TEST_CASE("Happy Path - LF");
    {
        retval = xiaInit("configs/unix.ini");
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaInit | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
    }

    TEST_CASE("Happy Path - CRLF");
    {
        retval = xiaInit("configs/udxp_usb2.ini");
        TEST_CHECK(retval == XIA_SUCCESS);
        TEST_MSG("xiaInit | %s", tst_msg(errmsg, MSGLEN, retval, XIA_SUCCESS));
    }
    cleanup();
}

TEST_LIST = {
    {"Add Detector Item", add_detector_item},
    {"Add Firmware Item", add_firmware_item},
    {"Add Module Item", add_module_item},
    {"Board Operation", board_operation},
    {"Close Log", close_log},
    {"Do Special Run", do_special_run},
    {"Download Firmware", download_firmware},
    {"Enable Log Output", enable_log_output},
    {"Exit System", exit_system},
    {"Gain Operation", gain_operation},
    {"Get Acquisition Values", get_acquisition_values},
    {"Get Detector Item", get_detector_item},
    {"Get Detectors", get_detectors},
    {"Get Error Text", get_error_text},
    {"Get Firmware Item", get_firmware_item},
    {"Get Firmware Sets", get_firmware_sets},
    {"Get Module Item", get_module_item},
    {"Get Modules", get_modules},
    {"Get Num Detectors", get_num_detectors},
    {"Get Num Firmware Sets", get_num_firmware_sets},
    {"Get Num Modules", get_num_modules},
    {"Get Num Params", get_num_params},
    {"Get Num PTRRs", get_num_ptrrs},
    {"Get Param Data", get_param_data},
    {"Get Param Name", get_param_name},
    {"Get Parameter", get_parameter},
    {"Get Run Data", get_run_data},
    {"Get Special Run Data", get_special_run_data},
    {"Get Version Info", get_version_info},
    {"Init Handel", init_handel},
    {"Load System", load_system},
    {"Modify Detector Item", modify_detector_item},
    {"Modify Firmware Item", modify_firmware_item},
    {"Modify Module Item", modify_module_item},
    {"New Detector", new_detector},
    {"New Firmware", new_firmware},
    {"New Module", new_module},
    {"Remove Acquisition Values", remove_acquisition_values},
    {"Remove Detector", remove_detector},
    {"Remove Firmware", remove_firmware},
    {"Remove Module", remove_module},
    {"Save System", save_system},
    {"Set Acquisition Values", set_acquisition_values},
    {"Set Log Level", set_log_level},
    {"Set Log Output", set_log_output},
    {"Set Parameter", set_parameter},
    {"Start Run", start_run},
    {"Stop Run", stop_run},
    {"Suppress Log Output", suppress_log_output},
    {"Update User Params", update_user_params},
    {"XIA Init", xia_init},
    {NULL, NULL} /* zeroed record marking the end of the list */
};
