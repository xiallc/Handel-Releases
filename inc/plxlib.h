/*
 * The PLX API (and all code from the SDK) is
 * Copyright (c) 2003 PLX Technology Inc
 *
 * Copyright (c) 2004 X-ray Instrumentation Associates
 *               2005-2017 XIA LLC
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

#ifndef __PLXLIB_H__
#define __PLXLIB_H__

#include "Dlldefs.h"
#include "xia_common.h"

#ifdef _WIN32
#pragma warning(disable : 4214)
#pragma warning(disable : 4201)
#endif /* _WIN32 */

#include "PlxApi.h"

#ifdef _WIN32
#pragma warning(default : 4201)
#pragma warning(default : 4214)
#endif /* _WIN32 */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Public API
 */
XIA_EXPORT void XIA_API plx_print_error(int err, char* errorstring);
XIA_EXPORT int XIA_API plx_open_slot(unsigned short id, byte_t bus, byte_t slot,
                                     HANDLE* h);
XIA_EXPORT int XIA_API plx_close_slot(HANDLE h);
XIA_EXPORT int XIA_API plx_read_long(HANDLE h, unsigned long addr, unsigned long* data);
XIA_EXPORT int XIA_API plx_write_long(HANDLE h, unsigned long addr, unsigned long data);
XIA_EXPORT int XIA_API plx_read_block(HANDLE h, unsigned long addr, unsigned long len,
                                      unsigned long n_dead, unsigned long* data);

#ifdef PLXLIB_DEBUG
XIA_EXPORT void XIA_API plx_set_file_DEBUG(char* f);
XIA_EXPORT void XIA_API plx_dump_vmap_DEBUG(void);
#endif

#ifdef __cplusplus
}
#endif

/*
 * Structs
 */
typedef struct _virtual_map {
    PLX_UINT_PTR* addr;
    PLX_DEVICE_OBJECT* device;
    PLX_NOTIFY_OBJECT* events;
    PLX_INTERRUPT* intrs;
    boolean_t* registered;
    unsigned long n;
} virtual_map_t;

typedef struct _API_ERRORS {
    PLX_STATUS code;
    char* text;
} API_ERRORS;

typedef struct _XIA_PLX_ERRORS {
    int code;
    char* text;
} XIA_PLX_ERRORS;

/*
 * Constants
 */

#define PLX_PCI_SPACE_0 2
#define EXTERNAL_MEMORY_LOCAL_ADDR 0x100000

#endif /* __PLXLIB_H__ */
