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

#ifndef XERXESDEF_H
#define XERXESDEF_H

/* Have we defined the EXPORT and IMPORT macros? */
#ifndef XERXES_PORTING_DEFINED
#define XERXES_PORTING_DEFINED

#ifdef XERXES_USE_DLL /* Linking to a DLL libraries */

#ifdef _WIN32

#ifdef XERXES_MAKE_DLL
#define XERXES_EXPORT __declspec(dllexport)
#define XERXES_IMPORT __declspec(dllimport)

#ifndef WIN32_XERXES_VBA /* Libraries for Visual Basic require STDCALL */
#define XERXES_API
#else
#define XERXES_API _stdcall
#endif /* Endif for WIN32_VBA */

#else /* Then we are making a static link library */
#define XERXES_EXPORT
#define XERXES_IMPORT __declspec(dllimport)

#ifndef WIN32_XERXES_VBA /* Libraries for Visual Basic require STDCALL */
#define XERXES_API
#else
#define XERXES_API _stdcall
#endif /* Endif for WIN32_VBA */

#endif /* Endif for XERXES_MAKE_DLL */

#else

#ifdef XERXES_MAKE_DLL
#define XERXES_EXPORT
#define XERXES_IMPORT extern
#define XERXES_API
#else /* Then we are making a static link library */
#define XERXES_EXPORT
#define XERXES_IMPORT extern
#define XERXES_API
#endif /* Endif for XERXES_MAKE_DLL */

#endif /* Endif for _WIN32 */

#else /* We are using static libraries */

#ifdef _WIN32

#ifdef XERXES_MAKE_DLL
#define XERXES_EXPORT __declspec(dllexport)
#define XERXES_IMPORT extern
#define XERXES_API
#else /* Then we are making a static link library */
#define XERXES_EXPORT
#define XERXES_IMPORT extern
#define XERXES_API
#endif /* Endif for XERXES_MAKE_DLL */

#else

#ifdef XERXES_MAKE_DLL
#define XERXES_EXPORT
#define XERXES_IMPORT extern
#define XERXES_API
#else /* Then we are making a static link library */
#define XERXES_EXPORT
#define XERXES_IMPORT extern
#define XERXES_API
#endif /* Endif for XERXES_MAKE_DLL */

#endif /* Endif for _WIN32 */

#endif /* Endif for XERXES_USE_DLL */

#endif /* Endif for XERXES_DXP_DLL_DEFINED */

#ifndef _XERXES_SWITCH_
#define _XERXES_SWITCH_ 1

#ifdef __STDC__
#define _XERXES_PROTO_ 1
#endif /* end of __STDC__    */

#ifdef _MSC_VER
#ifndef _XERXES_PROTO_
#define _XERXES_PROTO_ 1
#endif
#endif /* end of _MSC_VER    */

#endif /* end of _XERXES_SWITCH_*/

#endif /* Endif for XERXESDEF_H */
