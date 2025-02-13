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

#ifndef XIA_SYSTEM_H
#define XIA_SYSTEM_H

#include "handeldef.h"
#include "handel_generic.h"
#include "xia_handel_structures.h"
#include "xia_common.h"

/* Function pointers used for interaction with the PSL layer */
typedef int (*validateDefaults_FP)(XiaDefaults*);
typedef int (*validateModule_FP)(Module*);
typedef int (*downloadFirmware_FP)(int detChan, char* type, char* file, Module* m,
                                   char* rawFile, XiaDefaults* defs);
typedef int (*setAcquisitionValues_FP)(int, char*, void*, XiaDefaults*, FirmwareSet*,
                                       CurrentFirmware*, char*, Detector*, int, Module*,
                                       int);
typedef int (*getAcquisitionValues_FP)(int, char*, void*, XiaDefaults*);
typedef int (*gainOperation_FP)(int detChan, char* name, void* value, Detector* det,
                                int modChan, Module* m, XiaDefaults* defs);
typedef int (*gainCalibrate_FP)(int detChan, Detector* det, int modChan, Module* m,
                                XiaDefaults* defs, double delta);
typedef int (*startRun_FP)(int detChan, unsigned short resume, XiaDefaults* defs,
                           Module* m);
typedef int (*stopRun_FP)(int detChan, Module* m);
typedef int (*getRunData_FP)(int detChan, char* name, void* value, XiaDefaults* defs,
                             Module* m);
typedef int (*doSpecialRun_FP)(int detChan, char* name, void* info,
                               XiaDefaults* defaults, Detector* detector,
                               int detector_chan);
typedef int (*getSpecialRunData_FP)(int, char*, void*, XiaDefaults*);
typedef int (*getParameter_FP)(int, const char*, unsigned short*);
typedef int (*setParameter_FP)(int, const char*, unsigned short);
typedef int (*userSetup_FP)(int detChan, XiaDefaults* defaults,
                            FirmwareSet* firmwareSet, CurrentFirmware* currentFirmware,
                            char* detectorType, Detector* detector, int detector_chan,
                            Module* module, int modChan);
typedef int (*moduleSetup_FP)(int detChan, XiaDefaults* defaults, Module* module);
typedef int (*getDefaultAlias_FP)(char*, char**, double*);
typedef int (*getNumParams_FP)(int, unsigned short*);
typedef int (*getParamData_FP)(int, char*, void*);
typedef int (*getParamName_FP)(int, unsigned short, char*);
typedef int (*freeSCAs_FP)(Module* m, unsigned int);
typedef int (*boardOperation_FP)(int, char*, void*, XiaDefaults*);

typedef unsigned int (*getNumDefaults_FP)(void);

typedef int (*unHook_FP)(int);

/* Structs */
struct PSLFuncs {
    validateDefaults_FP validateDefaults;
    validateModule_FP validateModule;
    downloadFirmware_FP downloadFirmware;
    setAcquisitionValues_FP setAcquisitionValues;
    getAcquisitionValues_FP getAcquisitionValues;
    gainOperation_FP gainOperation;
    gainCalibrate_FP gainCalibrate;
    startRun_FP startRun;
    stopRun_FP stopRun;
    getRunData_FP getRunData;
    doSpecialRun_FP doSpecialRun;
    getSpecialRunData_FP getSpecialRunData;
    getDefaultAlias_FP getDefaultAlias;
    getNumDefaults_FP getNumDefaults;
    getParameter_FP getParameter;
    setParameter_FP setParameter;
    userSetup_FP userSetup;
    moduleSetup_FP moduleSetup;
    getNumParams_FP getNumParams;
    getParamData_FP getParamData;
    getParamName_FP getParamName;
    boardOperation_FP boardOperation;
    freeSCAs_FP freeSCAs;
    unHook_FP unHook;
};
typedef struct PSLFuncs PSLFuncs;

#endif /* XIA_SYSTEM_H */
