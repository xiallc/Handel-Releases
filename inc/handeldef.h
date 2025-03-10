/*
 * Copyright (c) 2004 X-ray Instrumentation Associates
 *               2005-2013 XIA LLC
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

#ifndef HANDELDEF_H
#define HANDELDEF_H

#if defined(_WIN64) || defined(_WIN32)
#define _USE_MATH_DEFINES
#define XIA_HANDEL_WINDOWS

#ifndef NOMINMAX
#define NOMINMAX
#endif

#define HANDEL_EXPORT __declspec(dllexport)
#define HANDEL_IMPORT __declspec(dllimport)
#define HANDEL_API _stdcall

#define HANDLE_PATHNAME_SEP '\\'

#else
#define HANDEL_EXPORT
#define HANDEL_IMPORT extern
#define HANDEL_API

#define HANDLE_PATHNAME_SEP '/'
#endif

#if defined(__STDC__) || defined(_MSC_VER)
#define _HANDEL_PROTO_ 1
#endif


#if __GNUC__
#define HANDEL_PRINTF(_s, _f) __attribute__((format(printf, _s, _f)))
#else
#define HANDEL_PRINTF(_s, _f)
#endif

#endif /* Endif for HANDELDEF_H */
