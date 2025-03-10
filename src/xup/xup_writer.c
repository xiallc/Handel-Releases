/*
 * Copyright (c) 2002-2004, X-ray Instrumentation Associates
 *               2005-2014, XIA LLC
 * All rights reserved.
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
 *   * Neither the name of X-ray Instrumentation Associates
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
 *
 *
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "xia_assert.h"
#include "xup_writer.h"

#ifdef __VLD_MEM_DBG__
#include <vld.h>
#endif

typedef struct _Section {
    unsigned long size;
    byte_t* data;
    struct _Section* next;
    unsigned short opt_offset;
    byte_t offset;
} section_t;

typedef struct _Key {
    byte_t key[8];
    unsigned short size;
    unsigned short ptr;
} key_t;

/* Byte encrypter */
static byte_t eb(byte_t p);

static void writeRawSections(byte_t* s);
static unsigned short xupByteToUS(byte_t lo, byte_t hi);
static byte_t xupXORChksum(unsigned long size, byte_t* data);
static void xupCalculateAccessCode(byte_t* sn, unsigned short chksum, byte_t* acode);
static void removeSections();

/* Here is where you would add an
 * array if you wanted to handle multiple
 * handles.
 */
static FILE* fp = NULL;

/* Here is where you would add an array
 * to implement multiple versions and
 * encryption keys.
 */
static key_t ekey = {{0xF6, 0x37, 0xAC, 0xDD, 0x05, 0xC2, 0x1F, 0x61}, 8, 0};

static unsigned short chk = 0;
static unsigned short nSecs = 0;
static unsigned long secsSize = 0;
static section_t* sects = NULL;

DLL_SPEC int DLL_API OpenXUP(char* name, unsigned short* h) {
    removeSections();
    fp = fopen(name, "wb");
    if (fp == NULL) {
        *h = 0xFFFF;
        return -1;
    }
    *h = 0x0000;
    srand((unsigned) time(NULL));
    return 0;
}

DLL_SPEC void DLL_API CloseXUP(unsigned short h) {
    UNUSED(h);
    removeSections();
    if (fp != NULL) {
        fclose(fp);
    }
}

DLL_SPEC int DLL_API AddSection(byte_t offset, unsigned short opt_mem_offset,
                                unsigned long size, byte_t* data, unsigned short* idx) {
    unsigned short i;

    section_t* sec = NULL;
    section_t* cur = NULL;
    section_t* prev = NULL;

    sec = (section_t*) malloc(sizeof(section_t));

    if (sec == NULL) {
        return -1;
    }

    sec->next = NULL;
    sec->size = size;
    sec->opt_offset = opt_mem_offset;
    sec->offset = offset;

    sec->data = (byte_t*) malloc(size * sizeof(byte_t));

    if (sec->data == NULL) {
        free(sec);
        return -1;
    }

    memcpy(sec->data, data, size);

    if (sects == NULL) {
        sects = sec;
        *idx = 0;
    } else {
        prev = sects;
        cur = sects;
        i = 0;

        while (cur->next != NULL) {
            cur = cur->next;
            prev = cur;
            i++;
        }

        cur = prev;
        cur->next = sec;
        *idx = i;
    }

    nSecs++;
    /* Note: This doesn't take into account the 2 extra bytes required
     * when the offset is defined as a generic memory address.
     */
    secsSize += 5 + size;

    return 0;
}

DLL_SPEC int DLL_API RemoveSection(unsigned short idx) {
    UNUSED(idx);
    return 0;
}

/**
 * @brief Write the current sections to a byte array, calculate
 *         the checksum and write the complete file.
 */
DLL_SPEC int DLL_API WriteXUP(unsigned short h, unsigned short gid, byte_t req) {
    int r;

    byte_t ac = 0x00;
    byte_t ver = (byte_t) CURRENT_VERSION;
    byte_t b = 0x00;

    unsigned long i;

    byte_t* rawSections = NULL;

    UNUSED(h);

    if (fp == NULL) {
        return -1;
    }

    rawSections = (byte_t*) malloc(secsSize);

    if (rawSections == NULL) {
        return -1;
    }

    writeRawSections(rawSections);

    chk = (unsigned short) xupXORChksum(secsSize, rawSections);

    /* Write header first */
    fwrite(&gid, sizeof(gid), 1, fp);
    fwrite(&ver, sizeof(ver), 1, fp);

    switch (req) {
        case NO_ACCESS:
            ac = 0x32;
            break;

        case ACCESS_REQUIRED:
            do {
                r = rand();
                ac = (byte_t) ((r / RAND_MAX) * 256);
            } while (ac == 0x32);
            break;

        default:
            ac = 0xFF;
            break;
    }

    fwrite(&ac, sizeof(ac), 1, fp);
    fwrite(&chk, sizeof(chk), 1, fp);

    /** START ENCRYPTED CODE HERE **/

    b = eb((byte_t) (nSecs & 0xFF));
    fwrite(&b, sizeof(b), 1, fp);
    b = eb((byte_t) ((nSecs >> 8) & 0xFF));
    fwrite(&b, sizeof(b), 1, fp);

    for (i = 0; i < secsSize; i++) {
        b = eb(rawSections[i]);
        fwrite(&b, sizeof(b), 1, fp);
    }

    free(rawSections);
    return 0;
}

