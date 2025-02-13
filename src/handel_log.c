/*
 * Copyright (c) 2002-2004 X-ray Instrumentation Associates
 *               2005-2011 XIA LLC
 * All rights reserved
 *
 * Redistribution and use in source and binary forms,
 * with or without modification, are permitted provided
 * that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above
 *     copyright notice, this list of conditions and the
 *     following disclaimer.
 *   * Redistributions in binary form must reproduce the
 *     above copyright notice, this list of conditions and the
 *     following disclaimer in the documentation and/or other
 *     materials provided with the distribution.
 *   * Neither the name of XIA LLC
 *     nor the names of its contributors may be used to endorse
 *     or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdarg.h>
#include <stdio.h>

#include "handel_generic.h"
#include "handel_log.h"
#include "handeldef.h"
#include "xia_handel.h"
#include "handel_errors.h"

/**
 * Format the log messages into this buffer.
 */
static char formatBuffer[2048];

/**
 * This routine enables the logging output
 */
HANDEL_EXPORT int HANDEL_API xiaEnableLogOutput(void) {
    int status;

    if (handel_md_enable_log == NULL) {
        status = xiaInitHandel();

        if (status != XIA_SUCCESS)
            return status;
    }

    status = handel_md_enable_log();
    return status;
}

/**
 * This routine disables the logging output
 */
HANDEL_EXPORT int HANDEL_API xiaSuppressLogOutput(void) {
    int status;

    if (handel_md_suppress_log == NULL) {
        status = xiaInitHandel();

        if (status != XIA_SUCCESS)
            return status;
    }

    status = handel_md_suppress_log();
    return status;
}

/**
 * This routine sets the maximum level at which log messages will be
 * displayed.
 *
 * int level;                            Input: Level to set the logging to
 */
HANDEL_EXPORT int HANDEL_API xiaSetLogLevel(int level) {
    int status;

    if (handel_md_set_log_level == NULL) {
        status = xiaInitHandel();

        if (status != XIA_SUCCESS)
            return status;
    }

    status = handel_md_set_log_level(level);
    return status;
}

/**
 * This routine sets the output stream for the logging routines. By default,
 * the output is sent to stdout.
 *
 * char *filename;                    Input: name of file to redirect reporting
 */
HANDEL_EXPORT int HANDEL_API xiaSetLogOutput(char* filename) {
    int status;
    if (handel_md_output == NULL) {
        status = xiaInitHandel();

        if (status != XIA_SUCCESS)
            return status;
    }

    handel_md_output(filename);
    return XIA_SUCCESS;
}

/**
 * This routine outputs the log.
 * If it's called before Handel is properly initialized do nothing
 */
HANDEL_SHARED void HANDEL_API xiaLog(int level, const char* file, int line, int status,
                                     const char* func, const char* fmt, ...) {
    va_list args;

    va_start(args, fmt);

    /*
     * Cannot use vsnprintf on MinGW currently because is generates
     * an unresolved external.
     */
    vsprintf(formatBuffer, fmt, args);

    formatBuffer[sizeof(formatBuffer) - 1] = '\0';

    if (handel_md_log != NULL)
        handel_md_log(level, (char*) func, formatBuffer, status, (char*) file, line);

    va_end(args);
}

/**
 * This routine closes the logging stream
 */
HANDEL_EXPORT int HANDEL_API xiaCloseLog(void) {
    int status;
    if (handel_md_output == NULL) {
        status = xiaInitHandel();

        if (status != XIA_SUCCESS)
            return status;
    }

    handel_md_output(NULL);
    return XIA_SUCCESS;
}
