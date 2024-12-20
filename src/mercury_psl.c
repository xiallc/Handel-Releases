/* PSL driver for the Mercury hardware. */

/*
 * Copyright (c) 2007-2020 XIA LLC
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

#include "xia_assert.h"
#include "xia_system.h"
#include "xia_psl.h"
#include "xia_handel.h"

#include "psl_common.h"

#include "md_generic.h"

#include "xerxes.h"
#include "xerxes_errors.h"
#include "xia_xerxes.h"

#include "handel_constants.h"
#include "handel_errors.h"

#include "mercury.h"
#include "psl_mercury.h"

#include "psldef.h"

#include "fdd.h"


PSL_EXPORT int PSL_API mercury_PSLInit(PSLFuncs *funcs);

/* Helpers */
PSL_STATIC double psl__GetClockTick(void);
PSL_STATIC int psl__UpdateFilterParams(int detChan, int modChan, double pt,
                                       XiaDefaults *defs, FirmwareSet *fs,
                                       Module *m, Detector *det);
PSL_STATIC int psl__UpdateTrigFilterParams(Module *m, int detChan,
                                        XiaDefaults *defs);
PSL_STATIC int psl__UpdateGain(int detChan, int modChan, XiaDefaults *defs,
                               Module *m, Detector *det);
PSL_STATIC int psl__UpdateVariableGain(int detChan, int modChan,
                XiaDefaults *defs, Module *m, Detector *det);
 PSL_STATIC int psl__UpdateSwitchedGain(int detChan, int modChan,
                XiaDefaults *defs, Module *m, Detector *det);
PSL_STATIC int psl__GetEVPerADC(int detChan, XiaDefaults *defs, double *eVPerADC);
 PSL_STATIC int psl_CalculateEvPerADC(int detChan, XiaDefaults *defs,
                parameter_t *SWGAIN, double *eVPerADC);
PSL_STATIC int psl__GetSystemGain(double *g);
PSL_STATIC int psl__GetStatisticsBlock(int detChan, unsigned long *stats);
PSL_STATIC int psl__ExtractELivetime(int modChan, unsigned long *stats,
                                     double *eLT);
PSL_STATIC int psl__ExtractRealtime(int modChan, unsigned long *stats,
                                    double *rt);
PSL_STATIC int psl__ExtractTLivetime(int modChan, unsigned long *stats,
                                     double *tLT);
PSL_STATIC int psl__ExtractTriggers(int modChan, unsigned long *stats,
                                    double *trigs);
PSL_STATIC int psl__ExtractEvents(int modChan, unsigned long *stats,
                                  double *evts);
PSL_STATIC int psl__ExtractOverflows(int modChan, unsigned long *stats,
                                     double *overs);
PSL_STATIC int psl__ExtractUnderflows(int modChan, unsigned long *stats,
                                      double *unders);
PSL_STATIC int psl__SwitchFirmware(int detChan, double type, int modChan,
                                   double pt, FirmwareSet *fs, Module *m);
PSL_STATIC int psl__WakeDSP(int detChan);
PSL_STATIC int psl__UpdateRawParamAcqValue(int detChan, char *name, void *value,
                                           XiaDefaults *defaults);
PSL_STATIC unsigned long psl__GetMCAPixelBlockSize(XiaDefaults *defs,
                                                   Module *m);
PSL_STATIC int psl__SyncTempCalibrationValues(int detChan, Module *m,
                                              XiaDefaults *defs);
PSL_STATIC int psl__ApplyTempCalibrationValues(int detChan, XiaDefaults *defs);
PSL_STATIC boolean_t psl__IsMercuryOem(int detChan);
PSL_STATIC int psl__UpdateThresholds(int detChan, int modChan, XiaDefaults *defs,
                               Module *m, Detector *det);
PSL_STATIC boolean_t psl__AcqRemoved(const char *name);
PSL_STATIC int psl__ReadoutPeakingTime(int detChan, double *peaking_time);

/* Helper functions for mapping mode */
PSL_STATIC int psl__UpdateParams(int detChan, unsigned short type, int modChan,
                                 char *name, void *value, char *detType,
                                 XiaDefaults *defs, Module *m,
                                 Detector *det, FirmwareSet *fs);
PSL_STATIC int psl__IsMapping(int detChan, unsigned short allowed,
                              boolean_t *isMapping);
PSL_STATIC int psl__SwitchSystemFPGA(int detChan, int modChan, FirmwareSet *fs,
                             char *detType, double pt, int nKeywords,
                             char **keywords, char *rawFile, Module *m,
                             boolean_t *downloaded);
PSL_STATIC int psl__SetRegisterBit(int detChan, char *reg, int bit,
                                   boolean_t overwrite);
PSL_STATIC int psl__ClearRegisterBit(int detChan, char *reg, int bit);
PSL_STATIC int psl__CheckBit(int detChan, char *reg, int bit, boolean_t *isSet);
PSL_STATIC int psl__ClearBuffer(int detChan, char buf, boolean_t waitForEmpty);
PSL_STATIC int psl__GetBufferFull(int detChan, char buf, boolean_t *is_full);
PSL_STATIC int psl__GetBuffer(int detChan, char buf, unsigned long *data,
                              XiaDefaults *defs, Module *m);

/* DSP Parameter Data Types */
PSL_STATIC int psl__GetParamValues(int detChan, void *value);

/* Firmware */
PSL_STATIC int psl__GetDSPName(int modChan, double pt, FirmwareSet *fs,
                               char *detType, char *name, char *rawName);
PSL_STATIC int psl__GetFiPPIName(int modChan, double pt, FirmwareSet *fs,
                                 char *detType, char *name, char *rawName);
PSL_STATIC int psl__DownloadFiPPIA(int detChan, char *file, char *rawFile,
                                   Module *m);
PSL_STATIC int psl__DownloadFiPPIADSPNoWake(int detChan, char *file,
                                            char *rawFile, Module *m);
PSL_STATIC int psl__DownloadDSP(int detChan, char *file, char *rawFile,
                                Module *m);

/* Board operations */
PSL_STATIC int psl__Apply(int detChan, char *name, XiaDefaults *defs,
                          void *value);
PSL_STATIC int psl__GetMCR(int detChan, char *name, XiaDefaults *defs,
                           void *value);
PSL_STATIC int psl__GetMFR(int detChan, char *name, XiaDefaults *defs,
                           void *value);
PSL_STATIC int psl__GetCSR(int detChan, char *name, XiaDefaults *defs,
                           void *value);
PSL_STATIC int psl__GetCVR(int detChan, char *name, XiaDefaults *defs,
                           void *value);
PSL_STATIC int psl__GetSVR(int detChan, char *name, XiaDefaults *defs,
                           void *value);
PSL_STATIC int psl__SetBufferDone(int detChan, char *name, XiaDefaults *defs,
                                  void *value);
PSL_STATIC int psl__MapPixelNext(int detChan, char *name, XiaDefaults *defs,
                                 void *value);
PSL_STATIC int psl__GetSerialNumber(int detChan, char *name, XiaDefaults *defs,
                                    void *value);
PSL_STATIC int psl__SetSerialNumber(int detChan, char *name, XiaDefaults *defs,
                                    void *value);
PSL_STATIC int psl__GetTemperature(int detChan, char *name, XiaDefaults *defs,
                                   void *value);
PSL_STATIC int psl__GetUSBVersion(int detChan, char *name, XiaDefaults *defs,
                                void *value);
PSL_STATIC int psl__GetBoardFeatures(int detChan, char *name, XiaDefaults *defs,
                                   void *value);

/* Gain Operations */
PSL_STATIC int psl__GainCalibrate(int detChan, Detector *det,
                                  int modChan, Module *m, XiaDefaults *defs,
                                  void *value);

/* Special runs */
PSL_STATIC int psl__DoTrace(int detChan, short type, double *info, boolean_t isDebug);
PSL_STATIC int psl__CalibrateRcTime(int detChan, void *value, XiaDefaults *defs);
PSL_STATIC int psl__AdjustOffsets(int detChan, void *value, XiaDefaults *defs);

/* Special Run data */
PSL_STATIC int psl__GetADCTraceLen(int detChan, void *value, XiaDefaults *defs);
PSL_STATIC int psl__GetADCTrace(int detChan, void *value, XiaDefaults *defs);
PSL_STATIC int psl__GetBaseHistoryLen(int detChan, void *value, XiaDefaults *defs);

/* Run data */
PSL_STATIC int psl__GetMCALength(int detChan, void *value, XiaDefaults *defs,
                                 Module *m);
PSL_STATIC int psl__GetMCA(int detChan, void *value, XiaDefaults *defs,
                           Module *m);
PSL_STATIC int psl__GetBaselineLength(int detChan, void *value,
                                      XiaDefaults *defs, Module *m);
PSL_STATIC int psl__GetBaseline(int detChan, void *value, XiaDefaults *defs,
                                Module *m);
PSL_STATIC int psl__GetRunActive(int detChan, void *value, XiaDefaults *defs,
                                 Module *m);
PSL_STATIC int psl__GetRealtime(int detChan, void *value, XiaDefaults *defs,
                                Module *m);
PSL_STATIC int psl__GetTotalEvents(int detChan, void *value, XiaDefaults *defs,
                                   Module *m);
PSL_STATIC int psl__GetMCAEvents(int detChan, void *value, XiaDefaults *defs,
                                 Module *m);
PSL_STATIC int psl__GetTLivetime(int detChan, void *value, XiaDefaults *defs,
                                 Module *m);
PSL_STATIC int psl__GetELivetime(int detChan, void *value, XiaDefaults *defs,
                                 Module *m);
PSL_STATIC int psl__GetICR(int detChan, void *value, XiaDefaults *defs,
                           Module *m);
PSL_STATIC int psl__GetOCR(int detChan, void *value, XiaDefaults *defs,
                           Module *m);
PSL_STATIC int psl__GetModuleStatistics(int detChan, void *value,
                                        XiaDefaults * defs, Module *m);
PSL_STATIC int psl__GetModuleStatistics2(int detChan, void *value,
                                         XiaDefaults * defs, Module *m);
PSL_STATIC int psl__GetSCALength(int detChan, void *value, XiaDefaults *defs,
                                 Module *m);
PSL_STATIC int psl__GetMaxSCALength(int detChan, void *value, XiaDefaults *defs,
                                    Module *m);
PSL_STATIC int psl__GetSCAData(int detChan, void *value, XiaDefaults *defs,
                               Module *m);
PSL_STATIC int psl__GetBufferFullA(int detChan, void *value, XiaDefaults *defs,
                                   Module *m);
PSL_STATIC int psl__GetBufferFullB(int detChan, void *value, XiaDefaults *defs,
                                   Module *m);
PSL_STATIC int psl__GetBufferLen(int detChan, void *value, XiaDefaults *defs,
                                 Module *m);
PSL_STATIC int psl__GetBufferA(int detChan, void *value, XiaDefaults *defs,
                               Module *m);
PSL_STATIC int psl__GetBufferB(int detChan, void *value, XiaDefaults *defs,
                               Module *m);
PSL_STATIC int psl__GetCurrentPixel(int detChan, void *value, XiaDefaults *defs,
                                    Module *m);
PSL_STATIC int psl__GetBufferOverrun(int detChan, void *value,
                                     XiaDefaults *defs, Module *m);
PSL_STATIC int psl__GetModuleMCA(int detChan, void *value,
                                 XiaDefaults * defs, Module *m);
PSL_STATIC int psl__GetTriggers(int detChan, void *value, XiaDefaults *defs,
                                Module *m);
PSL_STATIC int psl__GetUnderflows(int detChan, void *value, XiaDefaults *defs,
                                  Module *m);
PSL_STATIC int psl__GetOverflows(int detChan, void *value, XiaDefaults *defs,
                                 Module *m);
PSL_STATIC int psl__GetListBufferLenA(int detChan, void *value,
                                      XiaDefaults *defs, Module *m);
PSL_STATIC int psl__GetListBufferLenB(int detChan, void *value,
                                      XiaDefaults *defs, Module *m);
PSL_STATIC int psl__GetListBufferLen(int detChan, char buf, unsigned long *len);

/* Acquisition value getters/setters */
PSL_STATIC int psl__GetPeakingTime(int detChan, void *value, XiaDefaults *defs);
PSL_STATIC int psl__SetPeakingTime(int detChan, int modChan, char *name,
                                   void *value, char *detType,
                                   XiaDefaults *defs, Module *m,
                                   Detector *det, FirmwareSet *fs);
PSL_STATIC int psl__SetMinGapTime(int detChan, int modChan, char *name,
                                  void *value, char *detType,
                                  XiaDefaults *defs, Module *m,
                                  Detector *det, FirmwareSet *fs);
PSL_STATIC int psl__SetDynamicRng(int detChan, int modChan, char *name,
                                  void *value, char *detType,
                                  XiaDefaults *defs, Module *m, Detector *det,
                                  FirmwareSet *fs);
PSL_STATIC int psl__SetCalibEV(int detChan, int modChan, char *name,
                               void *value, char *detType,
                               XiaDefaults *defs, Module *m, Detector *det,
                               FirmwareSet *fs);
PSL_STATIC int psl__SetMCABinWidth(int detChan, int modChan, char *name,
                                   void *value, char *detType,
                                   XiaDefaults *defs, Module *m, Detector *det,
                                   FirmwareSet *fs);
PSL_STATIC int psl__SetTThresh(int detChan, int modChan, char *name,
                               void *value, char *detType,
                               XiaDefaults *defs, Module *m, Detector *det,
                               FirmwareSet *fs);
PSL_STATIC int psl__SetBThresh(int detChan, int modChan, char *name,
                               void *value, char *detType,
                               XiaDefaults *defs, Module *m, Detector *det,
                               FirmwareSet *fs);
PSL_STATIC int psl__SetEThresh(int detChan, int modChan, char *name,
                               void *value, char *detType,
                               XiaDefaults *defs, Module *m, Detector *det,
                               FirmwareSet *fs);
PSL_STATIC int psl__SetPresetType(int detChan, int modChan, char *name,
                                  void *value, char *detType, XiaDefaults *defs,
                                  Module *m, Detector *det, FirmwareSet *fs);
PSL_STATIC int psl__SetPresetValue(int detChan, int modChan, char *name,
                                   void *value, char *detType, XiaDefaults *defs,
                                   Module *m, Detector *det, FirmwareSet *fs);
PSL_STATIC int psl__SetPolarity(int detChan, int modChan, char *name,
                                void *value, char *detType,
                                XiaDefaults *defs, Module *m, Detector *det,
                                FirmwareSet *fs);
PSL_STATIC int psl__SynchPolarity(int detChan, int det_chan, Module *m,
                                  Detector *det, XiaDefaults *defs);
PSL_STATIC int psl__SetResetDelay(int detChan, int modChan, char *name,
                                  void *value, char *detType,
                                  XiaDefaults *defs, Module *m, Detector *det,
                                  FirmwareSet *fs);
PSL_STATIC int psl__SynchResetDelay(int detChan, int det_chan, Module *m,
                                    Detector *det, XiaDefaults *defs);
PSL_STATIC int psl__SetDecayTime(int detChan, int modChan, char *name,
                                 void *value, char *detType,
                                 XiaDefaults *defs, Module *m, Detector *det,
                                 FirmwareSet *fs);
PSL_STATIC int psl__SynchDecayTime(int detChan, int det_chan, Module *m,
                                   Detector *det, XiaDefaults *defs);
PSL_STATIC int psl__SetPreampGain(int detChan, int modChan, char *name,
                                  void *value, char *detType,
                                  XiaDefaults *defs, Module *m, Detector *det,
                                  FirmwareSet *fs);
PSL_STATIC int psl__SynchPreampGain(int detChan, int det_chan, Module *m,
                                    Detector *det, XiaDefaults *defs);
PSL_STATIC int psl__SetNumMCAChans(int detChan, int modChan, char *name,
                                   void *value, char *detType,
                                   XiaDefaults *defs, Module *m, Detector *det,
                                   FirmwareSet *fs);
PSL_STATIC int psl__SetGapTime(int detChan, int modChan, char *name, void *value,
                               char *detType, XiaDefaults *defs, Module *m,
                               Detector *det, FirmwareSet *fs);
PSL_STATIC int psl__GetGapTime(int detChan, void *value, XiaDefaults *defs);
PSL_STATIC int psl__SetTrigPeakingTime(int detChan, int modChan, char *name,
                                       void *value,
                                       char *detType, XiaDefaults *defs, Module *m,
                                       Detector *det, FirmwareSet *fs);
PSL_STATIC int psl__SetTrigGapTime(int detChan, int modChan, char *name, void *value,
                                   char *detType, XiaDefaults *defs, Module *m,
                                   Detector *det, FirmwareSet *fs);
PSL_STATIC int psl__SetMaxWidth(int detChan, int modChan, char *name,
                                void *value, char *detType,
                                XiaDefaults *defs, Module *m,
                                Detector *det, FirmwareSet *fs);
PSL_STATIC int psl__SetBaseAvg(int detChan, int modChan, char *name, void *value,
                               char *detType, XiaDefaults *defs, Module *m,
                               Detector *det, FirmwareSet *fs);
PSL_STATIC int psl__SetPreampType(int detChan, int modChan, char *name,
                                  void *value, char *detType,
                                  XiaDefaults *defs, Module *m, Detector *det,
                                  FirmwareSet *fs);
PSL_STATIC int psl__SynchPreampType(int detChan, int det_chan, Module *m,
                                    Detector *det, XiaDefaults *defs);
PSL_STATIC int psl__SetPeakSampleOffset(int detChan, int modChan, char *name,
                                        void *value, char *detType,
                                        XiaDefaults *defs, Module *m,
                                        Detector *det, FirmwareSet *fs);
PSL_STATIC int psl__SetPeakIntervalOffset(int detChan, int modChan, char *name,
                                          void *value, char *detType,
                                          XiaDefaults *defs, Module *m,
                                          Detector *det, FirmwareSet *fs);
PSL_STATIC int psl__SetNumberSCAs(int detChan, int modChan, char *name,
                                  void *value, char *detType, XiaDefaults *defs,
                                  Module *m, Detector *det, FirmwareSet *fs);
PSL_STATIC int psl__SetSCA(int detChan, int modChan, char *name,
                           void *value, char *detType, XiaDefaults *defs,
                           Module *m, Detector *det, FirmwareSet *fs);
PSL_STATIC int psl__SetNumMapPixels(int detChan, int modChan, char *name,
                                    void *value, char *detType,
                                    XiaDefaults *defs, Module *m,
                                    Detector *det, FirmwareSet *fs);
PSL_STATIC int psl__SetNumMapPtsBuffer(int detChan, int modChan, char *name,
                                       void *value, char *detType,
                                       XiaDefaults *defs, Module *m,
                                       Detector *det, FirmwareSet *fs);
PSL_STATIC int psl__GetNumMapPtsBuffer(int detChan, void *value,
                                       XiaDefaults *defs);
PSL_STATIC int psl__SetMappingMode(int detChan, int modChan, char *name,
                                   void *value, char *detType,
                                   XiaDefaults *defs, Module *m,
                                   Detector *det, FirmwareSet *fs);
PSL_STATIC int psl__SetPixelAdvanceMode(int detChan, int modChan, char *name,
                                        void *value, char *detType,
                                        XiaDefaults *defs, Module *m,
                                        Detector *det, FirmwareSet *fs);
PSL_STATIC int psl__SetInputLogicPolarity(int detChan, int modChan, char *name,
                                          void *value, char *detType,
                                          XiaDefaults *defs, Module *m,
                                          Detector *det, FirmwareSet *fs);
PSL_STATIC int psl__SetSyncCount(int detChan, int modChan, char *name,
                                 void *value, char *detType,
                                 XiaDefaults *defs, Module *m,
                                 Detector *det, FirmwareSet *fs);
PSL_STATIC int psl__SetGateIgnore(int detChan, int modChan, char *name,
                                  void *value, char *detType,
                                  XiaDefaults *defs, Module *m,
                                  Detector *det, FirmwareSet *fs);
PSL_STATIC int psl__SetDeltaTemp(int detChan, int modChan, char *name,
                                 void *value, char *detType, XiaDefaults *defs,
                                 Module *m, Detector *det, FirmwareSet *fs);
PSL_STATIC int psl__SetTempCorrection(int detChan, int modChan, char *name,
                                      void *value, char *detType, XiaDefaults *defs,
                                      Module *m, Detector *det, FirmwareSet *fs);
PSL_STATIC int psl__SetPeakMode(int detChan, int modChan, char *name,
                                void *value, char *detType,
                                XiaDefaults *defs, Module *m,
                                Detector *det, FirmwareSet *fs);
PSL_STATIC int psl__SetListModeVariant(int detChan, int modChan, char *name,
                                void *value, char *detType,
                                XiaDefaults *defs, Module *m,
                                Detector *det, FirmwareSet *fs);
PSL_STATIC int psl__SetTriggerOutput(int detChan, int modChan, char *name,
                                     void *value, char *detType,
                                     XiaDefaults *defs, Module *m,
                                     Detector *det, FirmwareSet *fs);
PSL_STATIC int psl__SetLivetimeOuput(int detChan, int modChan, char *name,
                                     void *value, char *detType,
                                     XiaDefaults *defs, Module *m,
                                     Detector *det, FirmwareSet *fs);
PSL_STATIC int psl__GetCalibratedDac(int detChan, void *value,
                                     XiaDefaults *defs);
PSL_STATIC int psl__SetCalibratedDac(int detChan, int modChan, char *name,
                                     void *value, char *detType,
                                     XiaDefaults *defs, Module *m,
                                     Detector *det, FirmwareSet *fs);
PSL_STATIC int psl__GetCalibratedGain(int detChan, void *value,
                                      XiaDefaults *defs);
PSL_STATIC int psl__SetCalibratedGain(int detChan, int modChan, char *name,
                                      void *value, char *detType,
                                      XiaDefaults *defs, Module *m,
                                      Detector *det, FirmwareSet *fs);
PSL_STATIC int psl__GetGainSlope(int detChan, void *value,
                                 XiaDefaults *defs);
PSL_STATIC int psl__SetGainSlope(int detChan, int modChan, char *name,
                                 void *value, char *detType,
                                 XiaDefaults *defs, Module *m,
                                 Detector *det, FirmwareSet *fs);
PSL_STATIC int psl__GetCalibratedChecksum(int detChan, void *value,
                                          XiaDefaults *defs);
PSL_STATIC int psl__SetCalibratedChecksum(int detChan, int modChan, char *name,
                                          void *value, char *detType,
                                          XiaDefaults *defs, Module *m,
                                          Detector *det, FirmwareSet *fs);
PSL_STATIC int psl__GetInputAttenuation(int detChan, void *value,
                                          XiaDefaults *defs);
PSL_STATIC int psl__SetInputAttenuation(int detChan, int modChan, char *name,
                                          void *value, char *detType,
                                          XiaDefaults *defs, Module *m,
                                          Detector *det, FirmwareSet *fs);
PSL_STATIC int psl__GetInputTermination(int detChan, void *value,
                                          XiaDefaults *defs);
PSL_STATIC int psl__SetInputTermination(int detChan, int modChan, char *name,
                                          void *value, char *detType,
                                          XiaDefaults *defs, Module *m,
                                          Detector *det, FirmwareSet *fs);
PSL_STATIC int psl__SetRcTimeContstant(int detChan, int modChan, char *name,
                                          void *value, char *detType,
                                          XiaDefaults *defs, Module *m,
                                          Detector *det, FirmwareSet *fs);
PSL_STATIC int psl__GetRcTime(int detChan, void *value, XiaDefaults *defs);
PSL_STATIC int psl__SetRcTime(int detChan, int modChan, char *name,
                                 void *value, char *detType,
                                 XiaDefaults *defs, Module *m, Detector *det,
                                 FirmwareSet *fs);
PSL_STATIC int psl__SetTriggerType(int detChan, int modChan, char *name,
                                 void *value, char *detType,
                                 XiaDefaults *defs, Module *m, Detector *det,
                                 FirmwareSet *fs);
PSL_STATIC int psl__SetTriggerPosition(int detChan, int modChan, char *name,
                                 void *value, char *detType,
                                 XiaDefaults *defs, Module *m, Detector *det,
                                 FirmwareSet *fs);
PSL_STATIC int psl__SetAdcOffset(int detChan, int modChan, char *name,
                                 void *value, char *detType,
                                 XiaDefaults *defs, Module *m, Detector *det,
                                 FirmwareSet *fs);
PSL_STATIC int psl__SetOffsetDac(int detChan, int modChan, char *name,
                                 void *value, char *detType,
                                 XiaDefaults *defs, Module *m, Detector *det,
                                 FirmwareSet *fs);
PSL_STATIC int psl__SetBaselineFactor(int detChan, int modChan, char *name,
                                 void *value, char *detType,
                                 XiaDefaults *defs, Module *m, Detector *det,
                                 FirmwareSet *fs);

/* These are the DSP parameter data types for pslGetParamData(). */
static ParamData_t PARAM_DATA[] =
{
    {   "values", psl__GetParamValues
    },
};

/* These are the allowed firmware types to download */
static FirmwareDownloader_t FIRMWARE[] =
{
    {   "fippi_a",             psl__DownloadFiPPIA
    },
    { "fippi_a_dsp_no_wake", psl__DownloadFiPPIADSPNoWake },
    { "dsp",                 psl__DownloadDSP },
};

/* These are the allowed special runs */
static SpecialRun specialRun[] =
{
    { "calibrate_rc_time",      psl__CalibrateRcTime },
    { "adjust_offsets",         psl__AdjustOffsets},
};

/* These are the allowed special run data types */
static SpecialRunData SPECIAL_RUN_DATA[] =
{
    {   "adc_trace_length",         psl__GetADCTraceLen
    },
    { "adc_trace",                psl__GetADCTrace },
    { "baseline_history_length",  psl__GetBaseHistoryLen },
    { "baseline_history",         psl__GetADCTrace },
};

/* These are the allowed board operations for this hardware */
static  BoardOperation boardOps[] =
{
    {   "apply",              psl__Apply
    },
    { "buffer_done",        psl__SetBufferDone },
    { "mapping_pixel_next", psl__MapPixelNext },
    { "get_mcr",            psl__GetMCR },
    { "get_mfr",            psl__GetMFR },
    { "get_csr",            psl__GetCSR },
    { "get_cvr",            psl__GetCVR },
    { "get_svr",            psl__GetSVR },
    { "get_serial_number",  psl__GetSerialNumber },
    { "set_serial_number",  psl__SetSerialNumber },
    { "get_temperature",    psl__GetTemperature },
    { "get_usb_version",    psl__GetUSBVersion},
    { "get_board_features", psl__GetBoardFeatures},
};

/* These are the allowed gain operations for this hardware */
static GainOperation gainOps[] =
{
    {"calibrate",             psl__GainCalibrate},
};

/* These are the allowed run data types */
static RunData runData[] =
{
    { "mca_length",          psl__GetMCALength },
    { "mca",                 psl__GetMCA },
    { "baseline_length",     psl__GetBaselineLength },
    { "baseline",            psl__GetBaseline },
    { "run_active",          psl__GetRunActive },
    { "runtime",             psl__GetRealtime },
    { "realtime",            psl__GetRealtime },
    { "events_in_run",       psl__GetTotalEvents },
    { "trigger_livetime",    psl__GetTLivetime },
    { "input_count_rate",    psl__GetICR },
    { "output_count_rate",   psl__GetOCR },
    { "livetime",            psl__GetELivetime },
    { "module_statistics",   psl__GetModuleStatistics },
    { "sca_length",          psl__GetSCALength },
    { "max_sca_length",	     psl__GetMaxSCALength},
    { "sca",                 psl__GetSCAData },
    { "buffer_full_a",       psl__GetBufferFullA },
    { "buffer_full_b",       psl__GetBufferFullB },
    { "buffer_len",          psl__GetBufferLen },
    { "buffer_a",            psl__GetBufferA },
    { "buffer_b",            psl__GetBufferB },
    { "current_pixel",       psl__GetCurrentPixel },
    { "buffer_overrun",      psl__GetBufferOverrun },
    { "module_mca",          psl__GetModuleMCA },
    { "energy_livetime",     psl__GetELivetime },
    { "module_statistics_2", psl__GetModuleStatistics2 },
    { "triggers",            psl__GetTriggers },
    { "underflows",          psl__GetUnderflows },
    { "overflows",           psl__GetOverflows },
    { "list_buffer_len_a",   psl__GetListBufferLenA },
    { "list_buffer_len_b",   psl__GetListBufferLenB },
    { "total_output_events", psl__GetTotalEvents },
    { "mca_events",          psl__GetMCAEvents }
};

static AcquisitionValue_t ACQ_VALUES[] =
{
    {   "peaking_time", TRUE_, FALSE_, MERCURY_UPDATE_NEVER, 20.0,
        psl__SetPeakingTime,   psl__GetPeakingTime, NULL
    },

    {   "minimum_gap_time", TRUE_, FALSE_, MERCURY_UPDATE_NEVER, 0.060,
        psl__SetMinGapTime,    NULL, NULL
    },

    /* If you modify the default values for the calibration energy or the ADC
     * percent rule, be sure to update the dynamic range value as well.
     */
    {   "dynamic_range", TRUE_, FALSE_, MERCURY_UPDATE_NEVER, 47200.0,
        psl__SetDynamicRng,     NULL, NULL
    },

    {   "calibration_energy", TRUE_, FALSE_, MERCURY_UPDATE_NEVER, 5900.0,
        psl__SetCalibEV,        NULL, NULL
    },

    {   "mca_bin_width", TRUE_, FALSE_, MERCURY_UPDATE_NEVER, 10.0,
        psl__SetMCABinWidth,    NULL, NULL
    },

    {   "trigger_threshold", TRUE_, FALSE_, MERCURY_UPDATE_NEVER, 1000.0,
        psl__SetTThresh,        NULL, NULL
    },

    {   "baseline_threshold", TRUE_, FALSE_, MERCURY_UPDATE_NEVER, 1000.0,
        psl__SetBThresh,        NULL, NULL
    },

    {   "energy_threshold", TRUE_, FALSE_, MERCURY_UPDATE_NEVER, 0.0,
        psl__SetEThresh,        NULL, NULL
    },

    {   "preset_type",          TRUE_,  FALSE_, MERCURY_UPDATE_NEVER, 0.0,
        psl__SetPresetType,      NULL,  NULL
    },

    {   "preset_value",         TRUE_,  FALSE_, MERCURY_UPDATE_NEVER, 0.0,
        psl__SetPresetValue,     NULL,  NULL
    },

    {   "detector_polarity",    TRUE_, TRUE_, MERCURY_UPDATE_NEVER, 1.0,
        psl__SetPolarity,        NULL,  psl__SynchPolarity
    },

    {   "reset_delay",          TRUE_, TRUE_, MERCURY_UPDATE_NEVER, 10.0,
        psl__SetResetDelay,      NULL,  psl__SynchResetDelay
    },

    {   "decay_time",            TRUE_, TRUE_, MERCURY_UPDATE_NEVER, 10.0,
        psl__SetDecayTime,       NULL, psl__SynchDecayTime
    },

    {   "preamp_gain",          TRUE_, TRUE_, MERCURY_UPDATE_NEVER, 5.0,
        psl__SetPreampGain,      NULL,  psl__SynchPreampGain
    },

    {   "number_mca_channels",  TRUE_, FALSE_, MERCURY_UPDATE_NEVER, 2048.0,
        psl__SetNumMCAChans,     NULL, NULL
    },

    {   "gap_time",             TRUE_, FALSE_, MERCURY_UPDATE_NEVER, .240,
        psl__SetGapTime,         psl__GetGapTime,     NULL
    },

    {   "trigger_peaking_time", TRUE_, FALSE_, MERCURY_UPDATE_NEVER, .100,
        psl__SetTrigPeakingTime, NULL, NULL
    },

    {   "trigger_gap_time",     TRUE_, FALSE_, MERCURY_UPDATE_NEVER, 0.0,
        psl__SetTrigGapTime,     NULL, NULL
    },

    {   "maxwidth",              TRUE_, FALSE_, MERCURY_UPDATE_NEVER, 1.000,
        psl__SetMaxWidth,        NULL, NULL
    },

    {   "baseline_average",     TRUE_, FALSE_, MERCURY_UPDATE_NEVER, 256.0,
        psl__SetBaseAvg,         NULL, NULL
    },

    {   "preamp_type",           TRUE_, TRUE_, MERCURY_UPDATE_NEVER, 0.0,
        psl__SetPreampType,      NULL,    psl__SynchPreampType
    },

    {   "peak_sample_offset",    FALSE_, FALSE_, MERCURY_UPDATE_NEVER, 0.0,
        psl__SetPeakSampleOffset, NULL, NULL
    },

    {   "peak_interval_offset",  FALSE_, FALSE_, MERCURY_UPDATE_NEVER, 0.0,
        psl__SetPeakIntervalOffset, NULL, NULL
    },

    {   "number_of_scas",       TRUE_, FALSE_, MERCURY_UPDATE_NEVER, 0.0,
        psl__SetNumberSCAs,      NULL, NULL
    },

    {   "sca",                  FALSE_, FALSE_, MERCURY_UPDATE_NEVER, 0.0,
        psl__SetSCA,             NULL, NULL
    },

    /* Due to the use of STRNEQ() in pslSetAcquisitionValues(),
     * num_map_pixels_per_buffer must be listed before num_map_pixels.
     */
    {   "num_map_pixels_per_buffer", FALSE_, FALSE_, MERCURY_UPDATE_MAPPING, 0.0,
        psl__SetNumMapPtsBuffer, psl__GetNumMapPtsBuffer, NULL
    },

    {   "num_map_pixels",       FALSE_, FALSE_, MERCURY_UPDATE_MAPPING, 0.0,
        psl__SetNumMapPixels,    NULL, NULL
    },

    {   "mapping_mode",          TRUE_, FALSE_, MERCURY_UPDATE_NEVER, 0.0,
        psl__SetMappingMode,     NULL, NULL
    } ,

    {   "pixel_advance_mode",    FALSE_, FALSE_, MERCURY_UPDATE_MAPPING, 0.0,
        psl__SetPixelAdvanceMode, NULL, NULL
    },

    {   "input_logic_polarity",  TRUE_, FALSE_,
        MERCURY_UPDATE_MAPPING | MERCURY_UPDATE_MCA, 0.0,
        psl__SetInputLogicPolarity, NULL, NULL
    },

    {   "sync_count",            FALSE_, FALSE_, MERCURY_UPDATE_MAPPING, 0.0,
        psl__SetSyncCount,       NULL, NULL
    },

    {   "gate_ignore",           TRUE_, FALSE_,
        MERCURY_UPDATE_MAPPING | MERCURY_UPDATE_MCA, 0.0,
        psl__SetGateIgnore,      NULL, NULL
    },

    {   "delta_temp",             TRUE_, FALSE_, MERCURY_UPDATE_NEVER, 0.5,
        psl__SetDeltaTemp,         NULL,     NULL
    },

    {   "temp_correction",        TRUE_, FALSE_, MERCURY_UPDATE_NEVER, 0.0,
        psl__SetTempCorrection,         NULL,     NULL
    },

    {   "peak_mode",           TRUE_, FALSE_, MERCURY_UPDATE_NEVER,  0.0,
        psl__SetPeakMode,      NULL, NULL
    },

    {   "trigger_output",           TRUE_, FALSE_, MERCURY_UPDATE_NEVER,  0.0,
        psl__SetTriggerOutput,      NULL, NULL
    },

    {   "livetime_output",          TRUE_, FALSE_, MERCURY_UPDATE_NEVER,  0.0,
        psl__SetLivetimeOuput,      NULL, NULL
    },

    {   "calibrated_gain",          TRUE_, FALSE_, MERCURY_UPDATE_NEVER,  0.0,
        psl__SetCalibratedGain,      psl__GetCalibratedGain, NULL
    },

    {   "calibrated_dac",          TRUE_, FALSE_, MERCURY_UPDATE_NEVER,  0.0,
        psl__SetCalibratedDac,      psl__GetCalibratedDac, NULL
    },

    {   "calibrated_checksum",          TRUE_, FALSE_, MERCURY_UPDATE_NEVER,  0.0,
        psl__SetCalibratedChecksum,      psl__GetCalibratedChecksum, NULL
    },

    {   "gain_slope",           TRUE_, FALSE_, MERCURY_UPDATE_NEVER,  0.0,
        psl__SetGainSlope,      psl__GetGainSlope, NULL
    },

    {   "input_attenuation",          TRUE_, FALSE_, MERCURY_UPDATE_NEVER,  0.0,
        psl__SetInputAttenuation,      psl__GetInputAttenuation, NULL
    },

    {   "input_termination",          TRUE_, FALSE_, MERCURY_UPDATE_NEVER,  0.0,
        psl__SetInputTermination,      psl__GetInputTermination, NULL
    },

    {   "rc_time_constant",          TRUE_, FALSE_, MERCURY_UPDATE_NEVER,  0.0,
        psl__SetRcTimeContstant,      NULL, NULL
    },

    {   "rc_time",                  TRUE_, FALSE_, MERCURY_UPDATE_NEVER, 10.0,
        psl__SetRcTime,             psl__GetRcTime, NULL
    },

    {   "trace_trigger_type",       TRUE_, FALSE_, MERCURY_UPDATE_NEVER, 0.0,
        psl__SetTriggerType,         NULL, NULL
    },

    {   "trace_trigger_position",   TRUE_, FALSE_, MERCURY_UPDATE_NEVER, 0.0,
        psl__SetTriggerPosition,      NULL, NULL
    },

    {   "adc_offset",               TRUE_, FALSE_, MERCURY_UPDATE_NEVER, 0.0,
        psl__SetAdcOffset,          NULL, NULL
    },

    {   "offset_dac",               TRUE_, FALSE_, MERCURY_UPDATE_NEVER, 0.0,
        psl__SetOffsetDac,          NULL, NULL
    },

    {   "baseline_factor",          TRUE_, FALSE_, MERCURY_UPDATE_NEVER, 0.0,
        psl__SetBaselineFactor,     NULL, NULL
    },

    {   "list_mode_variant",        TRUE_, FALSE_, MERCURY_UPDATE_MAPPING, 0.0,
       psl__SetListModeVariant,     NULL, NULL
    },
};


