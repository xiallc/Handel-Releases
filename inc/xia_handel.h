/* Prototypes for Handel library routines */

/*
 * Copyright (c) 2004 X-ray Instrumentation Associates
 *               2005-2020 XIA LLC
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

#ifndef XIA_HANDEL_H
#define XIA_HANDEL_H

#include <stdio.h>
#include <stdlib.h>

#include "handel_generic.h"
#include "xia_common.h"
#include "xia_handel_structures.h"
#include "xia_module.h"
#include "xia_system.h"
#include "xia_xerxes_structures.h"

#include "md_generic.h"

#include "handeldef.h"

/* If this is compiled by a C++ compiler, make it clear that these are C routines */
#ifdef __cplusplus
extern "C" {
#endif

#ifdef _HANDEL_PROTO_
#include <stdio.h>

/*
 * following are internal prototypes for HANDEL.c routines
 */

HANDEL_EXPORT int HANDEL_API xiaInit(char* inifile);

HANDEL_EXPORT int HANDEL_API xiaInitHandel(void);
HANDEL_EXPORT int HANDEL_API xiaEnableLogOutput(void);
HANDEL_EXPORT int HANDEL_API xiaSuppressLogOutput(void);
HANDEL_EXPORT int HANDEL_API xiaSetLogLevel(int level);
HANDEL_EXPORT int HANDEL_API xiaSetLogOutput(char* filename);
HANDEL_EXPORT int HANDEL_API xiaCloseLog(void);
HANDEL_EXPORT int HANDEL_API xiaNewDetector(char* alias);
HANDEL_EXPORT int HANDEL_API xiaAddDetectorItem(char* alias, char* name, void* value);
HANDEL_EXPORT int HANDEL_API xiaModifyDetectorItem(char* alias, char* name,
                                                   void* value);
HANDEL_EXPORT int HANDEL_API xiaGetDetectorItem(char* alias, char* name, void* value);
HANDEL_EXPORT int HANDEL_API xiaGetNumDetectors(unsigned int* numDetectors);
HANDEL_EXPORT int HANDEL_API xiaGetDetectors(char* detectors[]);
HANDEL_EXPORT int HANDEL_API xiaGetDetectors_VB(unsigned int index, char* alias);
HANDEL_EXPORT int HANDEL_API xiaRemoveDetector(char* alias);
HANDEL_EXPORT int HANDEL_API xiaDetectorFromDetChan(int detChan, char* alias);
HANDEL_EXPORT int HANDEL_API xiaNewFirmware(char* alias);
HANDEL_EXPORT int HANDEL_API xiaAddFirmwareItem(char* alias, char* name, void* value);
HANDEL_EXPORT int HANDEL_API xiaModifyFirmwareItem(char* alias, unsigned short ptrr,
                                                   char* name, void* value);
HANDEL_EXPORT int HANDEL_API xiaGetFirmwareItem(char* alias, unsigned short ptrr,
                                                char* name, void* value);
HANDEL_EXPORT int HANDEL_API xiaGetNumFirmwareSets(unsigned int* numFirmware);
HANDEL_EXPORT int HANDEL_API xiaGetFirmwareSets(char* firmwares[]);
HANDEL_EXPORT int HANDEL_API xiaGetFirmwareSets_VB(unsigned int index, char* alias);
HANDEL_EXPORT int HANDEL_API xiaGetNumPTRRs(char* alias, unsigned int* numPTRR);
HANDEL_EXPORT int HANDEL_API xiaRemoveFirmware(char* alias);
HANDEL_EXPORT int HANDEL_API xiaNewModule(char* alias);
HANDEL_EXPORT int HANDEL_API xiaAddModuleItem(char* alias, char* name, void* value);
HANDEL_EXPORT int HANDEL_API xiaModifyModuleItem(char* alias, char* name, void* value);
HANDEL_EXPORT int HANDEL_API xiaGetModuleItem(char* alias, char* name, void* value);
HANDEL_EXPORT int HANDEL_API xiaGetNumModules(unsigned int* numModules);
HANDEL_EXPORT int HANDEL_API xiaGetModules(char* modules[]);
HANDEL_EXPORT int HANDEL_API xiaGetModules_VB(unsigned int index, char* alias);
HANDEL_EXPORT int HANDEL_API xiaRemoveModule(char* alias);
HANDEL_EXPORT int HANDEL_API xiaModuleFromDetChan(int detChan, char* alias);
HANDEL_EXPORT int HANDEL_API xiaAddChannelSetElem(unsigned int detChanSet,
                                                  unsigned int newChan);
HANDEL_EXPORT int HANDEL_API xiaRemoveChannelSetElem(unsigned int detChan,
                                                     unsigned int chan);
HANDEL_EXPORT int HANDEL_API xiaRemoveChannelSet(unsigned int detChan);
HANDEL_EXPORT int HANDEL_API xiaStartSystem(void);
HANDEL_EXPORT int HANDEL_API xiaDownloadFirmware(int detChan, char* type);
HANDEL_EXPORT int HANDEL_API xiaSetAcquisitionValues(int detChan, char* name,
                                                     void* value);
HANDEL_EXPORT int HANDEL_API xiaGetAcquisitionValues(int detChan, char* name,
                                                     void* value);
HANDEL_EXPORT int HANDEL_API xiaRemoveAcquisitionValues(int detChan, char* name);
HANDEL_EXPORT int HANDEL_API xiaUpdateUserParams(int detChan);
HANDEL_EXPORT int HANDEL_API xiaGainOperation(int detChan, char* name, void* value);
HANDEL_EXPORT int HANDEL_API xiaGainCalibrate(int detChan, double deltaGain);
HANDEL_EXPORT int HANDEL_API xiaStartRun(int detChan, unsigned short resume);
HANDEL_EXPORT int HANDEL_API xiaStopRun(int detChan);
HANDEL_EXPORT int HANDEL_API xiaGetRunData(int detChan, char* name, void* value);
HANDEL_EXPORT int HANDEL_API xiaDoSpecialRun(int detChan, char* name, void* info);
HANDEL_EXPORT int HANDEL_API xiaGetSpecialRunData(int detChan, char* name, void* value);
HANDEL_EXPORT int HANDEL_API xiaLoadSystem(char* type, char* filename);
HANDEL_EXPORT int HANDEL_API xiaSaveSystem(char* type, char* filename);
HANDEL_EXPORT int HANDEL_API xiaGetParameter(int detChan, const char* name,
                                             unsigned short* value);
HANDEL_EXPORT int HANDEL_API xiaSetParameter(int detChan, const char* name,
                                             unsigned short value);
HANDEL_EXPORT int HANDEL_API xiaGetNumParams(int detChan, unsigned short* value);
HANDEL_EXPORT int HANDEL_API xiaGetParamData(int detChan, char* name, void* value);
HANDEL_EXPORT int HANDEL_API xiaGetParamName(int detChan, unsigned short index,
                                             char* name);
HANDEL_EXPORT int HANDEL_API xiaBoardOperation(int detChan, char* name, void* value);
HANDEL_EXPORT int HANDEL_API xiaMemoryOperation(int detChan, char* name, void* value);
HANDEL_EXPORT int HANDEL_API xiaCommandOperation(int detChan, byte_t cmd,
                                                 unsigned int lenS, byte_t* send,
                                                 unsigned int lenR, byte_t* recv);
HANDEL_EXPORT int HANDEL_API xiaExit(void);

HANDEL_EXPORT int HANDEL_API xiaSetIOPriority(int pri);

HANDEL_EXPORT void HANDEL_API xiaGetVersionInfo(int* rel, int* min, int* maj,
                                                char* pretty);
HANDEL_EXPORT const char* HANDEL_API xiaGetErrorText(int errorcode);

#ifdef _DEBUG

HANDEL_EXPORT void HANDEL_API xiaDumpDetChanStruct(char* fileName);
HANDEL_EXPORT void HANDEL_API xiaDumpDSPParameters(int detChan, char* fileName);
HANDEL_EXPORT void HANDEL_API xiaDumpFirmwareStruct(char* fileName);
HANDEL_EXPORT void HANDEL_API xiaDumpModuleStruct(char* fileName);
HANDEL_EXPORT void HANDEL_API xiaDumpDefaultsStruct(char* fileName);

#endif /* _DEBUG */

#else /* Begin old style C prototypes */
/*
 * following are internal prototypes for handel layer subset of xerxes.c routines
 */
HANDEL_EXPORT int HANDEL_API xiaInit();
HANDEL_EXPORT int HANDEL_API xiaInitHandel();
HANDEL_EXPORT int HANDEL_API xiaEnableLogOutput();
HANDEL_EXPORT int HANDEL_API xiaSuppressLogOutput();
HANDEL_EXPORT int HANDEL_API xiaSetLogLevel();
HANDEL_EXPORT int HANDEL_API xiaSetLogOutput();
HANDEL_EXPORT int HANDEL_API xiaCloseLog();
HANDEL_EXPORT int HANDEL_API xiaNewDetector();
HANDEL_EXPORT int HANDEL_API xiaAddDetectorItem();
HANDEL_EXPORT int HANDEL_API xiaModifyDetectorItem();
HANDEL_EXPORT int HANDEL_API xiaGetDetectorItem();
HANDEL_EXPORT int HANDEL_API xiaGetNumDetectors();
HANDEL_EXPORT int HANDEL_API xiaGetDetectors();
HANDEL_EXPORT int HANDEL_API xiaGetDetectors_VB();
HANDEL_EXPORT int HANDEL_API xiaRemoveDetector();
HANDEL_EXPORT int HANDEL_API xiaNewFirmware();
HANDEL_EXPORT int HANDEL_API xiaAddFirmwareItem();
HANDEL_EXPORT int HANDEL_API xiaModifyFirmwareItem();
HANDEL_EXPORT int HANDEL_API xiaGetFirmwareItem();
HANDEL_EXPORT int HANDEL_API xiaGetNumFirmwareSets();
HANDEL_EXPORT int HANDEL_API xiaGetFirmwareSets();
HANDEL_EXPORT int HANDEL_API xiaGetFirmwareSets_VB();
HANDEL_EXPORT int HANDEL_API xiaGetNumPTRRs();
HANDEL_EXPORT int HANDEL_API xiaRemoveFirmware();
HANDEL_EXPORT int HANDEL_API xiaNewModule();
HANDEL_EXPORT int HANDEL_API xiaAddModuleItem();
HANDEL_EXPORT int HANDEL_API xiaModifyModuleItem();
HANDEL_EXPORT int HANDEL_API xiaGetModuleItem();
HANDEL_EXPORT int HANDEL_API xiaGetNumModules();
HANDEL_EXPORT int HANDEL_API xiaGetModules();
HANDEL_EXPORT int HANDEL_API xiaGetModules_VB();
HANDEL_EXPORT int HANDEL_API xiaRemoveModule();
HANDEL_EXPORT int HANDEL_API xiaAddChannelSetElem();
HANDEL_EXPORT int HANDEL_API xiaRemoveChannelSetElem();
HANDEL_EXPORT int HANDEL_API xiaRemoveChannelSet();
HANDEL_EXPORT int HANDEL_API xiaStartSystem();
HANDEL_EXPORT int HANDEL_API xiaDownloadFirmware();
HANDEL_EXPORT int HANDEL_API xiaSetAcquisitionValues();
HANDEL_EXPORT int HANDEL_API xiaGetAcquisitionValues();
HANDEL_EXPORT int HANDEL_API xiaRemoveAcquisitionValues();
HANDEL_EXPORT int HANDEL_API xiaUpdateUserParams();
HANDEL_EXPORT int HANDEL_API xiaGainCalibrate();
HANDEL_EXPORT int HANDEL_API xiaGainOperation();
HANDEL_EXPORT int HANDEL_API xiaStartRun();
HANDEL_EXPORT int HANDEL_API xiaStopRun();
HANDEL_EXPORT int HANDEL_API xiaGetRunData();
HANDEL_EXPORT int HANDEL_API xiaDoSpecialRun();
HANDEL_EXPORT int HANDEL_API xiaGetSpecialRunData();
HANDEL_EXPORT int HANDEL_API xiaLoadSystem();
HANDEL_EXPORT int HANDEL_API xiaSaveSystem();
HANDEL_EXPORT int HANDEL_API xiaGetParameter();
HANDEL_EXPORT int HANDEL_API xiaSetParameter();
HANDEL_EXPORT int HANDEL_API xiaGetNumParams();
HANDEL_EXPORT int HANDEL_API xiaGetParamData();
HANDEL_EXPORT int HANDEL_API xiaGetParamName();
HANDEL_EXPORT int HANDEL_API xiaBoardOperation();
HANDEL_EXPORT int HANDEL_API xiaMemoryOperation();
HANDEL_EXPORT int HANDEL_API xiaCommandOperation();
HANDEL_EXPORT int HANDEL_API xiaExit();

HANDEL_EXPORT int HANDEL_API xiaSetIOPriority();

HANDEL_EXPORT void HANDEL_API xiaGetVersionInfo();
HANDEL_EXPORT const char* HANDEL_API xiaGetErrorText();

#ifdef _DEBUG

HANDEL_EXPORT void HANDEL_API xiaDumpDetChanStruct();
HANDEL_EXPORT void HANDEL_API xiaDumpFirmwareStruct();
HANDEL_EXPORT void HANDEL_API xiaDumpModuleStruct();
HANDEL_EXPORT void HANDEL_API xiaDumpDefaultsStruct();

HANDEL_EXPORT void HANDEL_API xiaUnitTests();

#endif /* _DEBUG */

#endif /*   end if _HANDEL_PROTO_ */

/* If this is compiled by a C++ compiler, make it clear that these are C routines */
#ifdef __cplusplus
}
#endif

