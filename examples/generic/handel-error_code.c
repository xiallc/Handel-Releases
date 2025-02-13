/*
 * Example code to demonstrate translating error codes returned by Handel to text
 *
 * Supported devices are xMap, Saturn, STJ, Mercury / Mercury4, microDXP
 *
 * Copyright (c) 2008-2020, XIA LLC
 * All rights reserved
 *
 */

#include <stdio.h>

/* For Sleep() */
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

#include "handel.h"
#include "handel_errors.h"
#include "xerxes_errors.h"

int main() {
    printf("--- Checking Handel error code translationg.\n");

    printf("DXP_MEMORY_LENGTH       -- %s\n", xiaGetErrorText(DXP_MEMORY_LENGTH));
    printf("DXP_MALFORMED_FILE      -- %s\n", xiaGetErrorText(DXP_MALFORMED_FILE));
    printf("DXP_SUCCESS             -- %s\n", xiaGetErrorText(DXP_SUCCESS));

    printf("XIA_SIZE_MISMATCH       -- %s\n", xiaGetErrorText(XIA_SIZE_MISMATCH));
    printf("XIA_PRESET_VALUE_OOR    -- %s\n", xiaGetErrorText(XIA_PRESET_VALUE_OOR));
    printf("XIA_LIVEOUTPUT_OOR      -- %s\n", xiaGetErrorText(XIA_LIVEOUTPUT_OOR));

    printf("Non-existing error     -- %s\n", xiaGetErrorText(2048));

    return 0;
}