#define SCA_LIMIT_STR_LEN 3
#define DATA_MEMORY_STR_LEN 18

/* These are allowed in old ini files but not from the API. */
static const char* REMOVED_ACQ_VALUES[] = {
    "adc_percent_rule"
};

/*
 * Returns true if the given name is in the removed acquisition values list.
 */
PSL_STATIC boolean_t psl__AcqRemoved(const char *name)
{
    size_t i;

    for (i = 0; i < sizeof(REMOVED_ACQ_VALUES) / sizeof(REMOVED_ACQ_VALUES[0]); i++) {
        if (STREQ(name, REMOVED_ACQ_VALUES[i]))
            return TRUE_;
    }

    return FALSE_;
}

/*
 * Initializes the PSL functions for the Mercury hardware.
 */
PSL_EXPORT int PSL_API mercury_PSLInit(PSLFuncs *funcs)
{
    ASSERT(funcs != NULL);


    funcs->validateDefaults     = pslValidateDefaults;
    funcs->validateModule       = pslValidateModule;
    funcs->downloadFirmware     = pslDownloadFirmware;
    funcs->setAcquisitionValues = pslSetAcquisitionValues;
    funcs->getAcquisitionValues = pslGetAcquisitionValues;
    funcs->gainOperation        = pslGainOperation;
    funcs->gainCalibrate        = pslGainCalibrate;
    funcs->startRun             = pslStartRun;
    funcs->stopRun              = pslStopRun;
    funcs->getRunData          = pslGetRunData;
    funcs->doSpecialRun        = pslDoSpecialRun;
    funcs->getSpecialRunData   = pslGetSpecialRunData;
    funcs->getDefaultAlias     = pslGetDefaultAlias;
    funcs->getParameter        = pslGetParameter;
    funcs->setParameter        = pslSetParameter;
    funcs->moduleSetup            = pslModuleSetup;
    funcs->userSetup            = pslUserSetup;
    funcs->getNumDefaults       = pslGetNumDefaults;
    funcs->getNumParams         = pslGetNumParams;
    funcs->getParamData         = pslGetParamData;
    funcs->getParamName         = pslGetParamName;
    funcs->boardOperation       = pslBoardOperation;
    funcs->freeSCAs             = pslDestroySCAs;
    funcs->unHook               = pslUnHook;

    mercury_psl_md_alloc = utils->funcs->dxp_md_alloc;
    mercury_psl_md_free  = utils->funcs->dxp_md_free;

    return XIA_SUCCESS;
}


/*
 * Validate that the module is correctly configured for the Mercury
 * hardware.
 */
PSL_STATIC int pslValidateModule(Module *module)
{
    UNUSED(module);

    return XIA_SUCCESS;
}


/*
 * Validate that the defined defaults are correct for the
 * hardware.
 */
PSL_STATIC int pslValidateDefaults(XiaDefaults *defaults)
{
    UNUSED(defaults);

    return XIA_SUCCESS;
}


/*
 * Download the specified firmware to the hardware.
 */
PSL_STATIC int pslDownloadFirmware(int detChan, char *type, char *file,
                                   Module *m, char *rawFile, XiaDefaults *defs)
{
    int status;
    unsigned long int i;

    UNUSED(defs);


    ASSERT(type    != NULL);
    ASSERT(file    != NULL);
    ASSERT(m       != NULL);
    ASSERT(rawFile != NULL);


    for (i = 0; i < N_ELEMS(FIRMWARE); i++) {
        if (STREQ(type, FIRMWARE[i].name)) {

            status = FIRMWARE[i].fn(detChan, file, rawFile, m);

            if (status != XIA_SUCCESS) {
                sprintf(info_string, "Error downloading firmware '%s' to detChan %d",
                        type, detChan);
                pslLogError("pslDownloadFirmware", info_string, status);
                return status;
            }

            return XIA_SUCCESS;
        }
    }

    sprintf(info_string, "Unknown firmware type '%s' requested for download to "
            "detChan %d", type, detChan);
    pslLogError("pslDownloadFirmware", info_string, XIA_UNKNOWN_FIRM);
    return XIA_UNKNOWN_FIRM;
}


/*
 * The master routine used to set the specified acquisition value.
 *
 * This routine decodes the specified acquisition value and dispatches
 * the appropriate information to the routine responsible for modifying
 * adding/updating the acquisition value.
 *
 * - detChan The channel to set the acquisition value for.
 * - name The name of the acquisition value to set.
 * - value Pointer to the acquisition value data.
 * - defaults Pointer to the defaults assigned to this detChan.
 * - firmwareSet Pointer to the firmware set assigned to this detChan.
 * - currentFirmware Pointer to the current firmware assigned to this detChan.
 * - detectorType The detector preamplifier type: "RC" or "RESET".
 * - detector Pointer to the detector associated with this detChan.
 * - detector_chan The preamplifier on the detector assigned to this detChan.
 * - m Pointer to the module detChan is a member of.
 * - modChan
 */
PSL_STATIC int pslSetAcquisitionValues(int detChan, char *name, void *value,
                                       XiaDefaults *defaults,
                                       FirmwareSet *firmwareSet,
                                       CurrentFirmware *currentFirmware,
                                       char *detectorType, Detector *detector,
                                       int detector_chan, Module *m,
                                       int modChan)
{
    unsigned long i;
    int status;
    int error_status;

    double original_value = 0.0;

    UNUSED(currentFirmware);
    UNUSED(detector_chan);


    ASSERT(name        != NULL);
    ASSERT(value       != NULL);
    ASSERT(defaults    != NULL);
    ASSERT(firmwareSet != NULL);


    for (i = 0; i < N_ELEMS(ACQ_VALUES); i++) {
        if (STRNEQ(name, ACQ_VALUES[i].name)) {

            /* Cache the current value in case we need to rollback. */
            status = pslGetDefault(name, (void *)&original_value, defaults);
            ASSERT(status == XIA_SUCCESS);

            status = ACQ_VALUES[i].setFN(detChan, modChan, name, value, detectorType,
                                         defaults, m, detector, firmwareSet);

            if (status != XIA_SUCCESS) {
                /* Some acquisition values have to call pslSetDefault() before they
                 * can process the acquisition value. So, to be safe, we need to
                 * roll the acquisition value back.
                 *
                 * NOTE: We don't try and reset the value completely by calling
                 * pslSetAcquisitionValues() again as that could cause infinite recursion.
                 * We need to make it clear in the manual that the user should try and
                 * set the value again after an error. In practice, this may not be
                 * enough and we may have to try and call pslSetAcquisitionValues()
                 * again, but we will cross that bridge when we get to it.
                 */
                error_status = pslSetDefault(name, (void *)&original_value, defaults);
                ASSERT(error_status == XIA_SUCCESS);

                sprintf(info_string, "'%s' reverted to %.3f", name, original_value);
                pslLogInfo("pslSetAcquisitionValues", info_string);

                sprintf(info_string, "Error setting '%s' to %.3f for detChan %d",
                        name, *((double *)value), detChan);
                pslLogError("pslSetAcquisitionValues", info_string, status);
                return status;
            }

            status = pslSetDefault(name, value, defaults);
            /* It is an "impossible" event for this routine to fail */
            ASSERT(status == XIA_SUCCESS);

            return XIA_SUCCESS;
        }
    }


    if (pslIsUpperCase(name)) {

        status = psl__UpdateRawParamAcqValue(detChan, name, value, defaults);

        if (status != XIA_SUCCESS) {

            sprintf(info_string, "Error setting %s as DSP parameter for detChan %d",
                    name, detChan);
            pslLogError("pslSetAcquisitionValues", info_string, status);
            return status;
        }

        return XIA_SUCCESS;

    } else if (psl__AcqRemoved(name)) {
        sprintf(info_string, "ignoring deprecated acquisition value: %s", name);
        pslLogWarning("pslSetAcquisitionValues", info_string);
        return XIA_SUCCESS;
    }

    sprintf(info_string, "Unknown acquisition value '%s' for detChan %d", name,
            detChan);
    pslLogError("pslSetAcquisitionValues", info_string, XIA_UNKNOWN_VALUE);
    return XIA_UNKNOWN_VALUE;



}


/*
 * Updates the acquisition value list with the raw DSP parameter
 * specified in name.
 */
PSL_STATIC int psl__UpdateRawParamAcqValue(int detChan, char *name,
                                           void *value, XiaDefaults *defs)
{
    int status;


    ASSERT(name);
    ASSERT(value);
    ASSERT(defs);


    status = pslSetDefault(name, value, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting '%s' to %0.3f as an acquisition "
                "value for detChan %d.", name, *((double *)value), detChan);
        pslLogError("psl__UpdateRawParamAcqValue", info_string, status);
        return status;
    }

    status = pslSetParameter(detChan, name, (parameter_t)*((double *)value));

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting the DSP parameter '%s' to %hu "
                "for detChan %d.", name, (parameter_t)*((double *)value),
                detChan);
        pslLogError("psl__UpdateRawParamAcqValue", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Gets the current value of the requested acquisition value.
 *
 * If the acquisition value needs to be fetched using a custom operation,
 * the getter is called and the value from the defaults list is overwritten
 * with the value from the getter function.
 */
PSL_STATIC int pslGetAcquisitionValues(int detChan, char *name, void *value,
                                       XiaDefaults *defaults)
{
    unsigned long i;
    int status;


    ASSERT(name     != NULL);
    ASSERT(value    != NULL);
    ASSERT(defaults != NULL);


    /* Preload the returned value with what is currently in the defaults list
     * and then allow the individual acquisition values to update it if necessary.
     */
    status = pslGetDefault(name, value, defaults);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting acquisition value '%s' for detChan %d",
                name, detChan);
        pslLogError("pslGetAcquisitionValues", info_string, status);
        return status;
    }

    for (i = 0; i < N_ELEMS(ACQ_VALUES); i++) {
        if (STRNEQ(name, ACQ_VALUES[i].name)) {

            /* If the get function is not impelement just use the current values */
            if (ACQ_VALUES[i].getFN == NULL) {
                return XIA_SUCCESS;
            }

            status = ACQ_VALUES[i].getFN(detChan, value, defaults);

            if (status != XIA_SUCCESS) {
                sprintf(info_string, "Error updating '%s' for detChan %d", name,
                        detChan);
                pslLogError("pslGetAcquisitionValues", info_string, status);
                return status;
            }

            /* By definition, these updated values are not meant to be written
             * to the defaults list since doing so may corrupt the intent of the
             * current setting. For instance, if you have an acquisition value
             * where -1.0 means "maximize", then you always want to keep it at -1.0
             * even though -1.0 doesn't tell the user what the actual value
             * on the hardware is.
             */
            return XIA_SUCCESS;
        }
    }

    if (psl__AcqRemoved(name)) {
        sprintf(info_string, "ignoring deprecated acquisition value: %s", name);
        pslLogWarning("pslSetAcquisitionValues", info_string);
        return XIA_SUCCESS;
    }

    sprintf(info_string, "Unknown acquisition value '%s' for detChan %d", name,
            detChan);
    pslLogError("pslGetAcquisitionValues", info_string, XIA_UNKNOWN_VALUE);
    return XIA_UNKNOWN_VALUE;}


/*
 * Wrapper function for pslGainCalibrate
 */
PSL_STATIC int psl__GainCalibrate(int detChan, Detector *det,
                                  int modChan, Module *m, XiaDefaults *defs,
                                  void *value)
{
    double *deltaGain  = (double *)value;
    return pslGainCalibrate(detChan, det, modChan, m, defs, *deltaGain);
}


/*
 * Calibrates the gain using the specified delta.
 *
 * This adjusts the preamplifier gain by the inverse of the specified delta
 * since G = C1 / (C2 * preampGain) where C1 and C2 are constants in this
 * context.
 */
PSL_STATIC int pslGainCalibrate(int detChan, Detector *det,
                                int modChan, Module *m, XiaDefaults *defs,
                                double deltaGain)
{
    int status;

    double preampGain = 0.0;
    double threshold = 0.0;

    parameter_t TEMPCORRECTION = 0;
    parameter_t SETGDAC = 0;
    parameter_t GAINDAC = 0;

    ASSERT(defs != NULL);

    if (deltaGain <= 0) {
        sprintf(info_string, "Invalid gain scale factor %0.3f for "
                "detChan %d", deltaGain, detChan);
        pslLogError("pslGainCalibrate", info_string, XIA_GAIN_SCALE);
        return XIA_GAIN_SCALE;
    }

    /* This acquisition value must exist */
    status = pslGetDefault("preamp_gain", (void *)&preampGain, defs);
    ASSERT(status == XIA_SUCCESS);

    /* TODO mercury-4 doesn't support TEMPCORRECTION yet */
    if (m->number_of_channels == 1)
    {
        status = pslGetParameter(detChan, "TEMPCORRECTION", &TEMPCORRECTION);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error getting TEMPCORRECTION for detChan %d",
                    detChan);
            pslLogError("pslGainCalibrate", info_string, status);
            return status;
        }

        /* If temperature correction is enabled, actual GAINDAC value is in SETGDAC
         * Adjust target preamp_gain to GAINDAC - SETGDAC ratio according to
         * calculations in psl__UpdateVariableGain
         */
        if (TEMPCORRECTION != MERCURY_TEMP_NO_CORRECTION) {

            status = pslGetParameter(detChan, "GAINDAC", &GAINDAC);
            if (status != XIA_SUCCESS) {
                sprintf(info_string, "Error getting GAINDAC for detChan %d", detChan);
                pslLogError("pslGainCalibrate", info_string, status);
                return status;
            }

            status = pslGetParameter(detChan, "SETGDAC", &SETGDAC);
            if (status != XIA_SUCCESS) {
                sprintf(info_string, "Error getting SETGDAC for detChan %d", detChan);
                pslLogError("pslGainCalibrate", info_string, status);
                return status;
            }

            preampGain *= pow(10, (double) (GAINDAC - SETGDAC) / 32768.);

        }
    }

    preampGain *= 1.0 / deltaGain;

    /* Scale the default threshold here so that
     * the THRESHOLD parameter can be updated in psl__SetPreampGain
     */
    status = pslGetDefault("trigger_threshold", (void *)&threshold, defs);
    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting the trigger threshold for "
                "detChan %d", detChan);
        pslLogError("pslGainCalibrate", info_string, status);
        return status;
    }

    threshold *= deltaGain;
    status = pslSetDefault("trigger_threshold", (void *)&threshold, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting the trigger threshold to %0.3f for "
                "detChan %d", threshold, detChan);
        pslLogError("pslGainCalibrate", info_string, status);
        return status;
    }

    /* baseline threhold */
    status = pslGetDefault("baseline_threshold", (void *)&threshold, defs);
    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting the baseline threshold for "
                "detChan %d", detChan);
        pslLogError("pslGainCalibrate", info_string, status);
        return status;
    }

    threshold *= deltaGain;
    status = pslSetDefault("baseline_threshold", (void *)&threshold, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting the baseline threshold to %0.3f for "
                "detChan %d", threshold, detChan);
        pslLogError("pslGainCalibrate", info_string, status);
        return status;
    }

    /* energy threhold */
    status = pslGetDefault("energy_threshold", (void *)&threshold, defs);
    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting the energy threshold for "
                "detChan %d", detChan);
        pslLogError("pslGainCalibrate", info_string, status);
        return status;
    }

    threshold *= deltaGain;
    status = pslSetDefault("energy_threshold", (void *)&threshold, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting the energy threshold to %0.3f for "
                "detChan %d", threshold, detChan);
        pslLogError("pslGainCalibrate", info_string, status);
        return status;
    }

    /* This is the same routine that pslSetAcquisitionValues() uses to set
    * the acquisition value. We will also need to update the defaults since
    * pslSetAcquisitionValues() normally does that.
    */
    status = psl__SetPreampGain(detChan, modChan, NULL, (void *)&preampGain, "",
                                defs, m, det, NULL);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting the preamplifier gain to %0.3f for "
                "detChan %d", preampGain, detChan);
        pslLogError("pslGainCalibrate", info_string, status);
        return status;
    }

    status = pslSetDefault("preamp_gain", (void *)&preampGain, defs);
    ASSERT(status == XIA_SUCCESS);

    return XIA_SUCCESS;
}


/*
 * Starts a run on the specified channel.
 */
PSL_STATIC int pslStartRun(int detChan, unsigned short resume,
                           XiaDefaults *defaults, Module *m)
{
    int status;


    unsigned short ignored_gate   = 0;

    boolean_t isMapping = FALSE_;

    UNUSED(defaults);
    UNUSED(m);

    /* Only clear buffer if mapping mode firmware is running */
    status = psl__IsMapping(detChan, MAPPING_ANY, &isMapping);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error checking firmware type for detChan %d", detChan);
        pslLogError("pslStartRun", info_string, status);
        return status;
    }

    if (isMapping)
    {
        /* Initialize the mapping flag register. */
        status = psl__SetRegisterBit(detChan, "MFR", 12, TRUE_);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error initializing mapping registers for detChan '%d'",
                    detChan);
            pslLogError("pslStartRun", info_string, status);
            return status;
        }

        /* If using mapping mode firmware, we need to clear the buffers
         * before the run starts.
         */
        status = psl__ClearBuffer(detChan, 'a', TRUE_);

        /* Ignore an error that says we aren't using mapping mode firmware since
         * this check is always run.
         */
        if (status != XIA_SUCCESS && status != XIA_NO_MAPPING) {
            sprintf(info_string, "Error clearing buffer 'a' for detChan %d", detChan);
            pslLogError("pslStartRun", info_string, status);
            return status;
        }

        status = psl__ClearBuffer(detChan, 'b', TRUE_);

        if (status != XIA_SUCCESS && status != XIA_NO_MAPPING) {
            sprintf(info_string, "Error clearing buffer 'b' for detChan %d", detChan);
            pslLogError("pslStartRun", info_string, status);
            return status;
        }
    }

    status = dxp_start_one_run(&detChan, &ignored_gate, &resume);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error starting run on detChan = %d", detChan);
        pslLogError("pslStartRun", info_string, status);
        return status;
    }

    return XIA_SUCCESS;

}


/*
 * Stops a run on the specified channel.
 */
PSL_STATIC int pslStopRun(int detChan, Module *m)
{
    int status;

    UNUSED(m);


    status = dxp_stop_one_run(&detChan);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error stopping run on detChan = %d", detChan);
        pslLogError("pslStopRun", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Get the specified acquisition run data from the hardware.
 */
PSL_STATIC int pslGetRunData(int detChan, char *name, void *value,
                             XiaDefaults *defaults, Module *m)
{
    unsigned long i;
    int status;


    ASSERT(name     != NULL);
    ASSERT(value    != NULL);
    ASSERT(defaults != NULL);
    ASSERT(m        != NULL);


    if (STREQ(name, "livetime")) {
        pslLogWarning("pslGetRunData", "'livetime' is deprecated as a run data "
                      "type. Use 'trigger_livetime' or 'energy_livetime' "
                      "instead.");

    } else if (STREQ(name, "events_in_run")) {
        pslLogWarning("pslGetRunData", "'events_in_run' is deprecated as a run "
                      "data type. Use 'mca_events' or 'total_output_events' "
                      "instead.");
    }

    for (i = 0; i < N_ELEMS(runData); i++) {
        if (STREQ(name, runData[i].name)) {

            status = runData[i].fn(detChan, value, defaults, m);

            if (status != XIA_SUCCESS) {
                sprintf(info_string, "Error getting run data '%s' for detChan %d", name,
                        detChan);
                pslLogError("pslGetRunData", info_string, status);
                return status;
            }

            return XIA_SUCCESS;
        }
    }

    sprintf(info_string, "Unknown run data '%s' for detChan %d", name, detChan);
    pslLogError("pslGetRunData", info_string, XIA_BAD_NAME);

    return XIA_BAD_NAME;
}


/*
 * Performs the requested special run.
 */
PSL_STATIC int pslDoSpecialRun(int detChan, char *name, void *info,
                               XiaDefaults *defaults, Detector *detector,
                               int detector_chan)
{
    int status;
    int i;
    unsigned long int element_idx;

    boolean_t isDebug;
    boolean_t isMercuryOem = psl__IsMercuryOem(detChan);
    int traceTypesize;
    MercuryTraceType *traceTypeList;
    short TRACETYPE;

    UNUSED(info);
    UNUSED(detector);
    UNUSED(detector_chan);

    ASSERT(name != NULL);
    ASSERT(defaults != NULL);

    traceTypesize = isMercuryOem ?  N_ELEMS(traceTypesMercuryOem) : N_ELEMS(traceTypesMercury);
    traceTypeList = isMercuryOem ?  traceTypesMercuryOem : traceTypesMercury;

    /* Check for match in trace type */
    for (i = 0; i < traceTypesize; i++) {
        if (STREQ(traceTypeList[i].name, name)) {

            TRACETYPE = (short) traceTypeList[i].TRACETYPE;
            isDebug = (i == traceTypesize - 1);

            status = psl__DoTrace(detChan, TRACETYPE, (double *)info, isDebug);

            if (status != XIA_SUCCESS) {
                sprintf(info_string, "Error doing trace run '%s' type %hd on detChan %d",
                        name, TRACETYPE, detChan);
                pslLogError("pslDoSpecialRun", info_string, status);
                return status;
            }

            return XIA_SUCCESS;
        }
    }

    for (element_idx = 0; element_idx < N_ELEMS(specialRun); element_idx++) {
        if (STREQ(specialRun[element_idx].name, name)) {

            status = specialRun[element_idx].fn(detChan, info, defaults);

            if (status != XIA_SUCCESS) {
                sprintf(info_string, "Error doing special run '%s' on detChan %d",
                        name, detChan);
                pslLogError("pslDoSpecialRun", info_string, status);
                return status;
            }

            return XIA_SUCCESS;
        }
    }

    sprintf(info_string, "Unknown special run '%s' for detChan %d", name, detChan);
    pslLogError("pslDoSpecialRun", info_string, XIA_BAD_NAME);

    return XIA_BAD_NAME;

}


/*
 * Get the specified special run data from the hardware.
 */
PSL_STATIC int pslGetSpecialRunData(int detChan, char *name, void *value,
                                    XiaDefaults *defaults)
{
    int status;
    unsigned long int i;


    ASSERT(name     != NULL);
    ASSERT(value    != NULL);
    ASSERT(defaults != NULL);


    for (i = 0; i < N_ELEMS(SPECIAL_RUN_DATA); i++) {
        if (STREQ(SPECIAL_RUN_DATA[i].name, name)) {

            status = SPECIAL_RUN_DATA[i].fn(detChan, value, defaults);

            if (status != XIA_SUCCESS) {
                sprintf(info_string, "Error getting special run data '%s' for "
                        "detChan %d", name, detChan);
                pslLogError("pslGetSpecialRunData", info_string, status);
                return status;
            }

            return XIA_SUCCESS;
        }
    }

    sprintf(info_string, "Unknown special run data type '%s' for detChan %d",
            name, detChan);
    pslLogError("pslGetSpecialRunData", info_string, XIA_BAD_NAME);

    return XIA_BAD_NAME;
}


/*
 * Returns a list of the "default" defaults.
 */
PSL_STATIC int pslGetDefaultAlias(char *alias, char **names, double *values)
{
    unsigned long i;
    unsigned long def_idx;

    char *aliasName = "defaults_mercury";


    ASSERT(alias  != NULL);
    ASSERT(names  != NULL);
    ASSERT(values != NULL);


    for (i = 0, def_idx = 0; i < N_ELEMS(ACQ_VALUES); i++) {
        if (ACQ_VALUES[i].isDefault) {
            strcpy(names[def_idx], ACQ_VALUES[i].name);
            values[def_idx] = ACQ_VALUES[i].def;
            def_idx++;
        }
    }

    strcpy(alias, aliasName);

    return XIA_SUCCESS;
}


/*
 * Get the value of the specified DSP parameter from the hardware.
 */
PSL_STATIC int pslGetParameter(int detChan, const char *name,
                               unsigned short *value)
{
    int status;


    ASSERT(name != NULL);
    ASSERT(value != NULL);


    status = dxp_get_one_dspsymbol(&detChan, (char *)name, value);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error reading '%s' for detChan %d", name, detChan);
        pslLogError("pslGetParameter", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Set the specified DSP parameter on the hardware.
 */
PSL_STATIC int pslSetParameter(int detChan, const char *name,
                               unsigned short value)
{
    int status;


    ASSERT(name != NULL);


    status = dxp_set_one_dspsymbol(&detChan, (char *)name, &value);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error setting '%s' to %#hx for detChan %d",
                name, value, detChan);
        pslLogError("pslSetParameter", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Setup per-module settings, this is done after all the
 * acquisition values are set up.
 */
PSL_STATIC int pslModuleSetup(int detChan, XiaDefaults *defaults,
                              Module *m)
{
    int status;

    UNUSED(m);

    sprintf(info_string, "Applying per module setting for the module that "
            "includes detChan %d.", detChan);
    pslLogDebug("pslModuleSetup", info_string);

    status = psl__Apply(detChan, NULL, defaults, NULL);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error applying acquisition values for module "
                "that includes detChan %d", detChan);
        pslLogError("pslModuleSetup", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Sets all of the acquisition values to their initial setting and
 * configures the filter parameters.
 */
PSL_STATIC int pslUserSetup(int detChan, XiaDefaults *defaults,
                            FirmwareSet *firmwareSet,
                            CurrentFirmware *currentFirmware,
                            char *detectorType, Detector *detector,
                            int detector_chan, Module *m, int modChan)
{
    unsigned long i;
    int status;

    XiaDaqEntry *entry = NULL;


    ASSERT(defaults != NULL);


    entry = defaults->entry;
    ASSERT(entry != NULL);

    /* Some acquisition values require synchronization with another data
     * structure in the program prior to setting the initial acquisition
     * value.
     */
    for (i = 0; i < N_ELEMS(ACQ_VALUES); i++) {
        if (ACQ_VALUES[i].isSynch) {
            status = ACQ_VALUES[i].synchFN(detChan, detector_chan, m, detector,
                                           defaults);

            if (status != XIA_SUCCESS) {
                sprintf(info_string, "Error synchronizing '%s' for detChan %d",
                        ACQ_VALUES[i].name, detChan);
                pslLogError("pslUserSetup", info_string, status);
                return status;
            }
        }
    }

    while (entry != NULL) {

        /* Skip read-only acquisition values so we don't generate
         * warnings during startup.
         */
        if (STREQ(entry->name, "calibrated_gain") ||
                STREQ(entry->name, "calibrated_dac") ||
                STREQ(entry->name, "calibrated_checksum") ||
                STREQ(entry->name, "gain_slope")) {
            goto nextEntry;
        }

        status = pslSetAcquisitionValues(detChan, entry->name, (void *)&entry->data,
                                         defaults, firmwareSet, currentFirmware,
                                         detectorType, detector, detector_chan, m,
                                         modChan);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error setting '%s' to %0.3f for detChan %d",
                    entry->name, entry->data, detChan);
            pslLogError("pslUserSetup", info_string, status);
            return status;
        }

nextEntry:
        entry = entry->next;
    }

    return XIA_SUCCESS;
}


/*
 * Returns the number of "default" defaults.
 */
PSL_STATIC unsigned int pslGetNumDefaults(void)
{
    unsigned int i;

    unsigned int n;


    for (i = 0, n = 0; i < N_ELEMS(ACQ_VALUES); i++) {
        if (ACQ_VALUES[i].isDefault) {
            n++;
        }
    }

    return n;
}


/*
 * Get the number of DSP parameters defined for the given channel.
 */
PSL_STATIC int pslGetNumParams(int detChan, unsigned short *numParams)
{
    int status;


    ASSERT(numParams != NULL);

    status = dxp_max_symbols(&detChan, numParams);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error getting the number of DSP parameters for "
                "detChan %d", detChan);
        pslLogError("pslGetNumParams", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Returns the requested parameter data.
 */
PSL_STATIC int PSL_API pslGetParamData(int detChan, char *name, void *value)
{
    unsigned long i;
    int status;


    ASSERT(name != NULL);
    ASSERT(value != NULL);


    for (i = 0; i < N_ELEMS(PARAM_DATA); i++) {
        if (STREQ(name, PARAM_DATA[i].name)) {

            status = PARAM_DATA[i].fn(detChan, value);

            if (status != XIA_SUCCESS) {
                sprintf(info_string, "Error getting parameter data '%s' for detChan %d",
                        PARAM_DATA[i].name, detChan);
                pslLogError("pslGetParamData", info_string, status);
                return status;
            }

            return XIA_SUCCESS;
        }
    }

    sprintf(info_string, "Unknown parameter data type '%s' for detChan %d",
            name, detChan);
    pslLogError("pslGetParamData", info_string, XIA_UNKNOWN_PARAM_DATA);
    return XIA_UNKNOWN_PARAM_DATA;
}


/*
 * Helper routine for VB-type applications that are calling Handel.
 * Returns the name of the parameter listed at index. Ordinarily, one would
 * call pslGetParams(), which returns an array of all of the DSP parameter
 * names, but not all languages can interface to that type of an argument.
 *
 * name must be at least MAXSYMBOL_LEN characters long.
 */
PSL_STATIC int pslGetParamName(int detChan, unsigned short index, char *name)
{
    int status;


    ASSERT(name != NULL);


    status = dxp_symbolname_by_index(&detChan, &index, name);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error getting parameter located at index %hu for "
                "detChan %d", index, detChan);
        pslLogError("pslGetParamName", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Perform the specified gain operation to the hardware.
 */
PSL_STATIC int pslGainOperation(int detChan, char *name, void *value,
                                Detector *det, int modChan, Module *m, XiaDefaults *defs)
{
    int status;
    unsigned long i;

    ASSERT(name  != NULL);
    ASSERT(value != NULL);
    ASSERT(defs  != NULL);
    ASSERT(det   != NULL);
    ASSERT(m     != NULL);

    for (i = 0; i < N_ELEMS(gainOps); i++) {
        if (STREQ(name, gainOps[i].name)) {

            status = gainOps[i].fn(detChan, det, modChan, m, defs, value);

            if (status != XIA_SUCCESS) {
                sprintf(info_string, "Error doing gain operation '%s' for detChan %d",
                        name, detChan);
                pslLogError("pslGainOperation", info_string, status);
                return status;
            }

            return XIA_SUCCESS;
        }
    }

    sprintf(info_string, "Unknown gain operation '%s' for detChan %d", name,
            detChan);
    pslLogError("pslGainOperation", info_string, XIA_BAD_NAME);

    return XIA_BAD_NAME;
}


/*
 * Perform the specified board operation to the hardware.
 */
PSL_STATIC int pslBoardOperation(int detChan, char *name, void *value,
                                 XiaDefaults *defs)
{
    int status;
    unsigned long i;


    ASSERT(name  != NULL);
    ASSERT(value != NULL);
    ASSERT(defs  != NULL);


    for (i = 0; i < N_ELEMS(boardOps); i++) {
        if (STREQ(name, boardOps[i].name)) {

            status = boardOps[i].fn(detChan, name, defs, value);

            if (status != XIA_SUCCESS) {
                sprintf(info_string, "Error doing board operation '%s' for detChan %d",
                        name, detChan);
                pslLogError("pslBoardOperation", info_string, status);
                return status;
            }

            return XIA_SUCCESS;
        }
    }

    sprintf(info_string, "Unknown board operation '%s' for detChan %d", name,
            detChan);
    pslLogError("pslBoardOperation", info_string, XIA_BAD_NAME);

    return XIA_BAD_NAME;
}


/*
 * Cleans up any resources required by the communication protocol.
 *
 * Handel only passes in detChans that are actual channels, not channel sets.
 */
PSL_STATIC int pslUnHook(int detChan)
{
    int status;


    status = dxp_exit(&detChan);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error shutting down detChan = %d", detChan);
        pslLogError("pslUnHook", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Retrieve the length of the ADC trace buffer. Typically, this routine
 * is used to determine how much memory should be allocated before reading out
 * the ADC trace.
 */
PSL_STATIC int psl__GetADCTraceLen(int detChan, void *value, XiaDefaults *defs)
{
    int status;

    parameter_t TRACELEN = 0;

    UNUSED(defs);


    ASSERT(value != NULL);


    status = pslGetParameter(detChan, "TRACELEN", &TRACELEN);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error reading ADC trace buffer length for detChan %d",
                detChan);
        pslLogError("psl__GetADCTraceLen", info_string, status);
        return status;
    }

    *((unsigned long *)value) = (unsigned long)TRACELEN;

    return XIA_SUCCESS;
}

/* acquisition value peaking_time
 * Recalculate peaking time from SLOWLEN to ensure it's properly synced with
 * device value.
 */
PSL_STATIC int psl__GetPeakingTime(int detChan, void *value, XiaDefaults *defs)
{
    int status;
    double pt   = 0.0;

    UNUSED(defs);

    ASSERT(value != NULL);

     /* Re-calculate actual peaking time */
    status = psl__ReadoutPeakingTime(detChan, &pt);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error reading out peaking time for detChan %d", detChan);
        pslLogError("psl__GetPeakingTime", info_string, status);
        return status;
    }

    *((double *)value) = pt;

    return XIA_SUCCESS;
}

/*
 * Set the requested peaking time and update all of the appropriate
 * filter parameters.
 */
PSL_STATIC int psl__SetPeakingTime(int detChan, int modChan, char *name,
                                   void *value, char *detType,
                                   XiaDefaults *defs, Module *m,
                                   Detector *det, FirmwareSet *fs)
{
    int status;
    boolean_t isMercuryOem = psl__IsMercuryOem(detChan);

    double pt   = 0.0;

    double baseline_factor;

    char fippi[MAX_PATH_LEN];
    char rawFippi[MAXFILENAME_LEN];

    UNUSED(name);

    ASSERT(value != NULL);
    ASSERT(det != NULL);
    ASSERT(fs != NULL);
    ASSERT(m != NULL);

    pt = *((double *)value);

    if (isMercuryOem) {
        pslLogInfo("psl__SetPeakingTime", "Skipping firmware download for Mercury-OEM");

    } else {
        /* The peaking time is validated relative to the defined peaking time
         * ranges in the FDD file.
         */
        status = psl__GetFiPPIName(modChan, pt, fs, detType, fippi, rawFippi);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error getting FiPPI name at peaking time %0.2f "
                    "for detChan = %d", pt, detChan);
            pslLogError("psl__SetPeakingTime", info_string, status);
            return status;
        }

        sprintf(info_string, "Preparing to download FiPPI A to detChan %d", detChan);
        pslLogDebug("psl__SetPeakingTime", info_string);

        status = pslDownloadFirmware(detChan, "fippi_a", fippi, m,
                                     rawFippi, NULL);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error downloading FiPPI A '%s' to detChan %d",
                    fippi, detChan);
            pslLogError("psl__SetPeakingTime", info_string, status);
            return status;
        }
    }

    /* Automatically determine baseline_factor and update SLOWLEN for Mercury OEM
     */
    if (isMercuryOem) {
        baseline_factor = (pt <= 0.48) ? 0 : 1.;
        status = psl__SetBaselineFactor(detChan, modChan, NULL,
                                 &baseline_factor, NULL, defs, m, det, fs);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error setting baseline_factor for detChan %d", detChan);
            pslLogError("psl__SetPeakingTime", info_string, status);
            return status;
        }

        status = psl__Apply(detChan, NULL, defs, NULL);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error applying changes for detChan %d", detChan);
            pslLogError("psl__SetPeakingTime", info_string, status);
            return status;
        }
    }

    status = psl__UpdateFilterParams(detChan, modChan, pt, defs, fs, m, det);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error updating filter parameters for detChan %d",
                detChan);
        pslLogError("psl__SetPeakingTime", info_string, status);
        return status;
    }

    sprintf(info_string, "Filter update complete for peaking time = %0.2f for "
            "detChan %d", pt, detChan);
    pslLogDebug("psl__SetPeakingTime", info_string);


    /* Re-calculate actual peaking time */
    status = psl__ReadoutPeakingTime(detChan, &pt);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error reading out peaking time for detChan %d", detChan);
        pslLogError("psl__SetPeakingTime", info_string, status);
        return status;
    }

    *((double *)value) = pt;

    return XIA_SUCCESS;

}