int HANDEL_API xiaNewDefault(char* alias);
int HANDEL_API xiaAddDefaultItem(char* alias, char* name, void* value);
int HANDEL_API xiaModifyDefaultItem(char* alias, char* name, void* value);
int HANDEL_API xiaGetDefaultItem(char* alias, char* name, void* value);
int HANDEL_API xiaRemoveDefault(char* alias);
int HANDEL_API xiaReadIniFile(char* inifile);
int HANDEL_API xiaFreeDetector(Detector* detector);
int HANDEL_API xiaFreeFirmwareSet(FirmwareSet* firmwareSet);
int HANDEL_API xiaFreeFirmware(Firmware* firmware);
int HANDEL_API xiaFreeXiaDefaults(XiaDefaults* xiaDefaults);
int HANDEL_API xiaFreeXiaDaqEntry(XiaDaqEntry* entry);
int HANDEL_API xiaFreeModule(Module* module);
void HANDEL_API xiaLog(int level, const char* file, int line, int status,
                       const char* func, const char* fmt, ...) HANDEL_PRINTF(6, 7);
Detector* HANDEL_API xiaFindDetector(char* alias);
FirmwareSet* HANDEL_API xiaFindFirmware(char* alias);
XiaDefaults* HANDEL_API xiaFindDefault(char* alias);
Module* HANDEL_API xiaFindModule(char* alias);
boolean_t HANDEL_API xiaIsDetChanFree(int detChan);
int HANDEL_API xiaCleanDetChanList(void);
int HANDEL_API xiaAddDetChan(int type, unsigned int detChan, void* data);
int HANDEL_API xiaRemoveDetChan(unsigned int detChan);
void HANDEL_API xiaFreeDetSet(DetChanSetElem* head);
int HANDEL_API xiaGetBoardType(int detChan, char* boardType);
DetChanElement* HANDEL_API xiaGetDetChanHead(void);
FirmwareSet* HANDEL_API xiaGetFirmwareSetHead(void);
int HANDEL_API xiaMergeSort(void* data, int size, int esize, int i, int k,
                            int (*compare)(const void* key1, const void* key2));
