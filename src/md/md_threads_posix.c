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
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "xia_assert.h"
#include "xia_common.h"

#include "handel_errors.h"
#include "handeldef.h"

#include "md_generic.h"

#include "md_threads.h"

#pragma clang diagnostic ignored "-Wthread-safety-analysis"

XIA_SHARED int handel_md_thread_create(handel_md_Thread* thread) {
    int r = EBUSY;
    if (thread->handle == NULL) {
        struct sched_param param;
        pthread_attr_t attr;
        unsigned long ssize;
        size_t pageSize;

        param.sched_priority = thread->priority;

        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        if (thread->realtime) {
            pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
            pthread_attr_setschedparam(&attr, &param);
        } else {
            pthread_attr_setschedpolicy(&attr, SCHED_OTHER);
        }

        pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);

#ifdef IS_RTOS
        ssize = thread->stackSize;
#else
        /*
         * Add this overhead as some POSIX platforms have deep libc calls.
         */
        ssize = thread->stackSize + (128 * 1024);
#endif

        /*
         * If the stack is not the min make it the minimum.
         */
#if PTHREAD_STACK_MIN
        if (ssize < PTHREAD_STACK_MIN)
            ssize = PTHREAD_STACK_MIN;
#endif

        /*
         * Some hosts require the stack size to be page aligned.
         */
#ifdef IS_RTOS
        pageSize = getpagesize();
#else
        pageSize = (size_t) sysconf(_SC_PAGESIZE);
#endif

        ssize = (((ssize - 1) / pageSize) + 1) * pageSize;

        r = pthread_attr_setstacksize(&attr, ssize);

        if (r == 0) {
            pthread_t* pt = malloc(sizeof(pthread_t));
            if (pt != NULL) {
                thread->handle = pt;
                thread->state = handel_md_ThreadsReady;
                r = pthread_create(pt, &attr, (void* (*) (void*) ) thread->entryPoint,
                                   thread->argument);
                if (r != 0) {
                    free(thread->handle);
                    thread->handle = NULL;
                }
            } else {
                r = ENOMEM;
            }
        }

        pthread_attr_destroy(&attr);
    }

    return r;
}

XIA_SHARED int handel_md_thread_destroy(handel_md_Thread* thread) {
    int r = 0;

    if ((thread->state == handel_md_ThreadsReady) ||
        (thread->state == handel_md_ThreadsActive)) {
        pthread_t* pt = (pthread_t*) thread->handle;
        thread->handle = NULL;
        thread->state = handel_md_ThreadsDetached;
        if (pthread_self() == *pt) {
            free(pt);
            pthread_exit(0);
        }
        pthread_cancel(*pt);
        free(pt);
    }

    return r;
}

XIA_SHARED int handel_md_thread_self(handel_md_Thread* thread) {
    pthread_t pt = *((pthread_t*) thread->handle);
    return pthread_equal(pthread_self(), pt);
}

XIA_SHARED void handel_md_thread_sleep(unsigned int msecs) {
    struct timespec req;
    req.tv_sec = msecs / 1000;
    req.tv_nsec = (msecs % 1000) * 1000000;

    struct timespec rem;
    rem.tv_sec = 0;
    rem.tv_nsec = 0;

    for (;;) {
        if (nanosleep(&req, &rem) == 0)
            break;
        req = rem;
    }
}

XIA_SHARED int handel_md_thread_ready(handel_md_Thread* thread) {
    return thread->handle != NULL;
}

XIA_SHARED int handel_md_mutex_create(handel_md_Mutex* mutex) {
    int r = EBUSY;
    if (mutex->handle == NULL) {
        pthread_mutex_t* pm = malloc(sizeof(pthread_mutex_t));
        pthread_mutexattr_t att;
        if (pm != NULL) {
            pthread_mutexattr_init(&att);
            pthread_mutexattr_settype(&att, PTHREAD_MUTEX_RECURSIVE);
            r = pthread_mutex_init(pm, &att);
            if (r == 0)
                mutex->handle = pm;
        } else {
            r = ENOMEM;
        }
    }
    return r;
}

XIA_SHARED int handel_md_mutex_destroy(handel_md_Mutex* mutex) {
    int r = ENOENT;
    if (mutex->handle != NULL) {
        pthread_mutex_t* pm = (pthread_mutex_t*) mutex->handle;
        r = pthread_mutex_destroy(pm);
        free(pm);
        mutex->handle = NULL;
    }
    return r;
}

