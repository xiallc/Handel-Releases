/*
 * Copyright (c) 2012 XIA LLC
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

#ifndef MD_SHIM_H
#define MD_SHIM_H

#include <stdarg.h>
#include <stdio.h>

#include "xia_mddef.h"
#include "md_generic.h"

XIA_MD_SHARED int dxp_md_enable_log(void);
XIA_MD_SHARED int dxp_md_suppress_log(void);
XIA_MD_SHARED int dxp_md_set_log_level(int level);
XIA_MD_SHARED void dxp_md_log(int level, const char* routine, const char* message,
                              int error, const char* file, int line);
XIA_MD_SHARED void dxp_md_output(const char* filename);

XIA_MD_SHARED void dxp_md_error(const char* routine, const char* message,
                                int* error_code, const char* file, int line);
XIA_MD_SHARED void dxp_md_warning(const char* routine, const char* message,
                                  const char* file, int line);
XIA_MD_SHARED void dxp_md_info(const char* routine, const char* message,
                               const char* file, int line);
XIA_MD_SHARED void dxp_md_debug(const char* routine, const char* message,
                                const char* file, int line);

/* Generic logging macros for use outside of context of XERXES */
#define dxp_md_log_error(x, y, z)                                                      \
    dxp_md_log(MD_ERROR, (x), (y), (z), __FILE__, __LINE__)
#define dxp_md_log_warning(x, y) dxp_md_log(MD_WARNING, (x), (y), 0, __FILE__, __LINE__)
#define dxp_md_log_info(x, y) dxp_md_log(MD_INFO, (x), (y), 0, __FILE__, __LINE__)
#define dxp_md_log_debug(x, y) dxp_md_log(MD_DEBUG, (x), (y), 0, __FILE__, __LINE__)

#endif /* MD_SHIM_H */
