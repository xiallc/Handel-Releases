/*
 * Copyright (c) 2006-2016 XIA LLC
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

#ifndef __XIA_FILE_H__
#define __XIA_FILE_H__

#include <stdio.h>

#include "Dlldefs.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

XIA_SHARED FILE* xia_fopen(const char* name, const char* mode, char* file, int line);
XIA_SHARED int xia_fclose(FILE* fp);
XIA_SHARED int xia_num_open_handles(void);
XIA_SHARED void xia_print_open_handles(FILE* stream);
XIA_SHARED void xia_print_open_handles_stdout(void);
XIA_SHARED FILE* xia_find_file(const char* name, const char* mode);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/* Common macros */
#define xia_file_open(name, mode) xia_fopen(name, mode, __FILE__, __LINE__)
#define xia_file_close(fp) xia_fclose(fp)

#endif /* __XIA_FILE_H__ */