int HANDEL_API xiaGetNumFirmware(Firmware* firmware);
int xiaFirmComp(const void* key1, const void* key2);
int HANDEL_API xiaInsertSort(Firmware** head,
                             int (*compare)(const void* key1, const void* key2));
Detector* HANDEL_API xiaGetDetectorHead(void);
int HANDEL_API xiaGetElemType(int detChan);
void HANDEL_API xiaClearTags(void);
DetChanElement* HANDEL_API xiaGetDetChanPtr(int detChan);
char* HANDEL_API xiaGetAliasFromDetChan(int detChan);
unsigned int HANDEL_API xiaGetModChan(unsigned int detChan);
XiaDefaults* HANDEL_API xiaGetDefaultFromDetChan(unsigned int detChan);
int HANDEL_API xiaBuildXerxesConfig(void);
Module* HANDEL_API xiaGetModuleHead(void);
double HANDEL_API xiaGetValueFromDefaults(char* name, char* alias);
int HANDEL_API xiaGetDSPNameFromFirmware(char* alias, double peakingTime,
                                         char* dspName);
int HANDEL_API xiaUserSetup(void);
int HANDEL_API xiaGetFippiNameFromFirmware(char* alias, double peakingTime,
                                           char* fippiName);
int HANDEL_API xiaGetValueFromFirmware(char* alias, double peakingTime, char* name,
                                       char* value);
