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

/* PMT List-Mode Specification. */

#ifndef STJ_BUFFERS_H
#define STJ_BUFFERS_H

#define word unsigned long

/* Each buffer contains a header followed by a series of event records.
 * The records are represented with structs for easy access to named fields
 * instead of pulling out values by index separately.
 *
 * Numbers represented as multiple 32-bit words in the buffer use a are
 * represented in the structs as word arrays. These are low byte first and
 * have to be converted for use in user programs. Macros are provided that
 * take a word pointer and convert the required number of words to a single
 * value.
 *
 * For example:
 *     word *buffer = ...; // assume buffer is malloced or receive as a parameter
 *     struct header *h = (struct header *)buffer;
 *     unsigned long event_id = MAKE_WORD32(h->event_id);
 */

#define MAKE_WORD32(x) (unsigned long) ((x)[0] | ((unsigned long) (x)[1] << 16))
#define MAKE_WORD64(x)                                                                 \
    (unsigned long long) ((x)[0] | ((unsigned long long) (x)[1] << 16) |               \
                          ((unsigned long long) (x)[2] << 32) |                        \
                          ((unsigned long long) (x)[3] << 48))

/* 256-word buffer header. */
struct header {
    word tag0;
    word tag1;
    word header_size;
    word mapping_mode;
    word run_number;
    word buffer_number[2];
    word buffer_id;
    word events;
    word start_event_id[2];
    word module;
    word reserved0[13];
    word total_words[2];
    word reserved1[5];
    word user[32];
    word list_mode_variant;
    word words_per_event;
    word events_again;
    word reserved2[189];
};

/* Base record for anode and dynode-master event records. */
struct event_record_base {
    word tag;
    word list_mode_variant;
    word event_id[4];
    word time[4];
};

/* 272-word Variant 0xA: Anode MPX-32D record. */
struct event_record_anode {
    struct event_record_base stamp;
    word reserved[6];
    word energy[256];
};

/* 272-word Variant 0xB: Dynode-Master MPX-32D record. */
struct event_record_dynode {
    struct event_record_base stamp;
    word multiplicity;
    word mask1;
    word mask2;
    word reserved[3];
    word energy[32];
    word reserved1[224];
};

union event_record {
    struct event_record_anode anode;
    struct event_record_dynode dynode;
};

struct pmt_buffer {
    struct header header;
    union event_record* events; /* get the count from header */
};

enum { AnodeVariant = 0xA, DynodeVariant = 0xB, PMTAllVariant = 0xF };

#endif /* STJ_BUFFERS_H */
