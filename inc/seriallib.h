/*
 * Copyright (c) 2004 X-ray Instrumentation Associates
 *               2005-2016 XIA LLC
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

#ifndef _SERIALLIB_H_
#define _SERIALLIB_H_

/* Error Codes */
enum {
    SERIAL_SUCCESS = 0,
    SERIAL_CREATE_FILE,
    SERIAL_COMM_STATE,
    SERIAL_COMM_TIMEOUTS,
    SERIAL_READ_FILE,
    SERIAL_READ_SIZE,
    SERIAL_WRITE_FILE,
    SERIAL_WRITE_SIZE,
    SERIAL_CLEAR_COMM,
    SERIAL_CLOSE_HANDLE,
    SERIAL_PORT_NOINIT,
    SERIAL_INIT,
    SERIAL_PORT_SETUP,
    SERIAL_UNINIT,
    SERIAL_WRITE,
    SERIAL_READ
};

/* Quasi-boolean values */
#define TRUE_ (1 == 1)
#define FALSE_ (1 == 0)

#define UNUSED(x) ((x) = (x))

/*
 * Special read error structure to use in debugging this persistent
 * problem where bytes are dropped over long reads
 */
typedef struct _serial_read_error {
    int status;
    int actual;
    int expected;
    int bytes_in_recv_buf;
    int size_recv_buf;
    int is_hardware_overrun;
    int is_software_overrun;
} serial_read_error_t;

#endif /* _SERIALLIB_H_ */
