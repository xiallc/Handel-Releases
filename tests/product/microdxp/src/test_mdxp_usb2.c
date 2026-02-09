/* SPDX-License-Identifier: Apache-2.0 */

/*
 * Copyright Thursday, January 29, 2026 XIA LLC, All rights reserved.
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

/** @file test_microdxp_usb.c
 * @brief Tests the microDXP using the USB2 protocol.
 */

/*
 * Test program for microDXP USB2 response issue, loops infinitely on two
 * commands 0x49, 0x42, checking the response for correct size and checksum.
 *
 * Requires two parameters, 1. "device_number" for the microDXP (usually 0)
 * 2. a delay in seconds.
 *
 * argument: [PORT], [SLEEP_SECONDS]
 * example usage: microdxp_usb_test.exe 0 0.02
 *
 * Copyright (c) 2018 XIA LLC
 * All rights reserved
 *
 */
#include <signal.h>
#include <stdio.h>

#include <util/xia_sleep.h>
#include <xia_common.h>
#include <xia_usb2.h>
#include <xia_usb2_errors.h>

#include <acutest.h>

const unsigned int RETVAL_CMD_POS = 1;
const unsigned int RETVAL_LEN_LO_POS = 2;
const unsigned int RETVAL_LEN_HI_POS = 3;
const unsigned int RETVAL_SUCCESS_POS = 4;

const byte_t RS232_ESCAPE = 0x1B;
const byte_t RETVAL_SUCCESS = 0x00;

#define XIA_UART_ADDRESS 0x01000000

void serial_number(void) {
    byte_t cmd[5] = {RS232_ESCAPE, 0x48, 0x00, 0x00, 0x48};
    byte_t ret[23] = {0};

    int mod_num = 0;
    int retval = XIA_USB2_SUCCESS;
    while (retval == XIA_USB2_SUCCESS) {
        HANDLE h = NULL;

        retval = xia_usb2_open(mod_num, &h);
        if (retval == XIA_USB2_DEVICE_NOT_FOUND) {
            break;
        }
        TEST_ASSERT(retval == XIA_USB2_SUCCESS);
        TEST_MSG("device %i open failed with %i", 0, retval);

        retval = xia_usb2_write(h, XIA_UART_ADDRESS, 5, cmd);
        TEST_ASSERT(retval == XIA_USB2_SUCCESS);
        TEST_MSG("device %i write failed with %i", 0, retval);

        retval = xia_usb2_read(h, XIA_UART_ADDRESS, 23, ret);
        TEST_ASSERT(retval == XIA_USB2_SUCCESS);
        TEST_MSG("device %i read failed with %i", 0, retval);

        retval = xia_usb2_close(h);
        TEST_ASSERT(retval == XIA_USB2_SUCCESS);
        TEST_MSG("device %i close failed with %i", 0, retval);

        TEST_ASSERT(ret[0] == RS232_ESCAPE);
        TEST_MSG("%i != %i", ret[0], RS232_ESCAPE);

        TEST_ASSERT(ret[RETVAL_CMD_POS] == 0x48);
        TEST_MSG("%i != %i", ret[1], 0x48);

        TEST_ASSERT(ret[RETVAL_SUCCESS_POS] == RETVAL_SUCCESS);
        TEST_MSG("%i != %i", retval, 0);

        printf("\nsn: ");
        for (unsigned int i = 5; i < 21; i++) {
            printf("%c", ret[i]);
        }
        mod_num++;
    }
    printf("\n");
}