XIA_SHARED int handel_md_mutex_lock(handel_md_Mutex* mutex) {
    int r = ENOENT;
    if (mutex->handle != NULL) {
        pthread_mutex_t* pm = (pthread_mutex_t*) mutex->handle;
        r = pthread_mutex_lock(pm);
    }
    return r;
}

XIA_SHARED int handel_md_mutex_unlock(handel_md_Mutex* mutex) {
    int r = ENOENT;
    if (mutex->handle != NULL) {
        pthread_mutex_t* pm = (pthread_mutex_t*) mutex->handle;
        r = pthread_mutex_unlock(pm);
    }
    return r;
}

XIA_SHARED int handel_md_mutex_trylock(handel_md_Mutex* mutex) {
    int r = ENOENT;
    if (mutex->handle != NULL) {
        pthread_mutex_t* pm = (pthread_mutex_t*) mutex->handle;
        r = pthread_mutex_trylock(pm);
    }
    return r;
}

XIA_SHARED int handel_md_mutex_ready(handel_md_Mutex* mutex) {
    return mutex->handle != NULL;
}

/*
 * The event is a mutex and condition variable.
 */
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool set;
} eventInternal;

XIA_SHARED int handel_md_event_create(handel_md_Event* event) {
    int r = EBUSY;
    if (event->handle == NULL) {
        eventInternal* ei = malloc(sizeof(eventInternal));
        pthread_mutexattr_t att;
        if (ei != NULL) {
            pthread_mutexattr_init(&att);
            pthread_mutexattr_settype(&att, PTHREAD_MUTEX_RECURSIVE);
            r = pthread_mutex_init(&ei->mutex, &att);
            if (r == 0)
                r = pthread_cond_init(&ei->cond, 0);
            if (r) {
                free(ei);
                ei = NULL;
            }
            ei->set = false;
            event->handle = ei;
        } else {
            r = ENOMEM;
        }
    }
    return r;
}

XIA_SHARED int handel_md_event_destroy(handel_md_Event* event) {
    int r = ENOENT;
    if (event->handle != NULL) {
        eventInternal* ei = (eventInternal*) event->handle;
        r = pthread_cond_destroy(&ei->cond);
        int rr = pthread_mutex_destroy(&ei->mutex);
        if (r == 0)
            r = rr;
        free(ei);
        event->handle = NULL;
    }
    return r;
}

XIA_SHARED int handel_md_event_wait(handel_md_Event* event, unsigned int timeout) {
    int r = ENOENT;
    if (event->handle != NULL) {
        eventInternal* ei = (eventInternal*) event->handle;
        if (timeout != 0) {
            struct timespec t;
            r = clock_gettime(CLOCK_REALTIME, &t);
            if (r == 0) {
                t.tv_sec += timeout / 1000;
                timeout = (timeout % 1000) * 1000000;
                if ((t.tv_nsec + timeout) >= 1000000000L)
                    t.tv_sec++;
                t.tv_nsec = (t.tv_nsec + timeout) % 1000000000L;
                for (;;) {
                    int rr;
                    r = pthread_mutex_lock(&ei->mutex);
                    if (r)
                        break;
                    if (!ei->set)
                        r = pthread_cond_timedwait(&ei->cond, &ei->mutex, &t);
                    ei->set = false;
                    rr = pthread_mutex_unlock(&ei->mutex);
                    if (r != EAGAIN) {
                        if (r == 0)
                            r = rr;
                        break;
                    }
                    if (rr) {
                        r = rr;
                        break;
                    }
                }
            }
        } else {
            for (;;) {
                int rr;
                r = pthread_mutex_lock(&ei->mutex);
                if (r)
                    break;
                if (!ei->set)
                    r = pthread_cond_wait(&ei->cond, &ei->mutex);
                ei->set = false;
                rr = pthread_mutex_unlock(&ei->mutex);
                if (r != EAGAIN) {
                    if (r == 0)
                        r = rr;
                    break;
                }
                if (rr) {
                    r = rr;
                    break;
                }
            }
        }
        if (r == ETIMEDOUT)
            r = THREADING_TIMEOUT;
    }
    return r;
}

XIA_SHARED int handel_md_event_signal(handel_md_Event* event) {
    int r = ENOENT;
    if (event->handle != NULL) {
        eventInternal* ei = (eventInternal*) event->handle;
        r = pthread_mutex_lock(&ei->mutex);
        if (r == 0) {
            int rr;
            ei->set = true;
            r = pthread_cond_signal(&ei->cond);
            rr = pthread_mutex_unlock(&ei->mutex);
            if (r)
                r = rr;
        }
    }
    return r;
}

XIA_SHARED int handel_md_event_ready(handel_md_Event* event) {
    return event->handle != NULL;
}