PSL_STATIC int psl__ReadoutPeakingTime(int detChan, double *peaking_time)
{
    int status;

    parameter_t SLOWLEN    = 0;
    parameter_t DECIMATION = 0;

    double tick = psl__GetClockTick();

    status = pslGetParameter(detChan, "SLOWLEN", &SLOWLEN);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting slow filter length for detChan %d",
                detChan);
        pslLogError("psl__ReadoutPeakingTime", info_string, status);
        return status;
    }

    status = pslGetParameter(detChan, "DECIMATION", &DECIMATION);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting decimation for detChan %d", detChan);
        pslLogError("psl__ReadoutPeakingTime", info_string, status);
        return status;
    }

    /* Scale this back to microseconds */
    *peaking_time =
        (double)(SLOWLEN * tick * pow(2.0, (double)DECIMATION)) * 1.0e6;

    return XIA_SUCCESS;
}

/*
 * Get the ADC trace from the board.
 *
 *  Getting the data stops the control task. If you do an ADC trace special
 *  run then you are required to read the data out to properly stop the run.
 */
PSL_STATIC int psl__GetADCTrace(int detChan, void *value, XiaDefaults *defs)
{
    int status;

    short type = MERCURY_CT_TRACE;

    UNUSED(defs);

    ASSERT(value != NULL);


    status = dxp_get_control_task_data(&detChan, &type, value);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error reading ADC trace data for detChan %d", detChan);
        pslLogError("psl__GetADCTrace", info_string, status);
        return status;
    }

    status = dxp_stop_control_task(&detChan);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error stopping control task run on detChan %d",
                detChan);
        pslLogError("psl__GetADCTrace", info_string, status);
        return status;
    }

    return XIA_SUCCESS;

}


/*
 * Base clock tick for the Mercury. Currently a constant.
 *
 * This value may be non-constant in the future and may need to be determined
 * from a DSP parameter.
 */
PSL_STATIC double psl__GetClockTick(void)
{
    return 1.0 / MERCURY_CLOCK_SPEED;
}


/*
 * Get the correct FiPPI file name for specified module channel and
 * peaking time.
 *
 * The Mercury driver supports FDD files. An error is
 * returned if the Firmware Set does not define an FDD filename.
 *
 * The caller is responsible for allocating memory for name. A good size
 * to use is MAXFILENAME_LEN.
 */
PSL_STATIC int psl__GetFiPPIName(int modChan, double pt, FirmwareSet *fs,
                                 char *detType, char *name, char *rawName)
{
    int status;

    char *tmpPath = NULL;


    ASSERT(fs != NULL);
    ASSERT(detType != NULL);
    ASSERT(name != NULL);
    ASSERT(rawName != NULL);

    if (!fs->filename) {
        sprintf(info_string, "Only FDD files are supported for the Mercury "
                "(modChan = %d)", modChan);
        pslLogError("psl__GetFiPPIName", info_string, XIA_NO_FDD);
        return XIA_NO_FDD;
    }

    if (fs->tmpPath) {
        tmpPath = fs->tmpPath;
    } else {
        tmpPath = utils->funcs->dxp_md_tmp_path();
    }

    status = xiaFddGetFirmware(fs->filename, tmpPath, "fippi_a", pt, 0, NULL,
                               detType, name, rawName);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting FiPPI A filename from '%s' with a "
                "peaking time of %0.2f microseconds", fs->filename, pt);
        pslLogError("psl__GetFiPPIName", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Download FiPPI A to the hardware.
 *
 * Only downloads the requested firmware if it doesn't show that the board
 * is running it.
 */
PSL_STATIC int psl__DownloadFiPPIA(int detChan, char *file, char *rawFile,
                                   Module *m)
{
    int status;


    unsigned int i;
    unsigned int modChan = 0;


    ASSERT(file    != NULL);
    ASSERT(rawFile != NULL);
    ASSERT(m       != NULL);


    status = pslGetModChan(detChan, m, &modChan);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting module channel for detChan = %d",
                detChan);
        pslLogError("psl__DownloadFiPPIA", info_string, status);
        return status;
    }

    if (STREQ(rawFile, m->currentFirmware[modChan].currentFiPPI)) {
        sprintf(info_string, "Requested FiPPI '%s' is already running on "
                "detChan %d", file, detChan);
        pslLogInfo("psl__DownloadFiPPIA", info_string);
        return XIA_SUCCESS;
    }

    status = dxp_replace_fpgaconfig(&detChan, "a", file);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error switching to new FiPPI '%s' for detChan %d",
                file, detChan);
        pslLogError("psl__DownloadFiPPIA", info_string, status);
        return status;
    }

    /* Since we just downloaded the FiPPI for all 4 channels, set the current
     * firmware for all 4 channels to the new file name. This prevents Handel
     * from thinking that it needs to download the firmware 4 times. When we add
     * support for FiPPI B, this will be reduced to the 2 channels covered by
     * FiPPI A.
     */
    for (i = 0; i < m->number_of_channels; i++) {
        strcpy(m->currentFirmware[i].currentFiPPI, rawFile);
    }

    return XIA_SUCCESS;
}


PSL_STATIC int psl__UpdateFilterParams(int detChan, int modChan, double pt,
                                       XiaDefaults *defs, FirmwareSet *fs,
                                       Module *m, Detector *det)
{
    int status;

    unsigned short nFilter = 0;

    parameter_t DECIMATION = 0;
    parameter_t SLOWLEN    = 0;
    parameter_t SLOWGAP    = 0;
    parameter_t PEAKINT    = 0;
    parameter_t PEAKSAM    = 0;
    parameter_t PEAKMODE   = 0;

    int max_slowfilter = MAX_SLOWFILTER;

    double tick        = psl__GetClockTick();
    double sl          = 0.0;
    double sg          = 0.0;
    double gapTime     = 0.0;
    double psOffset    = 0.0;
    double piOffset    = 0.0;
    double gapMinAtDec = 0.0;
    double peakMode    = 0.0;
    double ptMin       = 0.0;
    double ptMax       = 0.0;

    parameter_t filter[2] = {0, 0};

    char psStr[20];
    char piStr[22];

    boolean_t isMercuryOem = psl__IsMercuryOem(detChan);

    ASSERT(fs != NULL);
    ASSERT(fs->filename != NULL);

    if (isMercuryOem) {
        ptMax = 40.96;
        max_slowfilter = 2048;
    }

    if (!isMercuryOem) {
        status = xiaFddGetNumFilter(fs->filename, pt, fs->numKeywords, fs->keywords,
                                    &nFilter);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error getting number of filter parameters from '%s' "
                    "for detChan %d", fs->filename, detChan);
            pslLogError("psl__UpdateFilterParams", info_string, status);
            return status;
        }

        if (nFilter != 2) {
            sprintf(info_string, "Number of filter parameters (%hu) in '%s' does not "
                    "match the number required for the hardware (%d).", nFilter,
                    fs->filename, 2);
            pslLogError("psl__UpdateFilterParams", info_string, XIA_N_FILTER_BAD);
            return XIA_N_FILTER_BAD;
        }

        status = xiaFddGetFilterInfo(fs->filename, pt, fs->numKeywords,
                           (const char **)fs->keywords, &ptMin, &ptMax, &filter[0]);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error getting filter parameter info from '%s' "
                    "for detChan %d", fs->filename, detChan);
            pslLogError("psl__UpdateFilterParams", info_string, status);
            return status;
        }
    }

    /* Calculate SLOWLEN */
    status = pslGetParameter(detChan, "DECIMATION", &DECIMATION);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting decimation for slow filter length "
                "calculation for detChan %d", detChan);
        pslLogError("psl__UpdateFilterParams", info_string, status);
        return status;
    }

    /* Scale tick to microseconds */
    tick *= 1.0e6;

    sprintf(info_string, "DECIMATION = %hu, tick = %0.6f, pt = %0.2f",
            DECIMATION, tick, pt);
    pslLogDebug("psl__UpdateFilterParams", info_string);

    sl = pt / (tick * pow(2.0, (double)DECIMATION));
    SLOWLEN = (parameter_t)ROUND(sl);

    if (SLOWLEN < MIN_SLOWLEN || SLOWLEN > max_slowfilter) {
        sprintf(info_string, "Calculated slow filter length (%hu) is not in the "
                "allowed range (%d, %d) for detChan %d", SLOWLEN, MIN_SLOWLEN,
                max_slowfilter, detChan);
        pslLogError("psl__UpdateFilterParams", info_string, XIA_SLOWLEN_OOR);
        return XIA_SLOWLEN_OOR;
    }

    /* Calculate SLOWGAP */
    status = pslGetDefault("minimum_gap_time", (void *)&gapTime, defs);
    ASSERT(status == XIA_SUCCESS);

    /* Remember, per #544, that the gap_time is the *minimum* gap time. At
     * decimations > 0, we'll probably end up with SLOWGAP = 3.
     */
    if (DECIMATION != 0) {
        gapMinAtDec = tick * pow(2.0, (double)DECIMATION) * 3.0;
        gapTime     = MAX(gapMinAtDec, gapTime);
    }

    sg = gapTime / (tick * pow(2.0, (double)DECIMATION));
    SLOWGAP = (parameter_t)ROUND(sg);

    sprintf(info_string, "Calculated SLOWGAP = %hu", SLOWGAP);
    pslLogDebug("psl__UpdateFilterParams", info_string);

    if (SLOWGAP > max_slowfilter) {
        sprintf(info_string, "Calculated slow filter gap length (%hu) is not in the "
                "allowed range(%d, %d) for detChan %d", SLOWGAP,
                MIN_SLOWGAP, max_slowfilter, detChan);
        pslLogError("psl__UpdateFilterParams", info_string, XIA_SLOWGAP_OOR);
        return XIA_SLOWGAP_OOR;
    }

    if ((SLOWLEN + SLOWGAP) > max_slowfilter) {
        sprintf(info_string, "Total slow filter length (%hd) is larger then the "
                "maximum allowed size (%d) for detChan %d",
                SLOWLEN + SLOWGAP, max_slowfilter, detChan);
        pslLogError("psl__UpdateFilterParams", info_string, XIA_SLOWGAP_OOR);
        return XIA_SLOWGAP_OOR;
    }

    status = pslSetParameter(detChan, "SLOWLEN", SLOWLEN);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting slow filter length to %hu for "
                "detChan %d", SLOWLEN, detChan);
        pslLogError("psl__UpdateFilterParams", info_string, status);
        return status;
    }

    status = pslSetParameter(detChan, "SLOWGAP", SLOWGAP);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting slow filter gap to %hu for detChan %d",
                SLOWGAP, detChan);
        pslLogError("psl__UpdateFilterParams", info_string, status);
        return status;
    }

    /* value should be ignored here, or else we need to pass in
     * a dummy value instead.
     */
    status = psl__Apply(detChan, NULL, defs, NULL);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error applying updated acquisition values for "
                "detChan %d", detChan);
        pslLogError("psl__UpdateFilterParams", info_string, status);
        return status;
    }

    sprintf(info_string, "Set SLOWLEN = %hu, SLOWGAP = %hu", SLOWLEN, SLOWGAP);
    pslLogDebug("psl__UpdateFilterParams", info_string);

    /* actual SLOWLEN and GAPTIME must be used for subsuquent calculation */
    status = pslGetParameter(detChan, "SLOWGAP", &SLOWGAP);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error get SLOWGAP for detChan %d", detChan);
        pslLogError("psl__UpdateFilterParams", info_string, status);
        return status;
    }

    status = pslGetParameter(detChan, "SLOWLEN", &SLOWLEN);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error get SLOWLEN for detChan %d", detChan);
        pslLogError("psl__UpdateFilterParams", info_string, status);
        return status;
    }


    /* Calculate other filter parameters from the filter info in the FDD file.
     * For the xMAP, we interpret the filter data as:
     *
     * filter[0] = PEAKINT offset
     * filter[1] = PEAKSAM offset
     */

    /* Use custom peak interval time if available. */
    sprintf(piStr, "peak_interval_offset%hu", DECIMATION);

    status = pslGetDefault(piStr, (void *)&piOffset, defs);

    if (status == XIA_SUCCESS) {
        PEAKINT = (parameter_t)(SLOWLEN + SLOWGAP +
                                (parameter_t)(piOffset /
                                              (tick * pow(2.0, DECIMATION))));

    } else {
        PEAKINT = (parameter_t)(SLOWLEN + SLOWGAP + filter[0]);
    }

    status = pslSetParameter(detChan, "PEAKINT", PEAKINT);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting peak interval to %hu for detChan %d",
                PEAKINT, detChan);
        pslLogError("psl__UpdateFilterParams", info_string, status);
        return status;
    }

    sprintf(info_string, "SLOWLEN = %hu, SLOWGAP = %hu, PEAKINT = %hu, "
        "offset = %.3f", SLOWLEN, SLOWGAP, PEAKINT, piOffset);
    pslLogDebug("psl__UpdateFilterParams", info_string);

    /* No need to set PEAKSAM if PEAKMODE is XIA_PEAK_SENSING_MODE */
    status = pslGetDefault("peak_mode", (void *)&peakMode, defs);
    ASSERT(status == XIA_SUCCESS);

    PEAKMODE = (parameter_t) peakMode;
    status = pslSetParameter(detChan, "PEAKMODE", PEAKMODE);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting engery filter peak mode to %0.2f for "
                "detChan %d", peakMode, detChan);
        pslLogError("psl__UpdateFilterParams", info_string, status);
        return status;
    }

    if (PEAKMODE != XIA_PEAK_SENSING_MODE) {
        /* If the user has defined a custom peak sampling value at this decimation
        * then it will override the value from the FDD file.
        */
        sprintf(psStr, "peak_sample_offset%hu", DECIMATION);

        status = pslGetDefault(psStr, (void *)&psOffset, defs);

        if (status == XIA_SUCCESS) {
            PEAKSAM = (parameter_t)(SLOWLEN + SLOWGAP -
                                    (parameter_t)(psOffset /
                                                  (tick * pow(2.0, DECIMATION))));

        } else {
            PEAKSAM = (parameter_t)(SLOWLEN + SLOWGAP - filter[1]);
        }

        status = pslSetParameter(detChan, "PEAKSAM", PEAKSAM);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error setting peak sample to %hu for detChan %d",
                    PEAKSAM, detChan);
            pslLogError("psl__UpdateFilterParams", info_string, status);
            return status;
        }
    }

    if (!isMercuryOem) {
        status = psl__UpdateGain(detChan, modChan, defs, m, det);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error updating gain for detChan %d", detChan);
            pslLogError("psl__UpdateFilterParams", info_string, status);
            return status;
        }
    }
    return XIA_SUCCESS;
}


/*
 * Set the minimum gap time for the slow filter.
 */
PSL_STATIC int psl__SetMinGapTime(int detChan, int modChan, char *name,
                                  void *value, char *detType,
                                  XiaDefaults *defs, Module *m,
                                  Detector *det, FirmwareSet *fs)
{
    int status;

    double pt = 0.0;

    UNUSED(name);
    UNUSED(detType);


    ASSERT(value != NULL);
    ASSERT(defs != NULL);


    status = pslSetDefault("minimum_gap_time", (void *)value, defs);
    ASSERT(status == XIA_SUCCESS);

    /* It feels a little odd to be pulling the peaking time out here, just to pass
    * it into a function that could it pull it out itself.
    */
    status = pslGetDefault("peaking_time", (void *)&pt, defs);
    ASSERT(status == XIA_SUCCESS);

    status = psl__UpdateFilterParams(detChan, modChan, pt, defs, fs, m, det);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error updating filter parameters after changing the "
                "slow filter minimum gap time for detChan %d", detChan);
        pslLogError("psl__SetMinGapTime", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Updates the gain setting based on the current acquisition values.
 */
PSL_STATIC int psl__UpdateGain(int detChan, int modChan, XiaDefaults *defs,
                               Module *m, Detector *det)
{
    int status;

    boolean_t isMercuryOem = psl__IsMercuryOem(detChan);

    ASSERT(defs != NULL);
    ASSERT(m != NULL);
    ASSERT(det != NULL);

    status = isMercuryOem ?
            psl__UpdateSwitchedGain(detChan, modChan, defs, m, det) :
            psl__UpdateVariableGain(detChan, modChan, defs, m, det);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error calculating new gain values for detChan %d",
                detChan);
        pslLogError("psl__UpdateGain", info_string, status);
        return status;
    }

    status = psl__SyncTempCalibrationValues(detChan, m, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error syncing temperature calibration after "
                "updating gain for detChan %d", detChan);
        pslLogError("psl__UpdateGain", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Updates thresholds after ev/ADC changes
 */
PSL_STATIC int psl__UpdateThresholds(int detChan, int modChan, XiaDefaults *defs,
                               Module *m, Detector *det)
{
    int status;

    double tt = 0.0;
    double bt = 0.0;
    double et = 0.0;

    ASSERT(defs != NULL);
    ASSERT(m != NULL);
    ASSERT(det != NULL);

    status = pslGetDefault("trigger_threshold", (void *)&tt, defs);
    ASSERT(status == XIA_SUCCESS);

    status = psl__SetTThresh(detChan, modChan, NULL, (void *)&tt, NULL, defs, m,
                             det, NULL);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error updating trigger threshold due to a change in "
                "gain for detChan %d", detChan);
        pslLogError("psl__UpdateThresholds", info_string, status);
        return status;
    }

    status = pslGetDefault("baseline_threshold", (void *)&bt, defs);
    ASSERT(status == XIA_SUCCESS);

    status = psl__SetBThresh(detChan, modChan, NULL, (void *)&bt, NULL, defs, m,
                             det, NULL);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error updating baseline threshold due to a change in "
                "gain for detChan %d", detChan);
        pslLogError("psl__UpdateThresholds", info_string, status);
        return status;
    }

    status = pslGetDefault("energy_threshold", (void *)&et, defs);
    ASSERT(status == XIA_SUCCESS);

    status = psl__SetEThresh(detChan, modChan, NULL, (void *)&et, NULL, defs, m,
                             det, NULL);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error updating energy threshold due to a change in "
                "gain for detChan %d", detChan);
        pslLogError("psl__UpdateThresholds", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Applies the current board settings.
 *
 * Performs the special apply run via. Xerxes. See dxp_do_apply() in mercury.c
 * for more information.
 */
PSL_STATIC int psl__Apply(int detChan, char *name, XiaDefaults *defs,
                          void *value)
{
    int status;

    short task = MERCURY_CT_APPLY;

    UNUSED(name);
    UNUSED(defs);
    UNUSED(value);


    status = dxp_start_control_task(&detChan, &task, NULL, NULL);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error starting 'apply' control task for detChan %d",
                detChan);
        pslLogError("psl__Apply", info_string, status);
        return status;
    }

    status = dxp_stop_control_task(&detChan);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error stopping 'apply' control task for detChan %d",
                detChan);
        pslLogError("psl__Apply", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Do a generic trace run.
 */
PSL_STATIC int psl__DoTrace(int detChan, short type, double *info, boolean_t isDebug)
{
    int status;
    double tick = psl__GetClockTick();

    parameter_t TRACEWAIT = 0;
    parameter_t TRACETYPE = type;

    short task = MERCURY_CT_TRACE;

    /* 'info' must be checked here since not all special runs require it to
     * be filled with data.
     */
    if (!info) {
        sprintf(info_string, "'info' must contain at least two elements: the "
                "# of times to execute the special run (1) and the trace wait "
                "value in microseconds, for detChan %d", detChan);
        pslLogError("psl__DoTrace", info_string, XIA_NULL_INFO);
        return XIA_NULL_INFO;
    }

    /* The trace interval is passed in as nanoseconds, so it must be scaled to
     * seconds.
     */
    TRACEWAIT = (parameter_t)ROUND(((info[1] * 1.0e-9) / tick) - 1.0);

    sprintf(info_string, "Doing%s trace run type %hd, info[1] %.3f, tracewait %hd",
                isDebug ? " debug" : "", type, info[1], TRACEWAIT);
    pslLogInfo("psl__DoTrace", info_string);

    /* Due to the rounding, the trace interval passed in by the user may
     * be slightly different then the actual value written to the DSP. We calculate
     * what the actual value is here and pass it back to the user.
     */
    info[1] = ((double)TRACEWAIT + 1.0) * tick;

    status = pslSetParameter(detChan, "TRACEWAIT", TRACEWAIT);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting the TRACEWAIT for detChan %d", detChan);
        pslLogError("psl__DoTrace", info_string, status);
        return status;
    }

    /* The last element of traceTypes is 'debug', which was put in place so that Jack
    * can run traces without changing the current value of the DSP parameter.
    */
    if (!isDebug) {
        status = pslSetParameter(detChan, "TRACETYPE", TRACETYPE);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error setting the TRACETYPE for detChan %d", detChan);
            pslLogError("psl__DoTrace", info_string, status);
            return status;
        }
    }

    status = dxp_start_control_task(&detChan, &task, NULL, NULL);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error starting tracetype %hd for detChan %d",
                type, detChan);
        pslLogError("psl__DoTrace", info_string, status);
        return status;
    }

    status = dxp_stop_control_task(&detChan);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error stopping control task for "
                "detChan %d", detChan);
        pslLogError("psl__DoTrace", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Returns the current MCA spectrum length to the user
 */
PSL_STATIC int psl__GetMCALength(int detChan, void *value, XiaDefaults *defs,
                                 Module *m)
{
    int status;

    unsigned int mcaLen = 0;

    UNUSED(defs);
    UNUSED(m);


    ASSERT(value != NULL);


    status = dxp_nspec(&detChan, &mcaLen);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error getting spectrum length for detChan %d",
                detChan);
        pslLogError("pslGetMCALength", info_string, status);
        return status;
    }

    sprintf(info_string, "MCA length = %u", mcaLen);
    pslLogDebug("pslGetMCALength", info_string);

    *((unsigned long *)value) = (unsigned long)mcaLen;

    return XIA_SUCCESS;
}


/*
 * Get the MCA spectrum
 */
PSL_STATIC int psl__GetMCA(int detChan, void *value, XiaDefaults *defs, Module *m)
{
    int status;

    UNUSED(defs);
    UNUSED(m);


    ASSERT(value != NULL);


    status = dxp_readout_detector_run(&detChan, NULL, NULL,
                                       (unsigned long *)value);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error reading MCA spectrum for detChan %d", detChan);
        pslLogError("psl__GetMCA", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Gets the length of the baseline buffer.
 */
PSL_STATIC int psl__GetBaselineLength(int detChan, void *value,
                                      XiaDefaults *defs, Module *m)
{
    int status;

    unsigned int len = 0;

    UNUSED(defs);
    UNUSED(m);


    status = dxp_nbase(&detChan, &len);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error getting baseline length for detChan %d",
                detChan);
        pslLogError("psl__GetBaselineLength", info_string, status);
        return status;
    }

    *((unsigned long *)value) = (unsigned long)len;

    return XIA_SUCCESS;
}

/*
 * Get the baseline data from Handel.
 */
PSL_STATIC int psl__GetBaseline(int detChan, void *value, XiaDefaults *defs,
                                Module *m)
{
    int status;

    unsigned long *base = (unsigned long *) value;

    UNUSED(defs);
    UNUSED(m);


    ASSERT(value != NULL);


    status = dxp_readout_detector_run(&detChan, NULL, base, NULL);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error reading baseline from Xerxes for detChan %d",
                detChan);
        pslLogError("psl__GetBaseline", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Calculate the variable gain.
 *
 * Calculates the variable gain based on existing acquisition values and
 * the preamplifier gain and returns the value of the DSP parameters
 * GAINDAC, BINSCALE and ESCALE.
 *
 * The total gain of the Mercury system is defined as:
 *
 * G = Gsys * Gvar,
 *
 * where Gsys is the system gain (psl__GetSystemGain()) and Gvar is the
 * gain due to the variable gain amplifier setting, which is set via.
 * GAINDAC and is one of the main results of the calculations in this routine.
 *
 * The user defines the total gain via. the calibration energy, preamplifier
 * gain, ADC percent rule and dynamic range. In principal, we only maintain
 * the ADC percent rule for backwards compatibilit with our other products.
 * The preferred gain setting parameters are dynamic range and calibration
 * energy.
 *
 * At the end of the function the hardware is updated with new values
 * of GAINDAC, BINSCALE and ESCALE computed by this routine.
 */
PSL_STATIC int psl__UpdateVariableGain(int detChan, int modChan,
                XiaDefaults *defs, Module *m, Detector *det)
{
    int status;
    int i;

    double totGain       = 0.0;
    double varGain       = 0.0;
    double sysGain       = 0.0;
    double adcRule       = 0.0;
    double calibEV       = 0.0;
    double eVPerADC      = 0.0;
    double eVPerBin      = 0.0;
    double binscale      = 0.0;
    double binScale      = 0.0;
    double varGainDB     = 0.0;
    double scaledTotGain = 0.0;
    double gaindac       = 0.0;
    double escale        = 0.0;

    double preampGain = det->gain[m->detector_chan[modChan]];
    parameter_t SLOWLEN, GAINDAC, BINSCALE, ESCALE;


    status = pslGetDefault("calibration_energy", (void *)&calibEV, defs);
    ASSERT(status == XIA_SUCCESS);

    status = psl__GetEVPerADC(detChan, defs, &eVPerADC);

    if (status != XIA_SUCCESS) {
        pslLogError("psl__UpdateVariableGain", "Error getting eV/ADC", status);
        return status;
    }

    adcRule = calibEV * 100.0 / (eVPerADC * MERCURY_ADC_RANGE);

    totGain = ((adcRule / 100.0) * MERCURY_INPUT_RANGE_MV) /
              ((calibEV / 1000.0) * preampGain);

    status = psl__GetSystemGain(&sysGain);

    if (status != XIA_SUCCESS) {
        pslLogError("psl__UpdateVariableGain", "Error getting the system gain", status);
        return status;
    }

    status = pslGetParameter(detChan, "SLOWLEN", &SLOWLEN);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting slow filter length for gain "
                "calculation for detChan %d", detChan);
        pslLogError("psl__UpdateVariableGain", info_string, status);
        return status;
    }

    /* Compute the DSP scaling factor (ESCALE) */
    escale = MAX(0, ceil(log((double)SLOWLEN) / log(2.0)) - 3.0);
    ESCALE = (parameter_t)ROUND(escale);

    sprintf(info_string, "SLOWLEN = %hu, escale = %0.3f", SLOWLEN, escale);
    pslLogDebug("psl__UpdateVariableGain", info_string);

    status = pslGetDefault("mca_bin_width", (void *)&eVPerBin, defs);
    ASSERT(status == XIA_SUCCESS);

    /* Compute BINSCALE and scale the total gain by the difference between
     * the actual value of BINSCALE and the rounded, DSP value of BINSCALE.
     */
    binscale  = ldexp((eVPerBin / eVPerADC) * (double)SLOWLEN, -(ESCALE));
    BINSCALE = (parameter_t)ROUND(binscale);

    sprintf(info_string, "eVPerBin = %0.3f, binscale = %0.3f", eVPerBin,
            binscale);
    pslLogDebug("psl__UpdateVariableGain", info_string);

    /* If the variable gain is out of range, it could be due to the
     * value of BINSCALE being slightly out of range. We want to re-run this
     * calculation and see if we can bring it back in range.
     */
    for (i = 0; i < MERCURY_MAX_BINFACT_ITERS; i++) {

        sprintf(info_string, "binscale = %0.3f, BINSCALE = %#hx", binscale,
                BINSCALE);
        pslLogDebug("psl__UpdateVariableGain", info_string);

        binScale = ((double)BINSCALE) / binscale;

        scaledTotGain = totGain * binScale;

        sprintf(info_string, "Scaled Total gain = %.3f", scaledTotGain);
        pslLogDebug("psl__UpdateVariableGain", info_string);

        sprintf(info_string, "System gain = %0.3f", sysGain);
        pslLogDebug("psl__UpdateVariableGain", info_string);

        varGain = scaledTotGain / sysGain;

        sprintf(info_string, "Variable gain = %0.3f", varGain);
        pslLogDebug("psl__UpdateVariableGain", info_string);

        varGainDB = 20.0 * log10(varGain);

        sprintf(info_string, "Variable gain = %0.3f dB", varGainDB);
        pslLogDebug("psl__UpdateVariableGain", info_string);

        if (varGainDB < -6.0 || varGainDB > 30.0) {
            if ((double)BINSCALE > binscale) {
                BINSCALE--;
            } else {
                BINSCALE++;
            }

        } else {
            /* Found a good combination of BINSCALE and gain */
            break;
        }
    }

    if (varGainDB < -6.0 || varGainDB > 30.0) {
        sprintf(info_string, "Variable gain of %0.3f dB is out-of-range",
                varGainDB);
        pslLogError("psl__UpdateVariableGain", info_string, XIA_GAIN_OOR);
        return XIA_GAIN_OOR;
    }

    varGainDB += 10.0;

    gaindac  = varGainDB * ((double)(0x1 << MERCURY_GAINDAC_BITS) /
                            MERCURY_GAINDAC_DB_RANGE);
    GAINDAC = (parameter_t)ROUND(gaindac);

    sprintf(info_string, "New gain settings for detChan %d: GAINDAC = %#hx, "
            "BINSCALE = %#hx, ESCALE = %#hx", detChan, GAINDAC, BINSCALE, ESCALE);
    pslLogDebug("psl__UpdateVariableGain", info_string);

    status = pslSetParameter(detChan, "GAINDAC", GAINDAC);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting the GAINDAC for detChan %d", detChan);
        pslLogError("psl__UpdateVariableGain", info_string, status);
        return status;
    }

    status = pslSetParameter(detChan, "BINSCALE", BINSCALE);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting BINSCALE for detChan %d", detChan);
        pslLogError("psl__UpdateVariableGain", info_string, status);
        return status;
    }

    status = pslSetParameter(detChan, "ESCALE", ESCALE);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting ESCALE for detChan %d", detChan);
        pslLogError("psl__UpdateVariableGain", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Sets the dynamic range composite value
 *
 * The dynamic range is really the energy range of 40% of the total ADC
 * range. We map this parameter to the corresponding calibration energy
 * at 5% of the total ADC range.
 */
PSL_STATIC int psl__SetDynamicRng(int detChan, int modChan, char *name,
                                  void *value, char *detType,
                                  XiaDefaults *defs, Module *m, Detector *det,
                                  FirmwareSet *fs)
{
    int status;

    UNUSED(fs);
    UNUSED(detType);
    UNUSED(name);

    ASSERT(value != NULL);
    ASSERT(defs != NULL);
    ASSERT(m != NULL);
    ASSERT(det != NULL);

    /* The dynamic_range will be updated in the defaults list after this
     * routine runs, but we need to update it earlier so that the gain routines
     * can use it.
     */
    status = pslSetDefault("dynamic_range", value, defs);
    /* It is impossible for this routine to fail */
    ASSERT(status == XIA_SUCCESS);

    status = psl__UpdateGain(detChan, modChan, defs, m, det);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error updating gain for detChan %d", detChan);
        pslLogError("psl__SetDynamicRng", info_string, status);
        return status;
    }

    status = psl__UpdateThresholds(detChan, modChan, defs, m, det);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error updating thresholds for detChan %d", detChan);
        pslLogError("psl__SetDynamicRng", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


PSL_STATIC int psl__SetCalibEV(int detChan, int modChan, char *name,
                               void *value, char *detType,
                               XiaDefaults *defs, Module *m, Detector *det,
                               FirmwareSet *fs)

{
    int status;

    UNUSED(fs);
    UNUSED(detType);
    UNUSED(name);

    ASSERT(value != NULL);
    ASSERT(defs != NULL);

    /* The calibration energy will be updated in the defaults list after this
     * routine runs, but we need to update it earlier so that the gain routines
     * can use it.
     */
    status = pslSetDefault("calibration_energy", value, defs);
    ASSERT(status == XIA_SUCCESS);

    status = psl__UpdateGain(detChan, modChan, defs, m, det);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error updating gain for detChan %d", detChan);
        pslLogError("psl__SetCalibEV", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/* acquisition value mca_bin_width
 */
PSL_STATIC int psl__SetMCABinWidth(int detChan, int modChan, char *name,
                                   void *value, char *detType,
                                   XiaDefaults *defs, Module *m, Detector *det,
                                   FirmwareSet *fs)
{
    int status;

    UNUSED(fs);
    UNUSED(detType);
    UNUSED(name);


    ASSERT(value != NULL);
    ASSERT(defs != NULL);


    /* The MCA bin width will be updated in the defaults list after this
     * routine runs, but we need to update it earlier so that the gain routines
     * can use it.
     */
    status = pslSetDefault("mca_bin_width", value, defs);
    /* It is impossible for this routine to fail */
    ASSERT(status == XIA_SUCCESS);

    status = psl__UpdateGain(detChan, modChan, defs, m, det);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error updating gain for detChan %d", detChan);
        pslLogError("psl__SetMCABinWidth", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * For regular mercury ev/ADC is only dependent on user acquisition value dynamic_range
 * Mercury-OEM requires a more complicated calculation
 */
PSL_STATIC int psl__GetEVPerADC(int detChan, XiaDefaults *defs, double *eVPerADC)
{
    int status;

    parameter_t SWGAIN = 0;
    double dynamicRng = 0.0;

    boolean_t isMercuryOem = psl__IsMercuryOem(detChan);

    ASSERT(defs != NULL);
    ASSERT(eVPerADC != NULL);

    if (isMercuryOem) {
        status = psl_CalculateEvPerADC(detChan, defs, &SWGAIN, eVPerADC);
        ASSERT(status == XIA_SUCCESS);
    } else {
        status = pslGetDefault("dynamic_range", (void *)&dynamicRng, defs);
        ASSERT(status == XIA_SUCCESS);

        *eVPerADC = (dynamicRng * 2.5) / MERCURY_ADC_RANGE;
    }

    return XIA_SUCCESS;
}


PSL_STATIC int psl__GetSystemGain(double *g)
{
    ASSERT(g != NULL);


    *g = MERCURY_SYSTEM_GAIN;

    return XIA_SUCCESS;
}


PSL_STATIC int psl__SetTThresh(int detChan, int modChan, char *name,
                               void *value, char *detType,
                               XiaDefaults *defs, Module *m, Detector *det,
                               FirmwareSet *fs)
{
    int status;

    double eVPerADC = 0.0;
    double *thresh  = (double *)value;

    parameter_t THRESHOLD = 0;

    UNUSED(det);
    UNUSED(m);
    UNUSED(modChan);
    UNUSED(fs);
    UNUSED(detType);
    UNUSED(name);


    ASSERT(value != NULL);
    ASSERT(defs != NULL);


    status = psl__GetEVPerADC(detChan, defs, &eVPerADC);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting eV/ADC for detChan %d", detChan);
        pslLogError("psl__SetTThresh", info_string, status);
        return status;
    }

    sprintf(info_string, "thresh = %0.2f, eV/ADC = %0.2f", *thresh, eVPerADC);
    pslLogDebug("psl__SetTThresh", info_string);

    THRESHOLD = (parameter_t)ROUND(*thresh / eVPerADC);

    sprintf(info_string, "THRESHOLD = %hu", THRESHOLD);
    pslLogDebug("psl__SetTThresh", info_string);

    status = pslSetParameter(detChan, "THRESHOLD", THRESHOLD);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting THRESHOLD to %.3f for detChan %d",
                *thresh, detChan);
        pslLogError("psl__SetTThresh", info_string, status);
        return status;
    }

    /* Re-calculate the threshold based on the rounded value of THRESHOLD and
     * pass it back to the user.
     */
    *thresh = (double)THRESHOLD * eVPerADC;

    return XIA_SUCCESS;
}


PSL_STATIC int psl__SetBThresh(int detChan, int modChan, char *name,
                               void *value, char *detType,
                               XiaDefaults *defs, Module *m, Detector *det,
                               FirmwareSet *fs)
{
    int status;

    double eVPerADC = 0.0;
    double *thresh  = (double *)value;

    parameter_t BASETHRESH = 0;

    UNUSED(modChan);
    UNUSED(m);
    UNUSED(det);
    UNUSED(fs);
    UNUSED(detType);
    UNUSED(name);


    ASSERT(value != NULL);
    ASSERT(defs != NULL);


    status = psl__GetEVPerADC(detChan, defs, &eVPerADC);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting eV/ADC for detChan %d", detChan);
        pslLogError("psl__SetBThresh", info_string, status);
        return status;
    }

    BASETHRESH = (parameter_t)ROUND(*thresh / eVPerADC);

    status = pslSetParameter(detChan, "BASETHRESH", BASETHRESH);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting BASETHRESH to %.3f for detChan %d",
                *thresh, detChan);
        pslLogError("psl__SetBThresh", info_string, status);
        return status;
    }

    /* Re-calculate the baseline threshold based on the rounded value of
     * BASETHRESH and pass it back to the user.
     */
    *thresh = (double)BASETHRESH * eVPerADC;

    return XIA_SUCCESS;
}