void monitor_dac(void) {
    byte_t cmd[10] = {RS232_ESCAPE, 0x40, 0x4, 0x0, 0x0, 0x29, 0x0, 0x2, 0x6F, 0x48};
    byte_t ret[8] = {0};

    int mod_num = 0;
    int retval = 0;
    while (retval == 0) {
        HANDLE h = NULL;
        retval = xia_usb2_open(mod_num, &h);
        if (retval == XIA_USB2_DEVICE_NOT_FOUND) {
            break;
        }
        TEST_ASSERT(retval == XIA_USB2_SUCCESS);
        TEST_MSG("device %i open failed with %i", 0, retval);

        /**
         * The ADC is a low-power device. We need to write it once to wake it up, a
         * second time to get the data.
         */
        for (unsigned int i = 0; i < 2; i++) {
            retval = xia_usb2_write(h, XIA_UART_ADDRESS, 10, cmd);
            TEST_ASSERT(retval == XIA_USB2_SUCCESS);
            TEST_MSG("device %i write failed with %i", 0, retval);
            xia_sleep(100);
        }

        retval = xia_usb2_read(h, XIA_UART_ADDRESS, 8, ret);
        TEST_ASSERT(retval == XIA_USB2_SUCCESS);
        TEST_MSG("device %i read failed with %i", 0, retval);

        retval = xia_usb2_close(h);
        TEST_ASSERT(retval == XIA_USB2_SUCCESS);
        TEST_MSG("device %i close failed with %i", 0, retval);

        TEST_ASSERT(ret[0] == RS232_ESCAPE);
        TEST_MSG("%i != %i", ret[0], RS232_ESCAPE);

        TEST_ASSERT(ret[RETVAL_SUCCESS_POS] == RETVAL_SUCCESS);
        TEST_MSG("%i != %i", ret[RETVAL_SUCCESS_POS], RETVAL_SUCCESS);

        unsigned short exp_rlen = 3;
        unsigned short rlen =
            BYTE_TO_WORD(ret[RETVAL_LEN_LO_POS], ret[RETVAL_LEN_HI_POS]);
        TEST_ASSERT(rlen == exp_rlen);
        TEST_MSG("%i != %i", retval, 0);

        unsigned int val = ret[7] + (ret[6] << 8) + (ret[5] << 16);
        printf("\nval: %i", val);
        mod_num++;
    }
    printf("\n");
}

