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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#if _WIN32
#pragma warning(disable : 4115)

/* For Sleep() */
#include <windows.h>
#else
#include <time.h>
#endif

#include "handel_errors.h"
#include "xia_usb2.h"

static void CHECK_ERROR(int status);
static int SLEEP(float seconds);
static void INThandler(int sig);
static void print_usage(char* name);
static void clean_up();

static boolean_t send_receive(int cmdlen, byte_t* cmd, int retlen, byte_t* receive);
static byte_t dxp_compute_chksum(unsigned int len, byte_t* data);

boolean_t stop = FALSE_;
HANDLE usb_handle = 0;

const int UART_ADDRESS = 0x01000000;

const int GET_BOARD_INFO_RETLEN = 26;
const int READ_DSP_PARAM_RETLEN = 10;

int main(int argc, char* argv[]) {
    int status;
    int port;
    int cmdlen;
    boolean_t success;
    float delay;

    byte_t receive[64] = {0};

    byte_t cmd_get_board_info[5] = {0x1B, 0x49, 0x00, 0x00, 0x49};
    byte_t cmd_read_dsp_param[7] = {0x1B, 0x42, 0x02, 0x00, 0x01, 0x00, 0x41};

    if (argc < 3) {
        print_usage(argv[0]);
        exit(1);
    }

    port = atoi(argv[1]);
    delay = (float) atof(argv[2]);

    signal(SIGINT, INThandler);

    printf("Connecting to microDXP at port %d... \n", port);

    status = xia_usb2_open(port, &usb_handle);
    CHECK_ERROR(status);

    printf("SUCCESS.\n");
    printf("Doing USB test. Press CTRL+C to stop.\n");

    while (!stop) {
        printf(".");
        SLEEP(delay);

        cmdlen = sizeof(cmd_read_dsp_param);
        success =
            send_receive(cmdlen, cmd_read_dsp_param, READ_DSP_PARAM_RETLEN, receive);

        if (stop || !success)
            break;

        printf(".");
        SLEEP(delay);

        cmdlen = sizeof(cmd_get_board_info);
        success =
            send_receive(cmdlen, cmd_get_board_info, GET_BOARD_INFO_RETLEN, receive);

        if (stop || !success)
            break;
    }

    clean_up();
    return 0;
}

static void clean_up() {
    if (usb_handle) {
        printf("\nClosing USB connection.\n");
        xia_usb2_close(usb_handle);
        usb_handle = 0;
    }
}

/*
 * This is just an example of how to handle error values.  A program
 * of any reasonable size should implement a more robust error
 * handling mechanism.
 */
static void CHECK_ERROR(int status) {
    /* XIA_SUCCESS is defined in handel_errors.h */
    if (status != XIA_SUCCESS) {
        printf("Error encountered! Status = %d\n", status);
        clean_up();
        exit(status);
    }
}

static void INThandler(int sig) {
    UNUSED(sig);
    stop = TRUE_;
}

static void print_usage(char* name) {
    fprintf(stdout, "\n");
    fprintf(stdout, "* argument: [PORT], [SLEEP_SECONDS]\n");
    fprintf(stdout, "* example usage: %s 0 0.02\n", name);
    fprintf(stdout, "\n");
    return;
}

static int SLEEP(float seconds) {
#if _WIN32
    DWORD wait = (DWORD) (1000.0 * (seconds));
    Sleep(wait);
#else
    unsigned long secs = (unsigned long) seconds;
    struct timespec req = {.tv_sec = secs,
                           .tv_nsec = ((seconds - secs) * 1000000000.0)};
    struct timespec rem = {.tv_sec = 0, .tv_nsec = 0};
    while (TRUE_) {
        if (nanosleep(&req, &rem) == 0)
            break;
        req = rem;
    }
#endif
    return XIA_SUCCESS;
}

/*
 * Computes a standard XOR checksum on the byte array that
 * is passed in using the following relation:
 * chksum_(i + 1) = chksum_(i) XOR data[i]
 */
static byte_t dxp_compute_chksum(unsigned int len, byte_t* data) {
    unsigned int i;
    byte_t chkSum;

    for (i = 0, chkSum = 0x00; i < len; i++) {
        chkSum = (byte_t) (chkSum ^ data[i]);
    }

    return chkSum;
}

static boolean_t send_receive(int cmdlen, byte_t* cmd, int retlen, byte_t* receive) {
    int status;
    int i;
    int receive_len;

    byte_t retChksm = 0x00;
    byte_t calcChksm = 0x00;

    status = xia_usb2_write(usb_handle, UART_ADDRESS, cmdlen, cmd);
    CHECK_ERROR(status);

    status = xia_usb2_read(usb_handle, UART_ADDRESS, retlen, receive);
    CHECK_ERROR(status);

    receive_len = (receive[2] | ((unsigned int) (receive[3]) << 8)) + 5;
    if (receive_len != retlen) {
        printf("Response size mismatch actual %d != expected %d...\n", receive_len,
               retlen);

        for (i = 0; i < cmdlen; i++) {
            printf("cmd[%d] = %#x\n", i, cmd[i]);
        }

        for (i = 0; i < retlen; i++) {
            printf("receive[%d] = %#x\n", i, receive[i]);
        }

        return FALSE_;
    }

    retChksm = receive[retlen - 1];
    calcChksm = dxp_compute_chksum((unsigned short) (retlen - 2), receive + 1);

    if (retChksm != calcChksm) {
        printf("Checksum mismatch actual %#x != expected %#x... \n", retChksm,
               calcChksm);
        return FALSE_;
    }

    return TRUE_;
}