PSL_STATIC int psl__SetEThresh(int detChan, int modChan, char *name,
                               void *value, char *detType,
                               XiaDefaults *defs, Module *m, Detector *det,
                               FirmwareSet *fs)

{
    int status;

    double eVPerADC = 0.0;
    double *thresh  = (double *)value;

    parameter_t SLOWTHRESH = 0;

    UNUSED(modChan);
    UNUSED(m);
    UNUSED(det);
    UNUSED(fs);
    UNUSED(detType);
    UNUSED(name);


    ASSERT(value != NULL);
    ASSERT(defs != NULL);


    status = psl__GetEVPerADC(detChan, defs, &eVPerADC);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting eV/ADC for detChan %d", detChan);
        pslLogError("psl__SetEThresh", info_string, status);
        return status;
    }

    SLOWTHRESH = (parameter_t)ROUND(*thresh / eVPerADC);

    status = pslSetParameter(detChan, "SLOWTHRESH", SLOWTHRESH);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting SLOWTHRESH to %.3f for detChan %d",
                *thresh, detChan);
        pslLogError("psl__SetEThresh", info_string, status);
        return status;
    }

    /* Re-calculate the baseline threshold based on the rounded value of
     * SLOWTHRESH and pass it back to the user.
     */
    *thresh = (double)SLOWTHRESH * eVPerADC;

    return XIA_SUCCESS;
}


/*
 * Set the preset run type.
 *
 * The allowed preset run types are defined in handel_constants.h.
 */
PSL_STATIC int psl__SetPresetType(int detChan, int modChan, char *name, void *value,
                                  char *detType, XiaDefaults *defs, Module *m,
                                  Detector *det, FirmwareSet *fs)
{
    int status;

    parameter_t PRESETTYPE = 0;

    double presetType = 0.0;

    UNUSED(name);
    UNUSED(modChan);
    UNUSED(detType);
    UNUSED(defs);
    UNUSED(m);
    UNUSED(det);
    UNUSED(fs);


    ASSERT(value != NULL);


    presetType = *((double *)value);

    /* The constants stored in handel_constants.h also happen to map directly
     * to PRESETTYPE as currently defined.
     */
    if (presetType != XIA_PRESET_NONE &&
            presetType != XIA_PRESET_FIXED_REAL &&
            presetType != XIA_PRESET_FIXED_LIVE &&
            presetType != XIA_PRESET_FIXED_EVENTS &&
            presetType != XIA_PRESET_FIXED_TRIGGERS) {

        sprintf(info_string, "Invalid preset run type specified: %0.1f",
                presetType);
        pslLogError("psl__SetPresetType", info_string, XIA_UNKNOWN_PRESET);
        return XIA_UNKNOWN_PRESET;
    }

    PRESETTYPE = (parameter_t)presetType;

    status = pslSetParameter(detChan, "PRESETTYPE", PRESETTYPE);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting PRESETTYPE to %hu for detChan %d",
                PRESETTYPE, detChan);
        pslLogError("psl__SetPresetType", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Set the preset run value.
 *
 * This value is interpreted differently depending on the preset run type,
 * which means that this value must be set *after* setting the preset type.
 *
 * For fixed realtime/livetime: Specify in seconds.
 * For count-based runs: Specify as counts.
 */
PSL_STATIC int psl__SetPresetValue(int detChan, int modChan, char *name, void *value,
                                   char *detType, XiaDefaults *defs, Module *m,
                                   Detector *det, FirmwareSet *fs)
{
    int status;

    double v    = 0.0;
    double len  = 0.0;
    double tick = psl__GetClockTick();

    unsigned long loLen = 0;
    unsigned long hiLen = 0;

    parameter_t PRESETTYPE = 0;
    parameter_t PRESETLEN  = 0;
    parameter_t PRESETLENA = 0;
    parameter_t PRESETLENB = 0;
    parameter_t PRESETLENC = 0;

    UNUSED(name);
    UNUSED(modChan);
    UNUSED(detType);
    UNUSED(defs);
    UNUSED(m);
    UNUSED(det);
    UNUSED(fs);


    ASSERT(value != NULL);


    status = pslGetParameter(detChan, "PRESETTYPE", &PRESETTYPE);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting current preset type for detChan %d",
                detChan);
        pslLogError("psl__SetPresetValue", info_string, status);
        return status;
    }

    v = *((double *)value);

    switch (PRESETTYPE) {

    case 0:
        /* Ignore since this is an indefinite run. */
        return XIA_SUCCESS;

    case 1:
    case 2:
        len = v / (tick * 16.0);
        break;

    case 3:
    case 4:
        len = v;
        break;

    default:
        /* It should be impossible for PRESETTYPE to be out-of-range. */
        FAIL();
    }

    hiLen = (unsigned long)floor(len / ldexp(1.0, 32));
    loLen = (unsigned long)ROUND(len - ((double)hiLen * ldexp(1.0, 32)));

    sprintf(info_string, "len = %0.0f, hiLen = %#lx, loLen = %#lx",
            len, hiLen, loLen);
    pslLogDebug("psl__SetPresetValue", info_string);

    PRESETLEN  = (parameter_t)(loLen & 0xFFFF);
    PRESETLENA = (parameter_t)(loLen >> 16);
    PRESETLENB = (parameter_t)(hiLen & 0xFFFF);
    PRESETLENC = (parameter_t)(hiLen >> 16);

    status = pslSetParameter(detChan, "PRESETLEN", PRESETLEN);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting the preset run length for detChan %d",
                detChan);
        pslLogError("psl__SetPresetValue", info_string, status);
        return status;
    }

    status = pslSetParameter(detChan, "PRESETLENA", PRESETLENA);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting the preset run length for detChan %d",
                detChan);
        pslLogError("psl__SetPresetValue", info_string, status);
        return status;
    }

    status = pslSetParameter(detChan, "PRESETLENB", PRESETLENB);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting the preset run length for detChan %d",
                detChan);
        pslLogError("psl__SetPresetValue", info_string, status);
        return status;
    }

    status = pslSetParameter(detChan, "PRESETLENC", PRESETLENC);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting the preset run length for detChan %d",
                detChan);
        pslLogError("psl__SetPresetValue", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Get the run active status for the hardware.
 */
PSL_STATIC int psl__GetRunActive(int detChan, void *value, XiaDefaults *defs,
                                 Module *m)
{
    int status;
    int active;

    UNUSED(m);
    UNUSED(defs);


    ASSERT(value != NULL);


    status = dxp_isrunning(&detChan, &active);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error getting run status for detChan %d", detChan);
        pslLogError("psl__GetRunActive", info_string, status);
        return status;
    }

    *((unsigned long *)value) = (unsigned long)active;


    return XIA_SUCCESS;
}

/*
 * Gets the runtime for the specified channel.
 */
PSL_STATIC int psl__GetRealtime(int detChan, void *value, XiaDefaults *defs,
                                Module *m)
{
    int status;
    unsigned long stats[MERCURY_MEMORY_BLOCK_SIZE];
    unsigned int modChan;

    UNUSED(m);
    UNUSED(defs);

    ASSERT(value != NULL);

    status = pslGetModChan(detChan, m, &modChan);
    /* Impossible for this to fail in a system properly configured by Handel. */
    ASSERT(status == XIA_SUCCESS);

    status = psl__GetStatisticsBlock(detChan, stats);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error reading statistics block for detChan %d", detChan);
        pslLogError("psl__GetRealtime", info_string, status);
        return status;
    }

    status = psl__ExtractRealtime(modChan, stats, (double *)value);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting energy livetime for detChan %d",
                detChan);
        pslLogError("psl__GetRealtime", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Get the events in run for the specified channel.
 *
 * This only returns the lower 32-bits of the events in run. For the complete
 * 64-bit value get "module_statistics".
 */
PSL_STATIC int psl__GetTotalEvents(int detChan, void *value, XiaDefaults *defs,
                                   Module *m)
{
    int status;

    unsigned long stats[MERCURY_MEMORY_BLOCK_SIZE];

    double mcaEvts;
    double unders;
    double overs;

    unsigned int modChan;

    UNUSED(defs);


    ASSERT(value);
    ASSERT(m);


    status = pslGetModChan(detChan, m, &modChan);
    /* Impossible for this to fail in a system properly configured by Handel. */
    ASSERT(status == XIA_SUCCESS);

    status = psl__GetStatisticsBlock(detChan, stats);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error reading statistics block for detChan %d.",
                detChan);
        pslLogError("psl__GetTotalEvents", info_string, status);
        return status;
    }

    status = psl__ExtractEvents(modChan, stats, &mcaEvts);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting the MCA event count from the "
                "statistics block for detChan %d.", detChan);
        pslLogError("psl__GetTotalEvents", info_string, status);
        return status;
    }

    status = psl__ExtractUnderflows(modChan, stats, &unders);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting the underflow event count from the "
                "statistics block for detChan %d.", detChan);
        pslLogError("psl__GetTotalEvents", info_string, status);
        return status;
    }

    status = psl__ExtractOverflows(modChan, stats, &overs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting the overflow event count from the "
                "statistics block for detChan %d.", detChan);
        pslLogError("psl__GetTotalEvents", info_string, status);
        return status;
    }

    /* This routine explicitly returns a 32-bit integral value. */
    *((unsigned long *)value) = (unsigned long)(mcaEvts + unders + overs);

    return XIA_SUCCESS;
}


/*
 * Get the trigger livetime for the specified channel.
 *
 * The trigger livetime is the same as Xerxes' notion of "livetime".
 */
PSL_STATIC int psl__GetTLivetime(int detChan, void *value, XiaDefaults *defs,
                                 Module *m)
{
    int status;

    unsigned int modChan;

    unsigned long stats[MERCURY_MEMORY_BLOCK_SIZE];

    UNUSED(defs);


    ASSERT(value);
    ASSERT(m);


    status = pslGetModChan(detChan, m, &modChan);
    /* Impossible for this to fail in a system properly configured by Handel. */
    ASSERT(status == XIA_SUCCESS);

    status = psl__GetStatisticsBlock(detChan, stats);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error reading statistics block for detChan %d.",
                detChan);
        pslLogError("psl__GetTLivetime", info_string, status);
        return status;
    }

    status = psl__ExtractTLivetime(modChan, stats, (double *)value);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error extracting the trigger livetime from "
                "the statistics block for detChan %d.", detChan);
        pslLogError("psl__GetTLivetime", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Get the input count rate for the specified channel.
 */
PSL_STATIC int psl__GetICR(int detChan, void *value, XiaDefaults *defs,
                           Module *m)
{
    int status;

    double tlt;
    double trigs;

    unsigned int modChan;

    unsigned long stats[MERCURY_MEMORY_BLOCK_SIZE];

    UNUSED(defs);


    ASSERT(value);
    ASSERT(m);


    status = pslGetModChan(detChan, m, &modChan);
    /* Impossible for this to fail in a system properly configured by Handel. */
    ASSERT(status == XIA_SUCCESS);

    status = psl__GetStatisticsBlock(detChan, stats);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error reading statistics block for detChan %d.",
                detChan);
        pslLogError("psl__GetICR", info_string, status);
        return status;
    }

    status = psl__ExtractTLivetime(modChan, stats, &tlt);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error extracting the trigger livetime from "
                "the statistics block for detChan %d.", detChan);
        pslLogError("psl__GetICR", info_string, status);
        return status;
    }

    status = psl__ExtractTriggers(modChan, stats, &trigs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error extracting the trigger count from the "
                "statistics block for detChan %d.", detChan);
        pslLogError("psl__GetICR", info_string, status);
        return status;
    }

    if (tlt > 0.0) {
        *((double *)value) = trigs / tlt;

    } else {
        *((double *)value)= 0.0;
    }

    return XIA_SUCCESS;
}


/*
 * Get the output count rate for the specified channel.
 */
PSL_STATIC int psl__GetOCR(int detChan, void *value, XiaDefaults *defs,
                           Module *m)
{
    int status;

    double rt;
    double mcaEvts;
    double unders;
    double overs;

    unsigned int modChan;

    unsigned long stats[MERCURY_MEMORY_BLOCK_SIZE];

    UNUSED(defs);


    ASSERT(value);
    ASSERT(m);


    status = pslGetModChan(detChan, m, &modChan);
    /* Impossible for this to fail in a system properly configured by Handel. */
    ASSERT(status == XIA_SUCCESS);

    status = psl__GetStatisticsBlock(detChan, stats);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error reading statistics block for detChan %d.",
                detChan);
        pslLogError("psl__GetOCR", info_string, status);
        return status;
    }

    status = psl__ExtractRealtime(modChan, stats, &rt);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error extracting the realtime from "
                "the statistics block for detChan %d.", detChan);
        pslLogError("psl__GetOCR", info_string, status);
        return status;
    }

    status = psl__ExtractEvents(modChan, stats, &mcaEvts);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error extracting the MCA event count from the "
                "statistics block for detChan %d.", detChan);
        pslLogError("psl__GetOCR", info_string, status);
        return status;
    }

    status = psl__ExtractUnderflows(modChan, stats, &unders);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error extracting the underflow event count from "
                "the statistics block for detChan %d.", detChan);
        pslLogError("psl__GetOCR", info_string, status);
        return status;
    }

    status = psl__ExtractOverflows(modChan, stats, &overs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error extracting the overflow event count from "
                "the statistics block for detChan %d.", detChan);
        pslLogError("psl__GetOCR", info_string, status);
        return status;
    }

    if (rt > 0.0) {
        *((double *)value) = (mcaEvts + unders + overs) / rt;

    } else {
        *((double *)value)= 0.0;
    }

    return XIA_SUCCESS;
}

/*
 * Read the energy livetime from the board.
 */
PSL_STATIC int psl__GetELivetime(int detChan, void *value, XiaDefaults *defs,
                                 Module *m)
{
    int status;

    unsigned int modChan;

    unsigned long stats[MERCURY_MEMORY_BLOCK_SIZE];

    UNUSED(m);
    UNUSED(defs);


    ASSERT(value != NULL);

    status = pslGetModChan(detChan, m, &modChan);
    /* Impossible for this to fail in a system properly configured by Handel. */
    ASSERT(status == XIA_SUCCESS);

    status = psl__GetStatisticsBlock(detChan, stats);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error reading statistics block for detChan %d", detChan);
        pslLogError("psl__GetELivetime", info_string, status);
        return status;
    }

    status = psl__ExtractELivetime(modChan, stats, (double *)value);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting energy livetime for detChan %d",
                detChan);
        pslLogError("psl__GetELivetime", info_string, status);
        return status;
    }

    return XIA_SUCCESS;

}

/*
 * Read the statistics block for the specified module from the
 * external memory.
 *
 * Callers are responsible for allocating enough memory for stats.
 */
PSL_STATIC int psl__GetStatisticsBlock(int detChan, unsigned long *stats)
{
    int status;

    char mem[MAXITEM_LEN];


    ASSERT(stats != NULL);

    sprintf(mem, "burst:%#x:%d", 0x00, MERCURY_MEMORY_BLOCK_SIZE);

    status = dxp_read_memory(&detChan, mem, stats);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error burst reading statistics block for detChan %d",
                detChan);
        pslLogError("psl__GetStatisticsBlock", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Extract the energy livetime for the specified _module_ channel
 * from the module statistics block.
 */
PSL_STATIC int psl__ExtractELivetime(int modChan, unsigned long *stats,
                                     double *eLT)
{
    unsigned long eLTOffset;

    double tick = psl__GetClockTick();

    ASSERT(stats != NULL);
    ASSERT(eLT   != NULL);
    ASSERT(modChan >= 0 && modChan < 4);

    eLTOffset = MERCURY_STATS_CHAN_OFFSET[modChan] + MERCURY_STATS_ELIVETIME_OFFSET;
    *eLT      = pslU64ToDouble(stats + eLTOffset) * tick * 16.0;

    return XIA_SUCCESS;
}


/*
 * Extract the realtime for the specified _module_ channel
 * from the module statistics block.
 */
PSL_STATIC int psl__ExtractRealtime(int modChan, unsigned long *stats,
                                    double *rt)
{
    unsigned long rtOffset;

    double tick = psl__GetClockTick();

    ASSERT(stats != NULL);
    ASSERT(rt    != NULL);
    ASSERT(modChan >= 0 && modChan < 4);

    rtOffset = MERCURY_STATS_CHAN_OFFSET[modChan] + MERCURY_STATS_REALTIME_OFFSET;
    *rt      = pslU64ToDouble(stats + rtOffset) * tick * 16.0;

    return XIA_SUCCESS;
}

/*
 * Returns the statistics for all of the channels on the
 * module that detChan is a part of. value is expected to be a
 * double array with at least 7 elements (for the Mercury) or 28
 * elements (for the Mercury-4). They are stored in the following
 * format:
 *
 * [ch0_runtime, ch0_trigger_livetime, ch0_energy_livetime, ch0_triggers,
 *  ch0_events, ch0_icr, ch0_ocr, ..., ch3_runtime, etc.]
 */
PSL_STATIC int psl__GetModuleStatistics(int detChan, void *value,
                                        XiaDefaults * defs, Module *m)
{
    int status;
    int i;

    unsigned long stats[MERCURY_MEMORY_BLOCK_SIZE];

    double *modStats = NULL;

    double tLT;
    double rt;
    double trigs;
    double evts;
    double unders;
    double overs;

    UNUSED(defs);


    ASSERT(value != NULL);
    ASSERT(m != NULL);


    modStats = (double *)value;

    status = psl__GetStatisticsBlock(detChan, stats);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error reading statistics block for detChan %d", detChan);
        pslLogError("psl__GetModuleStatistics", info_string, status);
        return status;
    }

    for (i = 0; i < (int) m->number_of_channels; i++) {
        status = psl__ExtractRealtime(i, stats, &rt);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error extracting runtime for detChan %d, modChan %d",
                    detChan, i);
            pslLogError("psl__GetModuleStatistics", info_string, status);
            return status;
        }

        modStats[i * 7] = rt;

        status = psl__ExtractTLivetime(i, stats, &tLT);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error extracting trigger livetime for detChan %d, "
                    "modChan %d", detChan, i);
            pslLogError("psl__GetModuleStatistics", info_string, status);
            return status;
        }

        modStats[(i * 7) + 1] = tLT;

        status = psl__ExtractELivetime(i, stats, &(modStats[(i * 7) + 2]));

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error extracting energy livetime for detChan %d, "
                    "modChan %d", detChan, i);
            pslLogError("psl__GetModuleStatistics", info_string, status);
            return status;
        }

        status = psl__ExtractTriggers(i, stats, &trigs);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error extracting triggers for detChan %d, "
                    "modChan %d", detChan, i);
            pslLogError("psl__GetModuleStatistics", info_string, status);
            return status;
        }

        modStats[(i * 7) + 3] = trigs;

        status = psl__ExtractEvents(i, stats, &evts);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error extracting events for detChan %d, "
                    "modChan %d", detChan, i);
            pslLogError("psl__GetModuleStatistics", info_string, status);
            return status;
        }

        modStats[(i * 7) + 4] = evts;

        status = psl__ExtractUnderflows(i, stats, &unders);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error extracting underflows for detChan %d, "
                    "modChan %d", detChan, i);
            pslLogError("psl__GetModuleStatistics", info_string, status);
            return status;
        }

        status = psl__ExtractOverflows(i, stats, &overs);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error extracting overflows for detChan %d, "
                    "modChan %d", detChan, i);
            pslLogError("psl__GetModuleStatistics", info_string, status);
            return status;
        }

        if (tLT != 0.0) {
            modStats[(i * 7) + 5] = trigs / tLT;
        } else {
            modStats[(i * 7) + 5] = 0.0;
        }

        if (rt != 0.0) {
            modStats[(i * 7) + 6] = (evts + overs + unders) / rt;
        } else {
            modStats[(i * 7) + 6] = 0.0;
        }
    }

    return XIA_SUCCESS;
}


/*
 * Extracts the trigger livetime for the specified module channel
 * from the statistics block.
 */
PSL_STATIC int psl__ExtractTLivetime(int modChan, unsigned long *stats,
                                     double *tLT)
{
    unsigned long tLTOffset;

    double tick = psl__GetClockTick();

    ASSERT(modChan >= 0 && modChan < 4);

    ASSERT(stats != NULL);
    ASSERT(tLT   != NULL);


    tLTOffset = MERCURY_STATS_CHAN_OFFSET[modChan] + MERCURY_STATS_TLIVETIME_OFFSET;
    *tLT      = pslU64ToDouble(stats + tLTOffset) * tick * 16.0;

    return XIA_SUCCESS;
}


/*
 * Extracts the triggers for the specified module channel from
 * the statistics block.
 */
PSL_STATIC int psl__ExtractTriggers(int modChan, unsigned long *stats,
                                    double *trigs)
{
    unsigned long trigsOffset;

    ASSERT(modChan >= 0 && modChan < 4);

    ASSERT(stats != NULL);
    ASSERT(trigs != NULL);


    trigsOffset = MERCURY_STATS_CHAN_OFFSET[modChan] + MERCURY_STATS_TRIGGERS_OFFSET;
    *trigs      = pslU64ToDouble(stats + trigsOffset);

    return XIA_SUCCESS;
}


/*
 * Extracts the events in run for the specified module channel from
 * the statistics block.
 */
PSL_STATIC int psl__ExtractEvents(int modChan, unsigned long *stats,
                                  double *evts)
{
    unsigned long evtsOffset;

    ASSERT(modChan >= 0 && modChan < 4);

    ASSERT(stats != NULL);
    ASSERT(evts  != NULL);

    evtsOffset = MERCURY_STATS_CHAN_OFFSET[modChan] + MERCURY_STATS_MCAEVENTS_OFFSET;
    *evts      = pslU64ToDouble(stats + evtsOffset);

    return XIA_SUCCESS;
}

/*
 * Extract the OVERFLOWS reported in the statistics block.
 */
PSL_STATIC int psl__ExtractOverflows(int modChan, unsigned long *stats,
                                     double *overs)
{
    unsigned long oversOffset;

    ASSERT(modChan >= 0 && modChan < 4);

    ASSERT(stats != NULL);
    ASSERT(overs != NULL);


    oversOffset = MERCURY_STATS_CHAN_OFFSET[modChan] + MERCURY_STATS_OVERFLOWS_OFFSET;
    *overs = pslU64ToDouble(stats + oversOffset);

    return XIA_SUCCESS;
}


/*
 * Extract the UNDERFLOWS reported in the statistics block.
 */
PSL_STATIC int psl__ExtractUnderflows(int modChan, unsigned long *stats,
                                      double *unders)
{
    unsigned long undersOffset;

    ASSERT(modChan >= 0 && modChan < 4);

    ASSERT(stats != NULL);
    ASSERT(unders != NULL);


    undersOffset = MERCURY_STATS_CHAN_OFFSET[modChan] + MERCURY_STATS_UNDERFLOWS_OFFSET;
    *unders = pslU64ToDouble(stats + undersOffset);

    return XIA_SUCCESS;
}

/*
 * Gets all of the DSP parameter values for the specified channel.
 */
PSL_STATIC int psl__GetParamValues(int detChan, void *value)
{
    int status;


    ASSERT(value != NULL);


    status = dxp_readout_detector_run(&detChan, (unsigned short *)value, NULL,
                                       NULL);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error getting DSP parameter values for detChan %d",
                detChan);
        pslLogError("psl__GetParamValues", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Set detector_polarity acquistion value.
 */
PSL_STATIC int psl__SetPolarity(int detChan, int modChan, char *name, void *value,
                                char *detType, XiaDefaults *defs, Module *m,
                                Detector *det, FirmwareSet *fs)
{
    int status;

    parameter_t POLARITY = 0;

    UNUSED(name);
    UNUSED(defs);
    UNUSED(fs);
    UNUSED(detType);


    ASSERT(value != NULL);

    POLARITY = (parameter_t)(*((double *)value));

    if ((POLARITY != 1) && (POLARITY != 0)) {
        sprintf(info_string, "User specified polarity '%hu' is not within the "
                "valid range (0,1) for detChan %d", POLARITY, detChan);
        pslLogError("psl__SetPolarity", info_string, XIA_POL_OOR);
        return XIA_POL_OOR;
    }

    status = pslSetParameter(detChan, "POLARITY", POLARITY);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting the polarity for detChan %d", detChan);
        pslLogError("psl__SetPolarity", info_string, status);
        return status;
    }

    /* Update the Detector configuration */
    det->polarity[m->detector_chan[modChan]] = POLARITY;

    status = psl__SyncTempCalibrationValues(detChan, m, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error syncing temperature calibration after "
                "updating polarity for detChan %d", detChan);
        pslLogError("psl__SetPolarity", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Synchronize the detector polarity in the Detector configuration
 * with the detector_polarity acquisition value.
 *
 * Handel assumes that the detector polarity specified in the Detector
 * configuration is correct and uses it to set the acquisition value
 * detector_polarity.
 */
PSL_STATIC int psl__SynchPolarity(int detChan, int det_chan, Module *m,
                                  Detector *det, XiaDefaults *defs)
{
    int status;

    double pol = 0.0;

    UNUSED(m);


    ASSERT(det != NULL);
    ASSERT(defs != NULL);


    pol = (double)(det->polarity[det_chan]);

    status = pslSetDefault("detector_polarity", (void *)&pol, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error synchronizing detector polarity for detChan %d",
                detChan);
        pslLogError("psl__SynchPolarity", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Set the reset delay interval.
 */
PSL_STATIC int psl__SetResetDelay(int detChan, int modChan, char *name, void *value,
                                  char *detType, XiaDefaults *defs, Module *m,
                                  Detector *det, FirmwareSet *fs)
{
    int status;

    double tick = 0.0;

    double *resetDelay = NULL;

    parameter_t RESETINT = 0;

    UNUSED(name);
    UNUSED(defs);
    UNUSED(m);
    UNUSED(fs);
    UNUSED(detType);


    ASSERT(value != NULL);
    ASSERT(det != NULL);


    /* Since this routine can be called (and will) be called for all modules
     * and configurations, we'll want to skip this step if the detector is not
     * reset-type.
     */
    if (det->type != XIA_DET_RESET) {
        sprintf(info_string, "Skipping setting reset delay: detChan %d is not a "
                "reset-type detector", detChan);
        pslLogInfo("psl__SetResetDelay", info_string);
        return XIA_SUCCESS;
    }

    /* This is in microseconds and it needs to be converted to seconds */
    resetDelay = (double *)value;

    /* Update the Detector configuration */
    det->typeValue[m->detector_chan[modChan]] = *resetDelay;

    *resetDelay = *resetDelay / 1.0e6;

    /* XXX Check limits here */


    tick = psl__GetClockTick();

    RESETINT = (parameter_t)ROUND(*resetDelay / tick);

    status = pslSetParameter(detChan, "RESETINT", RESETINT);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting reset delay to %0.6f microseconds for "
                "detChan %d", *resetDelay, detChan);
        pslLogError("psl__SetResetDelay", info_string, status);
        return status;
    }

    /* We have to refresh the value the user passed in since it may be different
     * due to the rounding that was necessary to convert the value to a DSP
     * parameter.
     */
    *resetDelay = (double)RESETINT * tick * 1.0e6;

    return XIA_SUCCESS;
}

/*
 * Synchronize the detector reset delay in the Detector configuration
 * with the reset_delay acquisition value.
 *
 * Handel assumes that the detector reset delay specified in the Detector
 * configuration is correct and uses it to set the acquisition value
 * reset_delay.
 */
PSL_STATIC int psl__SynchResetDelay(int detChan, int det_chan, Module *m,
                                    Detector *det, XiaDefaults *defs)
{
    int status;

    double resetDelay = 0.0;

    UNUSED(m);


    ASSERT(det != NULL);


    /* Since this routine can be called (and will) be called for all modules
     * and configurations, we'll want to skip this step if the detector is not
     * reset-type.
     */
    if (det->type != XIA_DET_RESET) {
        sprintf(info_string, "Skipping reset delay synch: detChan %d is not a "
                "reset-type detector", detChan);
        pslLogInfo("psl__SynchResetDelay", info_string);
        return XIA_SUCCESS;
    }

    resetDelay = det->typeValue[det_chan];

    status = pslSetDefault("reset_delay", (void *)&resetDelay, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error synchronizing the reset delay for detChan %d",
                detChan);
        pslLogError("psl__SynchResetDelay", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * acquisition value decay_time
 */
PSL_STATIC int psl__SetDecayTime(int detChan, int modChan, char *name,
                                 void *value, char *detType,
                                 XiaDefaults *defs, Module *m, Detector *det,
                                 FirmwareSet *fs)
{
    int status;

    double decayTime = 0.0;

    parameter_t RCTAU;
    parameter_t RCTAUFRAC;

    UNUSED(fs);
    UNUSED(defs);
    UNUSED(detType);
    UNUSED(name);


    ASSERT(det != NULL);
    ASSERT(m != NULL);
    ASSERT(value != NULL);


    if (det->type != XIA_DET_RCFEED) {
        sprintf(info_string, "Skipping setting RC decay time: detChan %d is not "
                "a RC-type preamplifier.", detChan);
        pslLogInfo("psl__SetDecayTime", info_string);
        return XIA_SUCCESS;
    }

    decayTime = *((double *)value);

    det->typeValue[m->detector_chan[modChan]] = decayTime;

    RCTAU     = (parameter_t)floor(decayTime);
    RCTAUFRAC = (parameter_t)ROUND((decayTime - (double)RCTAU) * 65536.0);

    status = pslSetParameter(detChan, "RCTAU", RCTAU);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting RCTAU to %#hx for a decay time of "
                "%0.6f microseconds for detChan %d", RCTAU, decayTime, detChan);
        pslLogError("psl__SetDecayTime", info_string, status);
        return status;
    }

    status = pslSetParameter(detChan, "RCTAUFRAC", RCTAUFRAC);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting RCTAUFRAC to %#hx for a decay time of "
                "%0.6f microseconds for detChan %d", RCTAUFRAC, decayTime, detChan);
        pslLogError("psl__SetDecayTime", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Synchronize the detector decay time in the Detector configuration
 * with the decay_time acquisition value.
 */
PSL_STATIC int psl__SynchDecayTime(int detChan, int det_chan, Module *m,
                                   Detector *det, XiaDefaults *defs)
{
    int status;

    double decayTime;

    UNUSED(m);


    ASSERT(det != NULL);
    ASSERT(defs != NULL);


    decayTime = det->typeValue[det_chan];

    status = pslSetDefault("decay_time", &decayTime, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error synchronizing decay time for detChan %d",
                detChan);
        pslLogError("psl__SynchDecayTime", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * aquisition value preamp_gain
 *
 * The preamplifier gain is considered to be part of the Detector configuration
 * so when setting it, the most important step (besides recalculating the
 * overall gain) is to update the Detector configuration value.
 */
PSL_STATIC int psl__SetPreampGain(int detChan, int modChan, char *name, void *value,
                                  char *detType, XiaDefaults *defs, Module *m,
                                  Detector *det, FirmwareSet *fs)
{
    int status;

    double preampGain = 0.0;

    UNUSED(name);
    UNUSED(fs);
    UNUSED(detType);


    ASSERT(value != NULL);
    ASSERT(det != NULL);
    ASSERT(defs != NULL);
    ASSERT(m != NULL);


    preampGain = *((double *)value);

    /* Update the Detector configuration */
    det->gain[m->detector_chan[modChan]] = preampGain;

    status = pslSetDefault("preamp_gain", &preampGain, defs);
    ASSERT(status == XIA_SUCCESS);

    status = psl__UpdateGain(detChan, modChan, defs, m, det);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error updating gain while setting preamplifier gain "
                "for detChan %d", detChan);
        pslLogError("psl__SetPreampGain", info_string, status);
        return status;
    }

    status = psl__UpdateThresholds(detChan, modChan, defs, m, det);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error updating thresholds for detChan %d", detChan);
        pslLogError("psl__SetPreampGain", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Synchronizes the preamplifier gain in the Detector configuration
 * with the preamp_gain acquisition value.
 *
 * Handel assumes that the preamplifier gain specified in the Detector
 * configuration is correct and uses it to set the the acquisition value
 * preamp_gain.
 *
 * This routine does not cause the gain to be recalculated.
 */
PSL_STATIC int psl__SynchPreampGain(int detChan, int det_chan, Module *m,
                                    Detector *det, XiaDefaults *defs)
{
    int status;

    double preampGain = 0.0;

    UNUSED(m);


    ASSERT(det != NULL);
    ASSERT(defs != NULL);


    preampGain = det->gain[det_chan];

    status = pslSetDefault("preamp_gain", (void *)&preampGain, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error synchronizing preamplifier gain for detChan %d",
                detChan);
        pslLogError("psl__SynchPreampGain", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Set the number of MCA channels.
 */
PSL_STATIC int psl__SetNumMCAChans(int detChan, int modChan, char *name, void *value,
                                   char *detType, XiaDefaults *defs, Module *m,
                                   Detector *det, FirmwareSet *fs)
{
    int status;

    parameter_t MCALIMLO = 0;
    parameter_t MCALIMHI = 0;

    double *mcaChans;
    int nMCAChans;

    UNUSED(name);
    UNUSED(modChan);
    UNUSED(defs);
    UNUSED(m);
    UNUSED(det);
    UNUSED(fs);
    UNUSED(detType);


    ASSERT(value != NULL);

    mcaChans = (double *)value;
    nMCAChans = (int) *mcaChans;

    /* Only allow multiples of MERCURY_MEMORY_BLOCK_SIZE */
    if (nMCAChans % MERCURY_MEMORY_BLOCK_SIZE != 0) {
        nMCAChans -= nMCAChans % MERCURY_MEMORY_BLOCK_SIZE;
        sprintf(info_string, "The number of MCA channels specified by the user '%f' "
                "is not a multiple of %d for detChan %d, it was reset to %d",
                *mcaChans, MERCURY_MEMORY_BLOCK_SIZE, detChan, nMCAChans);
        pslLogWarning("psl__SetNumMCAChans", info_string);
    }

    if ((nMCAChans > MAX_MCA_CHANNELS) || (nMCAChans < MIN_MCA_CHANNELS)) {
        sprintf(info_string, "The number of MCA channels specified by the user "
                ",'%d', is not in the allowed range (%f, %f) for detChan %d",
                nMCAChans, MIN_MCA_CHANNELS, MAX_MCA_CHANNELS, detChan);
        pslLogError("psl__SetNumMCAChans", info_string, XIA_BINS_OOR);
        return XIA_BINS_OOR;
    }

    /* In case mcaChans was updated, the value is passed back here */
    *mcaChans = (double) nMCAChans;

    status = pslGetParameter(detChan, "MCALIMLO", &MCALIMLO);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting MCA low limit while setting the number "
                "of MCA channels for detChan %d", detChan);
        pslLogError("psl__SetNumMCAChans", info_string, status);
        return status;
    }

    /* By convention, we always have an extra channel in the spectrum. That is
     * why there is no "- 1" in the following equation:
     */
    MCALIMHI  = (parameter_t)(MCALIMLO + nMCAChans);


    status = pslSetParameter(detChan, "MCALIMHI", MCALIMHI);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting upper MCA limit while setting the number "
                "of MCA channels for detChan %d", detChan);
        pslLogError("psl__SetNumMCAChans", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Set the slow filter gap time.
 */
PSL_STATIC int psl__SetGapTime(int detChan, int modChan, char *name,
                               void *value, char *detType, XiaDefaults *defs,
                               Module *m, Detector *det, FirmwareSet *fs)
{
    UNUSED(detChan);
    UNUSED(modChan);
    UNUSED(name);
    UNUSED(value);
    UNUSED(detType);
    UNUSED(defs);
    UNUSED(m);
    UNUSED(det);
    UNUSED(fs);


    return XIA_SUCCESS;
}


/*
 * Get the slow filter gap time.
 */
PSL_STATIC int psl__GetGapTime(int detChan, void *value, XiaDefaults *defs)
{
    int status;

    parameter_t SLOWGAP    = 0;
    parameter_t DECIMATION = 0;

    double tick = psl__GetClockTick();

    UNUSED(defs);


    ASSERT(value != NULL);


    status = pslGetParameter(detChan, "SLOWGAP", &SLOWGAP);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting slow filter gap time for detChan %d",
                detChan);
        pslLogError("psl__GetGapTime", info_string, status);
        return status;
    }

    status = pslGetParameter(detChan, "DECIMATION", &DECIMATION);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting decimation for detChan %d", detChan);
        pslLogError("psl__GetGapTime", info_string, status);
        return status;
    }

    /* Scale to microseconds */
    tick *= 1.0e6;

    *((double *)value) = tick * (double)SLOWGAP * pow(2.0, (double)DECIMATION);

    return XIA_SUCCESS;
}

/*
 * Set the trigger filter peaking time
 */
PSL_STATIC int psl__SetTrigPeakingTime(int detChan, int modChan, char *name, void *value,
                                       char *detType, XiaDefaults *defs, Module *m,
                                       Detector *det, FirmwareSet *fs)
{
    int status;

    UNUSED(name);
    UNUSED(modChan);
    UNUSED(detType);
    UNUSED(m);
    UNUSED(det);
    UNUSED(fs);


    ASSERT(value != NULL);
    ASSERT(defs != NULL);


    status = pslSetDefault("trigger_peaking_time", value, defs);
    ASSERT(status == XIA_SUCCESS);

    status = psl__UpdateTrigFilterParams(m, detChan, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error updating trigger filter parameters for detChan %d",
                detChan);
        pslLogError("psl__SetTrigPeakingTime", info_string, status);
        return status;
    }

    /* The peaking time may have changed, so update it for the user here. */
    status = pslGetDefault("trigger_peaking_time", value, defs);
    ASSERT(status == XIA_SUCCESS);

    return XIA_SUCCESS;
}


/*
 * Update the trigger filter parameters.
 */
PSL_STATIC int psl__UpdateTrigFilterParams(Module *m, int detChan,
                                            XiaDefaults *defs)
{
    int status;

    double trigPT = 0.0;
    double trigGT = 0.0;
    double tick   = psl__GetClockTick();
    double fl     = 0.0;
    double fg     = 0.0;
    double fscale = 0.0;

    parameter_t FASTLEN = 0;
    parameter_t FASTGAP = 0;
    parameter_t FSCALE  = 0;

    boolean_t isMercuryOem = psl__IsMercuryOem(detChan);

    UNUSED(m);

    ASSERT(defs != NULL);


    status = pslGetDefault("trigger_peaking_time", (void *)&trigPT, defs);
    ASSERT(status == XIA_SUCCESS);
    status = pslGetDefault("trigger_gap_time", (void *)&trigGT, defs);
    ASSERT(status == XIA_SUCCESS);

    /* Scale tick to microseconds */
    tick *= 1.0e6;

    fl = trigPT / tick;
    FASTLEN = (parameter_t)ROUND(fl);

    if (FASTLEN < MIN_FASTLEN || FASTLEN > MAX_FASTLEN) {
        sprintf(info_string, "Calculated trigger filter length (%hu) is not in the "
                "allowed range (%d, %d) for detChan %d", FASTLEN, MIN_FASTLEN,
                MAX_FASTLEN, detChan);
        pslLogError("psl__UpdateTrigFilterParams", info_string, XIA_FASTLEN_OOR);
        return XIA_FASTLEN_OOR;
    }

    fg = trigGT / tick;
    FASTGAP = (parameter_t)ROUND(fg);

    sprintf(info_string, "trigGT = %0.2f, fg = %0.2f, FASTGAP = %hu", trigGT, fg,
            FASTGAP);
    pslLogDebug("psl__UpdateTrigFilterParams", info_string);

    /* Don't worry too much about the limits on this. Just make sure that it
     * works with FASTLEN.
     */
    if ((FASTLEN + FASTGAP) > MAX_FASTFILTER) {
        sprintf(info_string, "Total fast filter length (%hd) is larger then the "
                "maximum allowed size (%d) for detChan %d",
                FASTLEN + FASTGAP, MAX_FASTFILTER, detChan);
        pslLogWarning("psl__UpdateTrigFilterParams", info_string);

        FASTGAP = (parameter_t)(MAX_FASTFILTER - FASTLEN);
        ASSERT(FASTGAP >= MIN_FASTGAP);

        sprintf(info_string, "Recalculated fast filter gap is %hu for detChan %d",
                FASTGAP, detChan);
        pslLogInfo("psl__UpdateTrigFilterParams", info_string);
    }

    status = pslSetParameter(detChan, "FASTLEN", FASTLEN);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting fast filter length for detChan %d",
                detChan);
        pslLogError("psl__UpdateTrigFilterParams", info_string, status);
        return status;
    }

    status = pslSetParameter(detChan, "FASTGAP", FASTGAP);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting fast filter gap for detChan %d",
                detChan);
        pslLogError("psl__UpdateTrigFilterParams", info_string, status);
        return status;
    }

    if (!isMercuryOem) {
        fscale = ceil(log((double)FASTLEN) / log(2.0)) - 1.0;
        FSCALE = (parameter_t)ROUND(fscale);

        status = pslSetParameter(detChan, "FSCALE", FSCALE);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error setting fast filter scaling for detChan %d",
                    detChan);
            pslLogError("psl__UpdateTrigFilterParams", info_string, status);
            return status;
        }
    }

    /* Recompute acquisition values based on -- potentially -- rounded DSP
     * parameter values.
     */
    fl = (double)FASTLEN * tick;
    status = pslSetDefault("trigger_peaking_time", (void *)&fl, defs);
    ASSERT(status == XIA_SUCCESS);

    fg = (double)FASTGAP * tick;
    status = pslSetDefault("trigger_gap_time", (void *)&fg, defs);
    ASSERT(status == XIA_SUCCESS);

    return XIA_SUCCESS;
}


