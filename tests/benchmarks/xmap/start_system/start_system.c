#include <stdio.h>
#include <stdlib.h>

#include "handel.h"
#include "handel_errors.h"

#include "xia_assert.h"

int main(int argc, char* argv[]) {
    int status;
    int n_iters;
    int i;

    if (argc != 2) {
        fprintf(stderr, "The first argument should be the number of times to "
                        "start the system.\n");
        exit(1);
    }

    sscanf(argv[1], "%d", &n_iters);

    for (i = 0; i < n_iters; i++) {
        status = xiaInit("xmap.ini");
        ASSERT(status == XIA_SUCCESS);

        status = xiaStartSystem();
        ASSERT(status == XIA_SUCCESS);

        status = xiaExit();
        ASSERT(status == XIA_SUCCESS);
    }

    exit(0);
}
