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

#ifndef XIA_XUP_H
#define XIA_XUP_H

#include "xia_common.h"

/*
 * Constants
 */
#define XUP_CURRENT_VERSION (unsigned short) 0x001
#define FIVE_SECTOR_LEN_BYTES 1280
#define SECTOR_LEN_BYTES 256
#define BACKUP_FLAG_OFFSET 0x20

/*
 * Structs
 */

typedef struct _Key {
    byte_t key[8];
    byte_t size;
    byte_t ptr;
} Key_t;

typedef struct _Section {
    unsigned long size;
    byte_t* data;
    byte_t offset;
} Section_t;

/*
 * Function pointers
 */

/* Decoder functions */
typedef int (*decodeFP)(char* xup, int detChan);

/* Downloading functions */
typedef int (*downloadFP)(int detChan, unsigned long size, byte_t buffer[]);

/*
 * Shared routines
 */

int xupProcess(int detChan, char* xup);
int xupWriteBackups(int detChan, char* xup);
int xupWriteHistory(int detChan, char* xup);
int xupReboot(int detChan);
int xupSetBackupPath(char* path);
int xupVerifyAccess(int detChan, char* xup);
int xupIsAccessRequired(char* xup, boolean_t* isRequired);
int xupCreateMasterParams(int detChan, char* target);
boolean_t xupIsChecksumValid(char* xup);

#endif /* XIA_XUP_H */