static byte_t eb(byte_t p) {
    byte_t c;

    unsigned short idx = ekey.ptr;

    c = (byte_t) (p ^ ekey.key[idx]);

    if (idx == ekey.size - 1) {
        ekey.ptr = 0;
    } else {
        ekey.ptr++;
    }

    return c;
}

DLL_SPEC void DLL_API DumpSections(char* name) {
    unsigned long i;

    section_t* s = sects;

    FILE* xupFile = NULL;

    xupFile = fopen(name, "w");

    if (xupFile == NULL) {
        return;
    }

    while (s != NULL) {
        fprintf(xupFile, "size         = %lu (%#lx)\n", s->size, s->size);

        for (i = 0; i < s->size; i++) {
            fprintf(xupFile, "data[%lu] = %#x\n", i, s->data[i]);
        }

        fprintf(xupFile, "offset       = %#x\n", s->offset);
        fprintf(xupFile, "opt_offset   = %#x\n", s->opt_offset);
        fprintf(xupFile, "next         = %#zx\n\n",
                (s->next ? (size_t) (s->next) : 0x00));

        s = s->next;
    }

    fclose(xupFile);
}

/**********
 * Computes a byte-wide XOR checksum from the supplied data.
 **********/
static byte_t xupXORChksum(unsigned long size, byte_t* data) {
    unsigned long i;
    byte_t chksum = 0x00;

    for (i = 0; i < size; i++) {
        chksum = (byte_t) (chksum ^ data[i]);
    }

    return chksum;
}

/**
 * @brief Write all the sections to a byte array.
 */
static void writeRawSections(byte_t* r) {
    unsigned long i = 0;
    unsigned long j;

    section_t* s = sects;

    ASSERT(r != NULL);

    while (s != NULL) {
        r[i++] = s->offset;

        if ((s->offset == 0x05) || (s->offset == 0x15)) {
            /* This should be an error since it isn't implemented
		 * yet.
		 */
            FAIL();
        }

        r[i++] = (byte_t) (s->size & 0xFF);
        r[i++] = (byte_t) ((s->size >> 8) & 0xFF);
        r[i++] = (byte_t) ((s->size >> 16) & 0xFF);
        r[i++] = (byte_t) ((s->size >> 24) & 0xFF);

        for (j = 0; j < s->size; j++) {
            r[i++] = s->data[j];
        }

        s = s->next;
    }

    ASSERT(i == secsSize);
}

/**
 * @brief Creates an Access Control File for the most recent XUP generated
 *         via WriteXUP().
 */
DLL_SPEC int DLL_API CreateACF(char* file, char* sn) {
    byte_t cs = 0x00;

    byte_t preChk[10];
    byte_t ac[8];

    FILE* acfFile = NULL;

    ASSERT(file != NULL);
    ASSERT(sn != NULL);

    xupCalculateAccessCode((byte_t*) sn, chk, ac);

    preChk[0] = (byte_t) 0x01;
    preChk[1] = (byte_t) 0x00;
    memcpy(preChk + 2, ac, 8);

    cs = xupXORChksum(10, preChk);

    acfFile = fopen(file, "wb");

    if (acfFile == NULL) {
        return -1;
    }

    fwrite(preChk, 10, 1, acfFile);
    fwrite(&cs, 1, 1, acfFile);

    fclose(acfFile);

    return 0;
}

/**********
 * Calculates an access code from a serial number and checksum.
 * The procedures here are explained in more detail in the 
 * specification: microDXP Software Security Model.
 **********/
static void xupCalculateAccessCode(byte_t* sn, unsigned short chksum, byte_t* acode) {
    unsigned short i;
    unsigned short us = 0x0000;

    unsigned short x[8];

    us = xupByteToUS(sn[0], sn[1]);

    x[0] = (unsigned short) (us ^ chksum);

    for (i = 0; i < 7; i++) {
        us = xupByteToUS(sn[(i * 2) + 2], sn[(i * 2) + 3]);
        x[i + 1] = (unsigned short) (us ^ x[i]);
    }

    /* Convert to the alphanumeric representation here */
    for (i = 0; i < 8; i++) {
        acode[i] = (byte_t) ((x[i] % 26) + 0x41);
    }
}

/**********
 * This routine turns 2 bytes
 * into an unsigned short.
 **********/
static unsigned short xupByteToUS(byte_t lo, byte_t hi) {
    unsigned short result;
    result = (unsigned short) (lo | ((unsigned short) hi << 8));
    return result;
}

/**
 * @brief Remove all linked list data
 */
static void removeSections() {
    section_t* sec = NULL;
    section_t* cur = sects;

    while (cur != NULL) {
        sec = cur;
        cur = sec->next;
        free(sec->data);
        free(sec);
    }

    nSecs = 0;
    secsSize = 0;
    sects = NULL;
}