/*
 * Sets the trigger filter gap time.
 */
PSL_STATIC int psl__SetTrigGapTime(int detChan, int modChan, char *name, void *value,
                                   char *detType, XiaDefaults *defs, Module *m,
                                   Detector *det, FirmwareSet *fs)
{
    int status;

    UNUSED(name);
    UNUSED(modChan);
    UNUSED(detType);
    UNUSED(m);
    UNUSED(det);
    UNUSED(fs);


    ASSERT(value != NULL);
    ASSERT(defs != NULL);


    status = pslSetDefault("trigger_gap_time", value, defs);
    ASSERT(status == XIA_SUCCESS);

    status = psl__UpdateTrigFilterParams(m, detChan, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error updating trigger filter parameters for detChan %d",
                detChan);
        pslLogError("psl__SetTrigGapTime", info_string, status);
        return status;
    }

    /* The gap time may have changed, so update it for the user here. */
    status = pslGetDefault("trigger_gap_time", value, defs);
    ASSERT(status == XIA_SUCCESS);

    return XIA_SUCCESS;
}


/*
 * Set the maximum width of the trigger filter pile-up inspection
 */
PSL_STATIC int psl__SetMaxWidth(int detChan, int modChan, char *name,
                                void *value, char *detType,
                                XiaDefaults *defs, Module *m,
                                Detector *det, FirmwareSet *fs)
{
    int status;

    parameter_t MAXWIDTH;

    /* Scale the tick to microseconds. */
    double tick = psl__GetClockTick() * 1.0e6;

    UNUSED(fs);
    UNUSED(det);
    UNUSED(m);
    UNUSED(defs);
    UNUSED(detType);
    UNUSED(name);
    UNUSED(modChan);


    ASSERT(value != NULL);


    MAXWIDTH = (parameter_t)ROUND(*((double *)value) / tick);

    if (MAXWIDTH < MIN_MAXWIDTH || MAXWIDTH > MAX_MAXWIDTH) {
        sprintf(info_string, "Requested max. width (%0.3f microseconds) is "
                "out-of-range (%0.3f, %0.3f) for detChan %d",
                *((double *)value), MIN_MAXWIDTH * tick, MAX_MAXWIDTH * tick,
                detChan);
        pslLogError("psl__SetMaxWidth", info_string, XIA_MAXWIDTH_OOR);
        return XIA_MAXWIDTH_OOR;
    }

    status = pslSetParameter(detChan, "MAXWIDTH", MAXWIDTH);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting max. width for detChan %d", detChan);
        pslLogError("psl__SetMaxWidth", info_string, status);
        return status;
    }

    *((double *)value) = (double)MAXWIDTH * tick;

    return XIA_SUCCESS;
}


/*
 * Set the baseline average length.
 *
 * Must be a power of 2, but if it isn't then it is silently rounded.
 */
PSL_STATIC int psl__SetBaseAvg(int detChan, int modChan, char *name, void *value,
                               char *detType, XiaDefaults *defs, Module *m,
                               Detector *det, FirmwareSet *fs)
{
    int status;

    parameter_t BLAVGDIV = 0;

    double len = 0.0;

    UNUSED(name);
    UNUSED(modChan);
    UNUSED(detType);
    UNUSED(defs);
    UNUSED(m);
    UNUSED(det);
    UNUSED(fs);


    ASSERT(value != NULL);


    len      = *((double *)value);
    BLAVGDIV = (parameter_t)(((parameter_t)ROUND(log(len) / log(2.0))) - 1);

    status = pslSetParameter(detChan, "BLAVGDIV", BLAVGDIV);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting baseline average to %f for detChan %d",
                len, detChan);
        pslLogError("psl__SetBaseAvg", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


PSL_STATIC int psl__SetPreampType(int detChan, int modChan, char *name,
                                  void *value, char *detType,
                                  XiaDefaults *defs, Module *m, Detector *det,
                                  FirmwareSet *fs)
{
    int status;

    double pt;
    double newPreampType;
    double resetDelay;
    double decayTime;
    double currentPreampType;

    UNUSED(name);


    ASSERT(value != NULL);
    ASSERT(defs != NULL);
    ASSERT(fs != NULL);
    ASSERT(det != NULL);
    ASSERT(m != NULL);
    ASSERT(detType != NULL);


    status = pslGetDefault("peaking_time", &pt, defs);
    ASSERT(status == XIA_SUCCESS);

    newPreampType = *((double *)value);

    status = pslGetDefault("preamp_type", &currentPreampType, defs);
    ASSERT(status == XIA_SUCCESS);

    if (newPreampType == currentPreampType) {
        sprintf(info_string, "Current preamplifier type is same as requested "
                "preamplifier type. Not switching.");
        pslLogInfo("psl__SetPreampType", info_string);
        return XIA_SUCCESS;
    }

    sprintf(info_string, "newPreampType = %f, preparing to switch firmware",
            newPreampType);
    pslLogDebug("psl__SetPreampType", info_string);

    status = psl__SwitchFirmware(detChan, newPreampType, modChan, pt, fs, m);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error switching firmware for detChan %d", detChan);
        pslLogError("psl__SetPreampType", info_string, status);
        return status;
    }

    if (newPreampType == XIA_PREAMP_RESET) {
        det->type = XIA_DET_RESET;

        /* Redownload the reset interval. */
        status = pslGetDefault("reset_delay", &resetDelay, defs);
        ASSERT(status == XIA_SUCCESS);

        det->typeValue[m->detector_chan[modChan]] = resetDelay;

        status = pslSetAcquisitionValues(detChan, "reset_delay", &resetDelay, defs,
                                         fs, &(m->currentFirmware[modChan]),
                                         "RESET", det, m->detector_chan[modChan],
                                         m, modChan);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error updating reset delay to %0.3f after "
                    "switching to reset firmware for detChan %d", resetDelay,
                    detChan);
            pslLogError("psl__SetPreampType", info_string, status);
            return status;
        }

    } else if (newPreampType == XIA_PREAMP_RC) {
        det->type = XIA_DET_RCFEED;

        /* Redownload the RC decay time. */
        status = pslGetDefault("decay_time", &decayTime, defs);
        ASSERT(status == XIA_SUCCESS);

        sprintf(info_string, "'decay_time' = %0.3f", decayTime);
        pslLogDebug("psl__SetPreampType", info_string);

        det->typeValue[m->detector_chan[modChan]] = decayTime;

        status = pslSetAcquisitionValues(detChan, "decay_time", &decayTime, defs,
                                         fs, &(m->currentFirmware[modChan]),
                                         "RC", det, m->detector_chan[modChan], m,
                                         modChan);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error updating RC decay time to %0.3f after "
                    "switching to RC feedback firmware for detChan %d", decayTime,
                    detChan);
            pslLogError("psl__SetPreampType", info_string, status);
            return status;
        }

    } else {
        sprintf(info_string, "Unknown preamplifier type (%0.1f) for detChan %d",
                newPreampType, detChan);
        pslLogError("psl__SetPreampType", info_string, XIA_UNKNOWN_PREAMP_TYPE);
        return XIA_UNKNOWN_PREAMP_TYPE;
    }

    /* If we don't update the preamp_type now, we will get stuck in an
     * infinite loop of acquisition value updating.
     */
    status = pslSetDefault("preamp_type", &newPreampType, defs);
    ASSERT(status == XIA_SUCCESS);

    status = pslUserSetup(detChan, defs, fs, &(m->currentFirmware[modChan]),
                          detType, det, m->detector_chan[modChan], m, modChan);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error reloading acquisition values after switching "
                "preamplifier types on detChan %d", detChan);
        pslLogError("psl__SetPreampType", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Synchronize the detector preamplifier type in the Detector
 * configuration with the preamp_type acquisition value.
 */
PSL_STATIC int psl__SynchPreampType(int detChan, int det_chan, Module *m,
                                    Detector *det, XiaDefaults *defs)
{
    int status;

    double type;

    UNUSED(m);
    UNUSED(det_chan);


    ASSERT(det != NULL);
    ASSERT(defs != NULL);


    switch (det->type) {
    case XIA_DET_RESET:
        type = XIA_PREAMP_RESET;
        break;
    case XIA_DET_RCFEED:
        type = XIA_PREAMP_RC;
        break;
    default:
        FAIL();
    }

    status = pslSetDefault("preamp_type", &type, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error synchronizing detector preamplifier type for "
                "detChan %d", detChan);
        pslLogError("psl__SynchPreampType", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Switches firmware (DSP, FiPPI) to the preamplifer type.
 */
PSL_STATIC int psl__SwitchFirmware(int detChan, double type, int modChan,
                                   double pt, FirmwareSet *fs, Module *m)
{
    int status;
    boolean_t isMercuryOem = psl__IsMercuryOem(detChan);

    char fippi[MAX_PATH_LEN];
    char dsp[MAX_PATH_LEN];
    char rawFippi[MAXFILENAME_LEN];
    char rawDSP[MAXFILENAME_LEN];
    char preamptype[MAXITEM_LEN];

    ASSERT(fs != NULL);
    ASSERT(m  != NULL);

    if (type == XIA_PREAMP_RESET) {
        strcpy(preamptype, "RESET");
    } else {
        strcpy(preamptype, "RC");
    }

    sprintf(info_string, "Switching to %s preamp", preamptype);
    pslLogDebug("psl__SwitchFirmware", info_string);

    if (!isMercuryOem) {
        status = psl__GetFiPPIName(modChan, pt, fs, preamptype, fippi, rawFippi);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Unable to get the name of the FiPPI that supports "
                    "%s preamplifiers for peaking time = %0.3f microseconds for "
                    "detChan %d", preamptype,pt, detChan);
            pslLogError("psl__SwitchFirmware", info_string, status);
            if (status == XIA_FILEERR) {
                /* Reset status to a more meaningful code */
                status = XIA_NOSUPPORTED_PREAMP_TYPE;
            }
            return status;
        }

        status = pslDownloadFirmware(detChan, "fippi_a_dsp_no_wake", fippi, m,
                                     rawFippi, NULL);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error downloading new FiPPI for peaking time = "
                    "%0.3f microseconds for detChan %d", pt, detChan);
            pslLogError("psl__SwitchFirmware", info_string, status);
            return status;
        }
    }

    status = psl__GetDSPName(modChan, pt, fs, preamptype, dsp, rawDSP);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Unable to get the DSP that supports %s "
                "preamplifiers for peaking time = %0.3f microseconds for "
                "detChan %d", preamptype, pt, detChan);
        pslLogError("psl__SwitchFirmware", info_string, status);
        return status;
    }

    status = pslDownloadFirmware(detChan, "dsp", dsp, m, rawDSP, NULL);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error downloading new DSP for peaking time = "
                "%0.3f microseconds for detChan %d", pt, detChan);
        pslLogError("psl__SwitchFirmware", info_string, status);
        return status;
    }

    status = psl__WakeDSP(detChan);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error waking new DSP for detChan %d", detChan);
        pslLogError("psl__SwitchFirmware", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Retrieve the name of the DSP for the requested detector preamplifier
 * type.
 */
PSL_STATIC int psl__GetDSPName(int modChan, double pt, FirmwareSet *fs,
                               char *detType, char *name, char *rawName)
{
    int status;

    char *tmpPath = NULL;


    ASSERT(fs != NULL);


    if (fs->filename == NULL) {
        sprintf(info_string, "Only FDD files are currently supported for the "
                "xMAP (modChan = %d)", modChan);
        pslLogError("psl__GetDSPName", info_string, XIA_NO_FDD);
        return XIA_NO_FDD;
    }

    if (fs->tmpPath == NULL) {
        tmpPath = utils->funcs->dxp_md_tmp_path();
    } else {
        tmpPath = fs->tmpPath;
    }

    status = xiaFddGetFirmware(fs->filename, tmpPath, "system_dsp", pt, 0, NULL,
                               detType, name, rawName);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting the DSP filename from '%s' with a "
                "peaking time of %0.3f microseconds", fs->filename, pt);
        pslLogError("psl__GetDSPName", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Downloads the requested file to FiPPI A, but doesn't wake the
 * DSP up after the download is complete. (Compare with
 * psl__DownloadFiPPIA().)
 */
PSL_STATIC int psl__DownloadFiPPIADSPNoWake(int detChan, char *file,
                                            char *rawFile, Module *m)
{
    int status;

    unsigned int i;
    unsigned int modChan = 0;


    ASSERT(file    != NULL);
    ASSERT(rawFile != NULL);
    ASSERT(m       != NULL);

    if (STREQ(rawFile, m->currentFirmware[modChan].currentFiPPI)) {
        sprintf(info_string, "Requested FiPPI '%s' is already running on detChan %d",
                file, detChan);
        pslLogInfo("psl__DownloadFiPPIA", info_string);
        return XIA_SUCCESS;
    }

    status = dxp_replace_fpgaconfig(&detChan, "a_dsp_no_wake", file);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error switching to new FiPPI '%s' for detChan %d",
                file, detChan);
        pslLogError("psl__DownloadFiPPIA", info_string, status);
        return status;
    }

    /* Since we just downloaded the FiPPI for all 4 channels, set the current
     * firmware for all 4 channels to the new file name. This prevents Handel
     * from thinking that it needs to download the firmware 4 times. When we add
     * support for FiPPI B, this will be reduced to the 2 channels covered by
     * FiPPI A.
     */
    for (i = 0; i < m->number_of_channels; i++) {
        strcpy(m->currentFirmware[i].currentFiPPI, rawFile);
    }

    return XIA_SUCCESS;
}


/*
 * Downloads the requested DSP code to the hardware.
 */
PSL_STATIC int psl__DownloadDSP(int detChan, char *file, char *rawFile,
                                Module *m)
{
    int status;

    unsigned int i;
    unsigned int modChan;


    ASSERT(file    != NULL);
    ASSERT(rawFile != NULL);
    ASSERT(m       != NULL);

    sprintf(info_string, "Changing DSP to '%s' for detChan %d", file, detChan);
    pslLogDebug("psl__DownloadDSP", info_string);

    status = pslGetModChan(detChan, m, &modChan);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting module channel for detChan = %d",
                detChan);
        pslLogError("psl__DownloadDSP", info_string, status);
        return status;
    }

    if (STREQ(rawFile, m->currentFirmware[modChan].currentDSP)) {
        sprintf(info_string, "Requested DSP '%s' is already running on detChan %d",
                file, detChan);
        pslLogInfo("psl__DownloadDSP", info_string);
        return XIA_SUCCESS;
    }

    status = dxp_replace_dspconfig(&detChan, file);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error downloading new DSP '%s' for detChan %d",
                file, detChan);
        pslLogError("psl__DownloadDSP", info_string, status);
        return status;
    }

    /* Even though the xMAP only has a single DSP, we need to update the "DSP"
     * for all of the channels in the module.
     */
    for (i = 0; i < m->number_of_channels; i++) {
        strcpy(m->currentFirmware[i].currentDSP, rawFile);
    }

    return XIA_SUCCESS;
}

/*
 * Tell the DSP to wake-up.
 */
PSL_STATIC int psl__WakeDSP(int detChan)
{
    int status;

    short task = MERCURY_CT_WAKE_DSP;


    status = dxp_start_control_task(&detChan, &task, NULL, NULL);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error starting control task to wake the DSP for "
                "detChan %d", detChan);
        pslLogError("psl__WakeDSP", info_string, status);
        return status;
    }

    status = dxp_stop_control_task(&detChan);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error stopping control task to wake the DSP for "
                "detChan %d", detChan);
        pslLogError("psl__WakeDSP", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Specify an optional peak sample time offset, in microseconds,
 * that overrides the value specified in the FDD file.
 */
PSL_STATIC int psl__SetPeakSampleOffset(int detChan, int modChan, char *name,
                                        void *value, char *detType,
                                        XiaDefaults *defs, Module *m,
                                        Detector *det, FirmwareSet *fs)
{
    int dec = 0;
    int status;
    int offset;

    parameter_t DECIMATION = 0;

    double pt   = 0.0;
    double tick = psl__GetClockTick() * 1.0e6;

    UNUSED(detType);


    ASSERT(name != NULL);
    ASSERT(defs != NULL);
    ASSERT(value != NULL);
    ASSERT(fs != NULL);
    ASSERT(m != NULL);
    ASSERT(det != NULL);


    /* Get the decimation that this value applies to so we can check
     * if we need to update PEAKSAM.
     */
    if (sscanf(name, "peak_sample_offset%d", &dec) != 1) {
        sprintf(info_string, "Malformed peak sample offset string '%s' for "
            "detChan %d", name, detChan);
        pslLogError("psl__SetPeakSampleOffset", info_string, XIA_BAD_NAME);
        return XIA_BAD_NAME;
    }

    if (dec != 0 && dec != 2 && dec != 4 && dec != 6) {
        sprintf(info_string, "Specified decimation (%d) is invalid. Allowed "
                "values are 0, 2, 4 and 6 for detChan %d",
                dec, detChan);
        pslLogError("psl__SetPeakSampleOffset", info_string, XIA_BAD_DECIMATION);
        return XIA_BAD_DECIMATION;
    }

    status = pslGetParameter(detChan, "DECIMATION", &DECIMATION);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting currenr decimation for detChan %d",
                detChan);
        pslLogError("psl__SetPeakSampleOffset", info_string, status);
        return status;
    }

    if (dec == (int)DECIMATION) {
        status = pslSetDefault(name, value, defs);
        ASSERT(status == XIA_SUCCESS);

        status = pslGetDefault("peaking_time", (void *)&pt, defs);
        ASSERT(status == XIA_SUCCESS);

        status = psl__UpdateFilterParams(detChan, modChan, pt, defs, fs, m, det);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error updating filter parameter after peak sample "
                    "offset changed to %0.3f for detChan %d", *((double *)value),
                    detChan);
            pslLogError("psl__SetPeakSampleOffset", info_string, status);
            return status;
        }
    }

    /* Calculate the actual offset time in decimated clock ticks. */
    offset = (int)ROUND(*((double *)value) / ldexp(tick, dec));
    *((double *)value) = offset * ldexp(tick, dec);

    return XIA_SUCCESS;
}


/*
 * Set the peak interval offset for the specified decimation.
 */
PSL_STATIC int psl__SetPeakIntervalOffset(int detChan, int modChan, char *name,
                                          void *value, char *detType,
                                          XiaDefaults *defs, Module *m,
                                          Detector *det, FirmwareSet *fs)
{
    int dec = 0;
    int status;
    int offset;

    parameter_t DECIMATION = 0;

    double pt   = 0.0;
    double tick = psl__GetClockTick() * 1.0e6;

    UNUSED(detType);


    ASSERT(name != NULL);
    ASSERT(defs != NULL);
    ASSERT(value != NULL);
    ASSERT(fs != NULL);
    ASSERT(m != NULL);
    ASSERT(det != NULL);


    /* Get the decimation that this value applies to so we can check
     * if we need to update PEAKINT.
     */
    if (sscanf(name, "peak_interval_offset%d", &dec) != 1) {
        sprintf(info_string, "Malformed peak interval offset string '%s' for "
            "detChan %d", name, detChan);
        pslLogError("psl__SetPeakIntervalOffset", info_string, XIA_BAD_NAME);
        return XIA_BAD_NAME;
    }

    if (dec != 0 && dec != 2 && dec != 4 && dec != 6) {
        sprintf(info_string, "Specified decimation (%d) is invalid. Allowed "
                "values are 0, 2, 4 and 6 for detChan %d",
                dec, detChan);
        pslLogError("psl__SetPeakIntervalOffset", info_string, XIA_BAD_DECIMATION);
        return XIA_BAD_DECIMATION;
    }

    status = pslGetParameter(detChan, "DECIMATION", &DECIMATION);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting currenr decimation for detChan %d",
                detChan);
        pslLogError("psl__SetPeakIntervalOffset", info_string, status);
        return status;
    }

    if (dec == (int)DECIMATION) {
        status = pslSetDefault(name, value, defs);
        ASSERT(status == XIA_SUCCESS);

        status = pslGetDefault("peaking_time", (void *)&pt, defs);
        ASSERT(status == XIA_SUCCESS);

        status = psl__UpdateFilterParams(detChan, modChan, pt, defs, fs, m, det);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error updating filter parameter after peak interval "
                    "offset changed to %0.3f for detChan %d", *((double *)value),
                    detChan);
            pslLogError("psl__SetPeakIntervalOffset", info_string, status);
            return status;
        }
    }

    offset = (int)ROUND(*((double *)value) / ldexp(tick, dec));
    *((double *)value) = offset * ldexp(tick, dec);

    return XIA_SUCCESS;
}


/*
 * Get the length of the baseline history buffer.
 */
PSL_STATIC int psl__GetBaseHistoryLen(int detChan, void *value, XiaDefaults *defs)
{
    int status;

    parameter_t TRACELEN = 0;

    UNUSED(defs);


    ASSERT(value != NULL);


    status = pslGetParameter(detChan, "TRACELEN", &TRACELEN);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error reading TRACELEN from detChan %d", detChan);
        pslLogError("psl__GetBaseHistoryLen", info_string, status);
        return status;
    }

    *((unsigned long *)value) = (unsigned long)TRACELEN;

    return XIA_SUCCESS;
}


/*
 * Sets the number of SCAs for the module
 */
PSL_STATIC int psl__SetNumberSCAs(int detChan, int modChan, char *name, void *value,
                                  char *detType, XiaDefaults *defs, Module *m,
                                  Detector *det, FirmwareSet *fs)
{
    int status;

    size_t scaArraySize = 0;

    double nSCA = 0.0;

    unsigned short i;

    char limit[9];

    XiaDaqEntry *e = NULL;

    UNUSED(name);
    UNUSED(detType);
    UNUSED(defs);
    UNUSED(det);
    UNUSED(fs);


    ASSERT(value != NULL);
    ASSERT(m != NULL);

    nSCA = *((double *)value);

    if ((unsigned short)nSCA > MAX_NUM_INTERNAL_SCA) {
        sprintf(info_string, "Number of SCAs is greater then the maximum allowed "
                "%d for detChan %d", MAX_NUM_INTERNAL_SCA, detChan);
        pslLogError("psl__SetNumberSCAs", info_string, XIA_MAX_SCAS);
        return XIA_MAX_SCAS;
    }

    /* If the number of SCAs shrank then we need to remove the limits
     * that are greater then the new number of SCAs.
     * This is a little hacky and will be improved in the future.
     */
    if ((unsigned short)nSCA < m->ch[modChan].n_sca) {

        for (i = (unsigned short)nSCA; i < m->ch[modChan].n_sca; i++) {
            sprintf(info_string, "Removing sca%hu_* limits for detChan %d",
                    i, detChan);
            pslLogDebug("psl__SetNumberSCAs", info_string);

            sprintf(limit, "sca%hu_lo", i);

            status = pslRemoveDefault(limit, defs, &e);

            if (status != XIA_SUCCESS) {
                sprintf(info_string, "Unable to remove SCA limit '%s' for "
                        "detChan %d", limit, detChan);
                pslLogWarning("psl__SetNumberSCAs", info_string);

            }

            /* pslRemoveDefault will not free the returned XiaDaqEntry */
            if (e != NULL) {
                mercury_psl_md_free(e->name);
                mercury_psl_md_free(e);
            }

            sprintf(limit, "sca%hu_hi", i);

            status = pslRemoveDefault(limit, defs, &e);

            if (status != XIA_SUCCESS) {
                sprintf(info_string, "Unable to remove SCA limit '%s' for "
                        "detChan %d", limit, detChan);
                pslLogWarning("psl__SetNumberSCAs", info_string);
            }

            if (e != NULL) {
                mercury_psl_md_free(e->name);
                mercury_psl_md_free(e);
            }


        }
    }

    /* If any SCAs are previously defined, clear them out. In the future, this
     * is where we would allow the SCA array to be safely expanded (or compressed).
     */
    if ((m->ch[modChan].sca_lo != NULL) ||
            (m->ch[modChan].sca_hi != NULL)) {
        status = pslDestroySCAs(m, modChan);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error freeing SCAs in module '%s', detChan %d",
                    m->alias, detChan);
            pslLogError("psl__SetNumberSCAs", info_string, status);
            return status;
        }
    }

    status = pslSetParameter(detChan, "NUMSCA", (unsigned short)nSCA);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting the number of SCAs for detChan %d",
                detChan);
        pslLogError("psl__SetNumberSCAs", info_string, status);
        return status;
    }

    m->ch[modChan].n_sca = (unsigned short)nSCA;

    if (nSCA > 0) {

        scaArraySize = m->ch[modChan].n_sca * sizeof(unsigned short);

        m->ch[modChan].sca_lo = (unsigned short *)mercury_psl_md_alloc(scaArraySize);

        if (!m->ch[modChan].sca_lo) {
            m->ch[modChan].n_sca = 0;
            sprintf(info_string, "Unable to allocate %zu bytes for sca_lo on detChan %d",
                    scaArraySize, detChan);
            pslLogError("psl__SetNumberSCAs", info_string, XIA_NOMEM);
            return XIA_NOMEM;
        }

        memset(m->ch[modChan].sca_lo, 0, scaArraySize);

        m->ch[modChan].sca_hi = (unsigned short *)mercury_psl_md_alloc(scaArraySize);

        if (!m->ch[modChan].sca_hi) {
            m->ch[modChan].n_sca = 0;
            mercury_psl_md_free(m->ch[modChan].sca_lo);

            sprintf(info_string, "Unable to allocate %zu bytes for sca_hi on detChan %d",
                    scaArraySize, detChan);
            pslLogError("psl__SetNumberSCAs", info_string, XIA_NOMEM);
            return XIA_NOMEM;
        }

        memset(m->ch[modChan].sca_hi, 0, scaArraySize);

    }

    return XIA_SUCCESS;
}

/*
 * Set the SCA specified in the name.
 *
 * The name should have the format sca{n}_[lo|hi], where
 * n refers to the SCA #.
 */
PSL_STATIC int psl__SetSCA(int detChan, int modChan, char *name,
                           void *value, char *detType, XiaDefaults *defs,
                           Module *m, Detector *det, FirmwareSet *fs)
{
    int status;


    unsigned short scaNum = 0;

    parameter_t SCALIM = 0;

    unsigned long addr = 0;
    unsigned long data = 0;

    char *limParam = NULL;

    char limit[SCA_LIMIT_STR_LEN] = {0};
    char memory[DATA_MEMORY_STR_LEN] = {0};

    UNUSED(fs);
    UNUSED(det);
    UNUSED(defs);
    UNUSED(detType);


    ASSERT(m != NULL);
    ASSERT(STRNEQ(name, "sca"));

    if (sscanf(name, "sca%hu_%s", &scaNum, limit) != 2) {
        sprintf(info_string, "Malformed SCA string '%s' for detChan %d",
            name, detChan);
        pslLogError("psl__SetSCA", info_string, XIA_BAD_NAME);
        return XIA_BAD_NAME;
    }

    if (!(STREQ(limit, "lo") || STREQ(limit, "hi"))) {
        sprintf(info_string, "Malformed SCA string '%s': missing 'lo' or 'hi' "
                "specifier for detChan %d", name, detChan);
        pslLogError("psl__SetSCA", info_string, XIA_BAD_NAME);
        return XIA_BAD_NAME;
    }

    if (scaNum >= m->ch[modChan].n_sca) {
        sprintf(info_string, "Requested SCA number '%hu' is larger then the number "
                "of SCAs (%hu) for detChan %d", scaNum, m->ch[modChan].n_sca,
                detChan);
        pslLogError("psl__SetSCA", info_string, XIA_SCA_OOR);
        return XIA_SCA_OOR;
    }

    if (STREQ(limit, "lo")) {
        limParam = "SCALPTR";
    } else if (STREQ(limit, "hi")) {
        limParam = "SCAHPTR";
    } else {
        /* This is impossible. */
        FAIL();
    }

    status = pslGetParameter(detChan, limParam, &SCALIM);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting SCA limit parameter '%s' for detChan %d",
                limParam, detChan);
        pslLogError("psl__SetSCA", info_string, status);
        return status;
    }

    data = (unsigned long)(*((double *)value));

    addr = SCALIM + scaNum;

    sprintf(info_string, "SCA limit pointer value '%s' = %#lx", limParam, addr);
    pslLogDebug("psl__SetSCA", info_string);
    sprintf(info_string, "Preparing to set SCA limit: addr = %#lx", addr);
    pslLogDebug("psl__SetSCA", info_string);

    sprintf(memory, "data:%#lx:1", addr);

    status = dxp_write_memory(&detChan, memory, &data);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error writing SCA limit (%lu) for detChan %d",
                data, detChan);
        pslLogError("psl__SetSCA", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Get the maximum allowed number of SCA
 */
PSL_STATIC int psl__GetMaxSCALength(int detChan, void *value, XiaDefaults *defs,
                                    Module *m)
{
    UNUSED(detChan);
    UNUSED(defs);
    UNUSED(m);

    *((unsigned short *)value) = (unsigned short)MAX_NUM_INTERNAL_SCA;

    return XIA_SUCCESS;
}

/*
 * Get the length of the return SCA data array.
 */
PSL_STATIC int psl__GetSCALength(int detChan, void *value, XiaDefaults *defs,
                                 Module *m)
{
    int status;

    double nSCAs = 0.0;

    UNUSED(m);


    ASSERT(defs != NULL);


    status = pslGetDefault("number_of_scas", (void *)&nSCAs, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error finding 'number_of_scas' for detChan %d", detChan);
        pslLogError("psl__GetSCALength", info_string, status);
        return status;
    }

    *((unsigned short *)value) = (unsigned short)nSCAs;

    return XIA_SUCCESS;
}


/*
 * Get the SCA data array for the specified channel
 *
 * The user-supplied array, value, should be of type 'double'.
 */
PSL_STATIC int psl__GetSCAData(int detChan, void *value, XiaDefaults *defs,
                               Module *m)
{
    int status;
    int i;
    int j;

    unsigned int modChan;
    unsigned int addr = 0;
    unsigned int totalSCA = 0;

    unsigned long *sca = NULL;

    double nSCA = 0.0;

    parameter_t SCAMEMBASE = 0;
    char memory[DATA_MEMORY_STR_LEN];

    double *sca64 = (double *)value;

    ASSERT(defs != NULL);
    ASSERT(value != NULL);

    status = pslGetDefault("number_of_scas", (void *)&nSCA, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "'number_of_scas' is not in the acquisition value "
                "list for detChan %d. Are there SCAs configured for this channel?",
                detChan);
        pslLogError("psl__GetSCAData", info_string, status);
        return status;
    }

    if (nSCA == 0.0) {
        sprintf(info_string, "No SCAs defined for detChan = %d", detChan);
        pslLogError("psl__GetSCAData", info_string, XIA_SCA_OOR);
        return XIA_SCA_OOR;
    }

    status = pslGetParameter(detChan, "SCAMEMBASE", &SCAMEMBASE);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting SCA memory address for detChan %d", detChan);
        pslLogError("psl__GetSCAData", info_string, status);
        return status;
    }

    status = pslGetModChan(detChan, m, &modChan);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting module channel for detChan = %d",
                detChan);
        pslLogError("psl__GetSCAData", info_string, status);
        return status;
    }

    addr = (unsigned int)SCAMEMBASE + (modChan * MERCURY_SCA_CHAN_OFFSET);

    sprintf(info_string, "Reading out %d SCA value: addr = %#x", (int)nSCA, addr);
    pslLogDebug("psl__GetSCAData", info_string);

    /* The SCA values are 64 bits, total, so there are 2 32-bit words returned
    * per SCA.
    */
    totalSCA = (unsigned int)nSCA * 2;
    sca = (unsigned long *)mercury_psl_md_alloc(totalSCA * sizeof(unsigned long));

    if (!sca) {
        sprintf(info_string, "Error allocating %d bytes for 'sca'",
                (int)(totalSCA * sizeof(unsigned long)));
        pslLogError("psl__GetSCAData", info_string, XIA_NOMEM);
        return XIA_NOMEM;
    }

    sprintf(memory, "burst:%#x:%u", addr, totalSCA);
    status = dxp_read_memory(&detChan, memory, sca);

    if (status != DXP_SUCCESS) {
        mercury_psl_md_free(sca);
        sprintf(info_string, "Error reading sca value from memory %s for detChan %d",
                memory, detChan);
        pslLogError("psl__GetSCAData", info_string, status);
        return status;
    }

    for (i = 0, j = 0; i < (int)nSCA * 2; i += 2, j++) {
        sca64[j] = (double)sca[i] + ldexp(sca[i + 1], 32);
    }

    mercury_psl_md_free(sca);

    return XIA_SUCCESS;
}


