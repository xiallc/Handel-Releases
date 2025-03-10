/*
 * A generic test with memory leak and uninitialized read access to
 * check that memory leak detection is working
 *
 * Copyright (c) 2005-2015 XIA LLC
 * All rights reserved
 *
 */
#ifdef __VLD_MEM_DBG__
#include <vld.h>
#endif

#include <stdio.h>
#include <stdlib.h>

static void check_leak_detection_works();
static void check_uninitialized_read_detection_works();
static void check_access_beyond_bound_works();

int main() {
    check_leak_detection_works();
    check_access_beyond_bound_works();
    check_uninitialized_read_detection_works();

    return 0;
}

static void check_leak_detection_works() {
    /* Create an intentional leak to make sure 
     * the leak detection is working */
    unsigned long* leak = NULL;
    leak = malloc(123);
    printf("Check leak detection %lu.\r\n", leak[0]);
}

static void check_uninitialized_read_detection_works() {
    /* Create an intentional uninitialized read */
    unsigned long* uninitialized = NULL;
    unsigned long access;
    uninitialized = malloc(2 * sizeof(unsigned long));
    uninitialized[1] = 2;

    access = uninitialized[0];

    printf("Check uninitilized read detection %lu\r\n", access);
    free(uninitialized);
}

static void check_access_beyond_bound_works() {
    /* Create an intentional read beyong allocated bound */
    unsigned long* uninitialized = NULL;
    unsigned long access;
    uninitialized = malloc(2 * sizeof(unsigned long));

    access = uninitialized[3];

    printf("Check uninitilized read detection %lu\r\n", access);
    free(uninitialized);
}