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

#ifndef MD_THREADS_H
#define MD_THREADS_H

#include <stdarg.h>
#include <stdio.h>

#include "Dlldefs.h"

/*
 * Generic handle type.
 */
typedef void* handel_md_ThreadsHandle;

/*
 * Error codes returned.
 */
#define THREADING_NO_ERROR 0
#define THREADING_BUSY 65550
#define THREADING_TIMEOUT 65551

/*
 * States a thread can be in.
 */
typedef enum {
    handel_md_ThreadsDetached, /* Not started. */
    handel_md_ThreadsReady, /* Started but not running. */
    handel_md_ThreadsActive /* Started */
} handel_md_ThreadsState;

/*
 * Thread entry point.
 */
typedef void (*handel_md_ThreadsEntry)(void* arg);

/*
 * Structure to control a thread.
 */
typedef struct {
    handel_md_ThreadsHandle handle;
    handel_md_ThreadsState state;
    const char* name;
    int priority;
    unsigned int stackSize;
    unsigned int attributes;
    boolean_t realtime;
    handel_md_ThreadsEntry entryPoint;
    void* argument;
} handel_md_Thread;

/*
 * Structure to control a mutex.
 */
typedef struct {
    handel_md_ThreadsHandle handle;
    const char* name;
} handel_md_Mutex;

/*
 * Structure to control a event.
 */
typedef struct {
    handel_md_ThreadsHandle handle;
    const char* name;
} handel_md_Event;

/*
 * Threading.
 */
XIA_SHARED int handel_md_thread_create(handel_md_Thread* thread);
XIA_SHARED int handel_md_thread_destroy(handel_md_Thread* thread);
XIA_SHARED int handel_md_thread_self(handel_md_Thread* thread);
XIA_SHARED void handel_md_thread_sleep(unsigned int msecs);
XIA_SHARED int handel_md_thread_ready(handel_md_Thread* thread);

/*
 * Locks.
 */
XIA_SHARED int handel_md_mutex_create(handel_md_Mutex* mutex);
XIA_SHARED int handel_md_mutex_destroy(handel_md_Mutex* mutex);
XIA_SHARED int handel_md_mutex_lock(handel_md_Mutex* mutex);
XIA_SHARED int handel_md_mutex_unlock(handel_md_Mutex* mutex);
XIA_SHARED int handel_md_mutex_trylock(handel_md_Mutex* mutex);
XIA_SHARED int handel_md_mutex_ready(handel_md_Mutex* mutex);

/*
 * Events
 */
XIA_SHARED int handel_md_event_create(handel_md_Event* event);
XIA_SHARED int handel_md_event_destroy(handel_md_Event* event);
XIA_SHARED int handel_md_event_wait(handel_md_Event* event, unsigned int timeout);
XIA_SHARED int handel_md_event_signal(handel_md_Event* event);
XIA_SHARED int handel_md_event_ready(handel_md_Event* event);

#endif /* MD_THREADS_H */
