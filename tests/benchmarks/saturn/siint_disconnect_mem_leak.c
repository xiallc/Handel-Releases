#include <stdio.h>

#include "windows.h"

#include "handel.h"
#include "handel_errors.h"

#include "xia_assert.h"
#include "xia_common.h"

#define CHECK(x) ((x) == XIA_SUCCESS) ? nop() : exit(1)

static void nop(void);

int main(int argc, char* argv[]) {
    int status;
    int i;

    status = xiaInit(argv[1]);
    CHECK(status);

    status = xiaStartSystem();

    for (i = 0; i < 5; i++) {
        status = xiaInit(argv[1]);
        CHECK(status);
        status = xiaStartSystem();

        Sleep(300);
    }

    xiaExit();

    return 0;
}

void nop(void) {
    return;
}