/*
 * Gets the value of the MCR.
 */
PSL_STATIC int psl__GetMCR(int detChan, char *name, XiaDefaults *defs,
                           void *value)
{
    int status;

    UNUSED(defs);
    UNUSED(name);


    status = dxp_read_register(&detChan, "MCR", (unsigned long *)value);

    sprintf(info_string, "MCR = %#lx", *((unsigned long *)value));
    pslLogDebug("psl__GetMCR", info_string);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error reading MCR for detChan %d", detChan);
        pslLogError("psl__GetMCR", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Get the Mapping Flag Register.
 */
PSL_STATIC int psl__GetMFR(int detChan, char *name, XiaDefaults *defs,
                           void *value)
{
    int status;

    UNUSED(defs);
    UNUSED(name);


    status = dxp_read_register(&detChan, "MFR", (unsigned long *)value);

    sprintf(info_string, "MFR = %#lx", *((unsigned long *)value));
    pslLogDebug("psl__GetMFR", info_string);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error reading MFR for detChan %d", detChan);
        pslLogError("psl__GetMFR", info_string, status);
        return status;
    }

    return XIA_SUCCESS;

}


/*
 * Gets the Control Status Register.
 */
PSL_STATIC int psl__GetCSR(int detChan, char *name, XiaDefaults *defs,
                           void *value)
{
    int status;

    UNUSED(defs);
    UNUSED(name);


    status = dxp_read_register(&detChan, "CSR", (unsigned long *)value);

    sprintf(info_string, "CSR = %#lx", *((unsigned long *)value));
    pslLogDebug("psl__GetCSR", info_string);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error reading CSR for detChan %d", detChan);
        pslLogError("psl__GetCSR", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Read the CPLD Version Register.
 */
PSL_STATIC int psl__GetCVR(int detChan, char *name, XiaDefaults *defs,
                           void *value)
{
    int status;

    UNUSED(name);
    UNUSED(defs);


    ASSERT(value != NULL);


    status = dxp_read_register(&detChan, "CVR", (unsigned long *)value);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error reading CVR for detChan %d", detChan);
        pslLogError("psl__GetCVR", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Read the System FPGA Version Register.
 */
PSL_STATIC int psl__GetSVR(int detChan, char *name, XiaDefaults *defs,
                           void *value)
{
    int status;

    UNUSED(name);
    UNUSED(defs);


    ASSERT(value != NULL);


    status = dxp_read_register(&detChan, "SVR", (unsigned long *)value);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error reading SVR for detChan %d", detChan);
        pslLogError("psl__GetSVR", info_string, status);
        return status;
    }

    return XIA_SUCCESS;

}


/*
 * Queries board to see if it is running in mapping mode or not.
 */
PSL_STATIC int psl__IsMapping(int detChan,  unsigned short allowed,
                        boolean_t *isMapping)
{
    int status;

    status = psl__CheckBit(detChan, "VAR", MERCURY_VAR_DAQ_MODE, isMapping);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error reading firmware variant for detChan %d",
                detChan);
        pslLogError("psl__IsMapping", info_string, status);
        return status;
    }

    if (*isMapping) {
        parameter_t MAPPINGMODE;

        status = pslGetParameter(detChan, "MAPPINGMODE", &MAPPINGMODE);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error reading MAPPINGMODE for detChan %d",
                    detChan);
            pslLogError("psl__IsMapping", info_string, status);
            return status;
        }

        switch (MAPPINGMODE) {
        case MAPPINGMODE_MCA:
            *isMapping = (boolean_t)((allowed & MAPPING_MCA) > 0);
            break;

        case MAPPINGMODE_SCA:
            *isMapping = (boolean_t)((allowed & MAPPING_SCA) > 0);
            break;

        case MAPPINGMODE_LIST:
            *isMapping = (boolean_t)((allowed & MAPPING_LIST) > 0);
            break;

        default:
            FAIL();
            break;
        }

    } else {
        *isMapping = FALSE_;
    }

    return XIA_SUCCESS;
}


/*
 * Redownloads any acquisition values that meet the criteria specified
 * in type.
 */
PSL_STATIC int psl__UpdateParams(int detChan, unsigned short type,
                                 int modChan, char *name, void *value,
                                 char *detType, XiaDefaults *defs,
                                 Module *m, Detector *det, FirmwareSet *fs)
{
    unsigned long i;
    int status;

    XiaDaqEntry *entry = NULL;


    ASSERT(name != NULL);
    ASSERT(value != NULL);
    ASSERT(detType != NULL);
    ASSERT(m != NULL);
    ASSERT(det != NULL);
    ASSERT(fs != NULL);
    ASSERT(defs != NULL);


    entry = defs->entry;

    while (entry != NULL) {

        for (i = 0; i < N_ELEMS(ACQ_VALUES); i++) {
            if (STRNEQ(entry->name, ACQ_VALUES[i].name) &&
                    (ACQ_VALUES[i].update & type)) {

                /* We could also call ACQ_VALUES[i].setFN() directly here, but
                 * then we would lose the rollback support in pslSetAcquisitionValues().
                 * But the rollback support may not even be necessary.
                 */
                status = pslSetAcquisitionValues(detChan, entry->name,
                                                 (void *)&(entry->data), defs,
                                                 fs, &(m->currentFirmware[modChan]),
                                                 detType, det,
                                                 m->detector_chan[modChan], m, modChan);

                if (status != XIA_SUCCESS) {
                    sprintf(info_string, "Error updating acquisition value "
                            "'%s' to %0.3f for detChan %d", entry->name, entry->data,
                            detChan);
                    pslLogError("psl__UpdateParams", info_string, status);
                    return status;
                }

                break;

            } else if (STRNEQ(entry->name, ACQ_VALUES[i].name)) {
                /* If we find the name, but it isn't of the requested parameter type then
                 * we stop looking.
                 */
                break;
            }
        }

        entry = entry->next;
    }

    /* value should be ignored here, or else we need to pass in
     * a dummy value instead.
     */
    status = psl__Apply(detChan, name, defs, value);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error applying updated acquisition values for "
                "detChan %d", detChan);
        pslLogError("psl__UpdateParams", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Switches the system FPGA to the version specified by the keywords
 * argument.
 *
 * rawFile is set by the FDD library and returned to the caller.
 */
PSL_STATIC int psl__SwitchSystemFPGA(int detChan, int modChan, FirmwareSet *fs,
                                     char *detType, double pt, int nKeywords,
                                     char **keywords, char *rawFile, Module *m,
                                     boolean_t *downloaded)
{
    int status;


    char *tmpPath = NULL;

    char file[MAX_PATH_LEN];


    /* Check that if the caller says that there are keywords, there actually are. */
    ASSERT((nKeywords == 0) || (keywords != NULL));
    ASSERT(fs != NULL);
    /* The xMAP only supports using an FDD file. */
    ASSERT(fs->filename != NULL);
    ASSERT(m != NULL);


    *downloaded = FALSE_;

    if (fs->tmpPath) {
        tmpPath = fs->tmpPath;

    } else {
        tmpPath = utils->funcs->dxp_md_tmp_path();
    }

    status = xiaFddGetFirmware(fs->filename, tmpPath, "system_fpga", pt, nKeywords,
                               keywords, detType, file, rawFile);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting System FPGA from '%s' for detChan %d",
                fs->filename, detChan);
        pslLogError("psl__SwitchSystemFPGA", info_string, status);
        return status;
    }

    /* If the "new" system FPGA is already running on the board then we
     * don't have to redownload it.
     */
    if (STREQ(m->currentFirmware[modChan].currentSysFPGA, rawFile)) {
        sprintf(info_string, "Skipping system FPGA update: '%s' is already running "
                "on detChan %d", file, detChan);
        pslLogInfo("psl__SwitchSystemFPGA", info_string);
        return XIA_SUCCESS;
    }

    status = dxp_replace_fpgaconfig(&detChan, "system_fpga", file);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error updating System FPGA with '%s' for detChan %d",
                file, detChan);
        pslLogError("psl__SwitchSystemFPGA", info_string, status);
        return status;
    }

    *downloaded = TRUE_;

    return XIA_SUCCESS;
}


/*
 * Clears the requested buffer.
 *
 * This command blocks until the buffer is cleared. By default the max
 * buffer size is cleared. As in the XMAP, firmware supports the
 * register CLRBUFSIZE which can be set to the number of words to
 * clear in order to speed up this operation. Due to lack of demand,
 * no acquisition value has been added to expose the setting.
 *
 * Requires mapping firmware. Calling routines do not need to check
 * the firmware type before calling this routine. However, if mapping mode
 * firmware is not being used an error will be returned which the calling
 * routine can trap and ignore.
 *
 * Accepted buffers are 'a' and 'b'.
 */
PSL_STATIC int psl__ClearBuffer(int detChan, char buf, boolean_t waitForEmpty)
{
    int status;
    int done;
    int empty;

    float interval = .010f;
    float timeout = .1f;

    int n_polls = 0;
    int i;

    unsigned long mfr;
    boolean_t cleared   = FALSE_;

    switch (buf) {
    case 'a':
        done  = MERCURY_MFR_BUFFER_A_DONE;
        empty = MERCURY_MFR_BUFFER_A_EMPTY;
        break;
    case 'b':
        done  = MERCURY_MFR_BUFFER_B_DONE;
        empty = MERCURY_MFR_BUFFER_B_EMPTY;
        break;
    default:
        sprintf(info_string, "Specified buffer '%c' is not a valid buffer for "
                "detChan %d", buf, detChan);
        pslLogError("psl__ClearBuffer", info_string, XIA_UNKNOWN_BUFFER);
        return XIA_UNKNOWN_BUFFER;
    }

    status = psl__SetRegisterBit(detChan, "MFR", done, TRUE_);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting buffer '%c' to done for detChan %d",
                buf, detChan);
        pslLogError("psl__ClearBuffer", info_string, status);
        return status;
    }

    if (waitForEmpty) {
        n_polls = (int)ROUND(timeout / interval);

        for (i = 0; i < n_polls; i++) {
            status = psl__CheckBit(detChan, "MFR", empty, &cleared);

            if (status != XIA_SUCCESS) {
                sprintf(info_string, "Error waiting for buffer '%c' to clear on "
                        "detChan %d", buf, detChan);
                pslLogError("psl__ClearBuffer", info_string, status);
                return status;
            }

            if (cleared) return XIA_SUCCESS;
            utils->funcs->dxp_md_wait(&interval);
        }
    }

    status = psl__GetMFR(detChan, NULL, NULL, &mfr);
    sprintf(info_string, "Timeout waiting for buffer '%c' to be set to empty. "
            "MFR = %#lx", buf, mfr);
    pslLogError("psl__ClearBuffer", info_string, XIA_CLRBUFFER_TIMEOUT);

    return XIA_CLRBUFFER_TIMEOUT;
}


/*
 * Sets the specified bit in the requested register.
 *
 * Uses the read/modify/write idiom to set the register bit, so
 * all of the previous bit states are preserved.
 */
PSL_STATIC int psl__SetRegisterBit(int detChan, char *reg, int bit,
                                   boolean_t overwrite)
{
    int status;

    unsigned long val = 0;


    if (!overwrite) {
        status = dxp_read_register(&detChan, reg, &val);

        if (status != DXP_SUCCESS) {
            sprintf(info_string, "Error reading the '%s' for detChan %d", reg,
                    detChan);
            pslLogError("psl__SetRegisterBit", info_string, status);
            return status;
        }
    }

    val |= (0x1 << bit);

    status = dxp_write_register(&detChan, reg, &val);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error writing %#lx to the '%s' after setting bit %d "
                "for detChan %d", val, reg, bit, detChan);
        pslLogError("psl__SetRegisterBit", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Checks that the specified bit is set (or not) in the specified register.
 */
PSL_STATIC int psl__CheckBit(int detChan, char *reg, int bit, boolean_t *isSet)
{
    int status;

    unsigned long val = 0;


    ASSERT(reg != NULL);


    status = dxp_read_register(&detChan, reg, &val);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error reading the '%s' for detChan %d", reg, detChan);
        pslLogError("psl__ClearRegisterBit", info_string, status);
        return status;
    }

    *isSet = (boolean_t)(val & (1 << bit));

    return XIA_SUCCESS;
}


/*
 * Sets the total number of scan points when the hardware is run
 * in mapping mode.
  *
 * Setting the number of mapping points to 0.0 causes the mapping run to
 * continue indefinitely.
 */
PSL_STATIC int psl__SetNumMapPixels(int detChan, int modChan, char *name,
                                    void *value, char *detType,
                                    XiaDefaults *defs, Module *m,
                                    Detector *det, FirmwareSet *fs)
{
    int status;

    unsigned long NUMPIXELS = 0;

    UNUSED(modChan);
    UNUSED(name);
    UNUSED(detType);
    UNUSED(defs);
    UNUSED(m);
    UNUSED(det);
    UNUSED(fs);


    ASSERT(value != NULL);

    NUMPIXELS = (unsigned long)(*((double *)value));

    status = pslSetParameter(detChan, "NUMPIXELS",
                             (parameter_t)(NUMPIXELS & 0xFFFF));

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting the total number of scan points (%lu) "
                "for detChan %d", NUMPIXELS, detChan);
        pslLogError("psl__SetNumMapPixels", info_string, status);
        return status;
    }

    status = pslSetParameter(detChan, "NUMPIXELSA",
                             (parameter_t)((NUMPIXELS >> 16) & 0xFFFF));

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting the total number of scan points (%lu) "
                "for detChan %d", NUMPIXELS, detChan);
        pslLogError("psl__SetNumMapPixels", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Sets the number of scan points that should be in each buffer.
 *
 * Also, the value -1.0 means: Use the maximum size for points/buffer given
 * the size of my spectra.
 *
 * All buffer size validation is done by the DSP code.
 */
PSL_STATIC int psl__SetNumMapPtsBuffer(int detChan, int modChan, char *name,
                                       void *value, char *detType,
                                       XiaDefaults *defs, Module *m,
                                       Detector *det, FirmwareSet *fs)
{
    int status;

    double pixperbuf = 0.0;

    parameter_t PIXPERBUF = 0;

    UNUSED(modChan);
    UNUSED(name);
    UNUSED(detType);
    UNUSED(defs);
    UNUSED(m);
    UNUSED(det);
    UNUSED(fs);


    ASSERT(value != NULL);

    pixperbuf = *((double *)value);

    /* Tell the DSP to maximize the pixel points per buffer. */
    if (pixperbuf == -1.0) {
        PIXPERBUF = 0;

    } else {
        PIXPERBUF = (parameter_t)pixperbuf;
    }

    status = pslSetParameter(detChan, "PIXPERBUF", PIXPERBUF);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting the number of points per buffer for "
                "detChan %d", detChan);
        pslLogError("psl__SetNumMapPtsBuffer", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Gets the number of scan points in each buffer.
 */
PSL_STATIC int psl__GetNumMapPtsBuffer(int detChan, void *value,
                                       XiaDefaults *defs)
{
    int status;

    parameter_t PIXPERBUF = 0;

    UNUSED(defs);


    ASSERT(value != NULL);


    status = pslGetParameter(detChan, "PIXPERBUF", &PIXPERBUF);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error reading number of pixels per buffer from "
                "the hardware for detChan %d", detChan);
        pslLogError("psl__GetNumMapPtsBuffer", info_string, status);
        return status;
    }

    *((double *)value) = (double)PIXPERBUF;

    return XIA_SUCCESS;
}


/* acq value mapping_mode
 * Enables disables mapping mode by switching to the appropriate
 * firmware.
 *
 * Also used to indicate if mapping parameters should be downloaded to
 * the hardware during startup.
 */
PSL_STATIC int psl__SetMappingMode(int detChan, int modChan, char *name,
                                   void *value, char *detType,
                                   XiaDefaults *defs, Module *m,
                                   Detector *det, FirmwareSet *fs)
{
    int status;
    boolean_t isMercuryOem = psl__IsMercuryOem(detChan);

    unsigned int i;

    char rawFile[MAXFILENAME_LEN];

    char *mapKeywords[] = { "MAPPING" };

    boolean_t enabled = FALSE_;
    boolean_t updated = FALSE_;

    double pt = 0.0;

    parameter_t MAPPINGMODE = (parameter_t) *((double *)value);

    UNUSED(det);
    UNUSED(name);

    ASSERT(fs != NULL);
    ASSERT(value != NULL);
    ASSERT(defs != NULL);
    ASSERT(m != NULL);

    if (MAPPINGMODE > MAPPINGMODE_LIST ||
        MAPPINGMODE == MAPPINGMODE_SCA) {
        sprintf(info_string, "Unsupported mapping mode "
                "%hu for detChan %d", MAPPINGMODE, detChan);
        pslLogError("psl__SetMappingMode", info_string, XIA_UNKNOWN_MAPPING);
        return XIA_UNKNOWN_MAPPING;
    }

    if (!isMercuryOem && MAPPINGMODE > MAPPINGMODE_MCA) {
        sprintf(info_string, "Unsupported mapping mode "
                "%hu for detChan %d, only MCA mapping is supported on this device.",
                MAPPINGMODE, detChan);
        pslLogError("psl__SetMappingMode", info_string, XIA_UNKNOWN_MAPPING);
        return XIA_UNKNOWN_MAPPING;
    }

    enabled = *((double *)value) > 0 ? TRUE_ : FALSE_;

    status = pslGetDefault("peaking_time", (void *)&pt, defs);
    ASSERT(status == XIA_SUCCESS);

    status = pslSetParameter(detChan, "MAPPINGMODE", MAPPINGMODE);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error updating mode in the DSP for detChan %d",
                detChan);
        pslLogError("psl__SetMappingMode", info_string, status);
        return status;
    }

    if (enabled) {
        /* Mercury OEM does not require switching of FPGA */
        status = isMercuryOem ? XIA_SUCCESS :
            psl__SwitchSystemFPGA(detChan, modChan, fs, detType, pt,
                               N_ELEMS(mapKeywords), (char **)mapKeywords,
                               rawFile, m, &updated);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error switching to mapping firmware for detChan %d",
                    detChan);
            pslLogError("psl__SetMappingMode", info_string, status);
            return status;
        }

        /* Download the mapping-specific acquisition values now. */
        status = psl__UpdateParams(detChan, MERCURY_UPDATE_MAPPING, modChan, name,
                                   value, detType, defs, m, det, fs);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error updating mapping parameters after firmware "
                    "switched to mapping mode for detChan %d", detChan);
            pslLogError("psl__SetMappingMode", info_string, status);
            return status;
        }

        /* Write the DSP parameters that are used to fill the mapping buffers. */
        for (i = 0; i < m->number_of_channels; i++) {

            /* Skip if the channel is disabled. */
            if (m->channels[i] != -1) {

                /* If this is the first channel, then set the module number.
                 * If the first channel is disabled then this will be a problem.
                 */
                if (i == 0) {
                    status = pslSetParameter(m->channels[i], "MODNUM",
                                             (parameter_t)(m->channels[i] / 4));

                    if (status != XIA_SUCCESS) {
                        sprintf(info_string, "Error setting module number for mapping "
                                "buffer on detChan %d", m->channels[i]);
                        pslLogError("psl__SetMappingMode", info_string, status);
                        return status;
                    }
                }

                status = pslSetParameter(m->channels[i], "DETCHANNEL",
                                         (parameter_t)m->channels[i]);

                if (status != XIA_SUCCESS) {
                    sprintf(info_string, "Error setting detector channel for mapping "
                            "buffer on detChan %d", m->channels[i]);
                    pslLogError("psl__SetMappingMode", info_string, status);
                    return status;
                }

                status = pslSetParameter(m->channels[i], "DETELEMENT",
                                         (parameter_t)m->detector_chan[i]);

                if (status != XIA_SUCCESS) {
                    sprintf(info_string, "Error setting detector element for mapping "
                            "buffer on detChan %d", m->channels[i]);
                    pslLogError("psl__SetMappingMode", info_string, status);
                    return status;
                }
            }
        }

    } else {

        status =
            isMercuryOem ? XIA_SUCCESS :
            psl__SwitchSystemFPGA(detChan, modChan, fs, detType, pt, 0, NULL,
                                           rawFile, m, &updated);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error switching from mapping mode firmware for "
                    "detChan %d", detChan);
            pslLogError("psl__SetMappingMode", info_string, status);
            return status;
        }

        if (isMercuryOem)
            updated = TRUE_;

        if (updated) {

            /* Download the mapping-specific acquisition values now. */
            status = psl__UpdateParams(detChan, MERCURY_UPDATE_MCA, modChan, name,
                                       value, detType, defs, m, det, fs);

            if (status != XIA_SUCCESS) {
                sprintf(info_string, "Error updating MCA parameters after firmware "
                        "switched from mapping mode for detChan %d", detChan);
                pslLogError("psl__SetMappingMode", info_string, status);
                return status;
            }
        }
    }

    for (i = 0; i < m->number_of_channels; i++) {
        strcpy(m->currentFirmware[i].currentSysFPGA, rawFile);
    }

    return XIA_SUCCESS;
}


/*
 * Sets the specified buffer status to "done".
 *
 * Requires mapping firmware.
 *
 * Returns an error if the specified buffer is not 'a' or 'b'.
 */
PSL_STATIC int psl__SetBufferDone(int detChan, char *name, XiaDefaults *defs,
                                  void *value)
{
    int status;

    boolean_t isMapping = FALSE_;

    UNUSED(defs);
    UNUSED(name);

    ASSERT(value != NULL);

    status = psl__IsMapping(detChan, MAPPING_ANY, &isMapping);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error checking firmware type for detChan %d", detChan);
        pslLogError("psl__SetBufferDone", info_string, status);
        return status;
    }

    if (!isMapping) {
        sprintf(info_string, "Mapping mode firmware not running on detChan %d",
                detChan);
        pslLogError("psl__SetBufferDone", info_string, XIA_NO_MAPPING);
        return XIA_NO_MAPPING;
    }

    status = psl__ClearBuffer(detChan, *((char *)value), TRUE_);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting buffer '%c' status to 'done' for "
                "detChan %d", *((char *)value), detChan);
        pslLogError("psl__SetBufferDone", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Advances the mapping point to the next pixel.
 *
 * Requires mapping firmware.
 *
 * Requires mapping point control to be set to HOST, otherwise an
 * error is returned.
 */
PSL_STATIC int psl__MapPixelNext(int detChan, char *name, XiaDefaults *defs,
                                 void *value)
{
    int status;


    boolean_t isMapping = FALSE_;

    unsigned long mfr = 0;

    UNUSED(name);
    UNUSED(value);
    UNUSED(defs);


    status = psl__IsMapping(detChan, MAPPING_ANY, &isMapping);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error checking firmware type for detChan %d", detChan);
        pslLogError("psl__MapPixelNext", info_string, status);
        return status;
    }

    if (!isMapping) {
        sprintf(info_string, "Mapping mode firmware not running on detChan %d",
                detChan);
        pslLogError("psl__MapPixelNext", info_string, XIA_NO_MAPPING);
        return XIA_NO_MAPPING;
    }

    /* Set bit 13 to advance the pixel. */
    mfr = 0x2000;

    status = dxp_write_register(&detChan, "MFR", &mfr);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error writing Mapping Flag Register for detChan %d",
                detChan);
        pslLogError("psl__MapPixelNext", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Checks to see if Buffer A is full.
 *
 * Requires the mapping mode firmware to be running.
 */
PSL_STATIC int psl__GetBufferFullA(int detChan, void *value, XiaDefaults *defs,
                                   Module *m)
{
    int status;

    boolean_t isFull = FALSE_;

    UNUSED(defs);
    UNUSED(m);


    status = psl__GetBufferFull(detChan, 'a', &isFull);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting status of Buffer A for detChan %d",
                detChan);
        pslLogError("psl__GetBufferFullA", info_string, status);
        return status;
    }

    *((unsigned short *)value) = (unsigned short)isFull;

    return XIA_SUCCESS;
}


/*
 * Checks to see if Buffer B is full.
 *
 * Requires the mapping mode firmware to be running.
 */
PSL_STATIC int psl__GetBufferFullB(int detChan, void *value, XiaDefaults *defs,
                                   Module *m)
{
    int status;

    boolean_t isFull = FALSE_;

    UNUSED(defs);
    UNUSED(m);


    status = psl__GetBufferFull(detChan, 'b', &isFull);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting status of Buffer B for detChan %d",
                detChan);
        pslLogError("psl__GetBufferFullB", info_string, status);
        return status;
    }

    *((unsigned short *)value) = (unsigned short)isFull;

    return XIA_SUCCESS;
}


/*
 * Checks to see if the specified buffer is full or not.
 *
 * Requires the mapping mode firmware to be running.
 */
PSL_STATIC int psl__GetBufferFull(int detChan, char buf, boolean_t *is_full)
{

    int status;

    unsigned long fullMask = 0;
    unsigned long mfr = 0;

    boolean_t isMapping = FALSE_;


    ASSERT(buf == 'a' || buf == 'b');
    ASSERT(is_full != NULL);


    status = psl__IsMapping(detChan, MAPPING_ANY, &isMapping);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error determining if mapping mode was enabled for "
                "detChan %d", detChan);
        pslLogError("psl__GetBufferFull", info_string, status);
        return status;
    }

    if (!isMapping) {
        sprintf(info_string, "Mapping mode firmware is currently not running on "
                "detChan %d", detChan);
        pslLogError("psl__GetBufferFull", info_string, XIA_NO_MAPPING);
        return XIA_NO_MAPPING;
    }

    status = dxp_read_register(&detChan, "MFR", &mfr);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error reading buffer '%c' status for detChan %d",
                buf, detChan);
        pslLogError("psl__GetBufferFull", info_string, status);
        return status;
    }

    switch (buf) {
    case 'a':
        fullMask = 0x2;
        break;
    case 'b':
        fullMask = 0x20;
        break;
    default:
        break;
    }

    *is_full = (boolean_t)((mfr & fullMask) ? TRUE_ : FALSE_);

    return XIA_SUCCESS;
}


/*
 * Calculates the size of the buffer, in 16-bit words, that will be
 * returned by a call to xiaGetRunData("buffer_a" or "buffer_b").
 *
 * Requires mapping firmware.
 */
PSL_STATIC int psl__GetBufferLen(int detChan, void *value, XiaDefaults *defs,
                                 Module *m)
{
    int status;

    boolean_t isMapping = FALSE_;

    parameter_t MAPPINGMODE = 0;
    parameter_t PIXPERBUF = 0;

    unsigned long pixelBlockSize = 0;
    unsigned long bufferSize     = 0;

    UNUSED(m);


    ASSERT(defs  != NULL);
    ASSERT(value != NULL);


    status = psl__IsMapping(detChan, MAPPING_MCA | MAPPING_SCA, &isMapping);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error checking firmware type for detChan %d", detChan);
        pslLogError("psl__GetBufferLen", info_string, status);
        return status;
    }

    if (!isMapping) {
        sprintf(info_string, "Mapping mode firmware not running on detChan %d",
                detChan);
        pslLogError("psl__GetBufferLen", info_string, XIA_NO_MAPPING);
        return XIA_NO_MAPPING;
    }

    status = pslGetParameter(detChan, "MAPPINGMODE", &MAPPINGMODE);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error reading the mapping mode for detChan %d",
                detChan);
        pslLogError("psl__GetBufferLen", info_string, status);
        return status;
    }

    status = pslGetParameter(detChan, "PIXPERBUF", &PIXPERBUF);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error reading the number of pixel points in the "
                "buffer for detChan %d", detChan);
        pslLogError("psl__GetBufferLen", info_string, status);
        return status;
    }

    pixelBlockSize = psl__GetMCAPixelBlockSize(defs, m);

    bufferSize = MERCURY_BUFFER_BLOCK_SIZE + PIXPERBUF * pixelBlockSize;
    /* Buffer size better be less then 1M x 16-bits. */
    ASSERT(bufferSize <= 1048576);

    *((unsigned long *)value) = bufferSize;

    return XIA_SUCCESS;
}


/*
 * Calculates the size of each pixel block
 * in 16-bit words, that will be
 * returned by a call to xiaGetRunData("buffer_a" or "buffer_b").
 *
 * Requires mapping firmware.
 */
PSL_STATIC unsigned long psl__GetMCAPixelBlockSize(XiaDefaults *defs,
                                                   Module *m)
{
    int status;
    double mcaLen = 0.0;

    unsigned long pixelBlockSize = 0;

    UNUSED(m);

    status = pslGetDefault("number_mca_channels", (void *)&mcaLen, defs);
    ASSERT(status == XIA_SUCCESS);

    pixelBlockSize = (m->number_of_channels * (unsigned long)mcaLen) +
                     MERCURY_BUFFER_BLOCK_SIZE;

    return pixelBlockSize;

}


/*
 * Read mapping data from Buffer A.
 *
 * Requires mapping firmware.
 */
PSL_STATIC int psl__GetBufferA(int detChan, void *value, XiaDefaults *defs,
                               Module *m)
{
    int status;


    ASSERT(m    != NULL);
    ASSERT(defs != NULL);


    status = psl__GetBuffer(detChan, 'a', (unsigned long *)value, defs, m);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error reading Buffer A for detChan =  %d",
                detChan);
        pslLogError("psl__GetBufferA", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Read mapping data from Buffer B.
 *
 * Requires mapping firmware.
 */
PSL_STATIC int psl__GetBufferB(int detChan, void *value, XiaDefaults *defs,
                               Module *m)
{
    int status;


    ASSERT(m    != NULL);
    ASSERT(defs != NULL);


    status = psl__GetBuffer(detChan, 'b', (unsigned long *)value, defs, m);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error reading Buffer B for detChan =  %d",
                detChan);
        pslLogError("psl__GetBufferB", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Get the requested buffer from the external memory.
 *
 * Requires mapping firmware.
 *
 * Assumes that the proper amount of memory has been allocated for data.
 */
PSL_STATIC int psl__GetBuffer(int detChan, char buf, unsigned long *data,
                              XiaDefaults *defs, Module *m)
{
    int status;


    unsigned long len  = 0;
    unsigned long base = 0;

    boolean_t isMCAOrSCA;
    boolean_t isList;

    char memoryStr[36];

    ASSERT(data != NULL);
    ASSERT(buf == 'a' || buf == 'b');

    status = psl__IsMapping(detChan, MAPPING_MCA | MAPPING_SCA, &isMCAOrSCA);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error checking firmware type for detChan %d", detChan);
        pslLogError("psl__GetBuffer", info_string, status);
        return status;
    }

    status = psl__IsMapping(detChan, MAPPING_LIST, &isList);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error checking firmware type for detChan %d",
                detChan);
        pslLogError("psl__GetBuffer", info_string, status);
        return status;
    }

    if (!isMCAOrSCA && !isList) {
        sprintf(info_string, "Mapping mode firmware not running on detChan %d",
                detChan);
        pslLogError("psl__GetBuffer", info_string, XIA_NO_MAPPING);
        return XIA_NO_MAPPING;
    }

    /* Use "no word packing". Once packing support is included, we can update
     * the memory base here. (See DXP-Mercury Memory Map document for detail)
     */
    switch (buf) {
    case 'a':
        base = 0x4000000;
        break;
    case 'b':
        base = 0x6000000;
        break;
    default:
        FAIL();
        break;
    }

    if (!isList) {
        status = psl__GetBufferLen(detChan, (void *)&len, defs, m);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error getting length of buffer '%c' for detChan %d",
                    buf, detChan);
            pslLogError("psl__GetBuffer", info_string, status);
            return status;
        }
    } else {
        /* The list mode lengths are not a fixed size, unlike the
         * MCA/SCA mode buffer lengths.
         */
        status = psl__GetListBufferLen(detChan, buf, &len);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error getting the length of list mode "
                    "buffer '%c' for detChan %d.", buf, detChan);
            pslLogError("psl__GetBuffer", info_string, status);
            return status;
        }
    }

    sprintf(memoryStr, "burst_map:%#lx:%lu", base, len);

    status = dxp_read_memory(&detChan, memoryStr, data);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error reading memory for buffer '%c' on detChan %d",
                buf, detChan);
        pslLogError("psl__GetBuffer", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Gets the current mapping point.
 *
 * Requires mapping mode firmware.
 */
PSL_STATIC int psl__GetCurrentPixel(int detChan, void *value, XiaDefaults *defs,
                                    Module *m)
{
    int status;

    boolean_t isMapping = FALSE_;

    parameter_t PIXELNUM  = 0;
    parameter_t PIXELNUMA = 0;;

    UNUSED(defs);
    UNUSED(m);


    ASSERT(value != NULL);


    status = psl__IsMapping(detChan, MAPPING_ANY, &isMapping);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error checking firmware type for detChan %d", detChan);
        pslLogError("psl__GetCurrentPixel", info_string, status);
        return status;
    }

    if (!isMapping) {
        sprintf(info_string, "Mapping mode firmware not running on detChan %d",
                detChan);
        pslLogError("psl__GetCurrentPixel", info_string, XIA_NO_MAPPING);
        return XIA_NO_MAPPING;
    }

    status = pslGetParameter(detChan, "PIXELNUM", &PIXELNUM);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error reading current pixel number for detChan %d",
                detChan);
        pslLogError("psl__GetCurrentPixel", info_string, status);
        return status;
    }

    status = pslGetParameter(detChan, "PIXELNUMA", &PIXELNUMA);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error reading current pixel number for detChan %d",
                detChan);
        pslLogError("psl__GetCurrentPixel", info_string, status);
        return status;
    }

    *((unsigned long *)value) = WORD_TO_LONG(PIXELNUM, PIXELNUMA);

    sprintf(info_string, "Current pixel = %lu for detChan %d",
            *((unsigned long *)value), detChan);
    pslLogDebug("psl__GetCurrentPixel", info_string);

    return XIA_SUCCESS;
}


/*
 * Checks if a buffer overrun condition has been signaled.
 *
 * A value of 1 indicates a buffer overrun condition, while 0 indicates
 * that the buffer has not been overrun.
 *
 * Requires mapping mode to be enabled.
 */
PSL_STATIC int psl__GetBufferOverrun(int detChan, void *value,
                                     XiaDefaults *defs, Module *m)
{
    int status;


    boolean_t isMapping = FALSE_;

    unsigned long mfr = 0;

    UNUSED(m);
    UNUSED(defs);


    status = psl__IsMapping(detChan, MAPPING_ANY, &isMapping);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error determining if mapping mode was enabled for "
                "detChan %d", detChan);
        pslLogError("psl__GetBufferOverrun", info_string, status);
        return status;
    }

    if (!isMapping) {
        sprintf(info_string, "Mapping mode firmware is currently not running on "
                "detChan %d", detChan);
        pslLogError("psl__GetBufferOverrun", info_string, XIA_NO_MAPPING);
        return XIA_NO_MAPPING;
    }

    status = dxp_read_register(&detChan, "MFR", &mfr);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error reading Mapping Flag Register for detChan %d",
                detChan);
        pslLogError("psl__GetBufferOverrun", info_string, status);
        return status;
    }

    if (mfr & (1 << MERCURY_MFR_BUFFER_OVERRUN)) {
        *((unsigned short *)value) = 1;
    } else {
        *((unsigned short *)value) = 0;
    }

    return XIA_SUCCESS;
}


/*
 * Set the input logic polarity.
 */
PSL_STATIC int psl__SetInputLogicPolarity(int detChan, int modChan, char *name,
                                          void *value, char *detType,
                                          XiaDefaults *defs, Module *m,
                                          Detector *det, FirmwareSet *fs)

{
    int status;

    UNUSED(name);
    UNUSED(detType);
    UNUSED(defs);
    UNUSED(m);
    UNUSED(det);
    UNUSED(fs);
    UNUSED(modChan);


    ASSERT(value != NULL);

    if (*((double *)value) == 1.0) {
        status = psl__SetRegisterBit(detChan, "MCR", 2, FALSE_);
    } else {
        status = psl__ClearRegisterBit(detChan, "MCR", 2);
    }

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting input logic level to %0.3f for "
                "detChan %d", *((double *)value), detChan);
        pslLogError("psl__SetInputLogicPolarity", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Sets how the pixel is to be advanced.
 *
 * Currently only GATE and SYNCE are available. (Host advance is always available
 * so there is no need to set it explicitly.) The allowed advance types are
 * defined as constants in handel_constants.h.
 */
PSL_STATIC int psl__SetPixelAdvanceMode(int detChan, int modChan, char *name,
                                        void *value, char *detType,
                                        XiaDefaults *defs, Module *m,
                                        Detector *det, FirmwareSet *fs)
{
    int status;

    double mode = 0.0;

    UNUSED(modChan);
    UNUSED(name);
    UNUSED(detType);
    UNUSED(defs);
    UNUSED(m);
    UNUSED(det);
    UNUSED(fs);


    ASSERT(value != NULL);


    mode = *((double *)value);

    if (mode == XIA_MAPPING_CTL_GATE) {
        status = psl__ClearRegisterBit(detChan, "MCR", MERCURY_MCR_PIXEL_ADVANCE);

    } else if (mode == XIA_MAPPING_CTL_SYNC) {
        status = psl__SetRegisterBit(detChan, "MCR", MERCURY_MCR_PIXEL_ADVANCE, FALSE_);

    } else {
        status = XIA_UNKNOWN_PT_CTL;
    }

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting pixel advance mode to %0.3f for "
                "detChan %d", mode, detChan);
        pslLogError("psl__SetPixelAdvanceMode", info_string, status);
        return status;
    }

    /* If we want to do more when this setting changes, such as update
     * the input LEMO, this is where we would do it.
     */

    return XIA_SUCCESS;
}


