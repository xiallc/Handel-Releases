/*
 * Copyright (c) 2004 X-ray Instrumentation Associates
 *               2005-2012 XIA LLC
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

#ifndef __USBLIB_H__
#define __USBLIB_H__

#ifdef _WIN32
#include <windows.h>
#endif /* _WIN32 */

#include "xia_common.h"

#include "Dlldefs.h"

#define CTRL_SIZE 5

#ifdef __linux__
#define IN2 2
#define OUT1 1
#define OUT2 2
#define OUT4 4
#else /* __linux__ */
#define IN2 8
#define OUT1 0
#define OUT2 1
#define OUT4 3
#endif /* __linux__ */

/* Few definitions shamelessly copied from ezusbsys.h provided by Cypress */

#define Ezusb_IOCTL_INDEX 0x0800

typedef struct _BULK_TRANSFER_CONTROL {
    ULONG pipeNum;
} BULK_TRANSFER_CONTROL, *PBULK_TRANSFER_CONTROL;

/*
 * Perform an IN transfer over the specified bulk or interrupt pipe.
 *
 * lpInBuffer: BULK_TRANSFER_CONTROL stucture specifying the pipe number to read from
 * nInBufferSize: sizeof(BULK_TRANSFER_CONTROL)
 * lpOutBuffer: Buffer to hold data read from the device.
 * nOutputBufferSize: size of lpOutBuffer.  This parameter determines
 *    the size of the USB transfer.
 * lpBytesReturned: actual number of bytes read
 */
#define IOCTL_EZUSB_BULK_READ                                                          \
    CTL_CODE(FILE_DEVICE_UNKNOWN, Ezusb_IOCTL_INDEX + 19, METHOD_OUT_DIRECT,           \
             FILE_ANY_ACCESS)

/*
 * Perform an OUT transfer over the specified bulk or interrupt pipe.
 *
 * lpInBuffer: BULK_TRANSFER_CONTROL stucture specifying the pipe number to write to
 * nInBufferSize: sizeof(BULK_TRANSFER_CONTROL)
 * lpOutBuffer: Buffer of data to write to the device
 * nOutputBufferSize: size of lpOutBuffer.  This parameter determines
 *    the size of the USB transfer.
 * lpBytesReturned: actual number of bytes written
 */
#define IOCTL_EZUSB_BULK_WRITE                                                         \
    CTL_CODE(FILE_DEVICE_UNKNOWN, Ezusb_IOCTL_INDEX + 20, METHOD_IN_DIRECT,            \
             FILE_ANY_ACCESS)

/* Function Prototypes */
XIA_EXPORT int XIA_API xia_usb_close(HANDLE hDevice);
XIA_EXPORT int XIA_API xia_usb_open(char* device, HANDLE* hDevice);
XIA_EXPORT int XIA_API xia_usb_read(long address, long nWords, char* device,
                                    unsigned short* buffer);
XIA_EXPORT int XIA_API xia_usb_write(long address, long nWords, char* device,
                                     unsigned short* buffer);

#endif /* __USBLIB_H__ */