void led(void) {

    byte_t ret[11] = {0};

    int mod_num = 0;
    int retval = 0;
    while (retval == 0) {
        HANDLE h = NULL;
        retval = xia_usb2_open(mod_num, &h);
        if (retval == XIA_USB2_DEVICE_NOT_FOUND) {
            break;
        }
        TEST_ASSERT(retval == XIA_USB2_SUCCESS);
        TEST_MSG("device %i open failed with %i", 0, retval);

        unsigned short exp_period = 100;
        unsigned short exp_width = 3;
        TEST_CASE("Write LED values");
        {
            byte_t wcmd[11] = {RS232_ESCAPE, 0xC0, 0x06, 0x00, 0x00, 0x01,
                               0x64,         0x00, 0x03, 0x00, 0xA0};

            retval = xia_usb2_write(h, XIA_UART_ADDRESS, 11, wcmd);
            TEST_ASSERT(retval == XIA_USB2_SUCCESS);
            TEST_MSG("device %i write failed with %i", 0, retval);

            retval = xia_usb2_read(h, XIA_UART_ADDRESS, 11, ret);
            TEST_ASSERT(retval == XIA_USB2_SUCCESS);
            TEST_MSG("device %i read failed with %i", 0, retval);

            TEST_ASSERT(ret[0] == RS232_ESCAPE);
            TEST_MSG("%i != %i", ret[0], RS232_ESCAPE);

            TEST_ASSERT(ret[RETVAL_SUCCESS_POS] == RETVAL_SUCCESS);
            TEST_MSG("%i != %i", ret[RETVAL_SUCCESS_POS], RETVAL_SUCCESS);

            unsigned short exp_rlen = 6;
            unsigned short rlen =
                BYTE_TO_WORD(ret[RETVAL_LEN_LO_POS], ret[RETVAL_LEN_HI_POS]);
            TEST_ASSERT(rlen == exp_rlen);
            TEST_MSG("%i != %i", rlen, exp_rlen);

            TEST_ASSERT(ret[5] == 1);
            TEST_MSG("led not enabled: %i != %i", ret[5], 1);

            unsigned short period = BYTE_TO_WORD(ret[6], ret[7]);
            TEST_ASSERT(period == exp_period);
            TEST_MSG("invalid period: %i != %i", period, exp_period);

            unsigned short width = BYTE_TO_WORD(ret[8], ret[9]);
            TEST_ASSERT(width == exp_width);
            TEST_MSG("invalid width: %i != %i", width, exp_width);
        }

        TEST_CASE("Read LED Values");
        {
            byte_t rcmd[6] = {RS232_ESCAPE, 0xC0, 0x01, 0x00, 0x01, 0xC0};
            retval = xia_usb2_write(h, XIA_UART_ADDRESS, 6, rcmd);
            TEST_ASSERT(retval == XIA_USB2_SUCCESS);
            TEST_MSG("device %i write failed with %i", 0, retval);

            retval = xia_usb2_read(h, XIA_UART_ADDRESS, 11, ret);
            TEST_ASSERT(retval == XIA_USB2_SUCCESS);
            TEST_MSG("device %i read failed with %i", 0, retval);

            TEST_ASSERT(ret[0] == RS232_ESCAPE);
            TEST_MSG("%i != %i", ret[0], RS232_ESCAPE);

            TEST_ASSERT(ret[RETVAL_SUCCESS_POS] == RETVAL_SUCCESS);
            TEST_MSG("%i != %i", ret[RETVAL_SUCCESS_POS], RETVAL_SUCCESS);

            unsigned short exp_rlen = 6;
            unsigned short rlen =
                BYTE_TO_WORD(ret[RETVAL_LEN_LO_POS], ret[RETVAL_LEN_HI_POS]);
            TEST_ASSERT(rlen == exp_rlen);
            TEST_MSG("%i != %i", rlen, exp_rlen);

            TEST_ASSERT(ret[5] == 1);
            TEST_MSG("led not enabled: %i != %i", ret[5], 1);

            unsigned short period = BYTE_TO_WORD(ret[6], ret[7]);
            TEST_ASSERT(period == exp_period);
            TEST_MSG("invalid period: %i != %i", period, exp_period);

            unsigned short width = BYTE_TO_WORD(ret[8], ret[9]);
            TEST_ASSERT(width == exp_width);
            TEST_MSG("invalid width: %i != %i", width, exp_width);
        }

        TEST_CASE("Disable LED");
        {
            byte_t wcmd[11] = {RS232_ESCAPE, 0xC0, 0x06, 0x00, 0x00, 0x00,
                               0x64,         0x00, 0x03, 0x00, 0xA1};
            retval = xia_usb2_write(h, XIA_UART_ADDRESS, 11, wcmd);
            TEST_ASSERT(retval == XIA_USB2_SUCCESS);
            TEST_MSG("device %i write failed with %i", 0, retval);

            retval = xia_usb2_read(h, XIA_UART_ADDRESS, 11, ret);
            TEST_ASSERT(retval == XIA_USB2_SUCCESS);
            TEST_MSG("device %i read failed with %i", 0, retval);

            TEST_ASSERT(ret[0] == RS232_ESCAPE);
            TEST_MSG("%i != %i", ret[0], RS232_ESCAPE);

            TEST_ASSERT(ret[RETVAL_SUCCESS_POS] == RETVAL_SUCCESS);
            TEST_MSG("%i != %i", ret[RETVAL_SUCCESS_POS], RETVAL_SUCCESS);

            unsigned short exp_rlen = 6;
            unsigned short rlen =
                BYTE_TO_WORD(ret[RETVAL_LEN_LO_POS], ret[RETVAL_LEN_HI_POS]);
            TEST_ASSERT(rlen == exp_rlen);
            TEST_MSG("%i != %i", rlen, exp_rlen);

            TEST_ASSERT(ret[5] == 0);
            TEST_MSG("led not enabled: %i != %i", ret[5], 1);

            unsigned short period = BYTE_TO_WORD(ret[6], ret[7]);
            TEST_ASSERT(period == exp_period);
            TEST_MSG("invalid period: %i != %i", period, exp_period);

            unsigned short width = BYTE_TO_WORD(ret[8], ret[9]);
            TEST_ASSERT(width == exp_width);
            TEST_MSG("invalid width: %i != %i", width, exp_width);
        }

        retval = xia_usb2_close(h);
        TEST_ASSERT(retval == XIA_USB2_SUCCESS);
        TEST_MSG("device %i close failed with %i", 0, retval);

        mod_num++;
    }
    printf("\n");
}