/*
 * Clears the specified bit in the requested register.
 *
 * Uses the read/modify/write idiom to set the register bit, so
 * all of the previous bit states are preserved.
 */
PSL_STATIC int psl__ClearRegisterBit(int detChan, char *reg, int bit)
{
    int status;

    unsigned long val = 0;


    status = dxp_read_register(&detChan, reg, &val);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error reading the '%s' for detChan %d",
                reg, detChan);
        pslLogError("psl__ClearRegisterBit", info_string, status);
        return status;
    }

    val &= ~(0x1 << bit);

    status = dxp_write_register(&detChan, reg, &val);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error writing %#lx to the '%s' after clearing bit %d "
                "for detChan %d", val, reg, bit, detChan);
        pslLogError("psl__ClearRegisterBit", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Sets the SYNC count for the specified module.
 *
 * Sets the number of cycles on the SYNC line before the pixel is
 * advanced.
 *
 */
PSL_STATIC int psl__SetSyncCount(int detChan, int modChan, char *name,
                                 void *value, char *detType,
                                 XiaDefaults *defs, Module *m,
                                 Detector *det, FirmwareSet *fs)
{
    int status;

    unsigned long count = 0;

    UNUSED(modChan);
    UNUSED(name);
    UNUSED(detType);
    UNUSED(defs);
    UNUSED(m);
    UNUSED(det);
    UNUSED(fs);


    ASSERT(value != NULL);

    count = (unsigned long)(*((double *)value));

    status = dxp_write_register(&detChan, "SYNCCNT", &count);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error setting the number of SYNC counts to %lu "
                "for detChan %d", count, detChan);
        pslLogError("psl__SetSyncCount", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Sets the GATE ignore property for the specified module.
 *
 * This parameter is ignored if mapping mode is not currently active.
 */
PSL_STATIC int psl__SetGateIgnore(int detChan, int modChan, char *name,
                                  void *value, char *detType,
                                  XiaDefaults *defs, Module *m,
                                  Detector *det, FirmwareSet *fs)
{
    int status;

    UNUSED(modChan);
    UNUSED(name);
    UNUSED(detType);
    UNUSED(defs);
    UNUSED(m);
    UNUSED(det);
    UNUSED(fs);


    ASSERT(value != NULL);

    if (*((double *)value) == 1.0) {
        status = psl__SetRegisterBit(detChan, "MCR", 5, FALSE_);
    } else {
        status = psl__ClearRegisterBit(detChan, "MCR", 5);
    }

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting GATE ignore for detChan %d",
                detChan);
        pslLogError("psl__SetGateIgnore", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Get the serial number for device
 *  Note that for mercury there are two characters (one per unsigned short)
 *  in each word, thus the conversion.
 */
PSL_STATIC int psl__GetSerialNumber(int detChan, char *name, XiaDefaults *defs,
                                    void *value)
{
    int status;
    int i;
    int number_dwords = SERIAL_NUM_LEN / 2;

    unsigned short *buf;

    char *serialNum = (char *)value;
    char mem[MAXITEM_LEN];

    UNUSED(name);
    UNUSED(defs);

    ASSERT(value != NULL);

    buf = mercury_psl_md_alloc(number_dwords * sizeof(unsigned long));
    sprintf(mem, "eeprom:%#x:%d", BOARD_SER_NUM, number_dwords);

    status = dxp_read_memory(&detChan, mem, (unsigned long *)buf);

    if (status != DXP_SUCCESS) {
        mercury_psl_md_free(buf);
        sprintf(info_string, "Error reading serial number for detChan %d",
                detChan);
        pslLogError("psl__GetSerialNumber", info_string, status);
        return status;
    }

    for (i = 0; i < SERIAL_NUM_LEN; i++)
    {
        serialNum[i] = (char) buf[i];
    }

    /* Must allocate SERIAL_NUM_LEN + 1 for termination */
    serialNum[SERIAL_NUM_LEN] = (char)  '\0';

    mercury_psl_md_free(buf);

    return XIA_SUCCESS;
}


/*
 * Set the USB serial number for device
 *  TODO Check setup_board register status first
 *  It needs to be set by the user before calling this function
 */
PSL_STATIC int psl__SetSerialNumber(int detChan, char *name, XiaDefaults *defs,
                                    void *value)
{
    int status;
    int i;
    int number_dwords = SERIAL_NUM_LEN / 2;

    char *serialNum = (char *)value;
    char mem[MAXITEM_LEN];

    unsigned short *buf;

    UNUSED(name);
    UNUSED(defs);

    ASSERT(value != NULL);

    if (serialNum[SERIAL_NUM_LEN] != '\0') {
        sprintf(info_string, "Incorrect serial number format (%s) for detChan %d",
                serialNum, detChan);
        pslLogError("psl__SetSerialNumber", info_string, XIA_INVALID_STR);
        return XIA_INVALID_STR;
    }

    buf = mercury_psl_md_alloc(number_dwords * sizeof(unsigned long));

    for (i = 0; i < SERIAL_NUM_LEN; i++)
    {
        buf[i] = (unsigned short) serialNum[i];
    }

    sprintf(mem, "eeprom:%#x:%d", BOARD_SER_NUM, number_dwords);

    status = dxp_write_memory(&detChan, mem, (unsigned long *)buf);

    if (status != DXP_SUCCESS) {
        mercury_psl_md_free(buf);
        sprintf(info_string, "Error setting serial number for detChan %d",
                detChan);
        pslLogError("psl__SetSerialNumber", info_string, status);
        return status;
    } else {
        sprintf(info_string, "Serial number set to %s for detChan %d",
                serialNum, detChan);
        pslLogDebug("psl__SetSerialNumber", info_string);
    }

    mercury_psl_md_free(buf);
    return XIA_SUCCESS;
}



/*
 * Get the currenct temperature in a double
 *  Board operation get_temperature (double)
 */
PSL_STATIC int psl__GetTemperature(int detChan, char *name, XiaDefaults *defs,
                                   void *value)
{
    int status;
    double *temp = (double *)value;

    parameter_t TEMPERATURE = 0;
    parameter_t TEMPFRACTION = 0;

    UNUSED(name);
    UNUSED(defs);

    ASSERT(value != NULL);

    status = pslGetParameter(detChan, "TEMPERATURE", &TEMPERATURE);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting temperature for detChan %d",
                detChan);
        pslLogError("psl__GetTemperature", info_string, status);
        return status;
    }

    status = pslGetParameter(detChan, "TEMPFRACTION", &TEMPFRACTION);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting temperature fractopm for detChan %d",
                detChan);
        pslLogError("psl__GetTemperature", info_string, status);
        return status;
    }

    *temp = (double) TEMPERATURE + (double) TEMPFRACTION / 65536.;

    return XIA_SUCCESS;
}


/*
 * Sets the delta_temp acquisition value
 */
PSL_STATIC int psl__SetDeltaTemp(int detChan, int modChan, char *name,
                                 void *value, char *detType,
                                 XiaDefaults *defs, Module *m,
                                 Detector *det, FirmwareSet *fs)
{
    int status;
    parameter_t DELTATEMP = 0;

    UNUSED(modChan);
    UNUSED(name);
    UNUSED(detType);
    UNUSED(defs);
    UNUSED(m);
    UNUSED(det);
    UNUSED(fs);


    ASSERT(value != NULL);

    /* TODO mercury-4 doesn't support DELTATEMP yet */
    if (m->number_of_channels > 1)
    {
        pslLogDebug("psl__SetDeltaTemp", "Mercury-4 doesn't support DELTATEMP.");
        return XIA_SUCCESS;
    }

    /* DELTATEMP is measured in 16ths of a degree.*/
    DELTATEMP = (parameter_t) (*((double *)value) * 16.);

    status = pslSetParameter(detChan, "DELTATEMP", DELTATEMP);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting delta temperature for detChan %d",
                detChan);
        pslLogError("psl__SetDeltaTemp", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Sets the temp_correction acquisition value.
 */
PSL_STATIC int psl__SetTempCorrection(int detChan, int modChan, char *name,
                                      void *value, char *detType,
                                      XiaDefaults *defs, Module *m,
                                      Detector *det, FirmwareSet *fs)
{
    int status;
    parameter_t TEMPCORRECTION = 0;

    UNUSED(modChan);
    UNUSED(name);
    UNUSED(detType);
    UNUSED(defs);
    UNUSED(det);
    UNUSED(fs);

    ASSERT(value != NULL);

    /* TODO mercury-4 doesn't support TEMPCORRECTION yet */
    if (m->number_of_channels > 1)
    {
        pslLogDebug("psl__SetTempCorrection",
                    "Mercury-4 doesn't support TEMPCORRECTION.");
        return XIA_SUCCESS;
    }

    TEMPCORRECTION = (parameter_t) *((double *)value);

    if (TEMPCORRECTION > 2) {
        sprintf(info_string, "Specified temperature correction %hu is not a valid "
                "setting", TEMPCORRECTION);
        pslLogError("psl__SetTempCorrection", info_string, XIA_PARAMETER_OOR);
        return XIA_PARAMETER_OOR;
    }

    status = pslSetParameter(detChan, "TEMPCORRECTION", TEMPCORRECTION);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting temperature correction for detChan %d",
                detChan);
        pslLogError("psl__SetTempCorrection", info_string, status);
        return status;
    }

    /* Since the temperature calibration acq values are read-only,
    * the DSP parameters need to be set to defaults mannually
    * this needs to be done before temp_correction is set
    */
    if (TEMPCORRECTION != MERCURY_TEMP_NO_CORRECTION)
    {
        status = psl__ApplyTempCalibrationValues(detChan, defs);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error applying temperature calibration before "
                    "setting temp correction for detChan %d", detChan);
            pslLogError("psl__SetTempCorrection", info_string, status);
            return status;
        }
    }

    /* Temperature calibration values need to be synced after new correction */
    if (TEMPCORRECTION != MERCURY_TEMP_NO_CORRECTION)
    {
        status = psl__SyncTempCalibrationValues(detChan, m, defs);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error syncing temperature calibration after "
                    "setting temp correction for detChan %d", detChan);
            pslLogError("psl__SetTempCorrection", info_string, status);
            return status;
        }
    }

    return XIA_SUCCESS;
}


/*
 * Set peak mode for determining the energy from the energy filter output
 *
 * PEAKMODE = 0: XIA_PEAK_SENSING_MODE The largest filter value from a given
 * pulse will be used as the energy.
 *
 * PEAKMODE = 1: XIA_PEAK_SAMPLING_MODE. The energy filter value will be sampled
 * at a specific time determined by the setting of PEAKSAM.
 */
PSL_STATIC int psl__SetPeakMode(int detChan, int modChan, char *name,
                                void *value, char *detType,
                                XiaDefaults *defs, Module *m,
                                Detector *det, FirmwareSet *fs)
{
    int status;

    double peakMode = *((double *)value);
    double pt;

    UNUSED(name);
    UNUSED(detType);


    ASSERT(value != NULL);

    if ((peakMode != XIA_PEAK_SENSING_MODE) &&
            (peakMode != XIA_PEAK_SAMPLING_MODE)) {
        sprintf(info_string, "User specified peak mode %f is not within the "
                "valid range (0,1) for detChan %d", peakMode, detChan);
        pslLogError("psl__SetPeakMode", info_string, XIA_PEAKMODE_OOR);
        return XIA_PEAKMODE_OOR;
    }

    status = pslSetDefault("peak_mode", (void *)value, defs);
    ASSERT(status == XIA_SUCCESS);

    /* The actual update is done in psl__UpdateFilterParams
     * to make sure PEAKSAM can be recalculated
     */
    status = pslGetDefault("peaking_time", (void *)&pt, defs);
    ASSERT(status == XIA_SUCCESS);

    status = psl__UpdateFilterParams(detChan, modChan, pt, defs, fs, m, det);


    return XIA_SUCCESS;
}


/*
 * Reads out the entire MCA block for the module that detChan
 * is located in. This routine is an alternative to reading the MCA
 * our individually for each channel. This routine assumes that all of the
 * channels share the same size MCA.
 */
PSL_STATIC int psl__GetModuleMCA(int detChan, void *value,
                                 XiaDefaults *defs, Module *m)
{

    int status;

    unsigned long addr;
    unsigned long len;

    double nBins;

    char memStr[36];


    ASSERT(value != NULL);
    ASSERT(defs != NULL);
    ASSERT(m != NULL);


    /* Skip past the initial statistics block. */
    addr = MERCURY_MEMORY_BLOCK_SIZE;

    status = pslGetDefault("number_mca_channels", &nBins, defs);
    ASSERT(status == XIA_SUCCESS);

    /* We require that all channels use the same length MCA. */
    len = (unsigned long)(nBins * m->number_of_channels);

    sprintf(memStr, "burst:%#lx:%lu", addr, len);

    status = dxp_read_memory(&detChan, memStr, value);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error reading all MCA data for the module containing"
                "detChan %d", detChan);
        pslLogError("psl__GetModuleMCA", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Set output of the trigger signal
 *
 * 0: disabled (drive low)
 * 1: fast filter crosses threshold
 * 2: baseline filter crosses threshold
 * 3: energy filter crosses threshols
 */
PSL_STATIC int psl__SetTriggerOutput(int detChan, int modChan, char *name,
                                     void *value, char *detType,
                                     XiaDefaults *defs, Module *m,
                                     Detector *det, FirmwareSet *fs)
{
    int status;

    parameter_t TRIGOUTPUT;

    UNUSED(modChan);
    UNUSED(name);
    UNUSED(detType);
    UNUSED(defs);
    UNUSED(m);
    UNUSED(det);
    UNUSED(fs);

    ASSERT(value != NULL);

    TRIGOUTPUT = (parameter_t) *((double *)value);

    if ((TRIGOUTPUT != XIA_OUTPUT_DISABLED) &&
            (TRIGOUTPUT != XIA_OUTPUT_FASTFILTER) &&
            (TRIGOUTPUT != XIA_OUTPUT_BASELINEFILTER) &&
            (TRIGOUTPUT != XIA_OUTPUT_ENERGYFILTER)) {
        sprintf(info_string, "User specified trigger signal ouput %hu is not "
                "within the valid range (0-3) for detChan %d", TRIGOUTPUT, detChan);
        pslLogError("psl__SetTriggerOutput", info_string, XIA_TRIGOUTPUT_OOR);
        return XIA_TRIGOUTPUT_OOR;
    }

    status = pslSetParameter(detChan, "TRIGOUTPUT", TRIGOUTPUT);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting TRIGOUTPUT to %hu for detChan %d",
                TRIGOUTPUT, detChan);
        pslLogError("psl__SetTriggerOutput", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Set output of the livetime signal
 *
 * 0: disabled (drive low)
 * 1: fast filter over threshold
 * 2: baseline filter over threshold
 * 3: energy filter over threshold
 * 4: energy filter active (ie signal present, above virtual threshold) based
 * upon fast and baseline triggers
 */
PSL_STATIC int psl__SetLivetimeOuput(int detChan, int modChan, char *name,
                                     void *value, char *detType,
                                     XiaDefaults *defs, Module *m,
                                     Detector *det, FirmwareSet *fs)
{
    int status;

    parameter_t LIVEOUTPUT;

    UNUSED(modChan);
    UNUSED(name);
    UNUSED(detType);
    UNUSED(defs);
    UNUSED(m);
    UNUSED(det);
    UNUSED(fs);

    ASSERT(value != NULL);

    LIVEOUTPUT = (parameter_t) *((double *)value);


    if ((LIVEOUTPUT != XIA_OUTPUT_DISABLED) &&
            (LIVEOUTPUT != XIA_OUTPUT_FASTFILTER) &&
            (LIVEOUTPUT != XIA_OUTPUT_BASELINEFILTER) &&
            (LIVEOUTPUT != XIA_OUTPUT_ENERGYFILTER) &&
            (LIVEOUTPUT != XIA_OUTPUT_ENERGYACTIVE)) {
        sprintf(info_string, "User specified livetime signal ouput %hu is not "
                "within the valid range (0-4) for detChan %d", LIVEOUTPUT, detChan);
        pslLogError("psl__SetLivetimeOuput", info_string, XIA_LIVEOUTPUT_OOR);
        return XIA_LIVEOUTPUT_OOR;
    }

    status = pslSetParameter(detChan, "LIVEOUTPUT", LIVEOUTPUT);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting LIVEOUTPUT to %hu for detChan %d",
                LIVEOUTPUT, detChan);
        pslLogError("psl__SetLivetimeOuput", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Placeholder for calibrated_gain acq value setter.
 *  Trying to set the value during run time will generate a warning
 */
PSL_STATIC int psl__SetCalibratedGain(int detChan, int modChan, char *name,
                                      void *value, char *detType, XiaDefaults *defs,
                                      Module *m, Detector *det, FirmwareSet *fs)
{
    UNUSED(detChan);
    UNUSED(modChan);
    UNUSED(name);
    UNUSED(value);
    UNUSED(detType);
    UNUSED(defs);
    UNUSED(m);
    UNUSED(det);
    UNUSED(fs);

    pslLogWarning("psl__SetCalibratedGain",
                  "Acquisition value calibrated_gain is read-only");

    return XIA_SUCCESS;
}


/*
 * Get the calibrated_gain acq value
 */
PSL_STATIC int psl__GetCalibratedGain(int detChan, void *value, XiaDefaults *defs)
{
    int status;

    parameter_t GAINLOW = 0;
    parameter_t GAINHIGH = 0;

    UNUSED(defs);

    ASSERT(value != NULL);

    status = pslGetParameter(detChan, "GAINLOW", &GAINLOW);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error reading low calibrated gain for detChan %d",
                detChan);
        pslLogError("psl__GetCalibratedGain", info_string, status);
        return status;
    }

    status = pslGetParameter(detChan, "GAINHIGH", &GAINHIGH);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error reading high calibrated gain for detChan %d",
                detChan);
        pslLogError("psl__GetCalibratedGain", info_string, status);
        return status;
    }

    /* Both GAINHIGH and GAINLOW are stored in the double gain_calibration
     * acq value.
     */
    *((double *)value) =	(double)(GAINHIGH * 65536.) + (double)GAINLOW;

    status = pslSetDefault("calibrated_gain", value, defs);
    ASSERT(status == XIA_SUCCESS)	;

    return XIA_SUCCESS;
}


/*
 * Placeholder for calibrated_dac acq value setter.
 *  Trying to set the value during run time will generate a warning
 */
PSL_STATIC int psl__SetCalibratedDac(int detChan, int modChan, char *name,
                                     void *value, char *detType, XiaDefaults *defs,
                                     Module *m, Detector *det, FirmwareSet *fs)
{
    UNUSED(detChan);
    UNUSED(modChan);
    UNUSED(name);
    UNUSED(value);
    UNUSED(detType);
    UNUSED(defs);
    UNUSED(m);
    UNUSED(det);
    UNUSED(fs);

    pslLogWarning("psl__SetCalibratedDac",
                  "Acquisition value calibrated_dac is read-only");

    return XIA_SUCCESS;
}


/*
 * Placeholder for gain_slope acq value setter.
 *  Trying to set the value during run time will generate a warning
 */
PSL_STATIC int psl__SetGainSlope(int detChan, int modChan, char *name,
                                 void *value, char *detType, XiaDefaults *defs,
                                 Module *m, Detector *det, FirmwareSet *fs)
{
    UNUSED(detChan);
    UNUSED(modChan);
    UNUSED(name);
    UNUSED(value);
    UNUSED(detType);
    UNUSED(defs);
    UNUSED(m);
    UNUSED(det);
    UNUSED(fs);


    pslLogWarning("psl__SetGainSlope",
                  "Acquisition value gain_slope is read-only");

    return XIA_SUCCESS;
}


/*
 * Get the gain_slope acq value
 */
PSL_STATIC int psl__GetGainSlope(int detChan, void *value, XiaDefaults *defs)
{
    int status;

    parameter_t DGDACDGAIN = 0;

    UNUSED(defs);

    ASSERT(value != NULL);

    status = pslGetParameter(detChan, "DGDACDGAIN", &DGDACDGAIN);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error reading gain slope for detChan %d", detChan);
        pslLogError("psl__GetGainSlope", info_string, status);
        return status;
    }

    *((double *)value) = (double) DGDACDGAIN;

    status = pslSetDefault("gain_slope", value, defs);
    ASSERT(status == XIA_SUCCESS);

    return XIA_SUCCESS;
}


/*
 * Set the input_attenuation acq value
 */
PSL_STATIC int psl__SetInputAttenuation(int detChan, int modChan, char *name,
                                 void *value, char *detType, XiaDefaults *defs,
                                 Module *m, Detector *det, FirmwareSet *fs)
{
    int status;
    parameter_t INPUTATTEN;

    boolean_t isMercuryOem = psl__IsMercuryOem(detChan);

    UNUSED(fs);
    UNUSED(det);
    UNUSED(m);
    UNUSED(detType);
    UNUSED(modChan);
    UNUSED(defs);

    ASSERT(value);

    if (!isMercuryOem) {
        sprintf(info_string, "Skipping setting of %s for non OEM"
                " mercury at channel %d.", name, detChan);
        pslLogInfo("psl__SetInputAttenuation", info_string);
        return XIA_SUCCESS;
    }

    INPUTATTEN = (parameter_t)(*((double *)value));

    if (INPUTATTEN > (parameter_t)MERCURY_MAX_INPUTATTEN || INPUTATTEN < 0.) {
        sprintf(info_string, "Specified %s (%hu) out of range (0, %hu) "
                "for detChan %d.", name, INPUTATTEN, MERCURY_MAX_INPUTATTEN, detChan);
        pslLogError("psl__SetInputAttenuation", info_string, XIA_PARAMETER_OOR);
        return XIA_PARAMETER_OOR;
    }

    status = pslSetParameter(detChan, "INPUTATTEN", INPUTATTEN);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting %s to %hu "
                "for detChan %d.", name, INPUTATTEN, detChan);
        pslLogError("psl__SetInputAttenuation", info_string, status);
        return status;
    }

    /* Update gain paramters afterwards */
    status = psl__UpdateGain(detChan, modChan, defs, m, det);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error updating gain for detChan %d", detChan);
        pslLogError("psl__SetInputAttenuation", info_string, status);
        return status;
    }

    status = psl__UpdateThresholds(detChan, modChan, defs, m, det);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error updating thresholds for detChan %d", detChan);
        pslLogError("psl__SetInputAttenuation", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/* TODO remove after test so that it's synced with acq values
 * Get the input_attenuation acq value
 */
PSL_STATIC int psl__GetInputAttenuation(int detChan, void *value, XiaDefaults *defs)
{
    int status;
    parameter_t INPUTATTEN = 0;
    boolean_t isMercuryOem = psl__IsMercuryOem(detChan);

    UNUSED(defs);

    ASSERT(value != NULL);

    if (!isMercuryOem) {
        sprintf(info_string, "Skipping getting input_attenuation for non OEM"
                " mercury at channel %d.", detChan);
        pslLogInfo("psl__GetInputAttenuation", info_string);
        return XIA_SUCCESS;
    }

    status = pslGetParameter(detChan, "INPUTATTEN", &INPUTATTEN);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error reading input_attenuation for detChan %d", detChan);
        pslLogError("psl__GetInputAttenuation", info_string, status);
        return status;
    }

    *((double *)value) = (double) INPUTATTEN;

    status = pslSetDefault("input_attenuation", value, defs);
    ASSERT(status == XIA_SUCCESS)	;

    return XIA_SUCCESS;
}

/*
 * Set the input_termination acq value
 */
PSL_STATIC int psl__SetInputTermination(int detChan, int modChan, char *name,
                                 void *value, char *detType, XiaDefaults *defs,
                                 Module *m, Detector *det, FirmwareSet *fs)
{
    int status;
    parameter_t INPUTTERM;

    boolean_t isMercuryOem = psl__IsMercuryOem(detChan);

    UNUSED(fs);
    UNUSED(det);
    UNUSED(m);
    UNUSED(detType);
    UNUSED(modChan);
    UNUSED(defs);

    ASSERT(value);

    if (!isMercuryOem) {
        sprintf(info_string, "Skipping setting of %s for non OEM"
                " mercury at channel %d.", name, detChan);
        pslLogInfo("psl__SetInputTermination", info_string);
        return XIA_SUCCESS;
    }

    INPUTTERM = (parameter_t)(*((double *)value));

    if (INPUTTERM > (parameter_t)MERCURY_MAX_INPUTATTEN || INPUTTERM < 0.) {
        sprintf(info_string, "Specified %s (%hu) out of range (0, %hu) "
                "for detChan %d.", name, INPUTTERM, MERCURY_MAX_INPUTTERM, detChan);
        pslLogError("psl__SetInputTermination", info_string, XIA_PARAMETER_OOR);
        return XIA_PARAMETER_OOR;
    }

    status = pslSetParameter(detChan, "INPUTTERM", INPUTTERM);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting %s to %hu "
                "for detChan %d.", name, INPUTTERM, detChan);
        pslLogError("psl__SetInputTermination", info_string, status);
        return status;
    }

    /* Update gain paramters afterwards */
    status = psl__UpdateGain(detChan, modChan, defs, m, det);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error updating gain for detChan %d", detChan);
        pslLogError("psl__SetInputTermination", info_string, status);
        return status;
    }

    return XIA_SUCCESS;

}

/* TODO remove after test so that it's synced with acq values
 * Get the input_termination acq value
 */
PSL_STATIC int psl__GetInputTermination(int detChan, void *value, XiaDefaults *defs)
{
    int status;
    parameter_t INPUTTERM = 0;
    boolean_t isMercuryOem = psl__IsMercuryOem(detChan);

    UNUSED(defs);

    ASSERT(value != NULL);

    if (!isMercuryOem) {
        sprintf(info_string, "Skipping getting input_termination for non OEM"
                " mercury at channel %d.", detChan);
        pslLogInfo("psl__GetInputTermination", info_string);
        return XIA_SUCCESS;
    }

    status = pslGetParameter(detChan, "INPUTTERM", &INPUTTERM);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error reading input_termination for detChan %d", detChan);
        pslLogError("psl__GetInputTermination", info_string, status);
        return status;
    }

    *((double *)value) = (double) INPUTTERM;

    status = pslSetDefault("input_termination", value, defs);
    ASSERT(status == XIA_SUCCESS);

    return XIA_SUCCESS;
}

/*
 * Get the calibrated_dac acq value
 */
PSL_STATIC int psl__GetCalibratedDac(int detChan, void *value, XiaDefaults *defs)
{
    int status;

    parameter_t DACLOW = 0;
    parameter_t DACHIGH = 0;

    UNUSED(defs);

    ASSERT(value != NULL);

    status = pslGetParameter(detChan, "DACLOW", &DACLOW);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error reading low calibrated DAC for detChan %d",
                detChan);
        pslLogError("psl__GetCalibratedDac", info_string, status);
        return status;
    }

    status = pslGetParameter(detChan, "DACHIGH", &DACHIGH);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error reading high calibrated DAC for detChan %d",
                detChan);
        pslLogError("psl__GetCalibratedDac", info_string, status);
        return status;
    }

    /* Both DACHIGH and DACLOW are stored in the double acq value */
    *((double *)value) =	(double)(DACHIGH * 65536.) + (double)DACLOW;

    status = pslSetDefault("calibrated_dac", value, defs);
    ASSERT(status == XIA_SUCCESS)	;

    return XIA_SUCCESS;
}


/*
 * Placeholder for calibrated_checksum acq value setter.
 *  Trying to set the value during run time will generate a warning
 */
PSL_STATIC int psl__SetCalibratedChecksum(int detChan, int modChan, char *name,
                                          void *value, char *detType, XiaDefaults *defs,
                                          Module *m, Detector *det, FirmwareSet *fs)
{
    UNUSED(detChan);
    UNUSED(modChan);
    UNUSED(name);
    UNUSED(value);
    UNUSED(detType);
    UNUSED(defs);
    UNUSED(m);
    UNUSED(det);
    UNUSED(fs);


    pslLogWarning("psl__SetCalibratedChecksum",
                  "Acquisition value calibrated_checksum is read-only");

    return XIA_SUCCESS;
}


/*
 * Get the calibrated_checksum acq value
 */
PSL_STATIC int psl__GetCalibratedChecksum(int detChan, void *value, XiaDefaults *defs)
{
    int status;

    parameter_t GAINCALCHECK = 0;

    UNUSED(defs);

    ASSERT(value != NULL);

    status = pslGetParameter(detChan, "GAINCALCHECK", &GAINCALCHECK);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error reading calibration checksum for detChan %d",
                detChan);
        pslLogError("psl__GetCalibratedChecksum", info_string, status);
        return status;
    }

    *((double *)value) = (double) GAINCALCHECK;

    status = pslSetDefault("calibrated_checksum", value, defs);
    ASSERT(status == XIA_SUCCESS)	;

    return XIA_SUCCESS;
}


/*
 * Re-read the temperature calibration acquisition values after
 *	possible changes.
 */
PSL_STATIC int psl__SyncTempCalibrationValues(int detChan, Module *m,
                                              XiaDefaults *defs)
{
    int status;

    double calibrated_gain = 0;
    double calibrated_dac = 0;
    double gain_slope = 0.0;
    double calibrated_checksum = 0;

    parameter_t TEMPCORRECTION = 0;

    ASSERT(defs != NULL);

    /* TODO mercury-4 doesn't support TEMPCORRECTION yet */
    if (m->number_of_channels > 1)
    {
        pslLogDebug("psl__SyncTempCalibrationValues",
                    "Mercury-4 doesn't support TEMPCORRECTION.");
        return XIA_SUCCESS;
    }

    status = pslGetParameter(detChan, "TEMPCORRECTION", &TEMPCORRECTION);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting temperature correction for detChan %d",
                detChan);
        pslLogError("psl__SyncTempCalibrationValues", info_string, status);
        return status;
    }

    /* No need to sync calibration values if temperature correction is not set */
    if (TEMPCORRECTION == MERCURY_TEMP_NO_CORRECTION) {
        return XIA_SUCCESS;
    }

    /* Need to call apply before synching so that the values to be up to date */
    status = psl__Apply(detChan, NULL, defs, NULL);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error applying acquisition values after setting"
                " temperature calibration values for detChan %d", detChan);
        pslLogError("psl__SyncTempCalibrationValues", info_string, status);
        return status;
    }

    status = psl__GetGainSlope(detChan, (void *)&gain_slope, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error reading gain slope for detChan %d", detChan);
        pslLogError("psl__SyncTempCalibrationValues", info_string, status);
        return status;
    }

    status = psl__GetCalibratedDac(detChan, (void *)&calibrated_dac, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error reading calibrated dac for detChan %d", detChan);
        pslLogError("psl__SyncTempCalibrationValues", info_string, status);
        return status;
    }

    status = psl__GetCalibratedGain(detChan, (void *)&calibrated_gain, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error reading calibrated gain for detChan %d", detChan);
        pslLogError("psl__SyncTempCalibrationValues", info_string, status);
        return status;
    }

    status = psl__GetCalibratedChecksum(detChan, (void *)&calibrated_checksum, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error reading calibration checksum for detChan %d", detChan);
        pslLogError("psl__SyncTempCalibrationValues", info_string, status);
        return status;
    }

    sprintf(info_string, "Temperature calibration values %0.0lf, %0.0lf, %0.0lf, "
            "%0.0lf (calibrated_gain, calibrated_dac, gain_slope, "
            "calibrated_checksum) for detChan %d", calibrated_gain,
            calibrated_dac, gain_slope, calibrated_checksum, detChan);
    pslLogDebug("psl__SyncTempCalibrationValues", info_string);

    return XIA_SUCCESS;
}


/*
 * Apply temperature calibration acquisition values to DSP if they are
 *	specified in the defaults
 */
PSL_STATIC int psl__ApplyTempCalibrationValues(int detChan, XiaDefaults *defs)
{
    int status;

    parameter_t GAINLOW = 0;
    parameter_t GAINHIGH = 0;
    parameter_t DACLOW = 0;
    parameter_t DACHIGH = 0;
    parameter_t DGDACDGAIN = 0;
    parameter_t GAINCALCHECK = 0;
    parameter_t RESTOREGAIN = 1;

    double calibrated_gain = 0;
    double calibrated_dac = 0;
    double gain_slope = 0.0;
    double calibrated_checksum = 0;

    ASSERT(defs != NULL);

    status = pslGetDefault("calibrated_gain", (void *)&calibrated_gain, defs);

    /* If calibration acquisition values are not found, no need to continue */
    if (status == XIA_NOT_FOUND) {
        sprintf(info_string, "calibrated_gain not specified for detChan %d", detChan);
        pslLogDebug("psl__ApplyTempCalibrationValues", info_string);
        return XIA_SUCCESS;
    }

    status = pslGetDefault("calibrated_dac", (void *)&calibrated_dac, defs);

    /* If calibration acquisition values are not found, no need to continue */
    if (status == XIA_NOT_FOUND) {
        sprintf(info_string, "calibrated_dac not specified for detChan %d", detChan);
        pslLogDebug("psl__ApplyTempCalibrationValues", info_string);
        return XIA_SUCCESS;
    }

    status = pslGetDefault("gain_slope", (void *)&gain_slope, defs);

    /* If calibration acquisition values are not found, no need to continue */
    if (status == XIA_NOT_FOUND) {
        sprintf(info_string, "gain_slope not specified for detChan %d", detChan);
        pslLogDebug("psl__ApplyTempCalibrationValues", info_string);
        return XIA_SUCCESS;
    }

    status = pslGetDefault("calibrated_checksum", (void *)&calibrated_checksum, defs);

    /* If calibration acquisition values are not found, no need to continue */
    if (status == XIA_NOT_FOUND) {
        sprintf(info_string, "calibrated_checksum not specified for detChan %d", detChan);
        pslLogDebug("psl__ApplyTempCalibrationValues", info_string);
        return XIA_SUCCESS;
    }

    if (calibrated_gain == 0 || calibrated_dac == 0 ||
            gain_slope == 0 || calibrated_checksum == 0) {
        sprintf(info_string, "Gain calibration values are not non-zero for "
                " detChan %d, new calibration will be started instead.", detChan);
        pslLogDebug("psl__ApplyTempCalibrationValues", info_string);
        return XIA_SUCCESS;
    }

    sprintf(info_string, "Temperature calibration values %0.0lf, %0.0lf, %0.0lf, "
            "%0.0lf (calibrated_gain, calibrated_dac, gain_slope, "
            "calibrated_checksum) for detChan %d", calibrated_gain,
            calibrated_dac, gain_slope, calibrated_checksum, detChan);
    pslLogDebug("psl__ApplyTempCalibrationValues", info_string);

    GAINHIGH = (parameter_t) (calibrated_gain / 65536.);
    status = pslSetParameter(detChan, "GAINHIGH", GAINHIGH);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error applying GAINHIGH for detChan %d", detChan);
        pslLogError("psl__ApplyTempCalibrationValues", info_string, status);
        return status;
    }

    GAINLOW = (parameter_t) (calibrated_gain - (double)GAINHIGH * 65536.);
    status = pslSetParameter(detChan, "GAINLOW", GAINLOW);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error applying GAINLOW for detChan %d", detChan);
        pslLogError("psl__ApplyTempCalibrationValues", info_string, status);
        return status;
    }

    DACHIGH = (parameter_t) (calibrated_dac / 65536.);
    status = pslSetParameter(detChan, "DACHIGH", DACHIGH);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error applying DACHIGH for detChan %d", detChan);
        pslLogError("psl__ApplyTempCalibrationValues", info_string, status);
        return status;
    }

    DACLOW = (parameter_t) (calibrated_dac - (double)DACHIGH * 65536.);
    status = pslSetParameter(detChan, "DACLOW", DACLOW);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error applying DACLOW for detChan %d", detChan);
        pslLogError("psl__ApplyTempCalibrationValues", info_string, status);
        return status;
    }

    DGDACDGAIN = (parameter_t) gain_slope;
    status = pslSetParameter(detChan, "DGDACDGAIN", DGDACDGAIN);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error applying gain slope for detChan %d", detChan);
        pslLogError("psl__ApplyTempCalibrationValues", info_string, status);
        return status;
    }

    GAINCALCHECK = (parameter_t) calibrated_checksum;
    status = pslSetParameter(detChan, "GAINCALCHECK", GAINCALCHECK);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error applying GAINCALCHECK for detChan %d", detChan);
        pslLogError("psl__ApplyTempCalibrationValues", info_string, status);
        return status;
    }

    RESTOREGAIN = 1;
    status = pslSetParameter(detChan, "RESTOREGAIN", RESTOREGAIN);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting RESTOREGAIN for detChan %d", detChan);
        pslLogError("psl__ApplyTempCalibrationValues", info_string, status);
        return status;
    }

    status = psl__Apply(detChan, NULL, defs, NULL);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error applying acquisition values after setting"
                " temperature calibration values for detChan %d", detChan);
        pslLogError("psl__ApplyTempCalibrationValues", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Returns the statistics for all of the channels on the
 * module that detChan is a part of. value is expected to be a
 * double array with at least 9 elements (for the Mercury) or 36
 * elements (for the Mercury-4). They are stored in the following
 * format:
 *
 * [ch0_runtime, ch0_trigger_livetime, ch0_energy_livetime,
 *  ch0_triggers, ch0_events, ch0_icr, ch0_ocr, ch0_underflows,
 *  ch0_overflows, ..., ch3_runtime, etc.]
 */
PSL_STATIC int psl__GetModuleStatistics2(int detChan, void *value,
                                         XiaDefaults * defs, Module *m)
{
    int status;
    int i;
    int statsPerChan = 9;

    unsigned long stats[MERCURY_MEMORY_BLOCK_SIZE];

    double *modStats = NULL;

    double tLT;
    double rt;
    double trigs;
    double evts;
    double unders;
    double overs;

    UNUSED(defs);


    ASSERT(value != NULL);
    ASSERT(m != NULL);


    modStats = (double *)value;

    status = psl__GetStatisticsBlock(detChan, stats);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error reading statistics block for detChan %d",
                detChan);
        pslLogError("psl__GetModuleStatistics2", info_string, status);
        return status;
    }

    for (i = 0; i < (int)m->number_of_channels; i++) {
        int chanBase = i * statsPerChan;

        status = psl__ExtractRealtime(i, stats, &rt);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error extracting runtime for detChan %d, "
                    "modChan %d", detChan, i);
            pslLogError("psl__GetModuleStatistics2", info_string, status);
            return status;
        }

        modStats[chanBase] = rt;

        status = psl__ExtractTLivetime(i, stats, &tLT);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error extracting trigger livetime for "
                    "detChan %d, modChan %d", detChan, i);
            pslLogError("psl__GetModuleStatistics2", info_string, status);
            return status;
        }

        modStats[chanBase + 1] = tLT;

        status = psl__ExtractELivetime(i, stats, &(modStats[chanBase + 2]));

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error extracting energy livetime for "
                    "detChan %d, modChan %d", detChan, i);
            pslLogError("psl__GetModuleStatistics2", info_string, status);
            return status;
        }

        status = psl__ExtractTriggers(i, stats, &trigs);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error extracting triggers for detChan %d, "
                    "modChan %d", detChan, i);
            pslLogError("psl__GetModuleStatistics2", info_string, status);
            return status;
        }

        modStats[chanBase + 3] = trigs;

        status = psl__ExtractEvents(i, stats, &evts);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error extracting events for detChan %d, "
                    "modChan %d", detChan, i);
            pslLogError("psl__GetModuleStatistics2", info_string, status);
            return status;
        }

        modStats[chanBase + 4] = evts;

        status = psl__ExtractUnderflows(i, stats, &unders);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error extracting underflows for detChan %d, "
                    "modChan %d", detChan, i);
            pslLogError("psl__GetModuleStatistics2", info_string, status);
            return status;
        }

        modStats[chanBase + 7] = unders;

        status = psl__ExtractOverflows(i, stats, &overs);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error extracting overflows for detChan %d, "
                    "modChan %d", detChan, i);
            pslLogError("psl__GetModuleStatistics2", info_string, status);
            return status;
        }

        modStats[chanBase + 8] = overs;

        if (tLT != 0.0) {
            modStats[chanBase + 5] = trigs / tLT;
        } else {
            modStats[chanBase + 5] = 0.0;
        }

        if (rt != 0.0) {
            modStats[chanBase + 6] = (evts + overs + unders) / rt;
        } else {
            modStats[chanBase + 6] = 0.0;
        }
    }

    return XIA_SUCCESS;
}


