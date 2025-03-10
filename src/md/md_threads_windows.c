/*
 * Copyright (c) 2015 XIA LLC
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <windows.h>

#include "xia_assert.h"
#include "xia_common.h"

#include "handel_errors.h"
#include "handeldef.h"

#include "md_generic.h"

#include "md_threads.h"

XIA_SHARED int handel_md_thread_create(handel_md_Thread* thread) {
    int r = ERROR_INVALID_HANDLE;
    if (thread->handle == NULL) {
        r = ERROR_BUSY;

        /*
         * If the state is detached create the thread
         */
        if (thread->state == handel_md_ThreadsDetached) {
            HANDLE h = CreateThread(0, thread->stackSize,
                                    (LPTHREAD_START_ROUTINE) thread->entryPoint,
                                    thread->argument, 0, 0);
            if (h) {
                thread->handle = h;
                r = NO_ERROR;
            } else {
                r = GetLastError();
            }
        }
    }
    return r;
}

XIA_SHARED int handel_md_thread_destroy(handel_md_Thread* thread) {
    int r = NO_ERROR;
    if (thread->handle != NULL) {
        if ((thread->state == handel_md_ThreadsReady) ||
            (thread->state == handel_md_ThreadsActive)) {
            HANDLE h = (HANDLE) thread->handle;
            thread->handle = NULL;
            thread->state = handel_md_ThreadsDetached;
            if (GetCurrentThread() == h)
                ExitThread(0);
            TerminateThread(h, 0);
        }
    }
    return r;
}

XIA_SHARED int handel_md_thread_self(handel_md_Thread* thread) {
    int r = 0;
    if (thread->handle != NULL) {
        HANDLE h = (HANDLE) thread->handle;
        r = GetCurrentThread() == h;
    }
    return r;
}

XIA_SHARED void handel_md_thread_sleep(unsigned int msecs) {
    Sleep(msecs);
}

XIA_SHARED int handel_md_thread_ready(handel_md_Thread* thread) {
    return thread->handle != NULL;
}

XIA_SHARED int handel_md_mutex_create(handel_md_Mutex* mutex) {
    int r = ERROR_INVALID_HANDLE;
    if (mutex->handle == NULL) {
        CRITICAL_SECTION* cs = malloc(sizeof(CRITICAL_SECTION));
        InitializeCriticalSection(cs);
        mutex->handle = cs;
        r = NO_ERROR;
    }
    return r;
}

XIA_SHARED int handel_md_mutex_destroy(handel_md_Mutex* mutex) {
    int r = ERROR_INVALID_HANDLE;
    if (mutex->handle != NULL) {
        CRITICAL_SECTION* cs = (CRITICAL_SECTION*) mutex->handle;
        DeleteCriticalSection(cs);
        free(cs);
        mutex->handle = NULL;
        r = NO_ERROR;
    }
    return r;
}

XIA_SHARED int handel_md_mutex_lock(handel_md_Mutex* mutex) {
    int r = ERROR_INVALID_HANDLE;
    if (mutex->handle != NULL) {
        CRITICAL_SECTION* cs = (CRITICAL_SECTION*) mutex->handle;
        EnterCriticalSection(cs);
        r = NO_ERROR;
    }
    return r;
}

XIA_SHARED int handel_md_mutex_unlock(handel_md_Mutex* mutex) {
    int r = ERROR_INVALID_HANDLE;
    if (mutex->handle != NULL) {
        CRITICAL_SECTION* cs = (CRITICAL_SECTION*) mutex->handle;
        LeaveCriticalSection(cs);
        r = NO_ERROR;
    }
    return r;
}

XIA_SHARED int handel_md_mutex_trylock(handel_md_Mutex* mutex) {
    int r = ERROR_INVALID_HANDLE;
    if (mutex->handle != NULL) {
        CRITICAL_SECTION* cs = (CRITICAL_SECTION*) mutex->handle;
        r = TryEnterCriticalSection(cs) ? NO_ERROR : ERROR_BUSY;
    }
    return r;
}

XIA_SHARED int handel_md_mutex_ready(handel_md_Mutex* mutex) {
    return mutex->handle != NULL;
}

XIA_SHARED int handel_md_event_create(handel_md_Event* event) {
    int r = ERROR_INVALID_HANDLE;
    if (event->handle == NULL) {
        OSVERSIONINFOEX osvi;
        DWORDLONG mask = 0;
        BYTE op = VER_GREATER_EQUAL;
        BOOL isXPorLater;

        ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
        osvi.dwMajorVersion = 5;
        osvi.dwMinorVersion = 1;

        VER_SET_CONDITION(mask, VER_MAJORVERSION, op);
        VER_SET_CONDITION(mask, VER_MINORVERSION, op);

        isXPorLater =
            VerifyVersionInfo(&osvi, VER_MAJORVERSION | VER_MINORVERSION, mask);

        if (isXPorLater) {
            HANDLE h = CreateEvent(0, FALSE, FALSE, 0);
            if (h) {
                event->handle = h;
                r = NO_ERROR;
            } else {
                r = GetLastError();
            }
        } else {
            r = ERROR_OLD_WIN_VERSION;
        }
    }
    return r;
}

XIA_SHARED int handel_md_event_destroy(handel_md_Event* event) {
    int r = ERROR_INVALID_HANDLE;
    if (event->handle != NULL) {
        HANDLE h = (HANDLE) event->handle;
        r = CloseHandle(h) ? NO_ERROR : GetLastError();
        event->handle = 0;
    }
    return r;
}

XIA_SHARED int handel_md_event_wait(handel_md_Event* event, unsigned int timeout) {
    int r = ERROR_INVALID_HANDLE;
    if (event->handle != NULL) {
        HANDLE h = (HANDLE) event->handle;
        DWORD t = timeout ? timeout : INFINITE;
        DWORD w = WaitForSingleObject(h, t);
        if (w == WAIT_OBJECT_0)
            r = NO_ERROR;
        else if (w == WAIT_TIMEOUT)
            r = THREADING_TIMEOUT;
        else
            r = GetLastError();
    }
    return r;
}

XIA_SHARED int handel_md_event_signal(handel_md_Event* event) {
    int r = ERROR_INVALID_HANDLE;
    if (event->handle != NULL) {
        HANDLE h = (HANDLE) event->handle;
        r = SetEvent(h) ? NO_ERROR : GetLastError();
    }
    return r;
}

XIA_SHARED int handel_md_event_ready(handel_md_Event* event) {
    return event->handle != NULL;
}