//
// static boolean_t send_receive(int cmdlen, byte_t* cmd, int retlen, byte_t* receive) {
//     int status;
//     int i;
//     int receive_len;
//
//     byte_t retChksm = 0x00;
//     byte_t calcChksm = 0x00;
//
//     status = xia_usb2_write(usb_handle, XIA_UART_ADDRESS, cmdlen, cmd);
//     xia_error_check(status);
//
//     status = xia_usb2_read(usb_handle, XIA_UART_ADDRESS, retlen, receive);
//     xia_error_check(status);
//
//     receive_len = (receive[2] | ((unsigned int) (receive[3]) << 8)) + 5;
//     if (receive_len != retlen) {
//         printf("Response size mismatch actual %d != expected %d...\n", receive_len,
//                retlen);
//
//         for (i = 0; i < cmdlen; i++) {
//             printf("cmd[%d] = %#x\n", i, cmd[i]);
//         }
//
//         for (i = 0; i < retlen; i++) {
//             printf("receive[%d] = %#x\n", i, receive[i]);
//         }
//
//         return FALSE_;
//     }
//
//     retChksm = receive[retlen - 1];
//     calcChksm = xia_compute_chksum((unsigned short) (retlen - 2), receive + 1);
//
//     if (retChksm != calcChksm) {
//         printf("Checksum mismatch actual %#x != expected %#x... \n", retChksm,
//                calcChksm);
//         return FALSE_;
//     }
//
//     return TRUE_;
// }
//
// int temp(int argc, char* argv[]) {
//     if (argc < 3) {
//         fprintf(stdout, "\n");
//         fprintf(stdout, "usage: test_mdxp_usb2 <ini file> <test name>\n");
//         fprintf(stdout, "\n");
//         exit(1);
//     }
//
//     int status;
//     int port;
//     int cmdlen;
//     boolean_t success;
//
//     byte_t receive[64] = {0};
//
//     byte_t cmd_get_board_info[5] = {0x1B, 0x49, 0x00, 0x00, 0x49};
//     byte_t cmd_read_dsp_param[7] = {0x1B, 0x42, 0x02, 0x00, 0x01, 0x00, 0x41};
//
//     port = atoi(argv[1]);
//     const unsigned int delay = (unsigned int) atoi(argv[2]);
//
//     signal(SIGINT, INThandler);
//
//     printf("Connecting to microDXP at port %d... \n", port);
//
//     status = xia_usb2_open(port, &usb_handle);
//     xia_error_check(status);
//
//     printf("SUCCESS.\n");
//     printf("Doing USB test. Press CTRL+C to stop.\n");
//
//     while (!stop) {
//         printf(".");
//         xia_sleep(delay);
//
//         cmdlen = sizeof(cmd_read_dsp_param);
//         success =
//             send_receive(cmdlen, cmd_read_dsp_param, READ_DSP_PARAM_RETLEN, receive);
//
//         if (stop || !success)
//             break;
//
//         printf(".");
//         xia_sleep(delay);
//
//         cmdlen = sizeof(cmd_get_board_info);
//         success =
//             send_receive(cmdlen, cmd_get_board_info, GET_BOARD_INFO_RETLEN, receive);
//
//         if (stop || !success)
//             break;
//     }
//
//     xia_usb2_close(usb_handle);
//     xia_cleanup(0);
//     return EXIT_SUCCESS;
// }

TEST_LIST = {
    {"serial_number", serial_number},
    {"led", led},
    {"monitor_dac", monitor_dac},
    {NULL, NULL} /* zeroed record marking the end of the list */
};