/*
 * Returns the # of triggers in value.
 */
PSL_STATIC int psl__GetTriggers(int detChan, void *value, XiaDefaults *defs,
                                Module *m)
{
    int status;

    unsigned int modChan;

    unsigned long stats[MERCURY_MEMORY_BLOCK_SIZE];

    UNUSED(defs);
    UNUSED(m);


    ASSERT(value);


    status = psl__GetStatisticsBlock(detChan, stats);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error reading statistics block while getting "
                "the # of triggers for detChan %d.", detChan);
        pslLogError("psl__GetTriggers", info_string, status);
        return status;
    }

    status = pslGetModChan(detChan, m, &modChan);
    /* Failure to find a modChan in a properly configured system is
     * impossible.
     */
    ASSERT(status == XIA_SUCCESS);

    status = psl__ExtractTriggers(modChan, stats, (double *)value);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error extracting the # of triggers from the "
                "module statistics block for detChan %d.", detChan);
        pslLogError("psl__GetTriggers", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Returns the # of underflows in value.
 */
PSL_STATIC int psl__GetUnderflows(int detChan, void *value, XiaDefaults *defs,
                                  Module *m)
{
    int status;

    unsigned int modChan;

    unsigned long stats[MERCURY_MEMORY_BLOCK_SIZE];

    UNUSED(defs);
    UNUSED(m);


    ASSERT(value);


    status = psl__GetStatisticsBlock(detChan, stats);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error reading statistics block while getting "
                "the # of underflows for detChan %d.", detChan);
        pslLogError("psl__GetUnderflows", info_string, status);
        return status;
    }

    status = pslGetModChan(detChan, m, &modChan);
    /* Failure to find a modChan in a properly configured system is
     * impossible.
     */
    ASSERT(status == XIA_SUCCESS);

    status = psl__ExtractUnderflows(modChan, stats, (double *)value);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error extracting the # of underflows from the "
                "module statistics block for detChan %d.", detChan);
        pslLogError("psl__GetUnderflows", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Returns the # of overflows in value.
 */
PSL_STATIC int psl__GetOverflows(int detChan, void *value, XiaDefaults *defs,
                                 Module *m)
{
    int status;

    unsigned int modChan;

    unsigned long stats[MERCURY_MEMORY_BLOCK_SIZE];

    UNUSED(defs);
    UNUSED(m);


    ASSERT(value);


    status = psl__GetStatisticsBlock(detChan, stats);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error reading statistics block while getting "
                "the # of overflows for detChan %d.", detChan);
        pslLogError("psl__GetOverflows", info_string, status);
        return status;
    }

    status = pslGetModChan(detChan, m, &modChan);
    /* Failure to find a modChan in a properly configured system is
     * impossible.
     */
    ASSERT(status == XIA_SUCCESS);

    status = psl__ExtractOverflows(modChan, stats, (double *)value);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error extracting the # of overflows from the "
                "module statistics block for detChan %d.", detChan);
        pslLogError("psl__GetOverflows", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Returns the # of events in the MCA via value as a double.
 */
PSL_STATIC int psl__GetMCAEvents(int detChan, void *value, XiaDefaults *defs,
                                 Module *m)
{
    int status;

    unsigned int modChan;

    unsigned long stats[MERCURY_MEMORY_BLOCK_SIZE];

    UNUSED(defs);
    UNUSED(m);


    ASSERT(value);


    status = psl__GetStatisticsBlock(detChan, stats);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error reading statistics block while getting "
                "the # of MCA events for detChan %d.", detChan);
        pslLogError("psl__GetMCAEvents", info_string, status);
        return status;
    }

    status = pslGetModChan(detChan, m, &modChan);
    /* Failure to find a modChan in a properly configured system is
     * impossible.
     */
    ASSERT(status == XIA_SUCCESS);

    status = psl__ExtractEvents(modChan, stats, (double *)value);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error extracting the # of MCA events from the "
                "module statistics block for detChan %d.", detChan);
        pslLogError("psl__GetMCAEvents", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * Reads the USB firmware version number. Requires Alpha or Rev H
 * firmware.
 *
 * value returns the USB firmware version as an unsigned long
 * [3]Major [2]Minor [0]Build
 */
PSL_STATIC int psl__GetUSBVersion(int detChan, char *name, XiaDefaults *defs,
                                void *value)
{
    int status;

    char mem[MAXITEM_LEN];

    unsigned long* buf = (unsigned long *) value;
    unsigned long version;

    UNUSED(defs);
    UNUSED(name);
    ASSERT(value != NULL);

    /* Read 2 16-bit words from ADDRESS = 0x14000000
     * DATA0: High Byte = USB_MAJ_REV; Low Byte = Status (should be 0)
     * DATA1: High Byte = USB_BUILD_REV; Low Byte = USB_MIN_REV
     */
    sprintf(mem, "eeprom:%#x:%u", USB_VERSION_ADDRESS, 1u);
    status = dxp_read_memory(&detChan, mem, &version);

    if (status != DXP_SUCCESS) {
        pslLogError("psl__GetUSBVersion",
                    "Error reading USB firmware version.", status);
        return status;
    }

    sprintf(info_string, "Raw version = %#lx", version);
    pslLogDebug("psl__GetUSBVersion", info_string);

    *buf = (((version >> 8) & 0xFF) << 24) |
           (((version >> 16) & 0xFF) << 16) |
           ((version >> 24) & 0xFF);

    return XIA_SUCCESS;
}

/* Mercury-OEM
 * Quick test to determined whether connected board is Mercury OEM
 * by checking the loaded FDD for missing fippi_a
 */
PSL_STATIC boolean_t psl__IsMercuryOem(int detChan)
{
    int status;
    int modChan;

    Board *b = NULL;

    status = dxp_det_to_elec(&detChan, &b, &modChan);

    ASSERT(status == DXP_SUCCESS);
    ASSERT(b->system_fpga != NULL);

    return (b->fippi_a == NULL);
}

/*
 * Mercury-OEM: Calculate eVPerADC and SWGAIN
 * from acquisition value settings "dynamic_range", "preamp_gain", "input_attenuation"
 */
 PSL_STATIC int psl_CalculateEvPerADC(int detChan, XiaDefaults *defs,
                parameter_t *SWGAIN, double *eVPerADC)
{
    int status;

    double dynamic_range, input_attenuation, preamp_gain;
    double gDB, gSW, gainSwitch, analogGain;

    if(!psl__IsMercuryOem(detChan)) {
        status = XIA_UNSUPPORTED;
        pslLogError("psl_CalculateEvPerADC", "Switched gain is only "
            "supported for Mercury-OEM", status);
        return status;
    }

    status = pslGetDefault("preamp_gain", (void *)&preamp_gain, defs);
    ASSERT(status == XIA_SUCCESS);

    status = pslGetDefault("input_attenuation", (void *)&input_attenuation, defs);
    ASSERT(status == XIA_SUCCESS);

    status = pslGetDefault("dynamic_range", (void *)&dynamic_range, defs);
    ASSERT(status == XIA_SUCCESS);

    /* Estimate SWGAIN
     *
     * 1.7 * (SWGAIN + 1)   = Switched Gain(dB)
     * 10 ^ (gDB / 20)      = Switched Gain(gSW)
     * Switched Gain        = Anaglog Gain * 2^input_attenuation
     * Delta V Preamp       = dynamic_range * preamp_gain * 65536
     * Analog gain (V/V)    = 40% * 65536 (ADC_RANGE) / Delta V Preamp
     */

     /* preamp_gain in mv/keV needs to be scaled by 1000 * 1000 to V/V */
    analogGain  = 0.4 * 1000.0 * 1000.0 / (dynamic_range * preamp_gain);
    gSW         = analogGain / pow(2.0, input_attenuation);
    gDB         = 20.0 * log10(gSW);
    gainSwitch  = ROUND((gDB - SWITCHED_DB_LOWEST) / SWITCHED_DB_SPACING);
    *SWGAIN     = (parameter_t)MIN(MAX(gainSwitch, 0.0), 15.0);

    sprintf(info_string, "Mercury OEM preamp_gain = %0.4f mV/keV,"
            "dynamic_range = %0.4f eV, input_attenuation = %0.0f",
            preamp_gain, dynamic_range, input_attenuation);
    pslLogInfo("psl_CalculateEvPerADC", info_string);

    sprintf(info_string, "Ideal analogGain = %0.4f, Switched gain (V) = %0.4f, "
            "Switched gain (dB) = %0.4f, gainSwitch = %0.0f, Switched analog gain = %0.4f",
            analogGain, gSW, gDB, gainSwitch, analogGain);
    pslLogInfo("psl_CalculateEvPerADC", info_string);

    /* Round up analogGain to siwtched gain steps */
    analogGain = pow(10.0, 1.7 * (*SWGAIN + 1) / 20.0) / pow(2.0, input_attenuation);

    /* EvPerADC for MercuryOEM is 1/(ADCLSB/eV)
     * ADCLSB/eV = 0.001 keV/eV * 0.001 V/mV * 65536 ADCLSB/V * Preamp Gain [mV/keV] * Analog Gain
     */
    *eVPerADC =  1000.0 * 1000.0 / (65536.0 * analogGain * preamp_gain);

    sprintf(info_string, "Switched analog gain = %0.4f, eVPerADC = %0.4f",
            analogGain, *eVPerADC);
    pslLogInfo("psl_CalculateEvPerADC", info_string);

    return XIA_SUCCESS;
}

/*
 * Mercury-OEM: Set switched gain paramters MCAGAIN, MCAGAINEXP, SWGAIN on device
 * from acquisition value settings "preamp_gain", "input_attenuation"
 */
 PSL_STATIC int psl__UpdateSwitchedGain(int detChan, int modChan,
                XiaDefaults *defs, Module *m, Detector *det)
{
    int status;

    double mca_bin_width;
    double eVPerADC;
    double mcaGain;

    parameter_t SWGAIN = 0;
    parameter_t MCAGAIN = 0;
    signed short MCAGAINEXP = 0;

    UNUSED(det);
    UNUSED(m);
    UNUSED(modChan);

    if(!psl__IsMercuryOem(detChan)) {
        status = XIA_UNSUPPORTED;
        pslLogError("psl__UpdateSwitchedGain", "Switched gain is only "
            "supported for Mercury-OEM", status);
        return status;
    }

    status = psl_CalculateEvPerADC(detChan, defs, &SWGAIN, &eVPerADC);
    ASSERT(status == XIA_SUCCESS);

    status = pslGetDefault("mca_bin_width", (void *)&mca_bin_width, defs);
    ASSERT(status == XIA_SUCCESS);

    mcaGain = eVPerADC / mca_bin_width;

    /* Note that it's possible to have a negative MCAGAINEXP */
    MCAGAINEXP = (signed short)floor(log(mcaGain) / log(2.0));
    MCAGAIN = (parameter_t)(32768.0 * mcaGain / pow(2.0, (double)MCAGAINEXP));

    sprintf(info_string, "mca_bin_width = %0.4f, MCA gain = %0.4f, "
                "SWGAIN = %hu, MCAGAIN = %hu, MCAGAINEXP = %hi",
                mca_bin_width, mcaGain, SWGAIN, MCAGAIN, MCAGAINEXP);
    pslLogInfo("psl__UpdateSwitchedGain", info_string);

    status = pslSetParameter(detChan, "SWGAIN", SWGAIN);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting the SWGAIN for detChan %d", detChan);
        pslLogError("psl__UpdateSwitchedGain", info_string, status);
        return status;
    }

    status = pslSetParameter(detChan, "MCAGAIN", MCAGAIN);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting MCAGAIN for detChan %d", detChan);
        pslLogError("psl__UpdateSwitchedGain", info_string, status);
        return status;
    }

    status = pslSetParameter(detChan, "MCAGAINEXP", MCAGAINEXP);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting MCAGAINEXP for detChan %d", detChan);
        pslLogError("psl__UpdateSwitchedGain", info_string, status);
        return status;
    }

    /* TODO Is temp correction supported? */

    return XIA_SUCCESS;
}


/*
 * Set the rc_time_constant acq value
 */
PSL_STATIC int psl__SetRcTimeContstant(int detChan, int modChan, char *name,
                                 void *value, char *detType, XiaDefaults *defs,
                                 Module *m, Detector *det, FirmwareSet *fs)
{
    int status;
    parameter_t TAUCTRL;

    boolean_t isMercuryOem = psl__IsMercuryOem(detChan);

    UNUSED(fs);
    UNUSED(det);
    UNUSED(m);
    UNUSED(detType);
    UNUSED(modChan);
    UNUSED(defs);

    ASSERT(value);

    if (!isMercuryOem) {
        sprintf(info_string, "Skipping setting of %s for non OEM"
                " mercury at channel %d.", name, detChan);
        pslLogInfo("psl__SetRcTimeContstant", info_string);
        return XIA_SUCCESS;
    }

    TAUCTRL = (parameter_t)(*((double *)value));

    if (TAUCTRL > (parameter_t)MERCURY_MAX_TAUCTRL || TAUCTRL < 0.) {
        sprintf(info_string, "Specified %s (%hu) out of range (0, %hu) "
                "for detChan %d.", name, TAUCTRL, MERCURY_MAX_TAUCTRL, detChan);
        pslLogError("psl__SetRcTimeContstant", info_string, XIA_PARAMETER_OOR);
        return XIA_PARAMETER_OOR;
    }

    status = pslSetParameter(detChan, "TAUCTRL", TAUCTRL);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting %s to %hu "
                "for detChan %d.", name, TAUCTRL, detChan);
        pslLogError("psl__SetRcTimeContstant", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * board operation get_board_features,
 * returns unsigned long representing bit flags
 * defined in handel_constants
 */
PSL_STATIC int psl__GetBoardFeatures(int detChan, char *name, XiaDefaults *defs,
                                   void *value)
{
    boolean_t isMercuryOem = psl__IsMercuryOem(detChan);
    unsigned long *features = (unsigned long *)value;

    UNUSED(defs);
    UNUSED(name);

    *features = BOARD_SUPPORTS_NO_EXTRA_FEATURES;
    *features |= ((unsigned long) isMercuryOem) << BOARD_SUPPORTS_MERCURYOEM_FEATURES;

    return XIA_SUCCESS;
}

/*
 * special run calibrate_rc_time
*/
PSL_STATIC int psl__CalibrateRcTime(int detChan, void *value, XiaDefaults *defs)
{
    int status;
    short task = MERCURY_CT_CALIBRATE_RC;

    boolean_t isMercuryOem = psl__IsMercuryOem(detChan);

    UNUSED(defs);
    UNUSED(value);

    if (!isMercuryOem) {
        sprintf(info_string, "Skipping calibrate_rc_time special run for non OEM"
                " mercury at channel %d.", detChan);
        pslLogWarning("psl__CalibrateRcTime", info_string);
        return XIA_SUCCESS;
    }

    sprintf(info_string, "special run calibrate_rc_time on channel %d ", detChan);
    pslLogInfo("psl__CalibrateRcTime", info_string);

    status = dxp_start_control_task(&detChan, &task, NULL, NULL);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error starting MERCURY_CT_CALIBRATE_RC control "
            "task for detChan %d", detChan);
        pslLogError("psl__CalibrateRcTime", info_string, status);
        return status;
    }

    status = dxp_stop_control_task(&detChan);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error stopping MERCURY_CT_CALIBRATE_RC control "
            "task for detChan %d", detChan);
        pslLogError("psl__CalibrateRcTime", info_string, status);
        return status;
    }

    return DXP_SUCCESS;
}

/*
 * acquisition value rc_time
 * For non RC type mercury OEM this can be reset by calibration or changing
 * rc_time_constant
 */
PSL_STATIC int psl__GetRcTime(int detChan, void *value, XiaDefaults *defs)
{
    int status;

    double preamp_type = 0.0;
    double *rc_time = (double *)value;

    parameter_t RCTAU;
    parameter_t RCTAUFRAC;

    boolean_t isMercuryOem = psl__IsMercuryOem(detChan);

    UNUSED(defs);

    ASSERT(value != NULL);

    status = pslGetDefault("preamp_type", (void *)&preamp_type, defs);
    ASSERT(status == XIA_SUCCESS);

    if (!isMercuryOem) {
        sprintf(info_string, "Skipping get decay time: detChan %d is not "
                "a RC-type preamplifier or Mercury OEM.", detChan);
        pslLogInfo("psl__GetRcTime", info_string);
        return XIA_SUCCESS;
    }

    status = pslGetParameter(detChan, "RCTAU", &RCTAU);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting RCTAU for detChan %d", detChan);
        pslLogError("psl__GetRcTime", info_string, status);
        return status;
    }

    status = pslGetParameter(detChan, "RCTAUFRAC", &RCTAUFRAC);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting RCTAUFRAC for detChan %d", detChan);
        pslLogError("psl__GetRcTime", info_string, status);
        return status;
    }

    *rc_time  = (double) RCTAU + (double) RCTAUFRAC / 65536.;

    sprintf(info_string, "rc_time = %0.2f",  *rc_time);
    pslLogDebug("psl__GetRcTime", info_string);

    /* Update the defaults list */
    status = pslSetDefault("rc_time", rc_time, defs);

    return XIA_SUCCESS;
}

/*
 * acquisition value rc_time
 * Similar to decay_time setter but we skipping setting the type value
 */
PSL_STATIC int psl__SetRcTime(int detChan, int modChan, char *name,
                                 void *value, char *detType,
                                 XiaDefaults *defs, Module *m, Detector *det,
                                 FirmwareSet *fs)
{
    int status;

    double decayTime = 0.0;

    parameter_t RCTAU;
    parameter_t RCTAUFRAC;

    boolean_t isMercuryOem = psl__IsMercuryOem(detChan);

    UNUSED(fs);
    UNUSED(defs);
    UNUSED(detType);
    UNUSED(name);
    UNUSED(det);
    UNUSED(m);
    UNUSED(modChan);

    ASSERT(value != NULL);


    if (!isMercuryOem) {
        sprintf(info_string, "Skipping set rc_time: detChan %d is not "
                "Mercury OEM.", detChan);
        pslLogInfo("psl__SetRcTime", info_string);
        return XIA_SUCCESS;
    }

    decayTime = *((double *)value);

    RCTAU     = (parameter_t)floor(decayTime);
    RCTAUFRAC = (parameter_t)ROUND((decayTime - (double)RCTAU) * 65536.0);

    status = pslSetParameter(detChan, "RCTAU", RCTAU);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting RCTAU to %#hx for a decay time of "
                "%0.6f microseconds for detChan %d", RCTAU, decayTime, detChan);
        pslLogError("psl__SetRcTime", info_string, status);
        return status;
    }

    status = pslSetParameter(detChan, "RCTAUFRAC", RCTAUFRAC);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting RCTAUFRAC to %#hx for a decay time of "
                "%0.6f microseconds for detChan %d", RCTAUFRAC, decayTime, detChan);
        pslLogError("psl__SetRcTime", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * acquisition value trace_trigger_type
 *
 */
PSL_STATIC int psl__SetTriggerType(int detChan, int modChan, char *name,
                                 void *value, char *detType,
                                 XiaDefaults *defs, Module *m, Detector *det,
                                 FirmwareSet *fs)
{
    int status;

    parameter_t TRACETRIG;
    double trigType = *((double *)value);

    boolean_t isMercuryOem = psl__IsMercuryOem(detChan);

    UNUSED(fs);
    UNUSED(defs);
    UNUSED(detType);
    UNUSED(name);
    UNUSED(det);
    UNUSED(m);
    UNUSED(modChan);

    if (!isMercuryOem) {
        sprintf(info_string, "Skipping trace_trigger_type, not supported by "
                "non Mercury OEM variant");
        pslLogWarning("psl__SetTriggerType", info_string);
        return XIA_SUCCESS;
    }

    if (trigType < 0 || trigType > 255.0) {
        sprintf(info_string, "Trace trigger type %0f is out-of-range",
                trigType);
        pslLogError("psl__SetTriggerType", info_string, XIA_BAD_VALUE);
        return XIA_BAD_VALUE;
    }

    TRACETRIG = (parameter_t)trigType;

    status = pslSetParameter(detChan, "TRACETRIG", TRACETRIG);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting TRACETRIG for detChan %d", detChan);
        pslLogError("psl__SetTriggerType", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * acquisition value trace_trigger_position
 *
 */
PSL_STATIC int psl__SetTriggerPosition(int detChan, int modChan, char *name,
                                 void *value, char *detType,
                                 XiaDefaults *defs, Module *m, Detector *det,
                                 FirmwareSet *fs)
{
    int status;

    parameter_t TRACEPRETRIG;
    double trigPosition = *((double *)value);

    boolean_t isMercuryOem = psl__IsMercuryOem(detChan);

    UNUSED(fs);
    UNUSED(defs);
    UNUSED(detType);
    UNUSED(name);
    UNUSED(det);
    UNUSED(m);
    UNUSED(modChan);

    if (!isMercuryOem) {
        sprintf(info_string, "Skipping trace_trigger_position, not supported by "
                "non Mercury OEM variant");
        pslLogWarning("psl__SetTriggerPosition", info_string);
        return XIA_SUCCESS;
    }

    if (trigPosition < 0 || trigPosition > 255.0) {
        sprintf(info_string, "Trace trigger position %0f is out-of-range",
                trigPosition);
        pslLogError("psl__SetTriggerPosition", info_string, XIA_BAD_VALUE);
        return XIA_BAD_VALUE;
    }

    TRACEPRETRIG = (parameter_t)trigPosition;

    status = pslSetParameter(detChan, "TRACEPRETRIG", TRACEPRETRIG);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting TRACEPRETRIG for detChan %d", detChan);
        pslLogError("psl__SetTriggerPosition", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Special run adjust_offsets
 */
PSL_STATIC int psl__AdjustOffsets(int detChan, void *value, XiaDefaults *defs)
{
    int status;
    unsigned short SETOFFADC = 0;
    unsigned short SETODAC = 0;

    double offset  = *((double *)value);
    double offset_dac;
    short task = MERCURY_CT_SET_OFFADC;

    boolean_t isMercuryOem = psl__IsMercuryOem(detChan);

    ASSERT(value != NULL);

    if (!isMercuryOem) {
        sprintf(info_string, "Skipping adjust_offsets special run for non OEM"
                " mercury at channel %d.", detChan);
        pslLogError("psl__AdjustOffsets", info_string, XIA_NOSUPPORT_VALUE);
        return XIA_NOSUPPORT_VALUE;

    }

    sprintf(info_string, "special run adjust_offsets on channel %d ", detChan);
    pslLogInfo("psl__AdjustOffsets", info_string);

    if (offset > 65536 || offset < 0) {
        sprintf(info_string, "ADC offset %0f is out-of-range (%d, %d)",
                offset, 0, 65536);
        pslLogError("psl__AdjustOffsets", info_string, XIA_BAD_VALUE);
        return XIA_BAD_VALUE;
    }

    SETOFFADC = (unsigned short)offset;

    status = pslSetParameter(detChan, "SETOFFADC", SETOFFADC);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting SETOFFADC for detChan %d", detChan);
        pslLogError("psl__AdjustOffsets", info_string, status);
        return status;
    }

    status = dxp_start_control_task(&detChan, &task, NULL, NULL);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error starting MERCURY_CT_SET_OFFADC control "
            "task for detChan %d", detChan);
        pslLogError("psl__AdjustOffsets", info_string, status);
        return status;
    }

    status = dxp_stop_control_task(&detChan);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error stopping MERCURY_CT_SET_OFFADC control "
            "task for detChan %d", detChan);
        pslLogError("psl__AdjustOffsets", info_string, status);
        return status;
    }

    /* Sync adc_offset with actual value */
    status = pslSetDefault("adc_offset", &offset, defs);
    ASSERT(status == XIA_SUCCESS);

    /* Also sync the offset_dac here with actual value, so that it can
     * be saved and reload on restart
     */
    status = pslGetParameter(detChan, "SETODAC", &SETODAC);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting SETODAC for detChan %d", detChan);
        pslLogError("psl__AdjustOffsets", info_string, status);
        return status;
    }

    offset_dac = (double)SETODAC;

    status = pslSetDefault("offset_dac", &offset_dac, defs);
    ASSERT(status == XIA_SUCCESS);

    return XIA_SUCCESS;

}


/*
 * acquisition value adc_offset
 *
 */
PSL_STATIC int psl__SetAdcOffset(int detChan, int modChan, char *name,
                                 void *value, char *detType,
                                 XiaDefaults *defs, Module *m, Detector *det,
                                 FirmwareSet *fs)
{
    int status;

    parameter_t SETOFFADC;
    double adc_offset = *((double *)value);

    boolean_t isMercuryOem = psl__IsMercuryOem(detChan);

    UNUSED(fs);
    UNUSED(defs);
    UNUSED(detType);
    UNUSED(name);
    UNUSED(det);
    UNUSED(m);
    UNUSED(modChan);

    if (!isMercuryOem) {
        sprintf(info_string, "Skipping adc_offset, not supported by "
                "non Mercury OEM variant");
        pslLogWarning("psl__SetAdcOffset", info_string);
        return XIA_SUCCESS;
    }

    if (adc_offset < 0 || adc_offset > 65536.0) {
        sprintf(info_string, "adc_offset %0f is out-of-range",
                adc_offset);
        pslLogError("psl__SetAdcOffset", info_string, XIA_BAD_VALUE);
        return XIA_BAD_VALUE;
    }

    SETOFFADC = (parameter_t)adc_offset;

    status = pslSetParameter(detChan, "SETOFFADC", SETOFFADC);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting SETOFFADC for detChan %d", detChan);
        pslLogError("psl__SetAdcOffset", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


/*
 * acquisition value offset_dac
 *
 */
PSL_STATIC int psl__SetOffsetDac(int detChan, int modChan, char *name,
                                 void *value, char *detType,
                                 XiaDefaults *defs, Module *m, Detector *det,
                                 FirmwareSet *fs)
{
    int status;

    parameter_t SETODAC;
    double offset_dac = *((double *)value);

    boolean_t isMercuryOem = psl__IsMercuryOem(detChan);

    UNUSED(fs);
    UNUSED(defs);
    UNUSED(detType);
    UNUSED(name);
    UNUSED(det);
    UNUSED(m);
    UNUSED(modChan);

    if (!isMercuryOem) {
        sprintf(info_string, "Skipping offset_dac, not supported by "
                "non Mercury OEM variant");
        pslLogWarning("psl__SetOffsetDac", info_string);
        return XIA_SUCCESS;
    }

    if (offset_dac < 0 || offset_dac > 65536.0) {
        sprintf(info_string, "offset_dac %0f is out-of-range",
                offset_dac);
        pslLogError("psl__SetOffsetDac", info_string, XIA_BAD_VALUE);
        return XIA_BAD_VALUE;
    }

    SETODAC = (parameter_t)offset_dac;

    status = pslSetParameter(detChan, "SETODAC", SETODAC);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting SETODAC for detChan %d", detChan);
        pslLogError("psl__SetOffsetDac", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * acquisition value baseline_factor
 * Setting this will change SLOWLEN and require refreshing filter parameters
 */
PSL_STATIC int psl__SetBaselineFactor(int detChan, int modChan, char *name,
                                 void *value, char *detType,
                                 XiaDefaults *defs, Module *m, Detector *det,
                                 FirmwareSet *fs)
{
    int status;

    parameter_t BFACTOR;
    double baseline_factor = *((double *)value);

    boolean_t isMercuryOem = psl__IsMercuryOem(detChan);

    UNUSED(fs);
    UNUSED(defs);
    UNUSED(detType);
    UNUSED(name);
    UNUSED(det);
    UNUSED(m);
    UNUSED(modChan);

    if (!isMercuryOem) {
        sprintf(info_string, "Skipping baseline_factor, not supported by "
                "non Mercury OEM variant");
        pslLogWarning("psl__SetBaselineFactor", info_string);
        return XIA_SUCCESS;
    }

    if (baseline_factor < 0 || baseline_factor > 1) {
        sprintf(info_string, "baseline_factor %0f is out-of-range (0,1)",
                baseline_factor);
        pslLogError("psl__SetBaselineFactor", info_string, XIA_BAD_VALUE);
        return XIA_BAD_VALUE;
    }

    BFACTOR = (parameter_t)baseline_factor;

    status = pslSetParameter(detChan, "BFACTOR", BFACTOR);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting BFACTOR for detChan %d", detChan);
        pslLogError("psl__SetBaselineFactor", info_string, status);
        return status;
    }

    status = pslSetDefault("baseline_factor", &baseline_factor, defs);
    ASSERT(status == XIA_SUCCESS);

    return XIA_SUCCESS;

}

PSL_STATIC int psl__GetListBufferLenA(int detChan, void *value,
                                      XiaDefaults *defs, Module *m)
{
    int status;

    UNUSED(defs);
    UNUSED(m);

    ASSERT(value);


    status = psl__GetListBufferLen(detChan, 'a', (unsigned long *)value);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting the length of list mode buffer "
                "A for detChan %d.", detChan);
        pslLogError("psl__GetListBufferLenA", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


PSL_STATIC int psl__GetListBufferLenB(int detChan, void *value,
                                      XiaDefaults *defs, Module *m)
{
    int status;

    UNUSED(defs);
    UNUSED(m);

    ASSERT(value);


    status = psl__GetListBufferLen(detChan, 'b', (unsigned long *)value);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting the length of list mode buffer "
                "B for detChan %d.", detChan);
        pslLogError("psl__GetListBufferLenB", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


PSL_STATIC int psl__GetListBufferLen(int detChan, char buf, unsigned long *len)
{
    int status;

    boolean_t isMapping;

    parameter_t lenLow = 0xFFFF;
    parameter_t lenHigh = 0xFFFF;


    status = psl__IsMapping(detChan, MAPPING_LIST, &isMapping);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error checking if list mode is available for "
                "detChan %d.", detChan);
        pslLogError("psl__GetListBufferLen", info_string, status);
        return status;
    }

    if (!isMapping) {
        sprintf(info_string, "List mode firmware is not currently loaded for "
                "detChan %d.", detChan);
        pslLogError("psl__GetListBufferLen", info_string, XIA_NO_MAPPING);
        return XIA_NO_MAPPING;
    }

    switch(buf) {
    case 'a':
        status = pslGetParameter(detChan, "LISTBUFALEN", &lenLow);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error getting low word of list mode "
                    "buffer length for detChan %d.", detChan);
            pslLogError("psl__GetListBufferLen", info_string, status);
            return status;
        }

        status = pslGetParameter(detChan, "LISTBUFALENA", &lenHigh);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error getting high word of list mode "
                    "buffer length for detChan %d.", detChan);
            pslLogError("psl__GetListBufferLen", info_string, status);
            return status;
        }

        break;

    case 'b':
        status = pslGetParameter(detChan, "LISTBUFBLEN", &lenLow);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error getting low word of list mode "
                    "buffer length for detChan %d.", detChan);
            pslLogError("psl__GetListBufferLen", info_string, status);
            return status;
        }

        status = pslGetParameter(detChan, "LISTBUFBLENA", &lenHigh);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error getting high word of list mode "
                    "buffer length for detChan %d.", detChan);
            pslLogError("psl__GetListBufferLen", info_string, status);
            return status;
        }

        break;

    default:
        FAIL();
        break;
    }

    /* Only the bottom 4 bits of the high word should be set. The
     * maximum length of each buffer is 20 bits.
     */
    if ((lenHigh & 0xFFF0) != 0) {
        sprintf(info_string, "The upper word of the list buffer length "
                "stored in the DSP (%#hx) is malformed for detChan %d.",
                lenHigh, detChan);
        pslLogError("psl__GetListBufferLen", info_string, XIA_MALFORMED_LENGTH);
        return XIA_MALFORMED_LENGTH;
    }

    *len = WORD_TO_LONG(lenLow, lenHigh);

    return XIA_SUCCESS;
}


PSL_STATIC int psl__SetListModeVariant(int detChan, int modChan, char *name,
                                 void *value, char *detType,
                                 XiaDefaults *defs, Module *m, Detector *det,
                                 FirmwareSet *fs)
{
    int status;

    parameter_t LISTMODEVAR = (parameter_t)(*((double *)value));

    boolean_t isMapping;

    UNUSED(fs);
    UNUSED(det);
    UNUSED(m);
    UNUSED(detType);
    UNUSED(modChan);
    UNUSED(defs);

    ASSERT(value);

    status = psl__IsMapping(detChan, MAPPING_LIST, &isMapping);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error checking if list mode is available for "
                "detChan %d", detChan);
        pslLogError("psl__SetListModeVariant", info_string, status);
        return status;
    }

    if (!isMapping) {
        sprintf(info_string, "Skipping '%s' since list mode mapping is disabled "
                "for detChan %d.", name, detChan);
        pslLogInfo("psl__SetListModeVariant", info_string);
        return XIA_SUCCESS;
    }

    status = pslSetParameter(detChan, "LISTMODEVAR", LISTMODEVAR);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting list mode variant to %hu "
                "for detChan %d.", LISTMODEVAR, detChan);
        pslLogError("psl__SetListModeVariant", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}


