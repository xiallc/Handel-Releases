/*
 * Copyright (c) 2005-2016 XIA LLC
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

#ifndef __PSL_SATURN_H__
#define __PSL_SATURN_H__

/*
 * Function pointers
 */
typedef int (*Saturn_SetAcqValue_FP)(int detChan, void* value, FirmwareSet* fs,
                                     char* detType, XiaDefaults* defs,
                                     double preampGain, Module* m, Detector* det,
                                     int detector_chan);
typedef int (*Saturn_SynchAcqValue_FP)(int detChan, int det_chan, Module* m,
                                       Detector* det, XiaDefaults* defs);
typedef int (*Saturn_DoRunData_FP)(int detChan, void* value, XiaDefaults* defs);

/*
 * Structures
 */

/* A Saturn specific acquisition value */
typedef struct _Saturn_AcquisitionValue {
    char* name;
    boolean_t isDefault;
    boolean_t isSynch;
    double def;
    Saturn_SetAcqValue_FP setFN;
    Saturn_SynchAcqValue_FP synchFN;
} Saturn_AcquisitionValue_t;

typedef struct _Saturn_RunData {
    char* name;
    Saturn_DoRunData_FP fn;
} Saturn_RunData;

/*
 * Macros
 */

/* This saves us a lot of typing. */
#define ACQUISITION_VALUE(x)                                                           \
    PSL_STATIC int pslDo##x(int detChan, void* value, FirmwareSet* fs,                 \
                            char* detectorType, XiaDefaults* defs, double preampGain,  \
                            Module* m, Detector* det, int detector_chan)

/*
 * Constants
 */
#define MIN_MAXWIDTH 1
#define MAX_MAXWIDTH 255
#define MAX_NUM_INTERNAL_SCA 16
#define DSP_PARAM_MEM_LEN 256

/* Memory Management */
DXP_MD_ALLOC saturn_psl_md_alloc;
DXP_MD_FREE saturn_psl_md_free;

#endif /* __PSL_SATURN_H__ */