int HANDEL_API xiaLoadPSL(char* boardType, PSLFuncs* funcs);
XiaDefaults* HANDEL_API xiaGetDefaultsHead(void);
int HANDEL_API xiaGetAbsoluteChannel(int detChan, Module* module, unsigned int* chan);
int HANDEL_API xiaTagAllRunActive(Module* module, boolean_t state);

#include "xerxes_structures.h"

/* Create some useful abbreviations here so that I don't have to do utils->funcs->
 * everytime I want to use a function.
 */
DXP_MD_OUTPUT handel_md_output;
DXP_MD_ALLOC handel_md_alloc;
DXP_MD_FREE handel_md_free;
DXP_MD_PUTS handel_md_puts;
DXP_MD_WAIT handel_md_wait;
DXP_MD_FGETS handel_md_fgets;
DXP_MD_ENABLE_LOG handel_md_enable_log;
DXP_MD_SUPPRESS_LOG handel_md_suppress_log;
DXP_MD_SET_LOG_LEVEL handel_md_set_log_level;

/* Detector-type constants */
#define XIA_DET_UNKNOWN 0x0000
#define XIA_DET_RESET 0x0001
#define XIA_DET_RCFEED 0x0002

/* Statics used in multiple source files */

extern boolean_t isHandelInit;

/*
 *	Define the head of the Detector list
 */
extern Detector* xiaDetectorHead;

/*
 *	Define the head of the Firmware Sets LL
 */
extern FirmwareSet* xiaFirmwareSetHead;

/*
 *	Define the head of the XiaDefaults LL
 */
extern XiaDefaults* xiaDefaultsHead;

/*
 * This is the head of the DetectorChannel LL
 */
extern DetChanElement* xiaDetChanHead;

/*
 * This is the head of Module LL
 */
extern Module* xiaModuleHead;

/* MACROS for the Linked-Lists */
#define isListEmpty(x) (((x) == NULL) ? TRUE_ : FALSE_)
#define getListNext(x) ((x)->next)
#define getListPrev(x) ((x)->prev)

struct MoistureReading {
    int status;
    unsigned long value;
};

#endif /* Endif for XIA_HANDEL_H */
