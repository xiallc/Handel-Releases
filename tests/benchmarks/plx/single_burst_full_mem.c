/*
 * Performs a single burst of the entire external memory.
 *
 * $Id$
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "xia_common.h"

#include "plxlib.h"

#define MEMORY_BLOCK_MAX 1048576UL

static void check_status(int status, HANDLE h);

int main(int argc, char* argv[]) {
    int status;

    unsigned long i;

    unsigned long* data = NULL;

    byte_t bus;
    byte_t slot;

    HANDLE h;

    sscanf(argv[1], "%uc", &bus);
    sscanf(argv[2], "%uc", &slot);

    status = plx_open_slot((unsigned short) -1, bus, slot, &h);
    check_status(status, h);

    for (i = 1; i < MEMORY_BLOCK_MAX; i += 10000) {
        data = (unsigned long*) malloc(i * sizeof(unsigned long));
        assert(data != NULL);

        status = plx_read_block(h, 0x3000000, i, 2, data);
        free(data);
        check_status(status, h);

        printf("Transferred %lu 32-bit word(s).\n", i);
    }

    status = plx_close_slot(h);
    check_status(status, h);

    return 0;
}

/*
 * Basic error handling.
 */
static void check_status(int status, HANDLE h) {
    int ignored;

    if (status != 0) {
        printf("Status = %d, exiting...\n", status);
        ignored = plx_close_slot(h);
        exit(status);
    }
}
