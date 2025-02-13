/*
 * Pass through utility for microDxp
 *
 * Copyright (c) 2005-2017 XIA LLC
 * All rights reserved
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#if _WIN32

/* For Sleep() */
#include <windows.h>
#else
#include <sys/time.h>
#include <sys/resource.h>
#endif

#include "handel.h"
#include "handel_errors.h"
#include "md_generic.h"

static void CHECK_ERROR(int status);
static void print_usage(void);
static void start_system(char* ini_file);
static void setup_logging(char* log_name);
static void clean_up();
static void INThandler(int sig);

unsigned long* mca = NULL;
boolean_t stop = FALSE_;

int main(int argc, char* argv[]) {
    int status;
    int i;

    byte_t send[32] = {0x1B, 0xCC, 0x1B, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
                       0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x00, 0x01,
                       0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0xDC};

    int send_len = sizeof(send) / sizeof(send[0]);

    byte_t receive[32] = {0};
    int receive_len = sizeof(receive) / sizeof(receive[0]);

    void* value[4] = {send, &send_len, receive, &receive_len};

    if (argc < 2) {
        print_usage();
        exit(1);
    }

    signal(SIGINT, INThandler);

    setup_logging("handel.log");
    start_system(argv[1]);

    printf("Passthrough for microDxp, sending %d, receiving %d.\n", send_len,
           receive_len);
    printf("Send (0x)\n");
    for (i = 0; i < send_len; i++) {
        printf("%02X ", send[i]);
        if ((i + 1) % 16 == 0)
            printf("\n");
    }

    status = xiaBoardOperation(0, "passthrough", value);

    if (status == XIA_PASSTHROUGH)
        printf("ERROR Passthrough operation not supported\n");

    CHECK_ERROR(status);

    printf("Response (0x)\n");
    for (i = 0; i < receive_len; i++) {
        printf("%02X ", receive[i]);
        if ((i + 1) % 16 == 0)
            printf("\n");
    }
    printf("\n");

    clean_up();
    return 0;
}

static void INThandler(int sig) {
    UNUSED(sig);
    stop = TRUE_;
}

static void start_system(char* ini_file) {
    int status;

    printf("Loading the .ini file.\n");
    status = xiaInit(ini_file);
    CHECK_ERROR(status);

    /* Boot hardware */
    printf("Starting up the hardware.\n");
    status = xiaStartSystem();
    CHECK_ERROR(status);
}

static void setup_logging(char* log_name) {
    printf("Configuring the log file in %s.\n", log_name);
    xiaSetLogLevel(MD_DEBUG);
    xiaSetLogOutput(log_name);
}

static void clean_up() {
    printf("\nCleaning up Handel.\n");
    xiaExit();

    printf("Closing the Handel log file.\n");
    xiaCloseLog();

    if (mca)
        free(mca);
}

/*
 * This is just an example of how to handle error values.  A program
 * of any reasonable size should implement a more robust error
 * handling mechanism.
 */
static void CHECK_ERROR(int status) {
    /* XIA_SUCCESS is defined in handel_errors.h */
    if (status != XIA_SUCCESS) {
        printf("Error encountered! Status = %d, %s\n", status, xiaGetErrorText(status));
        clean_up();
        exit(status);
    }
}

static void print_usage(void) {
    fprintf(stdout, "\n");
    fprintf(stdout, "* argument: [.ini file]\n");
    fprintf(stdout, "\n");
    return;
}
