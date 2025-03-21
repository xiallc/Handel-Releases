/*
 * Copyright (c) 2002-2004 X-ray Instrumentation Associates
 * Copyright (c) 2005-2020 XIA LLC
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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "handel_errors.h"
#include "xia_assert.h"
#include "xia_common.h"
#include "xia_handel_structures.h"
#include "xia_module.h"
#include "xia_psl.h"
#include "xia_system.h"

#include "handel_constants.h"

#include "saturn.h"
#include "xerxes.h"
#include "xerxes_errors.h"
#include "xerxes_generic.h"

#include "fdd.h"

#include "psl_common.h"
#include "psl_saturn.h"

#define NUM_BITS_ADC 1024.0f

static double CLOCK_SPEED;

/* Prototypes */

PSL_EXPORT int PSL_API saturn_PSLInit(PSLFuncs* funcs);

PSL_STATIC int pslSynchResetDelay(int detChan, int det_chan, Module* m, Detector* det,
                                  XiaDefaults* defs);
PSL_STATIC int pslSynchDecayTime(int detChan, int det_chan, Module* m, Detector* det,
                                 XiaDefaults* defs);
PSL_STATIC int pslSynchPolarity(int detChan, int det_chan, Module* m, Detector* det,
                                XiaDefaults* defs);
PSL_STATIC int pslSynchPreampGain(int detChan, int det_chan, Module* m, Detector* det,
                                  XiaDefaults* defs);
PSL_STATIC int pslGetParamNames(int* detChan, char** list);

/* Hide all of the acquisition value prototypes behind this fancy macro. */
ACQUISITION_VALUE(PeakingTime);
ACQUISITION_VALUE(TriggerThreshold);
ACQUISITION_VALUE(EnergyThreshold);
ACQUISITION_VALUE(NumMCAChannels);
ACQUISITION_VALUE(MCALowLimit);
ACQUISITION_VALUE(MCABinWidth);
ACQUISITION_VALUE(ADCPercentRule);
ACQUISITION_VALUE(EnableGate);
ACQUISITION_VALUE(EnableBaselineCut);
ACQUISITION_VALUE(PreampGain);
ACQUISITION_VALUE(Polarity);
ACQUISITION_VALUE(ResetDelay);
ACQUISITION_VALUE(CalibrationEnergy);
ACQUISITION_VALUE(GapTime);
ACQUISITION_VALUE(TriggerPeakingTime);
ACQUISITION_VALUE(TriggerGapTime);
ACQUISITION_VALUE(DecayTime);
ACQUISITION_VALUE(BaselineCut);
ACQUISITION_VALUE(BaselineFilterLength);
ACQUISITION_VALUE(ActualGapTime);
PSL_STATIC int psl__SetMaxWidth(int detChan, void* value, FirmwareSet* fs,
                                char* detType, XiaDefaults* defs, double preampGain,
                                Module* m, Detector* det, int detector_chan);
PSL_STATIC int psl__SetPresetType(int detChan, void* value, FirmwareSet* fs,
                                  char* detType, XiaDefaults* defs, double preampGain,
                                  Module* m, Detector* det, int detector_chan);
PSL_STATIC int psl__SetPresetValue(int detChan, void* value, FirmwareSet* fs,
                                   char* detType, XiaDefaults* defs, double preampGain,
                                   Module* m, Detector* det, int detector_chan);
PSL_STATIC int psl__SetMCAStartAddress(int detChan, void* value, FirmwareSet* fs,
                                       char* detType, XiaDefaults* defs,
                                       double preampGain, Module* m, Detector* det,
                                       int detector_chan);
PSL_STATIC int psl__SetBThresh(int detChan, void* value, FirmwareSet* fs, char* detType,
                               XiaDefaults* defs, double preampGain, Module* m,
                               Detector* det, int detector_chan);
PSL_STATIC int psl__SetMinGapTime(int detChan, void* value, FirmwareSet* fs,
                                  char* detType, XiaDefaults* defs, double preampGain,
                                  Module* m, Detector* det, int detector_chan);

PSL_STATIC int PSL_API pslDoEnableInterrupt(int detChan, void* value);

PSL_STATIC int pslDoFilter(int detChan, char* name, void* value, XiaDefaults* defaults,
                           FirmwareSet* firmwareSet, double preampGain, Module* m);
PSL_STATIC int PSL_API pslDoParam(int detChan, char* name, void* value,
                                  XiaDefaults* defaults);
PSL_STATIC int PSL_API _pslDoNSca(int detChan, char* name, void* value, Module* m,
                                  XiaDefaults* defaults);
PSL_STATIC int psl__SynchNumberSCAs(int detChan, int det_chan, Module* m, Detector* det,
                                    XiaDefaults* defs);
PSL_STATIC int PSL_API _pslDoSCA(int detChan, char* name, void* value, Module* m,
                                 XiaDefaults* defaults);
PSL_STATIC int pslQuickRun(int detChan, XiaDefaults* defaults, Module* m);
PSL_STATIC int psl__GetMCALength(int detChan, void* value, XiaDefaults* defs);
PSL_STATIC int psl__GetMCAData(int detChan, void* value, XiaDefaults* defs);
PSL_STATIC int psl__GetSCAData(int detChan, void* value, XiaDefaults* defs);
PSL_STATIC int psl__GetSCALength(int detChan, void* value, XiaDefaults* defs);
PSL_STATIC int psl__GetMaxSCALength(int detChan, void* value, XiaDefaults* defaults);
PSL_STATIC int psl__GetLivetime(int detChan, void* value, XiaDefaults* defs);
PSL_STATIC int psl__GetRealtime(int detChan, void* value, XiaDefaults* defs);
PSL_STATIC int psl__GetICR(int detChan, void* value, XiaDefaults* defs);
PSL_STATIC int psl__GetOCR(int detChan, void* value, XiaDefaults* defs);
PSL_STATIC int psl__GetTotalEvents(int detChan, void* value, XiaDefaults* defs);
PSL_STATIC int psl__GetMCAEvents(int detChan, void* value, XiaDefaults* defs);
PSL_STATIC int psl__GetTriggers(int detChan, void* value, XiaDefaults* defs);
PSL_STATIC int psl__GetUnderflows(int detChan, void* value, XiaDefaults* defs);
PSL_STATIC int psl__GetOverflows(int detChan, void* value, XiaDefaults* defs);
PSL_STATIC int psl__GetBaselineLength(int detChan, void* value, XiaDefaults* defs);
PSL_STATIC int psl__GetBaseline(int detChan, void* value, XiaDefaults* defs);
PSL_STATIC int psl__GetRunActive(int detChan, void* value, XiaDefaults* defs);
PSL_STATIC int psl__GetModuleStatistics(int detChan, void* value, XiaDefaults* defs);
PSL_STATIC int psl__GetModuleStatistics2(int detChan, void* value, XiaDefaults* defs);
PSL_STATIC int psl__GetELivetime(int detChan, void* value, XiaDefaults* defs);
PSL_STATIC int PSL_API pslDoADCTrace(int detChan, void* info);
PSL_STATIC int PSL_API pslGetControlTaskLength(int detChan, short task, void* value);
PSL_STATIC int PSL_API pslGetControlTaskData(int detChan, short task, void* value);
PSL_STATIC int PSL_API pslGetControlTaskDataWithStop(int detChan, short task,
                                                     void* value);
PSL_STATIC int PSL_API pslDoBaseHistory(int detChan, void* info);
PSL_STATIC int PSL_API pslGetBaseHistory(int detChan, void* value);
PSL_STATIC int PSL_API pslDoControlTask(int detChan, short task, unsigned int len,
                                        void* info);
PSL_STATIC int PSL_API pslDoControlTaskWithoutStop(int detChan, short task,
                                                   unsigned int len, void* info);
PSL_STATIC int PSL_API pslCalculateGain(int detChan, double adcPercentRule,
                                        double calibEV, double preampGain,
                                        double MCABinWidth, parameter_t SLOWLEN,
                                        parameter_t* GAINDAC);
PSL_STATIC int pslDoGainSetting(int detChan, XiaDefaults* defaults, double preampGain,
                                Module* m);
PSL_STATIC double PSL_API pslCalculateSysGain(void);

PSL_STATIC int pslUpdateFilter(int detChan, double peakingTime, XiaDefaults* defaults,
                               FirmwareSet* firmwareSet, double preampGain, Module* m);
PSL_STATIC int PSL_API pslUpdateTriggerFilter(int detChan, XiaDefaults* defaults);
PSL_STATIC boolean_t PSL_API pslIsInterfaceValid(Module* module);
PSL_STATIC boolean_t PSL_API pslIsEPPAddressValid(Module* module);
PSL_STATIC boolean_t PSL_API pslIsNumChannelsValid(Module* module);
PSL_STATIC boolean_t PSL_API pslAreAllDefaultsPresent(XiaDefaults* defaults);

PSL_STATIC int PSL_API pslTauFinder(int detChan, XiaDefaults* defaults,
                                    Detector* detector, int detector_chan, void* vInfo);
PSL_STATIC double PSL_API pslTauFit(unsigned int* trace, unsigned long kmin,
                                    unsigned long kmax, double dt);
PSL_STATIC double PSL_API pslPhiValue(unsigned int* ydat, double qq, unsigned long kmin,
                                      unsigned long kmax);
PSL_STATIC double PSL_API pslThreshFinder(unsigned int* trace, double tau,
                                          unsigned short* randomSet, double adcDelay,
                                          double* ff, parameter_t FL, parameter_t FG,
                                          unsigned long adcLength);
PSL_STATIC void PSL_API pslRandomSwap(unsigned long adcLength,
                                      unsigned short* randomSet);

/* Clock utility routines */
PSL_STATIC int pslGetClockSpeed(int detChan, double* spd);

PSL_STATIC int psl__UpdateMCAAddressCache(int detChan, XiaDefaults* defs);
PSL_STATIC int psl__GetEVPerADC(XiaDefaults* defs, double* eVPerADC);
PSL_STATIC int psl__SetRunTasks(int detChan, void* value, int bit);
PSL_STATIC int psl__GetDSPBlock(int detChan, unsigned long* params);
PSL_STATIC int psl__ExtractRealtime(int detChan, unsigned long* params, double* rt);
PSL_STATIC int psl__ExtractTLivetime(int detChan, unsigned long* params, double* tlt);
PSL_STATIC int psl__ExtractELivetime(int detChan, unsigned long* params, double* elt);
PSL_STATIC int psl__ExtractTriggers(int detChan, unsigned long* params,
                                    unsigned long* trigs);
PSL_STATIC int psl__ExtractEvents(int detChan, unsigned long* params,
                                  unsigned long* evts);
PSL_STATIC int psl__ExtractUnders(int detChan, unsigned long* params,
                                  unsigned long* unders);
PSL_STATIC int psl__ExtractOvers(int detChan, unsigned long* params,
                                 unsigned long* overs);

/* Gain Operations */
PSL_STATIC int psl__GainCalibrate(int detChan, Detector* det, int modChan, Module* m,
                                  XiaDefaults* defs, void* value);
PSL_STATIC int psl__AdjustPercentRule(int detChan, Detector* det, int modChan,
                                      Module* m, XiaDefaults* defs, void* value);

static Saturn_AcquisitionValue_t ACQ_VALUES[] = {
    {"peaking_time", TRUE_, FALSE_, 8.0, pslDoPeakingTime, NULL},

    {"mca_bin_width", TRUE_, FALSE_, 20.0, pslDoMCABinWidth, NULL},

    {"number_mca_channels", TRUE_, FALSE_, 4096.0, pslDoNumMCAChannels, NULL},

    {"mca_low_limit", TRUE_, FALSE_, 0.0, pslDoMCALowLimit, NULL},

    {"energy_threshold", TRUE_, FALSE_, 0.0, pslDoEnergyThreshold, NULL},

    {"adc_percent_rule", TRUE_, FALSE_, 5.0, pslDoADCPercentRule, NULL},

    {"calibration_energy", TRUE_, FALSE_, 5900.0, pslDoCalibrationEnergy, NULL},

    {"gap_time", TRUE_, FALSE_, .150, pslDoGapTime, NULL},

    {"trigger_peaking_time", TRUE_, FALSE_, .200, pslDoTriggerPeakingTime, NULL},

    {"trigger_gap_time", TRUE_, FALSE_, 0.0, pslDoTriggerGapTime, NULL},

    {"trigger_threshold", TRUE_, FALSE_, 1000.0, pslDoTriggerThreshold, NULL},

    {"enable_gate", TRUE_, FALSE_, 0.0, pslDoEnableGate, NULL},

    {"enable_baseline_cut", TRUE_, FALSE_, 1.0, pslDoEnableBaselineCut, NULL},

    {"reset_delay", TRUE_, TRUE_, 50.0, pslDoResetDelay, pslSynchResetDelay},

    {"decay_time", TRUE_, TRUE_, 50.0, pslDoDecayTime, pslSynchDecayTime},

    {"detector_polarity", TRUE_, TRUE_, 1.0, pslDoPolarity, pslSynchPolarity},

    {"preamp_gain", TRUE_, TRUE_, 2.0, pslDoPreampGain, pslSynchPreampGain},

    {"baseline_cut", TRUE_, FALSE_, 5.0, pslDoBaselineCut, NULL},

    {"baseline_filter_length", TRUE_, FALSE_, 128.0, pslDoBaselineFilterLength, NULL},

    {"actual_gap_time", TRUE_, FALSE_, .150, pslDoActualGapTime, NULL},

    {"preset_type", TRUE_, FALSE_, 0.0, psl__SetPresetType, NULL},

    {"preset_value", TRUE_, FALSE_, 0.0, psl__SetPresetValue, NULL},

    {"mca_start_address", TRUE_, FALSE_, 0.0, psl__SetMCAStartAddress, NULL},

    {"baseline_threshold", TRUE_, FALSE_, 1000.0, psl__SetBThresh, NULL},

    {"maxwidth", TRUE_, FALSE_, 1.000, psl__SetMaxWidth, NULL},

    {"minimum_gap_time", TRUE_, FALSE_, 0.060, psl__SetMinGapTime, NULL},

    {"number_of_scas", TRUE_, FALSE_, 0.000, NULL, psl__SynchNumberSCAs},

};

char* defaultNames[] = {"peaking_time",
                        "mca_bin_width",
                        "number_mca_channels",
                        "mca_low_limit",
                        "energy_threshold",
                        "adc_percent_rule",
                        "calibration_energy",
                        "gap_time",
                        "trigger_peaking_time",
                        "trigger_gap_time",
                        "trigger_threshold",
                        "enable_gate",
                        "enable_baseline_cut",
                        "reset_delay",
                        "decay_time",
                        "detector_polarity",
                        "preamp_gain",
                        "baseline_cut",
                        "baseline_filter_length",
                        "actual_gap_time",
                        "baseline_threshold",
                        "max_width",
                        "minimum_gap_time",
                        "number_of_scas"};

static Saturn_RunData RUN_DATA[] = {{"mca_length", psl__GetMCALength},
                                    {"mca", psl__GetMCAData},
                                    {"livetime", psl__GetLivetime},
                                    {"trigger_livetime", psl__GetLivetime},
                                    {"runtime", psl__GetRealtime},
                                    {"realtime", psl__GetRealtime},
                                    {"input_count_rate", psl__GetICR},
                                    {"output_count_rate", psl__GetOCR},
                                    {"events_in_run", psl__GetTotalEvents},
                                    {"triggers", psl__GetTriggers},
                                    {"baseline_length", psl__GetBaselineLength},
                                    {"baseline", psl__GetBaseline},
                                    {"run_active", psl__GetRunActive},
                                    {"sca_length", psl__GetSCALength},
                                    {"max_sca_length", psl__GetMaxSCALength},
                                    {"sca", psl__GetSCAData},
                                    {"module_statistics", psl__GetModuleStatistics},
                                    {"energy_livetime", psl__GetELivetime},
                                    {"module_statistics_2", psl__GetModuleStatistics2},
                                    {"underflows", psl__GetUnderflows},
                                    {"overflows", psl__GetOverflows},
                                    {"mca_events", psl__GetMCAEvents},
                                    {"total_output_events", psl__GetTotalEvents}};

/* These are the allowed gain operations for this hardware */
static GainOperation gainOps[] = {
    {"calibrate", psl__GainCalibrate},
    {"adjust_percent_rule", psl__AdjustPercentRule},
};

#define PI 3.1415926535897932

/*
 * This routine takes a PSLFuncs structure and points the function pointers
 * in it at the local saturn "versions" of the functions.
 */
PSL_EXPORT int PSL_API saturn_PSLInit(PSLFuncs* funcs) {
    funcs->validateDefaults = pslValidateDefaults;
    funcs->validateModule = pslValidateModule;
    funcs->downloadFirmware = pslDownloadFirmware;
    funcs->setAcquisitionValues = pslSetAcquisitionValues;
    funcs->getAcquisitionValues = pslGetAcquisitionValues;
    funcs->gainOperation = pslGainOperation;
    funcs->gainCalibrate = pslGainCalibrate;
    funcs->startRun = pslStartRun;
    funcs->stopRun = pslStopRun;
    funcs->getRunData = pslGetRunData;
    funcs->doSpecialRun = pslDoSpecialRun;
    funcs->getSpecialRunData = pslGetSpecialRunData;
    funcs->getDefaultAlias = pslGetDefaultAlias;
    funcs->getParameter = pslGetParameter;
    funcs->setParameter = pslSetParameter;
    funcs->moduleSetup = pslModuleSetup;
    funcs->userSetup = pslUserSetup;
    funcs->getNumDefaults = pslGetNumDefaults;
    funcs->getNumParams = pslGetNumParams;
    funcs->getParamData = pslGetParamData;
    funcs->getParamName = pslGetParamName;
    funcs->boardOperation = pslBoardOperation;
    funcs->freeSCAs = pslDestroySCAs;
    funcs->unHook = pslUnHook;

    saturn_psl_md_alloc = utils->funcs->dxp_md_alloc;
    saturn_psl_md_free = utils->funcs->dxp_md_free;

    return XIA_SUCCESS;
}

/*
 * This routine validates module information specific to the dxpsaturn
 * product:
 *
 * 1) interface should be of type genericEPP or epp
 * 2) epp_address should be 0x278 or 0x378
 * 3) number_of_channels = 1
 */
PSL_STATIC int PSL_API pslValidateModule(Module* module) {
    if (!pslIsInterfaceValid(module)) {
        return XIA_MISSING_INTERFACE;
    }

    if (!pslIsEPPAddressValid(module)) {
        return XIA_MISSING_ADDRESS;
    }

    if (!pslIsNumChannelsValid(module)) {
        return XIA_INVALID_NUMCHANS;
    }

    return XIA_SUCCESS;
}

/*
 * This routine validates defaults information specific to the dxpsaturn
 * product.
 */
PSL_STATIC int PSL_API pslValidateDefaults(XiaDefaults* defaults) {
    int status;

    if (!pslAreAllDefaultsPresent(defaults)) {
        status = XIA_INCOMPLETE_DEFAULTS;
        sprintf(info_string, "Defaults with alias %s does not contain all defaults",
                defaults->alias);
        pslLogError("pslValidateDefaults", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Checks that the specified module is using a valid communication
 * interface for the Saturn/X10P product.
 */
PSL_STATIC boolean_t PSL_API pslIsInterfaceValid(Module* module) {
    boolean_t foundValidInterface = FALSE_;

    ASSERT(module != NULL);

#ifndef EXCLUDE_EPP
    foundValidInterface =
        (boolean_t) (foundValidInterface ||
                     (module->interface_info->type == XIA_EPP ||
                      module->interface_info->type == XIA_GENERIC_EPP));
#endif /* EXCLUDE_EPP */

#ifndef EXCLUDE_USB
    foundValidInterface =
        (boolean_t) (foundValidInterface || (module->interface_info->type == XIA_USB));
#endif /* EXCLUDE_USB */

#ifndef EXCLUDE_USB2
    foundValidInterface =
        (boolean_t) (foundValidInterface || (module->interface_info->type == XIA_USB2));
#endif /* EXCLUDE_USB2 */

    if (!foundValidInterface) {
        sprintf(info_string, "Invalid interface type = %u",
                module->interface_info->type);
        pslLogError("pslIsInterfaceValid", info_string, XIA_MISSING_INTERFACE);
        return FALSE_;
    }

    return TRUE_;
}

/*
 * Verify that the specified EPP address is valid
 *
 * This routine no longer checks any values since add-on EPP cards
 * use a different set of addresses then integrated EPP ports.
 */
PSL_STATIC boolean_t PSL_API pslIsEPPAddressValid(Module* module) {
    UNUSED(module);

    return TRUE_;
}

/*
 * This routine verfies that the number of channels is consistient with the
 * dxpx10p.
 */
PSL_STATIC boolean_t PSL_API pslIsNumChannelsValid(Module* module) {
    if (module->number_of_channels != 1) {
        return FALSE_;
    }

    return TRUE_;
}

/*
 * This routine checks that all of the defaults are present in the defaults
 * associated with this dxpx10p channel.
 */
PSL_STATIC boolean_t PSL_API pslAreAllDefaultsPresent(XiaDefaults* defaults) {
    boolean_t ptPresent = FALSE_;
    boolean_t trigPresent = FALSE_;
    boolean_t mcabinPresent = FALSE_;
    boolean_t nummcaPresent = FALSE_;
    boolean_t mcalowPresent = FALSE_;
    boolean_t enerPresent = FALSE_;
    boolean_t adcPresent = FALSE_;
    boolean_t energyPresent = FALSE_;
    boolean_t gaptimePresent = FALSE_;
    boolean_t triggerPTPresent = FALSE_;
    boolean_t triggerGapPresent = FALSE_;

    XiaDaqEntry* current = NULL;

    current = defaults->entry;
    while (current != NULL) {
        if (STREQ(current->name, "peaking_time")) {
            ptPresent = TRUE_;
        } else if (STREQ(current->name, "trigger_threshold")) {
            trigPresent = TRUE_;
        } else if (STREQ(current->name, "mca_bin_width")) {
            mcabinPresent = TRUE_;
        } else if (STREQ(current->name, "number_mca_channels")) {
            nummcaPresent = TRUE_;
        } else if (STREQ(current->name, "mca_low_limit")) {
            mcalowPresent = TRUE_;
        } else if (STREQ(current->name, "energy_threshold")) {
            enerPresent = TRUE_;
        } else if (STREQ(current->name, "adc_percent_rule")) {
            adcPresent = TRUE_;
        } else if (STREQ(current->name, "calibration_energy")) {
            energyPresent = TRUE_;
        } else if (STREQ(current->name, "gap_time")) {
            gaptimePresent = TRUE_;
        } else if (STREQ(current->name, "trigger_peaking_time")) {
            triggerPTPresent = TRUE_;
        } else if (STREQ(current->name, "trigger_gap_time")) {
            triggerGapPresent = TRUE_;
        }

        current = getListNext(current);
    }

    if ((ptPresent && trigPresent && mcabinPresent && nummcaPresent && mcalowPresent &&
         enerPresent && adcPresent && energyPresent && gaptimePresent &&
         triggerPTPresent && triggerGapPresent) != TRUE_) {
        return FALSE_;
    }

    return TRUE_;
}

/*
 * This routine handles downloading the requested kind of firmware through
 * XerXes.
 */
PSL_STATIC int PSL_API pslDownloadFirmware(int detChan, char* type, char* file,
                                           Module* m, char* rawFilename,
                                           XiaDefaults* defs) {
    int status;

    CurrentFirmware* currentFirmware = NULL;

    ASSERT(m != NULL);

    currentFirmware = m->currentFirmware;

    /* Immediately dismiss the types that aren't supported (currently) by the
     * X10P
     */
    if (STREQ(type, "user_fippi") || STREQ(type, "mmu")) {
        return XIA_NOSUPPORT_FIRM;
    }

    if (STREQ(type, "dsp")) {
        if (!STREQ(rawFilename, currentFirmware->currentDSP)) {
            status = dxp_replace_dspconfig(&detChan, file);

            if (status != DXP_SUCCESS) {
                sprintf(info_string, "Error changing to DSP '%s' for detChan %d", file,
                        detChan);
                pslLogError("pslDownloadFirmware", info_string, status);
                return status;
            }

            strcpy(currentFirmware->currentDSP, rawFilename);

            status = psl__UpdateMCAAddressCache(detChan, defs);

            if (status != XIA_SUCCESS) {
                sprintf(info_string,
                        "Error updating MCA start address cache for "
                        "detChan %d",
                        detChan);
                pslLogError("pslDownloadFirmware", info_string, status);
                return status;
            }
        }
    } else if (STREQ(type, "fippi")) {
        sprintf(info_string, "currentFirmware->currentFiPPI = %s",
                currentFirmware->currentFiPPI);
        pslLogDebug("pslDownloadFirmware", info_string);
        sprintf(info_string, "rawFilename = %s", rawFilename);
        pslLogDebug("pslDownloadFirmware", info_string);
        sprintf(info_string, "file = %s", file);
        pslLogDebug("pslDownloadFirmware", info_string);

        if (!STREQ(rawFilename, currentFirmware->currentFiPPI)) {
            status = dxp_replace_fpgaconfig(&detChan, "fippi", file);

            sprintf(info_string, "fippiFile = %s", rawFilename);
            pslLogDebug("pslDownloadFirmware", info_string);

            if (status != DXP_SUCCESS) {
                return status;
            }

            strcpy(currentFirmware->currentFiPPI, rawFilename);
        }
    } else {
        return XIA_UNKNOWN_FIRM;
    }

    return XIA_SUCCESS;
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
PSL_STATIC int pslSetAcquisitionValues(int detChan, char* name, void* value,
                                       XiaDefaults* defaults, FirmwareSet* firmwareSet,
                                       CurrentFirmware* currentFirmware,
                                       char* detectorType, Detector* detector,
                                       int detector_chan, Module* m, int modChan) {
    int status = XIA_SUCCESS;

    double presetType;

    UNUSED(modChan);
    UNUSED(currentFirmware);

    ASSERT(name != NULL);
    ASSERT(value != NULL);
    ASSERT(defaults != NULL);
    ASSERT(firmwareSet != NULL);
    ASSERT(detectorType != NULL);
    ASSERT(STRNEQ(detectorType, "RC") || STRNEQ(detectorType, "RESET"));
    ASSERT(detector != NULL);
    ASSERT(m != NULL);

    /* All of the calculations are dispatched to the appropriate routine. This
     * way, if the calculation ever needs to change, which it will, we don't
     * have to search in too many places to find the implementation.
     */
    if (STREQ(name, "peaking_time")) {
        status =
            pslDoPeakingTime(detChan, value, firmwareSet, detectorType, defaults,
                             detector->gain[detector_chan], m, detector, detector_chan);
    } else if (STREQ(name, "trigger_threshold")) {
        status = pslDoTriggerThreshold(detChan, value, firmwareSet, detectorType,
                                       defaults, detector->gain[detector_chan], m,
                                       detector, detector_chan);
    } else if (STREQ(name, "energy_threshold")) {
        status = pslDoEnergyThreshold(detChan, value, firmwareSet, detectorType,
                                      defaults, detector->gain[detector_chan], m,
                                      detector, detector_chan);
    } else if (STREQ(name, "number_mca_channels")) {
        status = pslDoNumMCAChannels(detChan, value, firmwareSet, detectorType,
                                     defaults, detector->gain[detector_chan], m,
                                     detector, detector_chan);
    } else if (STREQ(name, "mca_low_limit")) {
        status =
            pslDoMCALowLimit(detChan, value, firmwareSet, detectorType, defaults,
                             detector->gain[detector_chan], m, detector, detector_chan);
    } else if (STREQ(name, "mca_bin_width")) {
        status =
            pslDoMCABinWidth(detChan, value, firmwareSet, detectorType, defaults,
                             detector->gain[detector_chan], m, detector, detector_chan);
    } else if (STREQ(name, "adc_percent_rule")) {
        status = pslDoADCPercentRule(detChan, value, firmwareSet, detectorType,
                                     defaults, detector->gain[detector_chan], m,
                                     detector, detector_chan);
    } else if (STREQ(name, "enable_gate")) {
        status =
            pslDoEnableGate(detChan, value, firmwareSet, detectorType, defaults,
                            detector->gain[detector_chan], m, detector, detector_chan);
    } else if (STREQ(name, "enable_baseline_cut")) {
        status = pslDoEnableBaselineCut(detChan, value, firmwareSet, detectorType,
                                        defaults, detector->gain[detector_chan], m,
                                        detector, detector_chan);
    } else if (STREQ(name, "baseline_cut")) {
        status =
            pslDoBaselineCut(detChan, value, firmwareSet, detectorType, defaults,
                             detector->gain[detector_chan], m, detector, detector_chan);
    } else if (STREQ(name, "baseline_filter_length")) {
        status = pslDoBaselineFilterLength(detChan, value, firmwareSet, detectorType,
                                           defaults, detector->gain[detector_chan], m,
                                           detector, detector_chan);
    } else if (STREQ(name, "enable_interrupt")) {
        status = pslDoEnableInterrupt(detChan, value);
    } else if (STREQ(name, "calibration_energy")) {
        status = pslDoCalibrationEnergy(detChan, value, firmwareSet, detectorType,
                                        defaults, detector->gain[detector_chan], m,
                                        detector, detector_chan);
    } else if (STREQ(name, "gap_time")) {
        status =
            pslDoGapTime(detChan, value, firmwareSet, detectorType, defaults,
                         detector->gain[detector_chan], m, detector, detector_chan);
    } else if (STREQ(name, "reset_delay")) {
        status =
            pslDoResetDelay(detChan, value, firmwareSet, detectorType, defaults,
                            detector->gain[detector_chan], m, detector, detector_chan);
    } else if (STREQ(name, "decay_time")) {
        status =
            pslDoDecayTime(detChan, value, firmwareSet, detectorType, defaults,
                           detector->gain[detector_chan], m, detector, detector_chan);
    } else if (STREQ(name, "preamp_gain")) {
        status =
            pslDoPreampGain(detChan, value, firmwareSet, detectorType, defaults,
                            detector->gain[detector_chan], m, detector, detector_chan);
    } else if (STREQ(name, "detector_polarity")) {
        status =
            pslDoPolarity(detChan, value, firmwareSet, detectorType, defaults,
                          detector->gain[detector_chan], m, detector, detector_chan);
    } else if (STREQ(name, "trigger_peaking_time")) {
        status = pslDoTriggerPeakingTime(detChan, value, firmwareSet, detectorType,
                                         defaults, detector->gain[detector_chan], m,
                                         detector, detector_chan);
    } else if (STREQ(name, "trigger_gap_time")) {
        status = pslDoTriggerGapTime(detChan, value, firmwareSet, detectorType,
                                     defaults, detector->gain[detector_chan], m,
                                     detector, detector_chan);
    } else if (STREQ(name, "preset_type")) {
        status = psl__SetPresetType(detChan, value, firmwareSet, detectorType, defaults,
                                    detector->gain[detector_chan], m, detector,
                                    detector_chan);
    } else if (STREQ(name, "preset_value")) {
        status = psl__SetPresetValue(detChan, value, firmwareSet, detectorType,
                                     defaults, detector->gain[detector_chan], m,
                                     detector, detector_chan);
    } else if (STREQ(name, "preset_standard")) {
        presetType = XIA_PRESET_NONE;
        status = psl__SetPresetType(
            detChan, (void*) &presetType, firmwareSet, detectorType, defaults,
            detector->gain[detector_chan], m, detector, detector_chan);

        if (status != XIA_SUCCESS) {
            sprintf(info_string,
                    "Error setting preset type to standard run for "
                    "detChan %d",
                    detChan);
            pslLogError("pslSetAcquisitionValues", info_string, status);
            return status;
        }
    } else if (STREQ(name, "preset_runtime")) {
        presetType = XIA_PRESET_FIXED_REAL;
        status = psl__SetPresetType(
            detChan, (void*) &presetType, firmwareSet, detectorType, defaults,
            detector->gain[detector_chan], m, detector, detector_chan);

        if (status != XIA_SUCCESS) {
            sprintf(info_string,
                    "Error setting preset type to realtime run for "
                    "detChan %d",
                    detChan);
            pslLogError("pslSetAcquisitionValues", info_string, status);
            return status;
        }

        status = psl__SetPresetValue(detChan, value, firmwareSet, detectorType,
                                     defaults, detector->gain[detector_chan], m,
                                     detector, detector_chan);

        if (status != XIA_SUCCESS) {
            sprintf(info_string,
                    "Error setting preset run length for "
                    "detChan %d",
                    detChan);
            pslLogError("pslSetAcquisitionValues", info_string, status);
            return status;
        }
    } else if (STREQ(name, "preset_livetime")) {
        presetType = XIA_PRESET_FIXED_LIVE;
        status = psl__SetPresetType(
            detChan, (void*) &presetType, firmwareSet, detectorType, defaults,
            detector->gain[detector_chan], m, detector, detector_chan);

        if (status != XIA_SUCCESS) {
            sprintf(info_string,
                    "Error setting preset type to livetime run for "
                    "detChan %d",
                    detChan);
            pslLogError("pslSetAcquisitionValues", info_string, status);
            return status;
        }

        status = psl__SetPresetValue(detChan, value, firmwareSet, detectorType,
                                     defaults, detector->gain[detector_chan], m,
                                     detector, detector_chan);

        if (status != XIA_SUCCESS) {
            sprintf(info_string,
                    "Error setting preset run length for "
                    "detChan %d",
                    detChan);
            pslLogError("pslSetAcquisitionValues", info_string, status);
            return status;
        }
    } else if (STREQ(name, "preset_output")) {
        presetType = XIA_PRESET_FIXED_EVENTS;
        status = psl__SetPresetType(
            detChan, (void*) &presetType, firmwareSet, detectorType, defaults,
            detector->gain[detector_chan], m, detector, detector_chan);

        if (status != XIA_SUCCESS) {
            sprintf(info_string,
                    "Error setting preset type to output events run "
                    "for detChan %d",
                    detChan);
            pslLogError("pslSetAcquisitionValues", info_string, status);
            return status;
        }

        status = psl__SetPresetValue(detChan, value, firmwareSet, detectorType,
                                     defaults, detector->gain[detector_chan], m,
                                     detector, detector_chan);

        if (status != XIA_SUCCESS) {
            sprintf(info_string,
                    "Error setting preset run length for "
                    "detChan %d",
                    detChan);
            pslLogError("pslSetAcquisitionValues", info_string, status);
            return status;
        }
    } else if (STREQ(name, "preset_input")) {
        presetType = XIA_PRESET_FIXED_TRIGGERS;
        status = psl__SetPresetType(
            detChan, (void*) &presetType, firmwareSet, detectorType, defaults,
            detector->gain[detector_chan], m, detector, detector_chan);

        if (status != XIA_SUCCESS) {
            sprintf(info_string,
                    "Error setting preset type to input events run "
                    "for detChan %d",
                    detChan);
            pslLogError("pslSetAcquisitionValues", info_string, status);
            return status;
        }

        status = psl__SetPresetValue(detChan, value, firmwareSet, detectorType,
                                     defaults, detector->gain[detector_chan], m,
                                     detector, detector_chan);

        if (status != XIA_SUCCESS) {
            sprintf(info_string,
                    "Error setting preset run length for "
                    "detChan %d",
                    detChan);
            pslLogError("pslSetAcquisitionValues", info_string, status);
            return status;
        }
    } else if ((strncmp(name, "peakint", 7) == 0) ||
               (strncmp(name, "peaksam", 7) == 0)) {
        status = pslDoFilter(detChan, name, value, defaults, firmwareSet,
                             detector->gain[detector_chan], m);
    } else if (STRNEQ(name, "number_of_scas")) {
        status = _pslDoNSca(detChan, name, value, m, defaults);
    } else if (STRNEQ(name, "sca")) {
        status = _pslDoSCA(detChan, name, value, m, defaults);
    } else if (pslIsUpperCase(name)) {
        status = pslDoParam(detChan, name, value, defaults);
    } else if (STREQ(name, "actual_gap_time") || STREQ(name, "mca_start_address")) {
        /* Do Nothing, these are read-only acquisition values */
        sprintf(info_string,
                "Attempted to set a read-only acquisition value: "
                "'%s'",
                name);
        pslLogWarning("pslSetAcquisitionValues", info_string);
    } else if (STREQ(name, "baseline_threshold")) {
        status =
            psl__SetBThresh(detChan, value, firmwareSet, detectorType, defaults,
                            detector->gain[detector_chan], m, detector, detector_chan);
    } else if (STREQ(name, "maxwidth")) {
        status =
            psl__SetMaxWidth(detChan, value, firmwareSet, detectorType, defaults,
                             detector->gain[detector_chan], m, detector, detector_chan);
    } else if (STREQ(name, "minimum_gap_time")) {
        status = psl__SetMinGapTime(detChan, value, firmwareSet, detectorType, defaults,
                                    detector->gain[detector_chan], m, detector,
                                    detector_chan);
    } else {
        status = XIA_UNKNOWN_VALUE;
    }

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting '%s' to %0.3f for detchan %d.", name,
                *((double*) value), detChan);
        pslLogError("pslSetAcquisitionValues", info_string, status);
        return status;
    }

    status = dxp_upload_dspparams(&detChan);

    if (status != DXP_SUCCESS) {
        pslLogError("pslSetAcquisitionValues", "Error uploading params through Xerxes",
                    status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This routine does all of the steps required in modifying the peaking time
 * for a given X10P detChan:
 *
 * 1) Change FiPPI if necessary
 * 2) Update Filter Parameters
 * 3) Return "calculated" Peaking Time
 */
ACQUISITION_VALUE(PeakingTime) {
    int status;

    double peakingTime;

    char* tmpPath = NULL;

    char fippi[MAX_PATH_LEN];
    char rawFilename[MAXFILENAME_LEN];

    Firmware* firmware = NULL;

    UNUSED(det);
    UNUSED(detector_chan);

    peakingTime = *((double*) value);

    /* The code below is replacing an old algorithm that used to check the decimation
     * instead of the filename to determine if firmware needs to be downloaded or not.
     *
     * All of the comparison code is going to be in pslDownloadFirmware().
     */
    if (fs->filename == NULL) {
        firmware = fs->firmware;
        while (firmware != NULL) {
            if ((peakingTime >= firmware->min_ptime) &&
                (peakingTime <= firmware->max_ptime)) {
                strcpy(fippi, firmware->fippi);
                strcpy(rawFilename, firmware->fippi);
                break;
            }

            firmware = getListNext(firmware);
        }
    } else {
        /* filename is actually defined in this case */
        sprintf(info_string, "peakingTime = %.3f", peakingTime);
        pslLogDebug("pslDoPeakingTime", info_string);

        if (fs->tmpPath) {
            tmpPath = fs->tmpPath;
        } else {
            tmpPath = utils->funcs->dxp_md_tmp_path();
        }

        status = xiaFddGetFirmware(fs->filename, tmpPath, "fippi", peakingTime,
                                   (unsigned short) fs->numKeywords, fs->keywords,
                                   detectorType, fippi, rawFilename);

        if (status != XIA_SUCCESS) {
            sprintf(info_string,
                    "Error getting FiPPI file from %s at peaking time = %.3f",
                    fs->filename, peakingTime);
            pslLogError("pslDoPeakingTime", info_string, status);
            return status;
        }
    }

    status = pslDownloadFirmware(detChan, "fippi", fippi, m, rawFilename, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error downloading FiPPI to detChan %d", detChan);
        pslLogError("pslDoPeakingTime", info_string, status);
        return status;
    }

    pslLogDebug("pslDoPeakingTime", "Preparing to call pslUpdateFilter()");

    status = pslUpdateFilter(detChan, peakingTime, defs, fs, preampGain, m);
    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error updating filter information for detChan %d",
                detChan);
        pslLogError("pslDoPeakingTime", info_string, status);
        return status;
    }

    status = pslGetDefault("peaking_time", value, defs);
    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting peaking_time for detChan %d", detChan);
        pslLogError("pslDoPeakingTime", info_string, status);
        return status;
    }

    sprintf(info_string, "New peaking time = %.3f", peakingTime);
    pslLogDebug("pslDoPeakingTime", info_string);

    return XIA_SUCCESS;
}

/*
 * This routine sets the Trigger Threshold parameter on the DSP code based on
 * calculations from values in the defaults (when required) or those that
 * have been passed in.
 */
ACQUISITION_VALUE(TriggerThreshold) {
    int status;

    double clock_speed;
    double eVPerADC = 0.0;
    double adcPercentRule = 0.0;
    double dTHRESHOLD = 0.0;
    double thresholdEV = 0.0;
    double calibEV = 0.0;
    double triggerPeakingTime = 0.0;

    parameter_t FASTLEN;
    parameter_t THRESHOLD;

    UNUSED(fs);
    UNUSED(detectorType);
    UNUSED(preampGain);
    UNUSED(m);
    UNUSED(detector_chan);
    UNUSED(det);

    status = pslGetClockSpeed(detChan, &clock_speed);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting clock speed for detChan %d", detChan);
        pslLogError("pslDoTriggerThreshold", info_string, status);
        return status;
    }

    status = pslGetDefault("adc_percent_rule", (void*) &adcPercentRule, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting adc_percent_rule from detChan %d", detChan);
        pslLogError("pslDoTriggerThreshold", info_string, status);
        return status;
    }

    status = pslGetDefault("calibration_energy", (void*) &calibEV, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting calibration_energy from detChan %d",
                detChan);
        pslLogError("pslDoTriggerThreshold", info_string, status);
        return status;
    }

    eVPerADC = (double) (calibEV / ((adcPercentRule / 100.0) * NUM_BITS_ADC));

    /* We need to retrieve the FASTLEN from the default values, NOT from the DSP.  If
     * we get from the board, then we may have a value for FASTLEN that is not intended
     * to work with the desired trigger threshold as set in the defaults...
     */
    status = pslGetDefault("trigger_peaking_time", (void*) &triggerPeakingTime, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting trigger_peaking_time from detChan %d",
                detChan);
        pslLogError("pslDoTriggerThreshold", info_string, status);
        return status;
    }

    /* Calculate the FASTLEN */
    FASTLEN = (parameter_t) ROUND(triggerPeakingTime * clock_speed);

    thresholdEV = *((double*) value);
    dTHRESHOLD = ((double) FASTLEN * thresholdEV) / eVPerADC;
    THRESHOLD = (parameter_t) ROUND(dTHRESHOLD);

    /* The actual range to use is 0 < THRESHOLD < 255, but since THRESHOLD
     * is an unsigned short, any "negative" errors should show up as
     * sign extension problems and will be caught by THRESHOLD > 255.
     */
    if (THRESHOLD > 255) {
        return XIA_THRESH_OOR;
    }

    status = dxp_set_one_dspsymbol(&detChan, "THRESHOLD", &THRESHOLD);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error getting THRESHOLD from detChan %d", detChan);
        pslLogError("pslDoTriggerThreshold", info_string, status);
        return status;
    }

    /* Re-"calculate" the actual threshold. This _is_ a deterministic process since the
     * specified value of the threshold is only modified due to rounding...
     */
    thresholdEV = ((double) THRESHOLD * eVPerADC) / ((double) FASTLEN);
    *((double*) value) = thresholdEV;

    status = pslSetDefault("trigger_threshold", (void*) &thresholdEV, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting trigger_threshold for detChan %d", detChan);
        pslLogError("pslDoTriggerThreshold", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This routine translates the Energy Threshold value (in eV) into the
 * appropriate DSP parameter.
 */
ACQUISITION_VALUE(EnergyThreshold) {
    int status;

    double eVPerADC = 0.0;
    double adcPercentRule = 0.0;
    double dSLOWTHRESH = 0.0;
    double slowthreshEV = 0.0;
    double calibEV = 0.0;

    parameter_t SLOWLEN;
    parameter_t SLOWTHRESH;

    UNUSED(fs);
    UNUSED(detectorType);
    UNUSED(preampGain);
    UNUSED(m);
    UNUSED(detector_chan);
    UNUSED(det);

    status = pslGetDefault("adc_percent_rule", (void*) &adcPercentRule, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting adc_percent_rule from detChan %d", detChan);
        pslLogError("pslDoEnergyThreshold", info_string, status);
        return status;
    }

    status = pslGetDefault("calibration_energy", (void*) &calibEV, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting calibration_energy from detChan %d",
                detChan);
        pslLogError("pslDoEnergyThreshold", info_string, status);
        return status;
    }

    eVPerADC = (double) (calibEV / ((adcPercentRule / 100.0) * NUM_BITS_ADC));

    status = dxp_get_one_dspsymbol(&detChan, "SLOWLEN", &SLOWLEN);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error getting SLOWLEN from detChan %d", detChan);
        pslLogError("pslDoEnergyThreshold", info_string, status);
        return status;
    }

    slowthreshEV = *((double*) value);
    dSLOWTHRESH = ((double) SLOWLEN * slowthreshEV) / eVPerADC;
    SLOWTHRESH = (parameter_t) ROUND(dSLOWTHRESH);

    status = dxp_set_one_dspsymbol(&detChan, "SLOWTHRESH", &SLOWTHRESH);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error getting SLOWTHRESH from detChan %d", detChan);
        pslLogError("pslDoEnergyThreshold", info_string, status);
        return status;
    }

    /* Re-"calculate" the actual threshold. This _is_ a deterministic process since the
     * specified value of the threshold is only modified due to rounding...
     */
    slowthreshEV = ((double) SLOWTHRESH * eVPerADC) / ((double) SLOWLEN);
    *((double*) value) = slowthreshEV;

    status = pslSetDefault("energy_threshold", (void*) &slowthreshEV, defs);
    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting energy_threshold for detChan %d", detChan);
        pslLogError("pslDoEnergyThreshold", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This routine essentially sets the values of MCALIMHI.
 */
ACQUISITION_VALUE(NumMCAChannels) {
    int status;

    double numMCAChans;

    parameter_t MCALIMLO;
    parameter_t MCALIMHI;

    unsigned short paramIndex;
    unsigned short numParams;
    unsigned short* paramData;
    unsigned short lowLimit;
    unsigned short highLimit;

    UNUSED(detector_chan);
    UNUSED(det);
    UNUSED(m);
    UNUSED(preampGain);
    UNUSED(detectorType);
    UNUSED(fs);

    numMCAChans = *((double*) value);

    /* determine what the allowed limits are for the MCA bins. */
    /* Retrieve the index of MCALIMHI */
    status = dxp_get_symbol_index(&detChan, "MCALIMHI", &paramIndex);
    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Unable to retrieve the index for MCALIMHI for detChan %d",
                detChan);
        pslLogError("pslDoNumMCAChannels", info_string, status);
        return status;
    }

    /* How many parameters with this DSP code? */
    status = pslGetNumParams(detChan, &numParams);
    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Unable to determine the number of DSP parameters for detChan %d",
                detChan);
        pslLogError("pslDoNumMCAChannels", info_string, status);
        return status;
    }

    /* Allocate memory for the parameter data */
    paramData =
        (unsigned short*) saturn_psl_md_alloc(sizeof(unsigned short) * numParams);

    /* Retrieve the upper limits */
    status = pslGetParamData(detChan, "upper_bounds", (void*) paramData);
    if (status != XIA_SUCCESS) {
        saturn_psl_md_free(paramData);
        sprintf(
            info_string,
            "Unable to retrieve the upper limits for the DSP parameters for detChan %d",
            detChan);
        pslLogError("pslDoNumMCAChannels", info_string, status);
        return status;
    }

    /* store the high limit for MCALIMHI */
    highLimit = paramData[paramIndex];

    /* Retrieve the lower limits */
    status = pslGetParamData(detChan, "lower_bounds", (void*) paramData);
    if (status != XIA_SUCCESS) {
        saturn_psl_md_free(paramData);
        sprintf(
            info_string,
            "Unable to retrieve the upper limits for the DSP parameters for detChan %d",
            detChan);
        pslLogError("pslDoNumMCAChannels", info_string, status);
        return status;
    }

    /* store the low limit for MCALIMHI */
    lowLimit = paramData[paramIndex];

    /* Free up the memory */
    saturn_psl_md_free(paramData);

    /* do some bounds checking */
    if ((numMCAChans < lowLimit) || (numMCAChans > highLimit)) {
        status = XIA_BINS_OOR;
        sprintf(info_string, "Too many or too few bins specified: %.3f, limits: %u:%u",
                numMCAChans, lowLimit, highLimit);
        pslLogError("pslDoNumMCAChannels", info_string, status);
        return status;
    }

    status = dxp_get_one_dspsymbol(&detChan, "MCALIMLO", &MCALIMLO);
    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error getting MCALIMLO from detChan %d", detChan);
        pslLogError("pslDoNumMCAChannels", info_string, status);
        return status;
    }

    /* Adjust the HI limit by the setting of the low limit */
    MCALIMHI = (parameter_t) ROUND(numMCAChans + MCALIMLO);

    /* Need to do another range check here. There is a little ambiguity in the
     * calculation if the user chooses to run with MCALIMLO not set to zero.
     */
    if (MCALIMHI > highLimit) {
        status = XIA_BINS_OOR;
        sprintf(info_string,
                "Maximum bin # is out-of-range: MCALIMHI = %u, MCALIMLO = %u", MCALIMHI,
                MCALIMLO);
        pslLogError("pslDoNumMCAChannels", info_string, status);
        return status;
    }

    /* Write the MCALIMHI parameter to the DSP */
    status = dxp_set_one_dspsymbol(&detChan, "MCALIMHI", &MCALIMHI);
    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error setting MCALIMHI for detChan %d", detChan);
        pslLogError("pslDoNumMCAChannels", info_string, status);
        return status;
    }

    /* Store the number of channels in the defaults list */
    status = pslSetDefault("number_mca_channels", (void*) &numMCAChans, defs);
    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting number_mca_channels for detChan %d",
                detChan);
        pslLogError("pslDoNumMCAChannels", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This routine sets the low bin for acquisition.
 */
ACQUISITION_VALUE(MCALowLimit) {
    int status;

    double dMCALIMLO;
    double eVPerBin = 0.0;
    double lowLimitEV;

    parameter_t MCALIMLO;
    parameter_t MCALIMHI;

    UNUSED(fs);
    UNUSED(detectorType);
    UNUSED(preampGain);
    UNUSED(m);
    UNUSED(detector_chan);
    UNUSED(det);

    status = pslGetDefault("mca_bin_width", (void*) &eVPerBin, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting mca_bin_width for detChan %d", detChan);
        pslLogError("pslDoMCALowLimit", info_string, status);
        return status;
    }

    lowLimitEV = *((double*) value);
    dMCALIMLO = (double) (lowLimitEV / eVPerBin);
    MCALIMLO = (parameter_t) ROUND(dMCALIMLO);

    /* Compare MCALIMLO to MCALIMHI - 1 */
    status = dxp_get_one_dspsymbol(&detChan, "MCALIMHI", &MCALIMHI);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error getting MCALIMHI from detChan %d", detChan);
        pslLogError("pslDoMCALowLimit", info_string, status);
        return status;
    }

    if (MCALIMLO > (MCALIMHI - 1)) {
        status = XIA_BINS_OOR;
        sprintf(info_string, "Low MCA channel is specified too high: %u", MCALIMLO);
        pslLogError("pslDoMCALowLimit", info_string, status);
        return status;
    }

    status = dxp_set_one_dspsymbol(&detChan, "MCALIMLO", &MCALIMLO);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error setting MCALIMLO for detChan %d", detChan);
        pslLogError("pslDoMCALowLimit", info_string, status);
        return status;
    }

    status = pslSetDefault("mca_low_limit", (void*) &dMCALIMLO, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting mca_low_limit for detChan %d", detChan);
        pslLogError("pslDoMCALowLimit", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This routine sets the bin width, through the parameter BINFACT.
 */
PSL_STATIC int pslDoMCABinWidth(int detChan, void* value, FirmwareSet* fs,
                                char* detType, XiaDefaults* defaults, double preampGain,
                                Module* m, Detector* det, int detector_chan) {
    int status;

    UNUSED(fs);
    UNUSED(detType);
    UNUSED(detector_chan);
    UNUSED(det);

    status = pslSetDefault("mca_bin_width", value, defaults);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting mca_bin_width for detChan %d", detChan);
        pslLogError("pslDoMCABinWidth", info_string, status);
        return status;
    }

    status = pslDoGainSetting(detChan, defaults, preampGain, m);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting the Gain for detChan %d", detChan);
        pslLogError("pslDoMCABinWidth", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This performs the gain calculation, retrieving all the values from
 * the defaults.  Mainly a convenience routine.
 */
PSL_STATIC int pslDoGainSetting(int detChan, XiaDefaults* defaults, double preampGain,
                                Module* m) {
    int status;

    double adcPercentRule = 0.0;
    double calibEV = 0.0;
    double mcaBinWidth = 0.0;
    double threshold = 0.0;
    double slowthresh = 0.0;

    parameter_t GAINDAC;
    parameter_t SLOWLEN;

    status = pslGetDefault("adc_percent_rule", (void*) &adcPercentRule, defaults);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting adc_percent_rule for detChan %d", detChan);
        pslLogError("pslDoGainSetting", info_string, status);
        return status;
    }

    status = pslGetDefault("calibration_energy", (void*) &calibEV, defaults);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting calibration_energy for detChan %d",
                detChan);
        pslLogError("pslDoGainSetting", info_string, status);
        return status;
    }

    status = pslGetDefault("mca_bin_width", (void*) &mcaBinWidth, defaults);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting mca_bin_width for detChan %d", detChan);
        pslLogError("pslDoGainSetting", info_string, status);
        return status;
    }

    status = dxp_get_one_dspsymbol(&detChan, "SLOWLEN", &SLOWLEN);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error getting SLOWLEN from detChan %d", detChan);
        pslLogError("pslDoGainSetting", info_string, status);
        return status;
    }

    /* Calculate and set the gain */
    status = pslCalculateGain(detChan, adcPercentRule, calibEV, preampGain, mcaBinWidth,
                              SLOWLEN, &GAINDAC);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error calculating gain for detChan %d", detChan);
        pslLogError("pslDoGainSetting", info_string, status);
        return status;
    }

    status = dxp_set_one_dspsymbol(&detChan, "GAINDAC", &GAINDAC);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error setting GAINDAC for detChan %d", detChan);
        pslLogError("pslDoGainSetting", info_string, status);
        return status;
    }

    /* Call routines that depend on changes in the gain so that
     * their calculations will now be correct.
     */

    /* Use the "old" settings to recalculate the trigger threshold, slow
    * threshold and MCA bin width.
     */
    status = pslGetDefault("trigger_threshold", (void*) &threshold, defaults);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting mca_bin_width for detChan %d", detChan);
        pslLogError("pslDoGainSetting", info_string, status);
        return status;
    }

    status = pslDoTriggerThreshold(detChan, (void*) &threshold, NULL, NULL, defaults,
                                   preampGain, m, NULL, 0);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error updating trigger threshold for detChan %d",
                detChan);
        pslLogError("pslDoGainSetting", info_string, status);
        return status;
    }

    status = pslGetDefault("energy_threshold", (void*) &slowthresh, defaults);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting mca_bin_width for detChan %d", detChan);
        pslLogError("pslDoGainSetting", info_string, status);
        return status;
    }

    status = pslDoEnergyThreshold(detChan, (void*) &slowthresh, NULL, NULL, defaults,
                                  preampGain, m, NULL, 0);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error updating energy threshold for detChan %d", detChan);
        pslLogError("pslDoGainSetting", info_string, status);
        return status;
    }

    /* Calculate and set the gain (again). It's done twice, for now, to
     * protect against the case where we change all of these other parameters
     * and then find out the gain is out-of-range and, therefore, the whole
     * system is out-of-sync.
     */
    status = pslCalculateGain(detChan, adcPercentRule, calibEV, preampGain, mcaBinWidth,
                              SLOWLEN, &GAINDAC);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error calculating gain for detChan %d", detChan);
        pslLogError("pslDoGainSetting", info_string, status);
        return status;
    }

    status = dxp_set_one_dspsymbol(&detChan, "GAINDAC", &GAINDAC);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error setting GAINDAC for detChan %d", detChan);
        pslLogError("pslDoGainSetting", info_string, status);
        return status;
    }

    /* Start and stop a run to "set" the GAINDAC.
     * Reference: BUG ID #83
     */
    status = pslQuickRun(detChan, defaults, m);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error performing a quick run to set GAINDAC on detChan %d", detChan);
        pslLogError("pslDoGainSetting", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Actually changes the value of the defaults setting AND recalculates the
 * parameters that depend on the percent rule. This is a little different
 * then if you just modified the percent rule through the defaults, since
 * that doesn't recalculate any of the other acquisition parameters.
 */
ACQUISITION_VALUE(ADCPercentRule) {
    int status;

    UNUSED(detector_chan);
    UNUSED(det);
    UNUSED(detectorType);
    UNUSED(fs);

    status = pslSetDefault("adc_percent_rule", value, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting adc_percent_rule for detChan %d", detChan);
        pslLogError("pslDoADCPercentRule", info_string, status);
        return status;
    }

    status = pslDoGainSetting(detChan, defs, preampGain, m);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting the Gain for detChan %d", detChan);
        pslLogError("pslDoADCPercentRule", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This routine checks to see if the "enable gate" default is defined. If it
 * isn't then it just ignores the fact that this function is called. If this
 * routine was really cool then it would add that as a default, but it can't
 * so it won't.
 */
ACQUISITION_VALUE(EnableGate) {
    int status;

    UNUSED(detChan);
    UNUSED(detector_chan);
    UNUSED(det);
    UNUSED(m);
    UNUSED(preampGain);
    UNUSED(detectorType);
    UNUSED(fs);

    /* Set the enable_gate entry */
    status = pslSetDefault("enable_gate", value, defs);
    if (status != XIA_SUCCESS) {
        pslLogError("pslDoEnableGate", "Error setting enable_gate in the defaults",
                    status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This routine sets the reset delay for the X10P, this is also a detector
 * parameter.
 */
ACQUISITION_VALUE(ResetDelay) {
    int status;

    parameter_t RESETINT;

    UNUSED(m);
    UNUSED(preampGain);
    UNUSED(detectorType);
    UNUSED(fs);

    /* Only set the DSP parameter if the detector type is correct */
    if (det->type == XIA_DET_RESET) {
        /* DSP stores the reset interval in 0.25us ticks */
        RESETINT = (parameter_t) ROUND(4.0 * *((double*) value));

        /* Quick bounds check on the RESETINT */
        if (RESETINT > 16383) {
            RESETINT = 16383;
        }

        /* Write the new delay time to the DSP */
        status = dxp_set_one_dspsymbol(&detChan, "RESETINT", &RESETINT);

        if (status != DXP_SUCCESS) {
            sprintf(info_string, "Error setting RESETINT for detChan %d", detChan);
            pslLogError("pslDoResetDelay", info_string, status);
            return status;
        }

        /* Reset the value to a legal increment */
        *((double*) value) = ((double) RESETINT) * 0.25;

        /* Modify the detector structure with the new delay time */
        det->typeValue[detector_chan] = *((double*) value);
    }

    /* Set the reset_delay entry */
    status = pslSetDefault("reset_delay", value, defs);
    if (status != XIA_SUCCESS) {
        pslLogError("pslDoResetDelay", "Error setting reset_delay in the defaults",
                    status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This routine sets the decay time for the X10P, this is also a detector
 * parameter.
 */
ACQUISITION_VALUE(DecayTime) {
    int status;

    double decayTime;

    parameter_t RCTAU;
    parameter_t RCTAUFRAC;

    UNUSED(m);
    UNUSED(preampGain);
    UNUSED(detectorType);
    UNUSED(fs);

    /* Only set the DSP parameter if the detector type is correct */
    if (det->type == XIA_DET_RCFEED) {
        decayTime = *((double*) value);
        /* DSP stores the decay time as microsecond part and fractional part */
        RCTAU = (parameter_t) floor(decayTime);
        RCTAUFRAC = (parameter_t) ROUND((decayTime - (double) RCTAU) * 65536);

        /* Write the new delay time to the DSP */
        status = dxp_set_one_dspsymbol(&detChan, "RCTAU", &RCTAU);

        if (status != DXP_SUCCESS) {
            sprintf(info_string, "Error setting RCTAU for detChan %d", detChan);
            pslLogError("pslDoDecayTime", info_string, status);
            return status;
        }

        /* Write the new delay time to the DSP */
        status = dxp_set_one_dspsymbol(&detChan, "RCTAUFRAC", &RCTAUFRAC);

        if (status != DXP_SUCCESS) {
            sprintf(info_string, "Error setting RCTAUFRAC for detChan %d", detChan);
            pslLogError("pslDoDecayTime", info_string, status);
            return status;
        }

        /* Modify the detector structure with the new delay time */
        det->typeValue[detector_chan] = *((double*) value);
    }

    /* Set the decay_time entry */
    status = pslSetDefault("decay_time", value, defs);
    if (status != XIA_SUCCESS) {
        pslLogError("pslDoDecayTime", "Error setting decay_time in the defaults",
                    status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This routine sets the preamp gain in mV/keV for the saturn, this is also a detector
 * parameter.
 */
ACQUISITION_VALUE(PreampGain) {
    int status;

    double newGain = 0.0;

    UNUSED(preampGain);
    UNUSED(detectorType);
    UNUSED(fs);

    newGain = *((double*) value);

    /* set the gain with the new preampGain */
    status = pslDoGainSetting(detChan, defs, newGain, m);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting the Gain for detChan %d", detChan);
        pslLogError("pslDoPreampGain", info_string, status);
        return status;
    }

    /* Modify the detector structure with the new preamp gain */
    det->gain[detector_chan] = *((double*) value);

    /* Set the preamp_gain entry */
    status = pslSetDefault("preamp_gain", value, defs);
    if (status != XIA_SUCCESS) {
        pslLogError("pslDoPreampGain", "Error setting preamp_gain in the defaults",
                    status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This routine sets the polarity for the X10P, this is also a detector
 * parameter.
 */
ACQUISITION_VALUE(Polarity) {
    int status;

    double polarity;

    parameter_t POLARITY;

    UNUSED(preampGain);
    UNUSED(detectorType);
    UNUSED(fs);

    polarity = *((double*) value);
    POLARITY = (parameter_t) polarity;

    status = dxp_set_one_dspsymbol(&detChan, "POLARITY", &POLARITY);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error setting POLARITY for detChan %d", detChan);
        pslLogError("pslDoPolarity", info_string, status);
        return status;
    }

    /* Start and stop a run to "set" the polarity value.
     * Reference: BUG ID #17, #84
     */
    status = pslQuickRun(detChan, defs, m);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error performing a quick run to set POLARITY on detChan %d", detChan);
        pslLogError("pslDoPolarity", info_string, status);
        return status;
    }

    /* Set the entry in the detector structure for polarity */
    det->polarity[detector_chan] = (unsigned short) polarity;

    /* Set the polarity entry in the acquisition value list */
    status = pslSetDefault("detector_polarity", (void*) &polarity, defs);
    if (status != XIA_SUCCESS) {
        pslLogError("pslDoPolarity", "Error setting detector_polarity in the defaults",
                    status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This routine sets/resets the bit of RUNTASKS that controls the baseline cut
 * operation in the DSP.
 */
ACQUISITION_VALUE(EnableBaselineCut) {
    int status;

    parameter_t RUNTASKS;

    UNUSED(detector_chan);
    UNUSED(det);
    UNUSED(m);
    UNUSED(preampGain);
    UNUSED(detectorType);
    UNUSED(fs);

    /* Set the proper bit of the RUNTASKS DSP parameter */
    /* First retrieve RUNTASKS from the DSP */
    status = dxp_get_one_dspsymbol(&detChan, "RUNTASKS", &RUNTASKS);
    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error getting RUNTASKS for detChan %d", detChan);
        pslLogError("pslDoEnableBaselineCut", info_string, status);
        return status;
    }

    /* Set/reset the bit (masks are in saturn.h) */
    if (*((double*) value) == 1.0) {
        RUNTASKS |= BASELINE_CUT;
    } else {
        RUNTASKS &= ~BASELINE_CUT;
    }

    /* Finally write RUNTASKS back to the DSP */
    status = dxp_set_one_dspsymbol(&detChan, "RUNTASKS", &RUNTASKS);
    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error writing RUNTASKS for detChan %d", detChan);
        pslLogError("pslDoEnableBaselineCut", info_string, status);
        return status;
    }

    /* Set the enable_baseline_cut entry */
    status = pslSetDefault("enable_baseline_cut", value, defs);
    if (status != XIA_SUCCESS) {
        pslLogError("pslDoEnableBaselineCut",
                    "Error setting enable_baseline_cut in the defaults", status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This routine sets the value of the Baseline Cut in percent
 */
ACQUISITION_VALUE(BaselineCut) {
    int status;

    parameter_t BLCUT;

    double blcut;

    UNUSED(detector_chan);
    UNUSED(det);
    UNUSED(m);
    UNUSED(preampGain);
    UNUSED(detectorType);
    UNUSED(fs);

    /* Calculate the value for the baseline cut in 1.15 notation.  The value stored
     * as the acquisition value is in percent.
     */
    blcut = *((double*) value);
    BLCUT = (parameter_t) ROUND(32768. * blcut / 100.);

    /* Write BLCUT to the DSP */
    status = dxp_set_one_dspsymbol(&detChan, "BLCUT", &BLCUT);
    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error writing BLCUT for detChan %d", detChan);
        pslLogError("pslDoBaselineCut", info_string, status);
        return status;
    }

    /* Set the baseline_cut entry */
    status = pslSetDefault("baseline_cut", value, defs);
    if (status != XIA_SUCCESS) {
        pslLogError("pslDoBaselineCut", "Error setting baseline_cut in the defaults",
                    status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This routine sets the value of the Baseline Filter Length
 */
ACQUISITION_VALUE(BaselineFilterLength) {
    int status;

    parameter_t BLFILTER;

    double blfilter;

    UNUSED(detector_chan);
    UNUSED(det);
    UNUSED(m);
    UNUSED(preampGain);
    UNUSED(detectorType);
    UNUSED(fs);

    /* Calculate the value for 1/(baseline filter length)  in 1.15 notation.  The value stored
     * as the acquisition value is just samples.
     */
    blfilter = *((double*) value);
    /* sanity check on values of the filter length */
    if (blfilter < 1.) {
        blfilter = 1.;
    }
    if (blfilter > 32768) {
        blfilter = 32768;
    }
    BLFILTER = (parameter_t) ROUND(32768. / blfilter);

    /* Write BLFILTER to the DSP */
    status = dxp_set_one_dspsymbol(&detChan, "BLFILTER", &BLFILTER);
    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error writing BLFILTER for detChan %d", detChan);
        pslLogError("pslDoBaselineFilterLength", info_string, status);
        return status;
    }

    /* Set the baseline_cut entry */
    status = pslSetDefault("baseline_filter_length", value, defs);
    if (status != XIA_SUCCESS) {
        pslLogError("pslDoBaselineFilterLength",
                    "Error setting baseline_filter_length in the defaults", status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Not applicable to the X10P.
 */
PSL_STATIC int PSL_API pslDoEnableInterrupt(int detChan, void* value) {
    UNUSED(detChan);
    UNUSED(value);

    return XIA_SUCCESS;
}

/*
 * Like pslDoADCPercentRule(), this routine recalculates the gain from the
 * calibration energy point of view.
 */
ACQUISITION_VALUE(CalibrationEnergy) {
    int status;

    UNUSED(detector_chan);
    UNUSED(det);
    UNUSED(detectorType);
    UNUSED(fs);

    status = pslSetDefault("calibration_energy", value, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting calibration_energy for detChan %d",
                detChan);
        pslLogError("pslDoCalibrationEnergy", info_string, status);
        return status;
    }

    status = pslDoGainSetting(detChan, defs, preampGain, m);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting the Gain for detChan %d", detChan);
        pslLogError("pslDoCalibrationEnergy", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This routine gets the specified acquisition value from either the defaults
 * or from on-board parameters. I anticipate that this routine will be
 * much less complicated then setAcquisitionValues so I will do everything
 * in a single routine.
 */
PSL_STATIC int PSL_API pslGetAcquisitionValues(int detChan, char* name, void* value,
                                               XiaDefaults* defaults) {
    int status;

    double tmp = 0.0;

    UNUSED(detChan);

    /* Try to get it with pslGetDefault() and
     * if it isn't there after that, then
     * return an error.
     */
    status = pslGetDefault(name, (void*) &tmp, defaults);

    if (status != XIA_SUCCESS) {
        status = XIA_UNKNOWN_VALUE;
        sprintf(info_string, "Acquisition value %s is unknown", name);
        pslLogError("pslGetAcquisitionValues", info_string, status);
        return status;
    }

    *((double*) value) = tmp;

    return XIA_SUCCESS;
}

/*
 * This routine computes the value of GAINDAC based on the input values.
 * Also handles scaling due to the "discreteness" of BINFACT1. I'll try and
 * illustrate the equations as we go along, but the best reference is
 * probably found external to the program.
 */
PSL_STATIC int PSL_API pslCalculateGain(int detChan, double adcPercentRule,
                                        double calibEV, double preampGain,
                                        double MCABinWidth, parameter_t SLOWLEN,
                                        parameter_t* GAINDAC) {
    int status;

    double gSystem;
    double gTotal;
    double gBase = 1.0;
    double gVar;
    double gDB;
    double inputRange = 1000.0;
    double gaindacDB = 40.0;
    double gGAINDAC;
    double eVPerADC;
    double dBINFACT1;
    double binScale;
    double gaindacBits = 16.0;

    parameter_t BINFACT1;

    gSystem = pslCalculateSysGain();

    gTotal =
        ((adcPercentRule / 100.0) * inputRange) / ((calibEV / 1000.0) * preampGain);

    /* Scale gTotal as a BINFACT1 correction */
    eVPerADC = (double) (calibEV / ((adcPercentRule / 100.0) * NUM_BITS_ADC));
    dBINFACT1 = (MCABinWidth / eVPerADC) * (double) SLOWLEN * 4.0;
    BINFACT1 = (parameter_t) ROUND(dBINFACT1);

    /* Calculate the scale factor used to correct the gain */
    binScale = (double) BINFACT1 / dBINFACT1;

    /* Try to skip the invalid 0 binScale here */
    if (binScale == 0) {
        BINFACT1++;
        binScale = (double) BINFACT1 / dBINFACT1;
    }

    /* Adjust the gain by the BINFACT change.*/
    gTotal *= binScale;

    gVar = gTotal / (gSystem * gBase);

    /* Now we can start converting to GAINDAC */
    gDB = (20.0 * log10(gVar));

    if ((gDB < -6.0) || (gDB > 30.0)) {
        /* Try the other value of BINFACT1, it was rounded, but sometimes, this rounding can lead
         * us out of range, this should alleviate some of the cases where we go out of range, but
         * the consequence is that the gain will be changed more than if BINFACT had just been
         * rounded */
        if (BINFACT1 > dBINFACT1) {
            BINFACT1--;
        } else {
            BINFACT1++;
        }

        binScale = (double) BINFACT1 / dBINFACT1;

        /* Adjust the gain by the BINFACT change */
        gTotal *= binScale;

        gVar = gTotal / (gSystem * gBase);

        /* Now we can start converting to GAINDAC */
        gDB = (20.0 * log10(gVar));

        if ((gDB < -6.0) || (gDB > 30.0)) {
            status = XIA_GAIN_OOR;
            sprintf(info_string, "Gain value %.3f (in dB) is out-of-range", gDB);
            pslLogError("pslCalculateGain", info_string, status);
            return status;
        }
    }

    gDB += 10.0;

    gGAINDAC = gDB * ((double) (pow(2, gaindacBits) / gaindacDB));

    *GAINDAC = (parameter_t) ROUND(gGAINDAC);

    /* Must set the value of BINFACT1 anytime the gain changes */
    status = dxp_set_one_dspsymbol(&detChan, "BINFACT1", &BINFACT1);
    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error setting BINFACT1 for detChan %d", detChan);
        pslLogError("pslCalculateGain", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This calculates the system gain due to the analog portion of the system.
 */
PSL_STATIC double PSL_API pslCalculateSysGain(void) {
    double gInputBuffer = 1.0;
    double gInvertingAmp = 3240.0 / 499.0;
    double gVoltageDivider = 124.9 / 498.9;
    double gGaindacBuffer = 1.0;
    double gNyquist = 422.0 / 613.0;
    double gADCBuffer = 2.00;
    double gADC = 250.0 / 350.0;

    double gSystem;

    gSystem = gInputBuffer * gInvertingAmp * gVoltageDivider * gGaindacBuffer *
              gNyquist * gADCBuffer * gADC;

    return gSystem;
}

/*
 * This routine sets the clock speed for the X10P board. Eventually, we'd
 * like to read this info. from the interface. For now, we will set it
 * statically.
 */
PSL_STATIC int pslGetClockSpeed(int detChan, double* spd) {
    int status;

    parameter_t SYSMICROSEC = 0;

    status = dxp_get_one_dspsymbol(&detChan, "SYSMICROSEC", &SYSMICROSEC);

    if (status != DXP_SUCCESS) {
        sprintf(info_string,
                "Error getting SYSMICROSEC for detChan %d, using default speed",
                detChan);
        pslLogWarning("pslGetClockSpeed", info_string);
        SYSMICROSEC = (parameter_t) DEFAULT_CLOCK_SPEED;
    }

    if ((SYSMICROSEC != 20) && (SYSMICROSEC != 40)) {
        sprintf(info_string,
                "The return hardware clock speed is invalid (%hu) for "
                "detChan %d",
                SYSMICROSEC, detChan);
        pslLogError("pslGetClockSpeed", info_string, XIA_CLOCK_SPEED);
        return XIA_CLOCK_SPEED;
    }

    *spd = (double) SYSMICROSEC;

    return XIA_SUCCESS;
}

/*
 *  Adjusts the percent rule by delta.
 */
PSL_STATIC int psl__AdjustPercentRule(int detChan, Detector* det, int modChan,
                                      Module* m, XiaDefaults* defs, void* value) {
    int status;

    double oldADCPercentRule;
    double newADCPercentRule;

    double preampGain = 0.0;
    double delta = *((double*) value);

    UNUSED(modChan);
    UNUSED(m);
    UNUSED(det);

    status = pslGetDefault("adc_percent_rule", (void*) &oldADCPercentRule, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting adc_percent_rule for detChan %d", detChan);
        pslLogError("psl__AdjustPercentRule", info_string, status);
        return status;
    }

    newADCPercentRule = oldADCPercentRule * delta;

    status = pslSetDefault("adc_percent_rule", (void*) &newADCPercentRule, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting adc_percent_rule for detChan %d", detChan);
        pslLogError("psl__AdjustPercentRule", info_string, status);
        return status;
    }

    status = pslGetDefault("preamp_gain", (void*) &preampGain, defs);
    ASSERT(status == XIA_SUCCESS);

    status = pslDoGainSetting(detChan, defs, preampGain, m);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting the Gain for detChan %d", detChan);
        pslLogError("psl__AdjustPercentRule", info_string, status);
        return status;
    }

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error changing gain for detChan %d. Attempting to "
                "reset to previous value",
                detChan);
        pslLogError("psl__AdjustPercentRule", info_string, status);

        /* Try to reset the gain. If it doesn't work then you aren't really any
         * worse off then you were before.
         */
        status = pslDoGainSetting(detChan, defs, preampGain, m);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Wrapper function for pslGainCalibrate
 */
PSL_STATIC int psl__GainCalibrate(int detChan, Detector* det, int modChan, Module* m,
                                  XiaDefaults* defs, void* value) {
    double* deltaGain = (double*) value;
    return pslGainCalibrate(detChan, det, modChan, m, defs, *deltaGain);
}

/*
 * This routine adjusts the gain via. the preamp gain. As the name suggests,
 * this is mostly for situations where you are trying to calibrate the gain
 * with a fixed eV/ADC, which should cover 99% of the situations, if not the
 * full 100%.
 */
PSL_STATIC int pslGainCalibrate(int detChan, Detector* detector, int modChan, Module* m,
                                XiaDefaults* defaults, double deltaGain) {
    int status;

    double preampGain = 0.0;

    /* Calculate scaled preamp gain keeping in mind that it is an inverse
     * relationship.
     */
    status = pslGetDefault("preamp_gain", (void*) &preampGain, defaults);
    ASSERT(status == XIA_SUCCESS);

    preampGain *= 1.0 / deltaGain;

    /* Set the new preamp_gain, this will also set the detector structure entry,
     * defaults, and recalc gain.
     */
    status = pslDoPreampGain(detChan, (void*) &preampGain, NULL, NULL, defaults,
                             preampGain, m, detector, m->detector_chan[modChan]);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error calling doPreampGain for detChan %d", detChan);
        pslLogError("pslGainCalibrate", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This routine starts and stops a quick run, typically used to apply
 * hardware configuration changes like polarity and gain
 */
PSL_STATIC int pslQuickRun(int detChan, XiaDefaults* defaults, Module* m) {
    int status;

    float wait = (float) (20.0 / 1000.0);
    int timeout = 200;

    parameter_t BUSY = 0;
    parameter_t RUNIDENT = 0;
    parameter_t RUNIDENT2 = 0;

    /* Check that BUSY=6 or (BUSY=0 and RUNIDENT=RUNIDENT+1) before stopping the run
     * BUG ID #100
     */
    status = dxp_get_one_dspsymbol(&detChan, "RUNIDENT", &RUNIDENT);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error getting RUNIDENT for detChan %d", detChan);
        pslLogError("pslQuickRun", info_string, status);
        return status;
    }

    /* Increment RUNIDENT now, so that we don't need to in every loop while polling */
    RUNIDENT++;

    status = pslStartRun(detChan, 0, defaults, m);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error starting quick run on detChan %d", detChan);
        pslLogError("pslQuickRun", info_string, status);
        return status;
    }

    while (timeout > 0) {
        utils->funcs->dxp_md_wait(&wait);

        /* Get the new value of RUNIDENT */
        status = dxp_get_one_dspsymbol(&detChan, "RUNIDENT", &RUNIDENT2);

        if (status != DXP_SUCCESS) {
            sprintf(info_string, "Error getting RUNIDENT for detChan %d", detChan);
            pslLogError("pslQuickRun", info_string, status);
            return status;
        }

        /* Check that BUSY=6 before stopping the run
         * BUG ID #84
         */
        status = dxp_get_one_dspsymbol(&detChan, "BUSY", &BUSY);

        if (status != DXP_SUCCESS) {
            sprintf(info_string, "Error getting BUSY for detChan %d", detChan);
            pslLogError("pslQuickRun", info_string, status);
            return status;
        }

        /* Check if we can stop the run, else, decrement timeout and go again
         * BUG #100 fix for very short PRESET runs
         */
        if ((BUSY == 6) || ((BUSY == 0) && (RUNIDENT2 == RUNIDENT))) {
            status = pslStopRun(detChan, m);

            if (status != XIA_SUCCESS) {
                sprintf(info_string, "Error stopping quick run on detChan %d", detChan);
                pslLogError("pslQuickRun", info_string, status);
                return status;
            }

            return XIA_SUCCESS;
        }

        timeout--;
    }

    status = pslStopRun(detChan, m);

    status = XIA_TIMEOUT;
    sprintf(info_string,
            "Timeout (BUSY = %u) waiting to stop a quick run "
            "on detChan %d",
            BUSY, detChan);
    pslLogError("pslQuickRun", info_string, status);
    return status;
}

/*
 * This routine is responsible for calling the XerXes version of start run.
 * There may be a problem here involving multiple calls to start a run on a
 * module since this routine is iteratively called for each channel. May need
 * a way to start runs that takes the "state" of the module into account.
 */
PSL_STATIC int pslStartRun(int detChan, unsigned short resume, XiaDefaults* defs,
                           Module* m) {
    int status;

    double tmpGate = 0.0;

    unsigned short gate;

    UNUSED(m);

    status = pslGetDefault("enable_gate", (void*) &tmpGate, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error getting enable_gate information for run start on detChan %d",
                detChan);
        pslLogError("pslStartRun", info_string, status);
        return status;
    }

    gate = (unsigned short) tmpGate;

    status = dxp_start_one_run(&detChan, &gate, &resume);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error starting a run on detChan %d", detChan);
        pslLogError("pslStartRun", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This routine is responsible for calling the XerXes version of stop run.
 * With some hardware, all channels on a given module may be "stopped"
 * together. Not sure if calling stop multiple times do to the detChan
 * iteration procedure will cause problems or not.
 */
PSL_STATIC int pslStopRun(int detChan, Module* m) {
    int status;
    int i;

    /* Since dxp_md_wait() wants a time in seconds */
    float wait = 1.0f / 1000.0f;

    parameter_t BUSY = 0;

    UNUSED(m);

    status = dxp_stop_one_run(&detChan);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error stopping a run on detChan %d", detChan);
        pslLogError("pslStopRun", info_string, status);
        return status;
    }

    /* Allow time for run to end */
    status = utils->funcs->dxp_md_wait(&wait);

    /* If run is truly ended, then BUSY should equal 0 */
    for (i = 0; i < 4000; i++) {
        status = dxp_get_one_dspsymbol(&detChan, "BUSY", &BUSY);

        if (BUSY == 0) {
            return XIA_SUCCESS;
        }

        status = utils->funcs->dxp_md_wait(&wait);
    }

    sprintf(info_string,
            "Timeout (BUSY = %u) waiting for a run to end on "
            "detChan %d",
            BUSY, detChan);
    pslLogError("pslStopRun", info_string, XIA_TIMEOUT);
    return XIA_TIMEOUT;
}

/*
 * This routine retrieves the specified data from the board. In the case of
 * the X10P a run does not need to be stopped in order to retrieve the
 * specified information.
 */
PSL_STATIC int pslGetRunData(int detChan, char* name, void* value, XiaDefaults* defs,
                             Module* m) {
    int status;
    unsigned long i;

    UNUSED(m);

    ASSERT(defs != NULL);

    if (STREQ(name, "livetime")) {
        pslLogWarning("pslGetRunData",
                      "'livetime' is deprecated as a run data "
                      "type. Use 'trigger_livetime' or 'energy_livetime' "
                      "instead.");
    } else if (STREQ(name, "events_in_run")) {
        pslLogWarning("pslGetRunData",
                      "'events_in_run' is deprecated as a run "
                      "data type. Use 'mca_events' or 'total_output_events' "
                      "instead.");
    }

    for (i = 0; i < N_ELEMS(RUN_DATA); i++) {
        if (STREQ(name, RUN_DATA[i].name)) {
            status = RUN_DATA[i].fn(detChan, value, defs);

            if (status != XIA_SUCCESS) {
                sprintf(info_string, "Error getting run data '%s' for detChan %d", name,
                        detChan);
                pslLogError("pslGetRunData", info_string, status);
                return status;
            }

            return XIA_SUCCESS;
        }
    }

    sprintf(info_string, "Unknown run data type '%s' for detChan %d", name, detChan);
    pslLogError("pslGetRunData", info_string, XIA_BAD_NAME);
    return XIA_BAD_NAME;
}

/*
 * This routine sets value to the length of the MCA spectrum.
 */
PSL_STATIC int psl__GetMCALength(int detChan, void* value, XiaDefaults* defs) {
    int status;

    UNUSED(defs);

    ASSERT(value);

    status = dxp_nspec(&detChan, (unsigned int*) value);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error getting spectrum length for detChan %d", detChan);
        pslLogError("psl__GetMCALength", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This routine gets the MCA spectrum through XerXes.
 */
PSL_STATIC int psl__GetMCAData(int detChan, void* value, XiaDefaults* defs) {
    int status;

    double mcaStartAddress;
    double mcaLen;

    char memStr[64];

    ASSERT(value);
    ASSERT(defs);

    status = pslGetDefault("mca_start_address", &mcaStartAddress, defs);
    ASSERT(status == XIA_SUCCESS);

    status = pslGetDefault("number_mca_channels", &mcaLen, defs);
    ASSERT(status == XIA_SUCCESS);

    sprintf(memStr, "spectrum:%#hx:%lu", (unsigned short) mcaStartAddress,
            (unsigned long) mcaLen);

    status = dxp_read_memory(&detChan, memStr, (unsigned long*) value);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error reading memory '%s' for detChan %d", memStr,
                detChan);
        pslLogError("psl__GetMCAData", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This routine reads the livetime back from the board.
 */
PSL_STATIC int psl__GetLivetime(int detChan, void* value, XiaDefaults* defs) {
    int status;

    unsigned long dspParams[DSP_PARAM_MEM_LEN];

    UNUSED(defs);

    ASSERT(value);

    status = psl__GetDSPBlock(detChan, dspParams);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error getting all DSP parameters for detChan "
                "%d.",
                detChan);
        pslLogError("psl__GetLivetime", info_string, status);
        return status;
    }

    status = psl__ExtractTLivetime(detChan, dspParams, (double*) value);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error extracting the trigger livetime from the "
                "DSP parameter block for detChan %d.",
                detChan);
        pslLogError("psl__GetLivetime", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This routine reads the runtime back from the board.
 */
PSL_STATIC int psl__GetRealtime(int detChan, void* value, XiaDefaults* defs) {
    int status;

    unsigned long dspParams[DSP_PARAM_MEM_LEN];

    UNUSED(defs);

    ASSERT(value);

    status = psl__GetDSPBlock(detChan, dspParams);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error getting all DSP parameters for detChan "
                "%d.",
                detChan);
        pslLogError("psl__GetRealtime", info_string, status);
        return status;
    }

    status = psl__ExtractRealtime(detChan, dspParams, (double*) value);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error extracting the realtime from the DSP "
                "parameter block for detChan %d.",
                detChan);
        pslLogError("psl__GetRealtime", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This routine gets the Input Count Rate (ICR)
 */
PSL_STATIC int psl__GetICR(int detChan, void* value, XiaDefaults* defs) {
    int status;

    double tlt;

    unsigned long trigs;

    unsigned long dspParams[DSP_PARAM_MEM_LEN];

    UNUSED(defs);

    ASSERT(value);

    status = psl__GetDSPBlock(detChan, dspParams);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error getting all DSP parameters for detChan "
                "%d.",
                detChan);
        pslLogError("psl__GetICR", info_string, status);
        return status;
    }

    status = psl__ExtractTLivetime(detChan, dspParams, &tlt);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error extracting the trigger livetime from the "
                "DSP parameter block for detChan %d.",
                detChan);
        pslLogError("psl__GetICR", info_string, status);
        return status;
    }

    status = psl__ExtractTriggers(detChan, dspParams, &trigs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error extracting the trigger count from the "
                "DSP parameter block for detChan %d.",
                detChan);
        pslLogError("psl__GetICR", info_string, status);
        return status;
    }

    if (tlt > 0.0) {
        *((double*) value) = (double) trigs / tlt;
    } else {
        *((double*) value) = 0.0;
    }

    return XIA_SUCCESS;
}

/*
 * This routine gets the Output Count Rate (OCR)
 */
PSL_STATIC int psl__GetOCR(int detChan, void* value, XiaDefaults* defs) {
    int status;

    double rt;

    unsigned long mcaEvts;
    unsigned long unders;
    unsigned long overs;

    unsigned long dspParams[DSP_PARAM_MEM_LEN];

    UNUSED(defs);

    ASSERT(value);

    status = psl__GetDSPBlock(detChan, dspParams);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error getting all DSP parameters for detChan "
                "%d.",
                detChan);
        pslLogError("psl__GetOCR", info_string, status);
        return status;
    }

    status = psl__ExtractRealtime(detChan, dspParams, &rt);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error extracting the realtime from the "
                "DSP parameter block for detChan %d.",
                detChan);
        pslLogError("psl__GetOCR", info_string, status);
        return status;
    }

    status = psl__ExtractEvents(detChan, dspParams, &mcaEvts);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error extracting the MCA event count from the "
                "DSP parameter block for detChan %d.",
                detChan);
        pslLogError("psl__GetOCR", info_string, status);
        return status;
    }

    status = psl__ExtractUnders(detChan, dspParams, &unders);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error extracting the underflow event count from "
                "the DSP parameter block for detChan %d.",
                detChan);
        pslLogError("psl__GetOCR", info_string, status);
        return status;
    }

    status = psl__ExtractOvers(detChan, dspParams, &overs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error extracting the overflow event count from "
                "the DSP parameter block for detChan %d.",
                detChan);
        pslLogError("psl__GetOCR", info_string, status);
        return status;
    }

    if (rt > 0.0) {
        *((double*) value) = (double) (mcaEvts + unders + overs) / rt;
    } else {
        *((double*) value) = 0.0;
    }

    return XIA_SUCCESS;
}

/*
 * This routine gets the number of events in the run
 */
PSL_STATIC int psl__GetTotalEvents(int detChan, void* value, XiaDefaults* defs) {
    int status;

    unsigned long evts;
    unsigned long unders;
    unsigned long overs;

    unsigned long dspParams[DSP_PARAM_MEM_LEN];

    UNUSED(defs);

    ASSERT(value);

    status = psl__GetDSPBlock(detChan, dspParams);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error getting all DSP parameters for detChan "
                "%d.",
                detChan);
        pslLogError("psl__GetTotalEvents", info_string, status);
        return status;
    }

    status = psl__ExtractEvents(detChan, dspParams, &evts);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error extracting the MCA event count from the "
                "DSP parameter block for detChan %d.",
                detChan);
        pslLogError("psl__GetTotalEvents", info_string, status);
        return status;
    }

    status = psl__ExtractUnders(detChan, dspParams, &unders);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error extracting the underflows from the "
                "DSP parameter block for detChan %d.",
                detChan);
        pslLogError("psl__GetTotalEvents", info_string, status);
        return status;
    }

    status = psl__ExtractOvers(detChan, dspParams, &overs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error extracting the overflows from the "
                "DSP parameter block for detChan %d.",
                detChan);
        pslLogError("psl__GetTotalEvents", info_string, status);
        return status;
    }

    *((unsigned long*) value) = (unsigned long) (evts + unders + overs);

    return XIA_SUCCESS;
}

/*
 * This routine gets the number of triggers (FASTPEAKS) in the run
 */
PSL_STATIC int psl__GetTriggers(int detChan, void* value, XiaDefaults* defs) {
    int status;

    unsigned long dspParams[DSP_PARAM_MEM_LEN];

    UNUSED(defs);

    ASSERT(value);

    status = psl__GetDSPBlock(detChan, dspParams);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error getting all DSP parameters for detChan "
                "%d.",
                detChan);
        pslLogError("psl__GetTriggers", info_string, status);
        return status;
    }

    status = psl__ExtractTriggers(detChan, dspParams, (unsigned long*) value);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error extracting the triggers from the DSP "
                "parameter block for detChan %d.",
                detChan);
        pslLogError("psl__GetTriggers", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This routine gets the baseline length as determined by XerXes.
 */
PSL_STATIC int psl__GetBaselineLength(int detChan, void* value, XiaDefaults* defs) {
    int status;

    unsigned int baseLen;

    UNUSED(defs);

    ASSERT(value);

    status = dxp_nbase(&detChan, &baseLen);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error getting size of baseline histogram for detChan %d",
                detChan);
        pslLogError("psl__GetBaselineLength", info_string, status);
        return status;
    }

    *((unsigned long*) value) = (unsigned long) baseLen;

    return XIA_SUCCESS;
}

/*
 * This routine gets the baseline data from XerXes.
 */
PSL_STATIC int psl__GetBaseline(int detChan, void* value, XiaDefaults* defs) {
    int status;
    unsigned int baseLen;

    UNUSED(defs);

    ASSERT(value);

    status = dxp_nbase(&detChan, &baseLen);

    if (status != DXP_SUCCESS) {
        return status;
    }

    status = dxp_readout_detector_run(&detChan, NULL, (unsigned long*) value, NULL);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error reading baseline histogram from detChan %d",
                detChan);
        pslLogError("psl__GetBaseline", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This routine sets the run active information as retrieved
 * from Xerxes. The raw bitmask (from Xerxes) is actually
 * returned to the user with the appropriate constants
 * defined in handel.h.
 */
PSL_STATIC int psl__GetRunActive(int detChan, void* value, XiaDefaults* defs) {
    int status;
    int runActiveX;

    UNUSED(defs);

    ASSERT(value);

    status = dxp_isrunning(&detChan, &runActiveX);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error getting run active information for detChan %d",
                detChan);
        pslLogError("psl__GetRunActive", info_string, status);
        return status;
    }

    *((unsigned long*) value) = (unsigned long) runActiveX;

    return XIA_SUCCESS;
}

/*
 * This routine dispatches calls to the requested special run routine, when
 * that special run is supported by the x10p.
 */
PSL_STATIC int PSL_API pslDoSpecialRun(int detChan, char* name, void* info,
                                       XiaDefaults* defaults, Detector* detector,
                                       int detector_chan) {
    int status = XIA_SUCCESS;

    UNUSED(defaults);
    UNUSED(detector);
    UNUSED(detector_chan);

    if (STREQ(name, "adc_trace")) {
        status = pslDoADCTrace(detChan, info);
    } else if (STREQ(name, "baseline_history")) {
        status = pslDoBaseHistory(detChan, info);
    } else if (STREQ(name, "open_input_relay")) {
        status = pslDoControlTask(detChan, CT_SATURN_OPEN_INPUT_RELAY, 1, info);
    } else if (STREQ(name, "close_input_relay")) {
        status = pslDoControlTask(detChan, CT_SATURN_CLOSE_INPUT_RELAY, 1, info);
    } else if (STREQ(name, "read_external_memory")) {
        status = pslDoControlTaskWithoutStop(detChan, CT_SATURN_READ_MEMORY, 4, info);
    } else if (STREQ(name, "write_external_memory")) {
        status = pslDoControlTaskWithoutStop(detChan, CT_SATURN_WRITE_MEMORY, 4, info);
    } else if (STREQ(name, "measure_taurc")) {
        status = pslTauFinder(detChan, defaults, detector, detector_chan, info);
    } else {
        status = XIA_BAD_SPECIAL;
        sprintf(info_string, "%s is not a valid special run type", name);
        pslLogError("pslDoSpecialRun", info_string, status);
        return status;
    }

    return status;
}

/*
 * This routine does all of the work necessary to start and stop a special
 * run for the ADC data. A seperate call must be made to read the data out.
 */
PSL_STATIC int PSL_API pslDoADCTrace(int detChan, void* info) {
    int status;
    int infoStart[2];
    int infoInfo[3];
    int timeout = 1000;
    int i;

    unsigned int len = 2;

    short task = CT_SATURN_ADC;

    parameter_t BUSY;
    parameter_t TRACEWAIT;
    parameter_t maxTracewait = 16383;

    double* dInfo = (double*) info;
    double tracewait;
    /* In nanosec */
    double minTracewait = 100.0;
    double clockTick = 50;

    float waitTime;
    float pollTime;

    infoStart[0] = (int) dInfo[0];

    /* Compute TRACEWAIT keeping in mind that the minimum value for the X10P is
     * 100 ns.
     */
    tracewait = dInfo[1];

    if (tracewait < minTracewait) {
        sprintf(info_string,
                "tracewait of %.3f ns is too small. Setting it to %.3f ns.", tracewait,
                minTracewait);
        pslLogWarning("pslDoADCTrace", info_string);
        tracewait = minTracewait;
    }

    /* Refs #2208 "clockTick" is hardcoded here*/
    TRACEWAIT = (parameter_t) ROUND(((tracewait - minTracewait) / clockTick));

    /* See BUG ID #54 */
    if (TRACEWAIT > maxTracewait) {
        sprintf(info_string, "TRACEWAIT of %u is too big. Setting it to %u", TRACEWAIT,
                maxTracewait);
        pslLogWarning("pslDoADCTrace", info_string);
        TRACEWAIT = maxTracewait;
    }
    infoStart[1] = (int) TRACEWAIT;

    status = dxp_control_task_info(&detChan, &task, infoInfo);
    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error getting control task info for detChan %d", detChan);
        pslLogError("pslDoADCTrace", info_string, status);
        return status;
    }

    /* See BUG ID #55: set the properly adjusted wait time for the benefit of the calling routine */
    dInfo[1] = (double) (((double) TRACEWAIT * clockTick) + minTracewait);

    /* Set the wait time to be ADC length * either the trace wait time (between samples) or 400. ns whichever is greater
     * then convert to seconds for the md_wait() calls */
    waitTime = (float) (MAX(dInfo[1], 400.) * ((float) infoInfo[0]) / 1.0e9);
    pollTime = (float) ((float) infoInfo[2] / 1000.0);

    status = dxp_start_control_task(&detChan, &task, &len, infoStart);
    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error starting control task on detChan %d", detChan);
        pslLogError("pslDoADCTrace", info_string, status);
        return status;
    }

    utils->funcs->dxp_md_wait(&waitTime);

    for (i = 0; i < timeout; i++) {
        status = dxp_get_one_dspsymbol(&detChan, "BUSY", &BUSY);
        if (status != DXP_SUCCESS) {
            sprintf(info_string, "Error getting BUSY from detChan %d", detChan);
            pslLogError("pslDoADCTrace", info_string, status);
            return status;
        }

        if (BUSY == 0) {
            break;
        }

        if (i == (timeout - 1)) {
            status = XIA_TIMEOUT;
            sprintf(info_string, "Timeout waiting for BUSY to go to 0 on detChan %d",
                    detChan);
            pslLogError("pslDoADCTrace", info_string, status);
            return status;
        }

        utils->funcs->dxp_md_wait(&pollTime);
    }

    return XIA_SUCCESS;
}

/*
 * This routine is used to read external memory from the board.
 */
PSL_STATIC int PSL_API pslDoControlTask(int detChan, short task, unsigned int len,
                                        void* info) {
    int status;

    status = pslDoControlTaskWithoutStop(detChan, task, len, info);
    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error performing control task less the stop on detChan %d", detChan);
        pslLogError("pslDoControlTask", info_string, status);
        return status;
    }

    status = dxp_stop_control_task(&detChan);
    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error stopping control task on detChan %d", detChan);
        pslLogError("pslDoControlTask", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This routine is used to read external memory from the board.
 */
PSL_STATIC int PSL_API pslDoControlTaskWithoutStop(int detChan, short task,
                                                   unsigned int len, void* info) {
    int status;
    int infoInfo[3];
    int i;
    int timeout = 1000;

    parameter_t BUSY;

    float waitTime;
    float pollTime;

    status = dxp_control_task_info(&detChan, &task, infoInfo);
    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error getting control task info for detChan %d", detChan);
        pslLogError("pslDoControlTaskWithoutStop", info_string, status);
        return status;
    }

    waitTime = (float) ((float) infoInfo[1] / 1000.0);
    pollTime = (float) ((float) infoInfo[2] / 1000.0);

    status = dxp_start_control_task(&detChan, &task, &len, (int*) info);
    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error starting control task on detChan %d", detChan);
        pslLogError("pslDoControlTaskWithoutStop", info_string, status);
        return status;
    }

    utils->funcs->dxp_md_wait(&waitTime);

    for (i = 0; i < timeout; i++) {
        status = dxp_get_one_dspsymbol(&detChan, "BUSY", &BUSY);
        if (status != DXP_SUCCESS) {
            sprintf(info_string, "Error getting BUSY from detChan %d", detChan);
            pslLogError("pslDoControlTaskWithoutStop", info_string, status);
            return status;
        }

        if (BUSY == 0) {
            break;
        }

        if (i == (timeout - 1)) {
            status = XIA_TIMEOUT;
            sprintf(info_string, "Timeout waiting for BUSY to go to 0 on detChan %d",
                    detChan);
            pslLogError("pslDoControlTaskWithoutStop", info_string, status);
            return status;
        }

        utils->funcs->dxp_md_wait(&pollTime);
    }

    return XIA_SUCCESS;
}

/*
 * This routine parses out the actual data gathering to other routines.
 */
PSL_STATIC int PSL_API pslGetSpecialRunData(int detChan, char* name, void* value,
                                            XiaDefaults* defaults) {
    int status = XIA_SUCCESS;

    UNUSED(defaults);

    if (STREQ(name, "adc_trace_length")) {
        status = pslGetControlTaskLength(detChan, CT_SATURN_ADC, value);
    } else if (STREQ(name, "adc_trace")) {
        status = pslGetControlTaskDataWithStop(detChan, CT_SATURN_ADC, value);
    } else if (STREQ(name, "baseline_history_length")) {
        status = pslGetControlTaskLength(detChan, CT_SATURN_BASELINE_HIST, value);
    } else if (STREQ(name, "baseline_history")) {
        status = pslGetBaseHistory(detChan, value);
    } else if (STREQ(name, "external_memory_length")) {
        status = pslGetControlTaskLength(detChan, CT_SATURN_READ_MEMORY, value);
    } else if (STREQ(name, "external_memory")) {
        status = pslGetControlTaskDataWithStop(detChan, CT_SATURN_READ_MEMORY, value);
    } else {
        status = XIA_BAD_SPECIAL;
        sprintf(info_string, "%s is not a valid special run data type", name);
        pslLogError("pslGetSpecialRunData", info_string, status);
        return status;
    }

    return status;
}

/*
 * This routine get the size of the data returned by a special run.
 */
PSL_STATIC int PSL_API pslGetControlTaskLength(int detChan, short task, void* value) {
    int status;
    int info[3];

    status = dxp_control_task_info(&detChan, &task, info);
    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error getting control task info for detChan %d", detChan);
        pslLogError("pslGetControlTaskLength", info_string, status);
        return status;
    }

    *((unsigned long*) value) = (unsigned long) info[0];

    return XIA_SUCCESS;
}

/*
 * This routine gets the data results from a special run.
 */
PSL_STATIC int PSL_API pslGetControlTaskData(int detChan, short task, void* value) {
    int status;

    status = dxp_get_control_task_data(&detChan, &task, value);
    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error getting control task data for detChan %d, task %d",
                detChan, task);
        pslLogError("pslGetControlTaskData", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This routine gets special run data and stops the control task after
 * retrieving the data (useful for those tasks that require certain RUNTASK
 * bits to be turned off)
 */
PSL_STATIC int PSL_API pslGetControlTaskDataWithStop(int detChan, short task,
                                                     void* value) {
    int status;

    status = pslGetControlTaskData(detChan, task, value);
    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error getting control task data before stop for detChan %d, task %d",
                detChan, task);
        pslLogError("pslGetControlTaskDataWithStop", info_string, status);
        return status;
    }

    status = dxp_stop_control_task(&detChan);
    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error stopping control task on detChan %d, task %d",
                detChan, task);
        pslLogError("pslGetControlTaskDataWithStop", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This routine "starts" a baseline history run. What it actually does is
 * disable the "updating" of the baseline history buffer.
 */
PSL_STATIC int PSL_API pslDoBaseHistory(int detChan, void* info) {
    int status;
    int infoInfo[3];

    /*  unsigned int len = 1;
     */
    short task = CT_SATURN_BASELINE_HIST;

    float waitTime;

    parameter_t RUNTASKS;

    UNUSED(info);

    status = dxp_control_task_info(&detChan, &task, infoInfo);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error getting control task info for detChan %d", detChan);
        pslLogError("pslDoBaseHistory", info_string, status);
        return status;
    }

    waitTime = (float) ((float) infoInfo[1] / 1000.0);

    /*
      status = dxp_start_control_task(&detChan, &task, &len, (int *)info);

      if (status != DXP_SUCCESS) {
    sprintf(info_string, "Error starting control task on detChan %d", detChan);
    pslLogError("pslDoBaseHistory", info_string, status);
    return status;
      }
    */

    /* Instead of starting a run here, we just want to turn on the STOP_BASELINE bit */
    /* Set the proper bit of the RUNTASKS DSP parameter */
    /* First retrieve RUNTASKS from the DSP */
    status = dxp_get_one_dspsymbol(&detChan, "RUNTASKS", &RUNTASKS);
    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error getting RUNTASKS for detChan %d", detChan);
        pslLogError("pslDoBaseHistory", info_string, status);
        return status;
    }

    /* Set the bit (masks are in saturn.h) */
    RUNTASKS |= STOP_BASELINE;

    /* Finally write RUNTASKS back to the DSP */
    status = dxp_set_one_dspsymbol(&detChan, "RUNTASKS", &RUNTASKS);
    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error writing RUNTASKS for detChan %d", detChan);
        pslLogError("pslDoBaseHistory", info_string, status);
        return status;
    }

    /* Unlike most runs, we don't need to stop this one here since
     * stopping it will restart the filling of the history buffer.
     * Instead, we'll just wait the specified time and then return.
     */
    utils->funcs->dxp_md_wait(&waitTime);

    return XIA_SUCCESS;
}

/*
 * This routine gets the baseline history data from the frozen baseline
 * history buffer. As usual, it assumes that the proper amount of memory has
 * been allocated for the returned data array.
 */
PSL_STATIC int PSL_API pslGetBaseHistory(int detChan, void* value) {
    int status;

    short task = CT_SATURN_BASELINE_HIST;

    parameter_t RUNTASKS;

    status = dxp_get_control_task_data(&detChan, &task, value);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error getting control task data for detChan %d", detChan);
        pslLogError("pslGetBaseHistory", info_string, status);
        return status;
    }

    /* Instead of stoping a run here, we just want to turn the STOP_BASELINE bit back off */
    /* Set the proper bit of the RUNTASKS DSP parameter */
    /* First retrieve RUNTASKS from the DSP */
    status = dxp_get_one_dspsymbol(&detChan, "RUNTASKS", &RUNTASKS);
    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error getting RUNTASKS for detChan %d", detChan);
        pslLogError("pslGetBaseHistory", info_string, status);
        return status;
    }

    /* Set the bit (masks are in saturn.h) */
    RUNTASKS &= ~STOP_BASELINE;

    /* Finally write RUNTASKS back to the DSP */
    status = dxp_set_one_dspsymbol(&detChan, "RUNTASKS", &RUNTASKS);
    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error writing RUNTASKS for detChan %d", detChan);
        pslLogError("pslGetBaseHistory", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This routine creates a default w/ information specific to the x10p in it.
 */
PSL_STATIC int pslGetDefaultAlias(char* alias, char** names, double* values) {
    unsigned long i;
    unsigned long def_idx;

    char* aliasName = "defaults_saturn";

    ASSERT(alias != NULL);
    ASSERT(names != NULL);
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
 * This routine retrieves the value of the DSP parameter name from detChan.
 */
PSL_STATIC int PSL_API pslGetParameter(int detChan, const char* name,
                                       unsigned short* value) {
    int status;

    ASSERT(name != NULL);
    ASSERT(value != NULL);

    status = dxp_get_one_dspsymbol(&detChan, (char*) name, value);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error reading '%s' for detChan %d", name, detChan);
        pslLogError("pslGetParameter", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This routine sets the value of the DSP parameter name for detChan.
 */
PSL_STATIC int PSL_API pslSetParameter(int detChan, const char* name,
                                       unsigned short value) {
    int status;

    ASSERT(name != NULL);

    status = dxp_set_one_dspsymbol(&detChan, (char*) name, &value);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error setting '%s' to %#hx for detChan %d", name, value,
                detChan);
        pslLogError("pslSetParameter", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Setup per-module settings, this is done after all the
 * acquisition values are set up.
 */
PSL_STATIC int pslModuleSetup(int detChan, XiaDefaults* defaults, Module* m) {
    UNUSED(m);
    UNUSED(defaults);
    UNUSED(detChan);

    return XIA_SUCCESS;
}

/*
 * The whole point of this routine is to make the PSL layer start things
 * off by calling pslSetAcquistionValues() for the acquisition values it
 * thinks are appropriate for the X10P.
 */
PSL_STATIC int PSL_API pslUserSetup(int detChan, XiaDefaults* defaults,
                                    FirmwareSet* firmwareSet,
                                    CurrentFirmware* currentFirmware,
                                    char* detectorType, Detector* detector,
                                    int detector_chan, Module* m, int modChan) {
    unsigned long i;
    int status;

    XiaDaqEntry* entry = defaults->entry;

    ASSERT(defaults != NULL);
    ASSERT(entry != NULL);

    /* Some acquisition values require synchronization with another data
     * structure in the program prior to setting the initial acquisition
     * value.
     */
    for (i = 0; i < N_ELEMS(ACQ_VALUES); i++) {
        if (ACQ_VALUES[i].isSynch) {
            status =
                ACQ_VALUES[i].synchFN(detChan, detector_chan, m, detector, defaults);

            if (status != XIA_SUCCESS) {
                sprintf(info_string, "Error synchronizing '%s' for detChan %d",
                        ACQ_VALUES[i].name, detChan);
                pslLogError("pslUserSetup", info_string, status);
                return status;
            }
        }
    }

    /* We need to set SYSMICROSEC up properly. */
    status = pslQuickRun(detChan, defaults, m);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error applying clock settings for detChan %d", detChan);
        pslLogError("pslUserSetup", info_string, status);
        return status;
    }

    while (entry != NULL) {
        /* Do no attempt to set read-only acquisition values. */
        if (STREQ(entry->name, "actual_gap_time") ||
            STREQ(entry->name, "mca_start_address")) {
            goto nextEntry;
        }

        status = pslSetAcquisitionValues(
            detChan, entry->name, (void*) &entry->data, defaults, firmwareSet,
            currentFirmware, detectorType, detector, detector_chan, m, modChan);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error setting '%s' to %0.3lf for detChan %d",
                    entry->name, entry->data, detChan);
            pslLogError("pslUserSetup", info_string, status);
            return status;
        }

nextEntry:
        entry = entry->next;
    }

    /* Apply the value. */
    status = pslQuickRun(detChan, defaults, m);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error applying acquisition values for detChan %d",
                detChan);
        pslLogError("pslUserSetup", info_string, status);
        return status;
    }

    status = psl__UpdateMCAAddressCache(detChan, defaults);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error updating MCA start address cache for "
                "detChan %d",
                detChan);
        pslLogError("pslUserSetup", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This routine updates the filter parameters for the specified detChan using
 * information from the defaults and the firmware.
 */
PSL_STATIC int pslUpdateFilter(int detChan, double peakingTime, XiaDefaults* defaults,
                               FirmwareSet* firmwareSet, double preampGain, Module* m) {
    int status;

    unsigned short numFilter;
    unsigned short i;
    unsigned short ptrr = 0;

    double dSLOWLEN = 0.0;
    double dSLOWGAP = 0.0;
    double gapTime = 0.0;
    double tmpPIOff = 0.0;
    double tmpPSOff = 0.0;
    double actualGapTime = 0.0;
    double ptMin = 0.0;
    double ptMax = 0.0;

    char piStr[100];
    char psStr[100];

    Firmware* current = NULL;

    parameter_t SLOWLEN = 0;
    parameter_t SLOWGAP = 0;
    parameter_t PEAKINT = 0;
    parameter_t PEAKSAM = 0;
    parameter_t newDecimation = 0;

    parameter_t* filterInfo = NULL;

    status = pslGetClockSpeed(detChan, &CLOCK_SPEED);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting clock speed for detChan %d", detChan);
        pslLogError("pslUpdateFilter", info_string, status);
        return status;
    }

    if (firmwareSet->filename != NULL) {
        status = xiaFddGetNumFilter(firmwareSet->filename, peakingTime,
                                    firmwareSet->numKeywords, firmwareSet->keywords,
                                    &numFilter);

        if (status != XIA_SUCCESS) {
            pslLogError("pslUpdateFilter", "Error getting number of filter params",
                        status);
            return status;
        }

        filterInfo =
            (parameter_t*) saturn_psl_md_alloc(numFilter * sizeof(parameter_t));

        status = xiaFddGetFilterInfo(
            firmwareSet->filename, peakingTime, firmwareSet->numKeywords,
            (const char**) firmwareSet->keywords, &ptMin, &ptMax, filterInfo);

        if (status != XIA_SUCCESS) {
            saturn_psl_md_free(filterInfo);
            pslLogError("pslUpdateFilter", "Error getting filter information from FDD",
                        status);
            return status;
        }

        sprintf(info_string, "PI Offset = %u, PS Offset = %u", filterInfo[0],
                filterInfo[1]);
        pslLogDebug("pslUpdateFilter", info_string);

        /* Override the values loaded in from the FDD with values from the
         * defaults. Remember that when the user is using an FDD file they
         * don't need the _ptrr{n} specifier on their default
         * name. These aren't required so there is no reason to check
         * the status code.
         */

        status = pslGetDefault("peakint_offset", (void*) &tmpPIOff, defaults);

        if (status == XIA_SUCCESS) {
            filterInfo[0] = (parameter_t) tmpPIOff;
        }

        status = pslGetDefault("peaksam_offset", (void*) &tmpPSOff, defaults);

        if (status == XIA_SUCCESS) {
            filterInfo[1] = (parameter_t) tmpPSOff;
        }

        sprintf(info_string, "PI Offset = %u, PS Offset = %u", filterInfo[0],
                filterInfo[1]);
        pslLogDebug("pslUpdateFilter", info_string);
    } else {
        /* Fill the filter information in here using the FirmwareSet */
        current = firmwareSet->firmware;

        while (current != NULL) {
            if ((peakingTime >= current->min_ptime) &&
                (peakingTime <= current->max_ptime)) {
                filterInfo = (parameter_t*) saturn_psl_md_alloc(current->numFilter *
                                                                sizeof(parameter_t));
                for (i = 0; i < current->numFilter; i++) {
                    filterInfo[i] = current->filterInfo[i];
                }

                ptrr = current->ptrr;
            }

            current = getListNext(current);
        }

        if (filterInfo == NULL) {
            status = XIA_BAD_FILTER;
            pslLogError("pslUpdateFilter", "Error loading filter information", status);
            return status;
        }

        sprintf(piStr, "peakint_offset_ptrr%u", ptrr);
        sprintf(psStr, "peaksam_offset_ptrr%u", ptrr);

        /* In this case we just ignore the
         * error values, since the fact that
         * the acq. value is missing just means
         * that we don't want to use it.
         */
        status = pslGetDefault(piStr, (void*) &tmpPIOff, defaults);

        if (status == XIA_SUCCESS) {
            filterInfo[0] = (parameter_t) tmpPIOff;
        }

        status = pslGetDefault(psStr, (void*) &tmpPSOff, defaults);

        if (status == XIA_SUCCESS) {
            filterInfo[1] = (parameter_t) tmpPSOff;
        }
    }

    status = dxp_get_one_dspsymbol(&detChan, "DECIMATION", &newDecimation);

    if (status != DXP_SUCCESS) {
        saturn_psl_md_free(filterInfo);
        status = status;
        sprintf(info_string, "Error getting DECIMATION from detChan %d", detChan);
        pslLogError("pslUpdateFilter", info_string, status);
        return status;
    }

    /* Calculate SLOWLEN from board parameters */
    dSLOWLEN =
        (double) (peakingTime / ((1.0 / CLOCK_SPEED) * pow(2, (double) newDecimation)));
    SLOWLEN = (parameter_t) ROUND(dSLOWLEN);

    sprintf(info_string, "SLOWLEN = %u", SLOWLEN);
    pslLogDebug("pslUpdateFilter", info_string);

    if ((SLOWLEN > 28) || (SLOWLEN < 2)) {
        saturn_psl_md_free(filterInfo);
        status = XIA_SLOWLEN_OOR;
        sprintf(info_string,
                "Calculated value of SLOWLEN (%u) for detChan %d is out-of-range",
                SLOWLEN, detChan);
        pslLogError("pslUpdateFilter", info_string, status);
        return status;
    }

    /* Calculate SLOWGAP from minimum_gap_time and do sanity check */
    status = pslGetDefault("minimum_gap_time", (void*) &gapTime, defaults);
    if (status != XIA_SUCCESS) {
        saturn_psl_md_free(filterInfo);
        sprintf(info_string, "Error getting minimum_gap_time from detChan %d", detChan);
        pslLogError("pslUpdateFilter", info_string, status);
        return status;
    }

    sprintf(info_string, "minimum_gap_time for detChan %d = %.3f", detChan, gapTime);
    pslLogDebug("pslUpdateFilter", info_string);

    dSLOWGAP =
        (double) (gapTime / ((1.0 / CLOCK_SPEED) * pow(2, (double) newDecimation)));
    /* Always round SLOWGAP up...don't use std. round */
    SLOWGAP = (parameter_t) ceil(dSLOWGAP);

    sprintf(info_string, "SLOWGAP = %u", SLOWGAP);
    pslLogDebug("pslUpdateFilter", info_string);

    if (SLOWGAP > 29) {
        SLOWGAP = 29;
    }

    if (SLOWGAP < 3) {
        /* This isn't an error...the SLOWGAP just
         * can't be smaller then 3 which is fine
         * at decimations > 0.
         */
        SLOWGAP = 3;
        pslLogInfo("pslUpdateFilter", "Calculated SLOWGAP is < 3. Setting SLOWGAP = 3");
    }

    /* Check limit on total length of slow filter */
    if ((SLOWLEN + SLOWGAP) > 31) {
        /* Reduce Slowgap by enough to make it fit within this decimation */
        SLOWGAP = (parameter_t) (31 - SLOWLEN);
        sprintf(info_string,
                "SLOWLEN+SLOWGAP>32, setting SLOWGAP = %u with SLOWLEN = %u", SLOWGAP,
                SLOWLEN);
        pslLogInfo("pslUpdateFilter", info_string);
    }

    /* Set value equal to the new "real" peaking time and actual gap time */
    actualGapTime = (((double) SLOWGAP) / CLOCK_SPEED) * pow(2, (double) newDecimation);
    peakingTime = (((double) SLOWLEN) / CLOCK_SPEED) * pow(2, (double) newDecimation);

    status = pslSetDefault("peaking_time", (void*) &peakingTime, defaults);
    if (status != XIA_SUCCESS) {
        saturn_psl_md_free(filterInfo);
        sprintf(info_string, "Error setting peaking_time for detChan %d", detChan);
        pslLogError("pslUpdateFilter", info_string, status);
        return status;
    }

    status = pslSetDefault("actual_gap_time", (void*) &actualGapTime, defaults);
    if (status != XIA_SUCCESS) {
        saturn_psl_md_free(filterInfo);
        sprintf(info_string, "Error setting actual_gap_time for detChan %d", detChan);
        pslLogError("pslUpdateFilter", info_string, status);
        return status;
    }

    /* The X10P PSL interprets the filterInfo as follows:
     * filterInfo[0] = PEAKINT offset
     * filterInfo[1] = PEAKSAM offset
     */

    sprintf(info_string, "PI offset = %u, PS offset = %u", filterInfo[0],
            filterInfo[1]);
    pslLogDebug("pslUpdateFilter", info_string);

    status = dxp_set_one_dspsymbol(&detChan, "SLOWLEN", &SLOWLEN);
    if (status != DXP_SUCCESS) {
        saturn_psl_md_free(filterInfo);
        sprintf(info_string, "Error setting SLOWLEN for detChan %d", detChan);
        pslLogError("pslUpdateFilter", info_string, status);
        return status;
    }

    status = dxp_set_one_dspsymbol(&detChan, "SLOWGAP", &SLOWGAP);
    if (status != DXP_SUCCESS) {
        saturn_psl_md_free(filterInfo);
        sprintf(info_string, "Error setting SLOWGAP for detChan %d", detChan);
        pslLogError("pslUpdateFilter", info_string, status);
        return status;
    }

    PEAKINT = (parameter_t) (SLOWLEN + SLOWGAP + filterInfo[0]);

    status = dxp_set_one_dspsymbol(&detChan, "PEAKINT", &PEAKINT);

    if (status != DXP_SUCCESS) {
        saturn_psl_md_free(filterInfo);
        sprintf(info_string, "Error setting PEAKINT for detChan %d", detChan);
        pslLogError("pslUpdateFilter", info_string, status);
        return status;
    }

    PEAKSAM = (parameter_t) (PEAKINT - filterInfo[1]);

    status = dxp_set_one_dspsymbol(&detChan, "PEAKSAM", &PEAKSAM);
    if (status != DXP_SUCCESS) {
        saturn_psl_md_free(filterInfo);
        sprintf(info_string, "Error setting PEAKSAM for detChan %d", detChan);
        pslLogError("pslUpdateFilter", info_string, status);
        return status;
    }

    saturn_psl_md_free(filterInfo);

    /* Calculate the gain again.  It actually depends on the SLOWLEN
     * via BINFACT1
     */
    status = pslDoGainSetting(detChan, defaults, preampGain, m);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting the Gain for detChan %d", detChan);
        pslLogError("pslUpdateFilter", info_string, status);
        return status;
    }

    status = dxp_upload_dspparams(&detChan);

    if (status != DXP_SUCCESS) {
        sprintf(info_string,
                "Error uploading DSP parameters to internal memory for detChan %d",
                detChan);
        pslLogError("pslUpdateFilter", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This routine updates the specified filter information in the defaults
 * and then calls the update filter routine so that all of the
 * filter parameters will be brought up in sync.
 */
PSL_STATIC int pslDoFilter(int detChan, char* name, void* value, XiaDefaults* defaults,
                           FirmwareSet* firmwareSet, double preampGain, Module* m) {
    int status;

    double pt = 0.0;

    status = pslSetDefault(name, value, defaults);
    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting %s for detChan %d", name, detChan);
        pslLogError("pslDoFilter", info_string, status);
        return status;
    }

    status = pslGetDefault("peaking_time", (void*) &pt, defaults);
    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting peaking_time for detChan %d", detChan);
        pslLogError("pslDoFilter", info_string, status);
        return status;
    }

    pslLogDebug("pslDoFilter", "Preparing to call pslUpdateFilter()");

    status = pslUpdateFilter(detChan, pt, defaults, firmwareSet, preampGain, m);
    if (status != XIA_SUCCESS) {
        pslLogError("pslDoFilter", "Error updating filter information", status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This routine updates the value of the parameter_t in the
 * defaults and then writes it to the board.
 */
PSL_STATIC int PSL_API pslDoParam(int detChan, char* name, void* value,
                                  XiaDefaults* defaults) {
    int status;

    unsigned short val = 0x0000;

    double dTmp = 0.0;

    dTmp = *((double*) value);
    val = (unsigned short) dTmp;

    status = pslSetDefault(name, (void*) &dTmp, defaults);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting %s for detChan %d", name, detChan);
        pslLogError("pslDoParam", info_string, status);
        return status;
    }

    status = pslSetParameter(detChan, name, val);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting '%s' to '%u'", name, val);
        pslLogError("pslDoParam", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Sets the gap time for the slow filter which,
 * in turn sets the SLOWGAP.
 */
ACQUISITION_VALUE(GapTime) {
    int status;

    double peakingTime;

    parameter_t SLOWGAP = 0;

    UNUSED(detectorType);
    UNUSED(detector_chan);
    UNUSED(det);

    sprintf(info_string, "gap_time = %.3f", *((double*) value));
    pslLogDebug("pslDoGapTime", info_string);

    status = pslSetDefault("gap_time", value, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting gap_time for detChan %d", detChan);
        pslLogError("pslDoGapTime", info_string, status);
        return status;
    }

    /* Refs #1998 Reset minimum_gap_time whenever gap_time is set
       Ignore errors here since minimum_gap_time may not exist */
    status = pslSetDefault("minimum_gap_time", value, defs);

    status = pslGetDefault("peaking_time", (void*) &peakingTime, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting peaking_time from detChan %d", detChan);
        pslLogError("pslDoGapTime", info_string, status);
        return status;
    }

    pslLogDebug("pslDoGapTime", "Preparing to call pslUpdateFilter()");

    /* Our dirty secret is that SLOWGAP is really changed in
     * pslUpdateFilter() since other filter params depend on
     * it as well.
     */
    status = pslUpdateFilter(detChan, peakingTime, defs, fs, preampGain, m);
    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error updating filter information for detChan %d",
                detChan);
        pslLogError("pslDoGapTime", info_string, status);
        return status;
    }

    status = pslGetDefault("actual_gap_time", value, defs);
    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting actual_gap_time for detChan %d", detChan);
        pslLogError("pslDoGapTime", info_string, status);
        return status;
    }

    sprintf(info_string, "SLOWGAP = %u", SLOWGAP);
    pslLogDebug("pslDoGapTime", info_string);

    sprintf(info_string, "New (actual) gap_time for detChan %d is %.3f microseconds",
            detChan, *((double*) value));
    pslLogDebug("pslDoGapTime", info_string);

    return XIA_SUCCESS;
}

/*
 * This routine translates the fast filter peaking time
 * to FASTLEN.
 */
ACQUISITION_VALUE(TriggerPeakingTime) {
    int status;

    UNUSED(m);
    UNUSED(detector_chan);
    UNUSED(det);
    UNUSED(preampGain);
    UNUSED(detectorType);
    UNUSED(fs);

    status = pslSetDefault("trigger_peaking_time", value, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting trigger_peaking_time for detChan %d",
                detChan);
        pslLogError("pslDoTriggerPeakingTime", info_string, status);
        return status;
    }

    status = pslUpdateTriggerFilter(detChan, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error updating trigger filter for detChan %d", detChan);
        pslLogError("pslDoTriggerPeakingTime", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Sets the trigger gap time.
 */
ACQUISITION_VALUE(TriggerGapTime) {
    int status;

    UNUSED(detector_chan);
    UNUSED(det);
    UNUSED(m);
    UNUSED(preampGain);
    UNUSED(fs);
    UNUSED(detectorType);

    status = pslSetDefault("trigger_gap_time", value, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting trigger_gap_time for detChan %d", detChan);
        pslLogError("pslDoTriggerGapTime", info_string, status);
        return status;
    }

    status = pslUpdateTriggerFilter(detChan, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error updating trigger filter for detChan %d", detChan);
        pslLogError("pslDoTriggerGapTime", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

PSL_STATIC int PSL_API pslUpdateTriggerFilter(int detChan, XiaDefaults* defaults) {
    int status;

    double triggerPT = 0.0;
    double triggerGap = 0.0;

    parameter_t FASTLEN = 0;
    parameter_t FASTGAP = 0;

    status = pslGetClockSpeed(detChan, &CLOCK_SPEED);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting clock speed for detChan %d", detChan);
        pslLogError("pslUpdateTriggerFilter", info_string, status);
        return status;
    }

    status = pslGetDefault("trigger_peaking_time", (void*) &triggerPT, defaults);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting trigger_peaking_time for detChan %d",
                detChan);
        pslLogError("pslUpdateTriggerFilter", info_string, status);
        return status;
    }

    status = pslGetDefault("trigger_gap_time", (void*) &triggerGap, defaults);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting trigger_gap_time for detChan %d", detChan);
        pslLogError("pslUpdateTriggerFilter", info_string, status);
        return status;
    }

    /* Enforce the following hardware limits for
     * optimal fast filter performance:
     * 2 < FASTLEN < 28
     * 3 < FASTGAP < 29
     * FASTLEN + FASTGAP < 31
     */
    FASTLEN = (parameter_t) ROUND((triggerPT / (1.0 / CLOCK_SPEED)));

    /* This is an error condition for the X10P */
    if ((FASTLEN < 2) || (FASTLEN > 28)) {
        status = XIA_FASTLEN_OOR;
        sprintf(info_string,
                "Calculated value of FASTLEN (%u) for detChan %d is out-of-range",
                FASTLEN, detChan);
        pslLogError("pslUpdateTriggerFilter", info_string, status);
        return status;
    }

    FASTGAP = (parameter_t) ceil((triggerGap / (1.0 / CLOCK_SPEED)));

    if (FASTGAP > 29) {
        status = XIA_FASTGAP_OOR;
        sprintf(info_string,
                "Calculated value of FASTGAP (%u) for detChan %d is out-of-range",
                FASTGAP, detChan);
        pslLogError("pslUpdateTriggerFilter", info_string, status);
        return status;
    }

    if ((FASTLEN + FASTGAP) > 31) {
        status = XIA_FASTFILTER_OOR;
        sprintf(info_string, "Calculated length of slow filter (%u) exceeds 31",
                (parameter_t) FASTLEN + FASTGAP);
        pslLogError("pslUpdateTriggerFilter", info_string, status);
        return status;
    }

    status = dxp_set_one_dspsymbol(&detChan, "FASTLEN", &FASTLEN);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error setting FASTLEN for detChan %d", detChan);
        pslLogError("pslUpdateTriggerFilter", info_string, status);
        return status;
    }

    status = dxp_set_one_dspsymbol(&detChan, "FASTGAP", &FASTGAP);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error setting FASTGAP for detChan %d", detChan);
        pslLogError("pslUpdateTriggerFilter", info_string, status);
        return status;
    }

    status = dxp_upload_dspparams(&detChan);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error uploading DSP params for detChan %d", detChan);
        pslLogError("pslUpdateTriggerFilter", info_string, status);
        return status;
    }

    /* Re-calculate peaking time and gap time due to
     * the fact that FASTLEN and FASTGAP are rounded.
     */
    triggerPT = (double) ((double) FASTLEN * (1.0 / CLOCK_SPEED));
    triggerGap = (double) ((double) FASTGAP * (1.0 / CLOCK_SPEED));

    status = pslSetDefault("trigger_peaking_time", (void*) &triggerPT, defaults);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting trigger_peaking_time for detChan %d",
                detChan);
        pslLogError("pslUpdateTriggerFilter", info_string, status);
        return status;
    }

    status = pslSetDefault("trigger_gap_time", (void*) &triggerGap, defaults);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting trigger_gap_time for detChan %d", detChan);
        pslLogError("pslUpdateTriggerFilter", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

PSL_STATIC unsigned int pslGetNumDefaults(void) {
    unsigned long i;

    unsigned int n = 0;

    for (i = 0, n = 0; i < N_ELEMS(ACQ_VALUES); i++) {
        if (ACQ_VALUES[i].isDefault) {
            n++;
        }
    }

    return n;
}

/*
 * This routine gets the number of DSP parameters
 * for the specified detChan.
 */
PSL_STATIC int PSL_API pslGetNumParams(int detChan, unsigned short* numParams) {
    int status;

    status = dxp_max_symbols(&detChan, numParams);
    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error getting number of DSP params for detChan %d",
                detChan);
        pslLogError("pslGetNumParams", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This routine returns the parameter data
 * requested. Assumes that the proper
 * amount of memory has been allocated for
 * value.
 */
PSL_STATIC int PSL_API pslGetParamData(int detChan, char* name, void* value) {
    int status;

    if (STREQ(name, "names")) {
        status = pslGetParamNames(&detChan, (char**) value);
    } else if (STREQ(name, "values")) {
        status =
            dxp_readout_detector_run(&detChan, (unsigned short*) value, NULL, NULL);
    } else if (STREQ(name, "access")) {
        status = dxp_symbolname_limits(&detChan, (unsigned short*) value, NULL, NULL);
    } else if (STREQ(name, "lower_bounds")) {
        status = dxp_symbolname_limits(&detChan, NULL, (unsigned short*) value, NULL);
    } else if (STREQ(name, "upper_bounds")) {
        status = dxp_symbolname_limits(&detChan, NULL, NULL, (unsigned short*) value);
    } else {
        status = XIA_BAD_NAME;
        sprintf(info_string, "%s is not a valid name argument", name);
        pslLogError("pslGetParamData", info_string, status);
        return status;
    }

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error getting DSP parameter data (%s) for detChan %d",
                name, detChan);
        pslLogError("pslGetParamData", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Routine to return a list of DSP symbol names to the user.
 *
 * The user must allocate the memory for the list of symbols and the
 * integer containing the number of symbols.  If dynamic allocation
 * is desired, then a call to dxp_num_symbols() should be made and
 * the memory allocated accordingly.  All symbols have a maximum size
 * of MAX_DSP_PARAM_NAME_LEN characters.
 */
PSL_STATIC int pslGetParamNames(int* detChan, char** list) {
    int status;

    unsigned short i;
    unsigned short numParams;

    status = dxp_max_symbols(detChan, &numParams);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error getting number of DSP params for detChan %d",
                *detChan);
        pslLogError("pslGetParamNames", info_string, status);
        return status;
    }

    if (list == NULL) {
        status = DXP_NOMEM;
        sprintf(info_string, "No Memory Allocated for symbolnames");
        pslLogError("pslGetParamNames", info_string, status);
        return status;
    }

    /* Copy the list of parameter names */
    for (i = 0; i < numParams; i++) {
        status = dxp_symbolname_by_index(detChan, &i, list[i]);

        if (status != DXP_SUCCESS) {
            sprintf(info_string,
                    "Error getting DSP parameter name at index %u"
                    " for detChan %d",
                    i, *detChan);
            pslLogError("pslGetParamNames", info_string, status);
            return status;
        }
    }

    return XIA_SUCCESS;
}

/*
 * This routine is mainly a wrapper around dxp_symbolname_by_index()
 * since VB can't pass a string array into a DLL and, therefore,
 * is unable to use pslGetParams() to retrieve the parameters list.
 */
PSL_STATIC int PSL_API pslGetParamName(int detChan, unsigned short index, char* name) {
    int status;

    status = dxp_symbolname_by_index(&detChan, &index, name);

    if (status != DXP_SUCCESS) {
        sprintf(info_string,
                "Error getting DSP parameter name at index %u for detChan %d", index,
                detChan);
        pslLogError("pslGetParamName", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Perform the specified gain operation to the hardware.
 */
PSL_STATIC int pslGainOperation(int detChan, char* name, void* value, Detector* det,
                                int modChan, Module* m, XiaDefaults* defs) {
    int status;
    unsigned long i;

    ASSERT(name != NULL);
    ASSERT(value != NULL);
    ASSERT(defs != NULL);
    ASSERT(det != NULL);
    ASSERT(m != NULL);

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

    sprintf(info_string, "Unknown gain operation '%s' for detChan %d", name, detChan);
    pslLogError("pslGainOperation", info_string, XIA_BAD_NAME);

    return XIA_BAD_NAME;
}

PSL_STATIC int pslBoardOperation(int detChan, char* name, void* value,
                                 XiaDefaults* defs) {
    UNUSED(detChan);
    UNUSED(name);
    UNUSED(value);
    UNUSED(defs);

    return XIA_SUCCESS;
}

/*
 * Calls the associated Xerxes exit routine as part of
 * the board-specific shutdown procedures.
 */
PSL_STATIC int PSL_API pslUnHook(int detChan) {
    int status;

    status = dxp_exit(&detChan);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error shutting down detChan %d", detChan);
        pslLogError("pslUnHook", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Tau Finder
 *
 * This routine will acquire an ADC trace and automatically fit the exponential
 * decay to determine the "correct" tauRC value.  This requires an initial guess
 * to be passed in for the tauRC value that is within an order of magnitude of
 * the correct value.
 */
PSL_STATIC int PSL_API pslTauFinder(int detChan, XiaDefaults* defaults,
                                    Detector* detector, int detector_chan,
                                    void* vInfo) {
    int status = XIA_SUCCESS;

    unsigned int* trace = NULL;

    double* ff = NULL;
    /* dt is the time between ADC Trace samples. */
    double dt;
    double threshold;
    double avg;
    double maxTimeDiff;
    /* used to determine which tau fit was best */
    double localAmplitude;
    double s1;
    double s0;
    double tau;
    double info[2] = {0.};
    double* dInfo = (double*) vInfo;
    double dTemp;

    /* fast filter parameters */
    parameter_t FL;
    parameter_t FG;

    boolean_t* trig = NULL;

    unsigned short* randomSet = NULL;

    unsigned long timeStamp[2048];
    unsigned long k;
    unsigned long kmin;
    unsigned long n;
    unsigned long tcount;
    unsigned long maxTimeIndex = 0;
    unsigned long tfcount;
    unsigned long ulTemp;
    unsigned long t0;
    unsigned long t1;
    unsigned long adcLength = 0;
    unsigned long t0Step;

    /* Convert tau value to seconds */
    tau = dInfo[2] / 1.0e6;

    info[0] = 1.0;
    info[1] = dInfo[1];

    /* Get the length of the ADC trace data */
    status =
        pslGetSpecialRunData(detChan, "adc_trace_length", (void*) &adcLength, defaults);
    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting ADC Trace Length for detchan %i", detChan);
        pslLogError("pslTauFinder", info_string, status);
        return status;
    }

    /* Get the fast filter peaking time, FL */
    status = dxp_get_one_dspsymbol(&detChan, "FASTLEN", &FL);
    if (status != DXP_SUCCESS) {
        pslLogError("pslTauFinder", "Error getting FASTLEN from XERXES", status);
        return status;
    }

    /* Get the fast filter gap time, FG */
    status = dxp_get_one_dspsymbol(&detChan, "FASTGAP", &FG);
    if (status != DXP_SUCCESS) {
        pslLogError("pslTauFinder", "Error getting FASTGAP from XERXES", status);
    }

    /* Allocate memory for filter simulations, trace, triggers, and random index set */
    trace =
        (unsigned int*) utils->funcs->dxp_md_alloc(adcLength * sizeof(unsigned int));
    trig = (boolean_t*) utils->funcs->dxp_md_alloc(adcLength * sizeof(boolean_t));
    ff = (double*) utils->funcs->dxp_md_alloc(adcLength * sizeof(double));
    randomSet =
        (unsigned short*) saturn_psl_md_alloc(adcLength * sizeof(unsigned short));

    /* Generate random indices, fills the randomSet list of indices (in random order) */
    pslRandomSwap(adcLength, randomSet);

    localAmplitude = 0;
    /* take a maximum of 10 traces */
    for (tfcount = 0; tfcount < 10; tfcount++) {
        /* Tell module to store an ADC trace */
        status = pslDoADCTrace(detChan, info);
        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error getting ADC Trace for detchan %i", detChan);
            pslLogError("pslTauFinder", info_string, status);
            utils->funcs->dxp_md_free(trace);
            utils->funcs->dxp_md_free(trig);
            utils->funcs->dxp_md_free(ff);
            utils->funcs->dxp_md_free(randomSet);
            return status;
        }

        /* Set value of deltaTime between ADC samples */
        dt = info[1] * 1.0e-9;

        /* Get the ADC trace from the module */
        status = pslGetSpecialRunData(detChan, "adc_trace", (void*) trace, defaults);
        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error getting ADC Trace for detchan %i", detChan);
            pslLogError("pslTauFinder", info_string, status);
            utils->funcs->dxp_md_free(trace);
            utils->funcs->dxp_md_free(trig);
            utils->funcs->dxp_md_free(ff);
            utils->funcs->dxp_md_free(randomSet);
            return status;
        }

        /* Find a good noise threshold for the trace
         *  This call will fill ff and ff2 with data as well. */
        threshold = pslThreshFinder(trace, tau, randomSet, dt, ff, FL, FG, adcLength);

        /* minimum starting point in the filter output is 2*FL+FG, since you don't have enough information
         * to properly determine the filter values prior to this point */
        kmin = 2 * FL + FG;

        /* Zero out all the triggers in the beginning of the filter */
        for (k = 0; k < kmin; k += 1)
            trig[k] = 0;

        /* Find average FF shift.  This value will be used to correct the
        * fast filter for the DC offset contribution that remains after
        * the exponential correction is made (1.0-exp(-tau/(FL+FG)))*DCOffset */
        avg = 0.0;
        n = 0;
        for (k = kmin; k < (adcLength - 1); k++) {
            if (ff[k + 1] - ff[k] < threshold) {
                avg += ff[k];
                n += 1;
            }
        }
        /* Determine the average */
        avg /= n;
        /* Subtract this average contribution from the filter, this should
         * bring the baseline close to 0 */
        for (k = kmin; k < (adcLength - 1); k++)
            ff[k] -= avg;

        /* If any entry in the fast filter is above threshold, set the trig[] value to be 1 */
        for (k = kmin; k < (adcLength - 1); k++)
            trig[k] = (boolean_t) (ff[k] > threshold);

        /* Zero out the number of triggers */
        tcount = 0;
        /* Record where the triggers occur */
        for (k = kmin; k < (adcLength - 1); k++) {
            /* Its a trigger if the next trig entry is TRUE_ and the current is FALSE_ */
            if (trig[k + 1] && !trig[k] && tcount < 2048) {
                timeStamp[tcount] = k + 2;
                tcount++;
            }
        }

        switch (tcount) {
            /* If there were no triggers, then go to the next iteration of the outer
         * loop (tfcount) */
            case 0:
                continue;
            /* One trigger leaves only 1 time interval (after the trigger) */
            case 1:
                t0 = timeStamp[0] + 2 * FL + FG;
                t1 = adcLength - 2;
                break;
            /* else find the maximum time interval for this trace */
            default:
                maxTimeDiff = 0.0;
                /* Loop over all triggers, tracking the trigger with the
             * longest interval after the trigger */
                for (k = 0; k < (tcount - 1); k += 1) {
                    ulTemp = timeStamp[k + 1] - timeStamp[k];
                    if (ulTemp > maxTimeDiff) {
                        maxTimeDiff = ulTemp;
                        maxTimeIndex = k;
                    }
                }
                /* Special check for the last trigger (to end of trace) */
                if ((adcLength - timeStamp[tcount - 1]) < maxTimeDiff) {
                    t0 = timeStamp[maxTimeIndex] + 2 * FL + FG;
                    t1 = timeStamp[maxTimeIndex + 1] - 1;
                } else {
                    t0 = timeStamp[tcount - 1] + 2 * FL + FG;
                    t1 = adcLength - 2;
                }
                break;
        }

        /* If the time difference is less than 3*tau, then try again */
        if (((t1 - t0) * dt) < (3.0 * tau))
            continue;

        /* Now we are set to do a fit */
        t0Step = t0 + ((unsigned long) ROUND(6.0 * tau / dt + 4.0));
        t1 = MIN(t1, t0Step);

        s0 = 0;
        s1 = 0;
        /* Determine the amplitude of the step (approximate).  s0 and s1 are
         * filter sums on either side of the step, but it is not really a
         * good measure of the energy of the step since we do not know what
         * the gap/risetime of the step is.  We are merely taking the amplitude
         * as the FL samples before the step and the FL samples that are
         * 2*FL+FG after the step.  Also remember that these samples are
         * much farther apart than the real fast filter (ADC sample times).
         */
        kmin = t0 - (2 * FL + FG) - FL - 1;
        for (k = 0; k < FL; k++) {
            s0 += trace[kmin + k];
            s1 += trace[t0 + k];
        }
        /* If this step is the largest yet, then fit.  Must be some
        * relationship between the quality of the fit and the step size. */
        if ((s1 - s0) / FL > localAmplitude) {
            dTemp = pslTauFit(trace, t0, t1, dt);
            if (dTemp == -1.0) {
                sprintf(
                    info_string,
                    "Search failed to find interval between 100ns and 100ms for detchan %i",
                    detChan);
                pslLogWarning("pslTauFinder", info_string);
            } else if (dTemp == -2.0) {
                sprintf(
                    info_string,
                    "Binary search failed to find small enough interval for detchan %i",
                    detChan);
                pslLogWarning("pslTauFinder", info_string);
            } else if (dTemp > 0.0) {
                /* Looks like a positive value for tau, assign it and try for another. */
                tau = dTemp;
                /* Update the local amplitude */
                localAmplitude = (s1 - s0) / FL;
            } else {
                sprintf(info_string, "Bad tau returned: tau = %f for detchan %i", tau,
                        detChan);
                pslLogWarning("pslTauFinder", info_string);
            }
        }
    }

    /* Convert the tau value to microseconds */
    tau *= 1.0e6;
    /* Return the updated value to the user */
    dInfo[2] = tau;

    /* Update the defaults list with the new value */
    status = pslDoDecayTime(detChan, &tau, NULL, NULL, defaults, 0.0, NULL, detector,
                            detector_chan);
    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Unable to set the Decay Time for detchan %i", detChan);
        pslLogError("pslTauFinder", info_string, status);
        utils->funcs->dxp_md_free(trace);
        utils->funcs->dxp_md_free(trig);
        utils->funcs->dxp_md_free(ff);
        utils->funcs->dxp_md_free(randomSet);
        return status;
    }

    /* Clean up memory */
    utils->funcs->dxp_md_free(trace);
    utils->funcs->dxp_md_free(trig);
    utils->funcs->dxp_md_free(ff);
    utils->funcs->dxp_md_free(randomSet);

    return status;
}

/*
 *   TauFit
 *
 * Perform the exponential + offset fit to the trace data between kmin and kmax where the
 * data have a separation in time of dt (used to take the ADC trace)
 * Searches from 100ns to 100ms for tau.
 */
PSL_STATIC double PSL_API pslTauFit(unsigned int* trace, unsigned long kmin,
                                    unsigned long kmax, double dt) {
    double mutop;
    double mubot;
    double valbot;
    double eps;
    double dmu;
    double mumid;
    double valmid;

    unsigned long count;

    /* The error for an acceptable fit */
    eps = 1e-3;
    /* begin the search at tau=100ns (=1 / 10e6) */
    mubot = 10.e6;

    /* Determine the value of Phi for the starting point */
    valbot = pslPhiValue(trace, exp(-mubot * dt), kmin, kmax);

    count = 0;
    /* start the binary search progression search */
    do {
        /* Save the last valbot value */
        mutop = mubot;

        /* Divide the mu value by 2 (multiply tau by 2) */
        mubot = mubot / 2.0;

        /* Determine the value of phi */
        valbot = pslPhiValue(trace, exp(-mubot * dt), kmin, kmax);

        count++;

        /* Geometric search did not find an enclosing interval
         * tau now = 2^20*100ns = 100ms, this is as large as we search. */
        if (count > 20) {
            return -1.;
        }

        /* Loop until the Phi value crosses zero */
    } while (valbot > 0);

    /* Step back one mu value to get the interval */
    count = 0;
    do {
        /* Do a binary search for tau */
        mumid = (mutop + mubot) / 2.0;

        /* Determine the phi for this point */
        valmid = pslPhiValue(trace, exp(-mumid * dt), kmin, kmax);

        /* Correct either the lower or upper value depending on sign of Phi */
        if (valmid > 0) {
            mutop = mumid;
        } else {
            mubot = mumid;
        }

        /* Determine the difference in mu from top to bottom */
        dmu = mutop - mubot;
        /* Increment the counter */
        count++;

        /* Binary search could not find small enough interval */
        if (count > 20) {
            return (-2.);
        }
        /* Continue to search till the difference in mu is small enough */
    } while (fabs(dmu / mubot) > eps);

    /* Return the fit value */
    return 1.0 / mutop;
}

/*
 *   PhiValue
 *
 * This routine will calculate the minimum Chi^2 value at a value of qq = exp(-mu*dt) for an
 * exponential fit + background, where mu = 1/tau:
 *
 * chi^2 = SUM [i=0;i=n-1] (data[i] - b - a*exp(-mu*dt*i))^2
 *
 * Now take d(chi^2)/d(a,b,mu) which yield 3 equations....we want to minimize these equations
 * so set this derivative to zero, which leaves 3 equations and 3 unknowns.  One can
 * solve for a and b in terms of mu...this leaves one equation that is not pretty.  The value
 * of this equation (which we want to equal 0) is the return value for this function.
 *
 * Note: the value of mu is tied into the value exp(-mu*dt) that is passed to the routine,
 * so it is rather hidden within this routine.  The values of a and b solved in this routine
 * are the same as described above.
 *
 * fk = SUM[i=0;i=n-1] (exp (-i*mu*dt))
 * f2k = SUM[i=0;i=n-1] (exp (-2*i*mu*dt))
 * s0 = SUM[i=0;i=n-1] data[i]
 * s1 = SUM[i=0;i=n-1] exp(-i*mu*dt) * data[i]
 * s2 = SUM[i=0;i=n-1] i * exp(-i*mu*dt) * data[i] / exp(-mu*dt)
 *
 * ek and dk are terms from the 3rd equation of the system with some sums worked out to
 * finite values.
 */

PSL_STATIC double PSL_API pslPhiValue(unsigned int* ydat, double qq, unsigned long kmin,
                                      unsigned long kmax) {
    unsigned long ndat;
    unsigned long k;

    double s0;
    double s1;
    double s2;
    double qp;
    double a;
    double b;
    double fk;
    double f2k;
    double dk;
    double ek;
    double val;

    /* total number of points in fit */
    ndat = kmax - kmin + 1;
    /* Initialize sums */
    s0 = 0;
    s1 = 0;
    s2 = 0;
    /* initialize the exp(-k*mu*dt) factors */
    qp = 1;

    for (k = kmin; k <= kmax; k += 1) {
        /* Sum of all the data points */
        s0 += ydat[k];
        /* Sum of all the data points * exp(-k*mu*dt) */
        s1 += qp * ydat[k];
        /* Sum of all the data points * k * exp(-k*mu*dt) / exp(-mu*dt) */
        s2 += qp * ydat[k] * (k - kmin) / qq;
        qp *= qq;
    }

    /* worked out sum of exp(-k*mu*dt) */
    fk = (1 - pow(qq, ndat)) / (1 - qq);
    /* worked out sum of exp(-2*k*mu*dt) */
    f2k = (1 - pow(qq, (2 * ndat))) / (1 - qq * qq);
    /* worked out sums for the 3rd equation for coefficient of a */
    dk = qq * (1 - pow(qq, (2 * ndat))) / pow((1 - qq * qq), 2) -
         ndat * pow(qq, (2 * ndat - 1)) / (1 - qq * qq);
    /* worked out sums for the 3rd equation for coefficient of b */
    ek = (1 - pow(qq, ndat)) / pow((1 - qq), 2) - ndat * pow(qq, ndat - 1) / (1 - qq);
    /* Original code, small bug */
    /*   dk = qq * (1 - pow(qq, (2 * ndat - 2))) / pow((1 - qq * qq), 2) -
    (ndat - 1) * pow(qq, (2 * ndat - 1)) / (1 - qq * qq);
    ek = (1 - pow(qq, (ndat - 1))) / pow((1 - qq), 2) - (ndat - 1) * pow(qq, (ndat - 1)) / (1 - qq);*/
    /* solution of one coefficient (amplitude) */
    a = (ndat * s1 - fk * s0) / (ndat * f2k - fk * fk);
    /* solution of other coefficient (offset) */
    b = (s0 - a * fk) / ndat;

    /* The 3rd equation */
    val = s2 - a * dk - b * ek;

    return val;
}

/*
 *   ThreshFinder
 */

PSL_STATIC double PSL_API pslThreshFinder(unsigned int* trace, double tau,
                                          unsigned short* randomSet, double adcDelay,
                                          double* ff, parameter_t FL, parameter_t FG,
                                          unsigned long adcLength) {
    unsigned long kmin;
    unsigned long j;
    unsigned long k;
    unsigned long ndev;
    unsigned long n;
    unsigned long m;

    double xx;
    double c0;
    double sum0;
    double sum1;
    double deviation;
    double threshold;
    double dTemp;

    ndev = 8;

    /* number of samples that depends on this tau and time between samples */
    xx = adcDelay / tau;
    /* Exponential constant for the decay of the trace (guess based on user
    * supplied tau */
    c0 = exp(-xx * ((double) (FL + FG)));

    /* Start of the filter does not have enough information to do any
    * calculations, so start far enough into filter. */
    kmin = 2 * FL + FG;

    /* zero out the initial part,where the true filter values are unknown */
    for (k = 0; k < kmin; k += 1) {
        ff[k] = 0;
    }

    /* Calculate the fast filter values for the trace */
    for (k = kmin; k < adcLength; k += 1) {
        sum0 = 0;
        sum1 = 0;
        for (n = 0; n < FL; n++) {
            /* First sum */
            sum0 += trace[k - kmin + n];
            /* Skip a gap and peaking time for 2nd sum */
            sum1 += trace[k - kmin + FL + FG + n];
        }
        /* Difference is the filter, corrected for the exponential decay (c0) */
        ff[k] = sum1 - sum0 * c0;
    }

    /* Determine the average difference between the fast filter values
    * Use a randomized ordering */
    deviation = 0;
    /* Skip every two s.t. every entry is only used once */
    for (k = 0; k < adcLength; k += 2)
        deviation += fabs(ff[randomSet[k]] - ff[randomSet[k + 1]]);

    /* Average out the deviations over the whole set */
    deviation /= (adcLength / 2);
    /* The initial threshold guess is half of nDev * deviation, just some generous threshold  */
    threshold = ndev / 2 * deviation / 2;

    /* Do this 3 times to remove all steps from contributing to the threshold. */
    for (j = 0; j < 3; j++) {
        /* Do it all again, this time only for the entries that are below threshold.  This
        * will cut out most of the steps in the data */
        m = 0;
        deviation = 0;
        for (k = 0; k < adcLength; k += 2) {
            dTemp = fabs(ff[randomSet[k]] - ff[randomSet[k + 1]]);
            if (dTemp < threshold) {
                m += 1;
                deviation += dTemp;
            }
        }
        /* Average the deviations */
        deviation /= m;
        /* Change to sigma */
        deviation *= sqrt(PI) / 2;
        /* nDev*sigma is the new threshold */
        threshold = ndev * deviation;
    }

    return threshold;
}

/*
 *   RandomSwap
 *
 * Produce an array of random indices of length adcLength
 */
PSL_STATIC void PSL_API pslRandomSwap(unsigned long adcLength,
                                      unsigned short* randomSet) {
    double rshift;

    unsigned long nCards;
    unsigned long mixLevel;
    unsigned long imin;
    unsigned long imax;
    unsigned long k;

    unsigned short a;

    /* Fill the randomSet array with indices */
    for (k = 0; k < adcLength; k++) {
        randomSet[k] = (unsigned short) k;
    }

    /* nCards and mixLevel tells the routine how many times to "shuffle" the array */
    nCards = adcLength;
    mixLevel = 5;
    /* Limit the random number produced to the range 0 to (adcLength-1) */
    rshift = ((double) adcLength - 1.) / ((double) (RAND_MAX));

    for (k = 0; k < (mixLevel * nCards); k++) {
        /* Generate 2 random numbers for the indices */
        imin = (unsigned long) ROUND(((double) rand()) * rshift);
        imax = (unsigned long) ROUND(((double) rand()) * rshift);

        /* Swap the 2 entries in randomSet */
        a = randomSet[imax];
        randomSet[imax] = randomSet[imin];
        randomSet[imin] = a;
    }

    return;
}

/*
 * Synchronize the number_of_scas acquisition value
 */
PSL_STATIC int psl__SynchNumberSCAs(int detChan, int det_chan, Module* m, Detector* det,
                                    XiaDefaults* defs) {
    int status;

    double nsca = 0.0;
    unsigned int modChan = 0;

    UNUSED(det_chan);
    UNUSED(det);

    ASSERT(defs != NULL);
    ASSERT(m != NULL);

    status = pslGetModChan(detChan, m, &modChan);
    ASSERT(status == XIA_SUCCESS);

    nsca = m->ch[modChan].n_sca;

    status = pslSetDefault("number_of_scas", (void*) &nsca, defs);
    ASSERT(status == XIA_SUCCESS);

    return XIA_SUCCESS;
}

/*
 * This routine sets the number of SCAs in the Module.
 *
 * Sets the number of SCAs in the module. Assumes that the calling \n
 * routine has checked that the passed in pointers are valid.
 *
 * - detChan detChan to set the number of SCAs for
 * - name Pointer to name of the acquisition value
 * - value Pointer to the number of SCAs. Will be cast into a double.
 * - m Pointer to the module where detChan is assigned.
 * - defaults Pointer to the defaults assigned to this detChan.
 */
PSL_STATIC int PSL_API _pslDoNSca(int detChan, char* name, void* value, Module* m,
                                  XiaDefaults* defaults) {
    int status;

    size_t nBytes = 0;

    double nSCA = *((double*) value);

    unsigned int modChan = 0;
    unsigned short i;
    char limit[9];
    XiaDaqEntry* e = NULL;

    ASSERT(value != NULL);
    ASSERT(m != NULL);

    if ((unsigned short) nSCA > MAX_NUM_INTERNAL_SCA) {
        sprintf(info_string,
                "Number of SCAs is greater then the maximum allowed "
                "%u for detChan %d",
                MAX_NUM_INTERNAL_SCA, detChan);
        pslLogError("_pslDoNSca", info_string, XIA_MAX_SCAS);
        return XIA_MAX_SCAS;
    }

    /* This is an assertion because the Module should
     * be derived from the detChan in Handel. If the
     * detChan isn't assigned to Module then we have
     * a serious failure.
     */
    status = pslGetModChan(detChan, m, &modChan);
    ASSERT(status == XIA_SUCCESS);

    /* If the number of SCAs shrank then we need to remove the limits
     * that are greater then the new number of SCAs.
     * This is a little hacky and will be improved in the future.
     */
    if ((unsigned short) nSCA < m->ch[modChan].n_sca) {
        for (i = (unsigned short) nSCA; i < m->ch[modChan].n_sca; i++) {
            sprintf(info_string, "Removing sca%hu_* limits for detChan %d", i, detChan);
            pslLogDebug("_pslDoNSca", info_string);

            sprintf(limit, "sca%hu_lo", i);

            status = pslRemoveDefault(limit, defaults, &e);

            if (status != XIA_SUCCESS) {
                sprintf(info_string,
                        "Unable to remove SCA limit '%s' for "
                        "detChan %d",
                        limit, detChan);
                pslLogWarning("_pslDoNSca", info_string);
            }

            /* pslRemoveDefault will not free the returned XiaDaqEntry */
            if (e != NULL) {
                saturn_psl_md_free(e->name);
                saturn_psl_md_free(e);
            }

            sprintf(limit, "sca%hu_hi", i);

            status = pslRemoveDefault(limit, defaults, &e);

            if (status != XIA_SUCCESS) {
                sprintf(info_string,
                        "Unable to remove SCA limit '%s' for "
                        "detChan %d",
                        limit, detChan);
                pslLogWarning("_pslDoNSca", info_string);
            }

            if (e != NULL) {
                saturn_psl_md_free(e->name);
                saturn_psl_md_free(e);
            }
        }
    }

    /* Clear existing SCAs to prevent a memory leak */
    if ((m->ch[modChan].sca_lo != NULL) || (m->ch[modChan].sca_hi != NULL)) {
        status = pslDestroySCAs(m, modChan);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error freeing SCAs in module '%s', detChan '%d'",
                    m->alias, detChan);
            pslLogError("_pslDoNSca", info_string, status);
            return status;
        }
    }

    m->ch[modChan].n_sca = (unsigned short) nSCA;

    /* Set the appropriate DSP parameter */
    status = pslSetParameter(detChan, "NUMSCA", (unsigned short) nSCA);

    if (status != XIA_SUCCESS) {
        m->ch[modChan].n_sca = 0;
        sprintf(info_string, "NUMSCA not available in loaded firmware for detChan %d",
                detChan);
        pslLogError("_pslDoNSca", info_string, XIA_MISSING_PARAM);
        return XIA_MISSING_PARAM;
    }

    status = pslSetDefault(name, value, defaults);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting default for '%s' to '%.3f'", name,
                *((double*) value));
        pslLogError("_pslDoNSCA", info_string, status);
        return status;
    }

    /* Initialize the SCA memory */
    if (nSCA > 0) {
        nBytes = m->ch[modChan].n_sca * sizeof(unsigned short);

        m->ch[modChan].sca_lo = (unsigned short*) saturn_psl_md_alloc(nBytes);

        if (m->ch[modChan].sca_lo == NULL) {
            /* Should n_sca be reset to 0 on failure? */
            sprintf(info_string, "Error allocating %zu bytes for m->ch[modChan].sca_lo",
                    nBytes);
            pslLogError("_psDoNSca", info_string, XIA_NOMEM);
            return XIA_NOMEM;
        }

        memset(m->ch[modChan].sca_lo, 0, nBytes);

        m->ch[modChan].sca_hi = (unsigned short*) saturn_psl_md_alloc(nBytes);

        if (m->ch[modChan].sca_hi == NULL) {
            saturn_psl_md_free(m->ch[modChan].sca_lo);
            sprintf(info_string, "Error allocating %zu bytes for m->ch[modChan].sca_hi",
                    nBytes);
            pslLogError("_pslDoNSca", info_string, XIA_NOMEM);
            return XIA_NOMEM;
        }

        memset(m->ch[modChan].sca_hi, 0, nBytes);
    }

    return XIA_SUCCESS;
}

/*
 * Sets the SCA value specified in name
 *
 * Expects that name is in the format: sca{n}_[lo|hi] \n
 * where 'n' is the SCA number.
 */
PSL_STATIC int PSL_API _pslDoSCA(int detChan, char* name, void* value, Module* m,
                                 XiaDefaults* defaults) {
    int status;
    int i;

    /* These need constants */
    char bound[3];
    char scaName[MAX_DSP_PARAM_NAME_LEN];

    unsigned short bin = (unsigned short) (*(double*) value);
    unsigned short scaNum = 0;

    unsigned int modChan = 0;

    ASSERT(name != NULL);
    ASSERT(value != NULL);
    ASSERT(STRNEQ(name, "sca"));

    sscanf(name, "sca%hu_%s", &scaNum, bound);

    if ((!STRNEQ(bound, "lo")) && (!STRNEQ(bound, "hi"))) {
        sprintf(info_string,
                "Malformed SCA string '%s': missing bounds term 'lo' or 'hi'", name);
        pslLogError("_pslDoSCA", info_string, XIA_BAD_NAME);
        return XIA_BAD_NAME;
    }

    status = pslGetModChan(detChan, m, &modChan);
    ASSERT(status == XIA_SUCCESS);

    if (scaNum >= m->ch[modChan].n_sca) {
        sprintf(info_string,
                "Requested SCA number '%u' is larger then the number of SCAs '%u'",
                scaNum, m->ch[modChan].n_sca);
        pslLogError("_pslDoSCA", info_string, XIA_SCA_OOR);
        return XIA_SCA_OOR;
    }

    for (i = 0; i < 3; i++) {
        bound[i] = (char) toupper((int) bound[i]);
    }

    /* Primitive bounds check here: If either of the values
     * ("lo"/"hi") are 0 then we assume that they are not
     * currently set yet. If they are > 0 then we do some
     * simple bounds checking.
     */
    if (STRNEQ(bound, "LO")) {
        if ((m->ch[modChan].sca_hi[scaNum] != 0) &&
            (bin > m->ch[modChan].sca_hi[scaNum])) {
            sprintf(info_string,
                    "New %s value '%u' is greater then the existing high value '%u'",
                    name, bin, m->ch[modChan].sca_hi[scaNum]);
            pslLogError("_pslDoSCA", info_string, XIA_BIN_MISMATCH);
            return XIA_BIN_MISMATCH;
        }
    } else if (STRNEQ(bound, "HI")) {
        if ((m->ch[modChan].sca_lo[scaNum] != 0) &&
            (bin < m->ch[modChan].sca_lo[scaNum])) {
            sprintf(info_string,
                    "New %s value '%u' is less then the existing low value '%u'", name,
                    bin, m->ch[modChan].sca_lo[scaNum]);
            pslLogError("_pslDoSCA", info_string, XIA_BIN_MISMATCH);
            return XIA_BIN_MISMATCH;
        }
    } else {
        /* This is an impossible condition */
        FAIL();
    }

    /* Create the proper DSP parameter to write */
    sprintf(scaName, "SCA%u%s", scaNum, bound);

    status = pslSetParameter(detChan, scaName, bin);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Unable to set SCA '%s'", scaName);
        pslLogError("_pslDoSCA", info_string, status);
        return status;
    }

    if (STRNEQ(bound, "LO")) {
        m->ch[modChan].sca_lo[scaNum] = bin;
    } else if (STRNEQ(bound, "HI")) {
        m->ch[modChan].sca_hi[scaNum] = bin;
    } else {
        FAIL();
    }

    status = pslSetDefault(name, value, defaults);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting default for '%s' to '%.3f'", name,
                *((double*) value));
        pslLogError("_pslDoSCA", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Get the maximum allowed number of SCA
 */
PSL_STATIC int psl__GetMaxSCALength(int detChan, void* value, XiaDefaults* defaults) {
    UNUSED(detChan);
    UNUSED(defaults);

    *((unsigned short*) value) = (unsigned short) MAX_NUM_INTERNAL_SCA;

    return XIA_SUCCESS;
}

/*
 * Get the length of the SCA data buffer
 */
PSL_STATIC int psl__GetSCALength(int detChan, void* value, XiaDefaults* defs) {
    int status;

    double nSCAs = 0.0;

    unsigned short* scaLen = (unsigned short*) value;

    ASSERT(value != NULL);
    ASSERT(defs != NULL);

    status = pslGetDefault("number_of_scas", (void*) &nSCAs, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error finding 'number_of_scas' for detChan '%d'",
                detChan);
        pslLogError("psl__GetSCALength", info_string, status);
        return status;
    }

    *scaLen = (unsigned short) nSCAs;

    return XIA_SUCCESS;
}

/*
 * Gets the SCA Data buffer from Xerxes
 */
PSL_STATIC int psl__GetSCAData(int detChan, void* value, XiaDefaults* defs) {
    int status;
    int i;
    int j;

    double nSCA = 0.0;
    size_t totalSCA = 0;
    unsigned long* sca = NULL;

    unsigned long* userSCA = (unsigned long*) value;
    unsigned long addr = 0;
    char memStr[64];

    parameter_t SCADSTART;

    ASSERT(value != NULL);
    ASSERT(defs != NULL);

    status = pslGetDefault("number_of_scas", (void*) &nSCA, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error finding 'number_of_scas' for detChan '%d'",
                detChan);
        pslLogError("psl__GetSCAData", info_string, status);
        return status;
    }

    if (nSCA == 0.0) {
        sprintf(info_string, "No SCAs defined for detChan = %d", detChan);
        pslLogError("psl__GetSCAData", info_string, DXP_NO_SCA);
        return DXP_NO_SCA;
    }

    status = pslGetParameter(detChan, "SCADSTART", &SCADSTART);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error reading SCA memory location for detChan '%d'",
                detChan);
        pslLogError("psl__GetSCAData", info_string, status);
    }

    addr = (unsigned long) SCADSTART;

    sprintf(info_string, "Reading out %zu SCA value: addr = %#lx", (size_t) nSCA, addr);
    pslLogDebug("psl__GetSCAData", info_string);

    totalSCA = (size_t) nSCA * 2;
    sca = (unsigned long*) saturn_psl_md_alloc(totalSCA * sizeof(unsigned long));

    if (!sca) {
        sprintf(info_string, "Error allocating %zu bytes for 'sca'",
                totalSCA * sizeof(unsigned long));
        pslLogError("psl__GetSCAData", info_string, XIA_NOMEM);
        return XIA_NOMEM;
    }

    sprintf(memStr, "data:%#lx:%lu", addr, (unsigned long) totalSCA);
    status = dxp_read_memory(&detChan, memStr, sca);

    if (status != DXP_SUCCESS) {
        saturn_psl_md_free(sca);
        sprintf(info_string, "Error reading sca value from memory %s for detChan %d",
                memStr, detChan);
        pslLogError("psl__GetSCAData", info_string, status);
        return status;
    }

    /* the data readout from dxp_read_memory are padded ushorts
     * need to stitch them back into ulong
     */
    for (i = 0, j = 0; i < (int) nSCA * 2; i += 2, j++) {
        userSCA[j] = sca[i] + (sca[i + 1] << 16);
        sprintf(info_string, "%d, SCA %lu hi %#lx lo %#lx", j, userSCA[j], sca[i + 1],
                sca[i]);
        pslLogDebug("psl__GetSCAData", info_string);
    }

    saturn_psl_md_free(sca);
    return XIA_SUCCESS;
}

/*
 * Synchronize the reset delay acquisition value with the
 * Detector structure.
 */
PSL_STATIC int pslSynchResetDelay(int detChan, int det_chan, Module* m, Detector* det,
                                  XiaDefaults* defs) {
    int status;

    double resetDelay = 0.0;

    UNUSED(m);
    UNUSED(detChan);

    ASSERT(det != NULL);
    ASSERT(defs != NULL);

    if (det->type == XIA_DET_RESET) {
        resetDelay = det->typeValue[det_chan];

        status = pslSetDefault("reset_delay", (void*) &resetDelay, defs);
        ASSERT(status == XIA_SUCCESS);
    }

    return XIA_SUCCESS;
}

/*
 * Synchronize the decay time acquisition value with the Detector
 * structure.
 */
PSL_STATIC int pslSynchDecayTime(int detChan, int det_chan, Module* m, Detector* det,
                                 XiaDefaults* defs) {
    int status;

    double decayTime = 0.0;

    UNUSED(m);
    UNUSED(detChan);

    ASSERT(det != NULL);
    ASSERT(defs != NULL);

    if (det->type == XIA_DET_RCFEED) {
        decayTime = det->typeValue[det_chan];

        status = pslSetDefault("decay_time", (void*) &decayTime, defs);
        ASSERT(status == XIA_SUCCESS);
    }

    return XIA_SUCCESS;
}

/*
 * Synchronize the polarity acquisition value with the Detector
 * structure.
 */
PSL_STATIC int pslSynchPolarity(int detChan, int det_chan, Module* m, Detector* det,
                                XiaDefaults* defs) {
    int status;

    double pol = 0.0;

    UNUSED(m);
    UNUSED(detChan);

    ASSERT(det != NULL);
    ASSERT(defs != NULL);

    pol = (double) det->polarity[det_chan];

    status = pslSetDefault("detector_polarity", (void*) &pol, defs);
    ASSERT(status == XIA_SUCCESS);

    return XIA_SUCCESS;
}

/*
 * Synchronize the preamplifier gain acquistion value with the
 * Detector structure.
 */
PSL_STATIC int pslSynchPreampGain(int detChan, int det_chan, Module* m, Detector* det,
                                  XiaDefaults* defs) {
    int status;

    double gain = 0.0;

    UNUSED(m);
    UNUSED(detChan);

    ASSERT(det != NULL);
    ASSERT(defs != NULL);

    gain = det->gain[det_chan];

    status = pslSetDefault("preamp_gain", (void*) &gain, defs);
    ASSERT(status == XIA_SUCCESS);

    return XIA_SUCCESS;
}

/*
 * This routine is provided fro compatibility with the acquisition
 * values list ACQ_VALUES.
 */
ACQUISITION_VALUE(ActualGapTime) {
    UNUSED(detector_chan);
    UNUSED(det);
    UNUSED(m);
    UNUSED(preampGain);
    UNUSED(defs);
    UNUSED(detectorType);
    UNUSED(fs);
    UNUSED(value);
    UNUSED(detChan);

    /* Do Nothing */

    return XIA_SUCCESS;
}

/*
 * Set a preset run of the specified type.
 * To set the value that the preset run should use to determine when the
 * run is over, see psl__SetPresetValue().
 */
PSL_STATIC int psl__SetPresetType(int detChan, void* value, FirmwareSet* fs,
                                  char* detType, XiaDefaults* defs, double preampGain,
                                  Module* m, Detector* det, int detector_chan) {
    int status;

    parameter_t PRESET;

    UNUSED(fs);
    UNUSED(detType);
    UNUSED(preampGain);
    UNUSED(m);
    UNUSED(det);
    UNUSED(detector_chan);

    ASSERT(value != NULL);

    PRESET = (parameter_t) (*((double*) value));

    if (PRESET > (parameter_t) XIA_PRESET_FIXED_TRIGGERS) {
        sprintf(info_string,
                "Preset type '%u' is not a valid run type for "
                "detChan %d",
                PRESET, detChan);
        pslLogError("psl__SetPresetType", info_string, XIA_UNKNOWN_PRESET);
        return XIA_UNKNOWN_PRESET;
    }

    status = pslSetParameter(detChan, "PRESET", PRESET);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting preset type to '%u' for detChan %d", PRESET,
                detChan);
        pslLogError("psl__SetPresetType", info_string, status);
        return status;
    }

    status = pslSetDefault("preset_type", value, defs);
    ASSERT(status == XIA_SUCCESS);

    return XIA_SUCCESS;
}

/*
 * Set the run length based on the current preset run type defined.
 */
PSL_STATIC int psl__SetPresetValue(int detChan, void* value, FirmwareSet* fs,
                                   char* detType, XiaDefaults* defs, double preampGain,
                                   Module* m, Detector* det, int detector_chan) {
    int status;
    double clock_speed;
    double presetTick;

    parameter_t PRESET;
    parameter_t PRESETLEN0;
    parameter_t PRESETLEN1;

    unsigned long len = 0;

    UNUSED(fs);
    UNUSED(detType);
    UNUSED(preampGain);
    UNUSED(m);
    UNUSED(det);
    UNUSED(detector_chan);

    ASSERT(value != NULL);

    status = pslGetClockSpeed(detChan, &clock_speed);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting clock speed for detChan %d", detChan);
        pslLogError("psl__SetPresetValue", info_string, status);
        return status;
    }

    presetTick = 16.0 / (clock_speed * 1.0e6);

    /* The preset length is stored in a 32-bit value (2 x 16-bit words). */
    if (*((double*) value) > (ldexp(1.0, 32) - 1)) {
        sprintf(info_string,
                "Requested preset time/counts (%0.1f) is greater then "
                "the maximum allowed value for detChan %d",
                *((double*) value), detChan);
        pslLogError("psl__SetPresetValue", info_string, XIA_PRESET_VALUE_OOR);
        return XIA_PRESET_VALUE_OOR;
    }

    status = pslGetParameter(detChan, "PRESET", &PRESET);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting current preset run type for detChan %d",
                detChan);
        pslLogError("psl__SetPresetValue", info_string, status);
        return status;
    }

    switch (PRESET) {
        case (parameter_t) XIA_PRESET_NONE:
            /* There is nothing to set if we are doing a standard run. */
            return XIA_SUCCESS;
        case (parameter_t) XIA_PRESET_FIXED_REAL:
        case (parameter_t) XIA_PRESET_FIXED_LIVE:
            len = (unsigned long) (*((double*) value) / presetTick);
            break;
        case (parameter_t) XIA_PRESET_FIXED_EVENTS:
        case (parameter_t) XIA_PRESET_FIXED_TRIGGERS:
            len = (unsigned long) (*((double*) value));
            break;
        default:
            FAIL();
            break;
    }

    /* On the Saturn, PRESETLEN0 is actually the high word. */
    PRESETLEN1 = (parameter_t) (len & 0xFFFF);
    PRESETLEN0 = (parameter_t) ((len >> 16) & 0xFFFF);

    status = pslSetParameter(detChan, "PRESETLEN0", PRESETLEN0);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error setting high-word for preset run length on "
                "detChan %d",
                detChan);
        pslLogError("psl__SetPresetValue", info_string, status);
        return status;
    }

    status = pslSetParameter(detChan, "PRESETLEN1", PRESETLEN1);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error setting low-word for preset run length on "
                "detChan %d",
                detChan);
        pslLogError("psl__SetPresetValue", info_string, status);
        return status;
    }

    status = pslSetDefault("preset_value", value, defs);
    ASSERT(status == XIA_SUCCESS);

    return XIA_SUCCESS;
}

/*
 * The start address in the DSP of the MCA spectrum buffer.
 *
 * Note: This acquisition values is read-only and is completely managed by
 * Handel. Setting this value manually will have no effect. It is stored in
 * the acquisition values list merely to speed up the MCA readout times.
 */
PSL_STATIC int psl__SetMCAStartAddress(int detChan, void* value, FirmwareSet* fs,
                                       char* detType, XiaDefaults* defs,
                                       double preampGain, Module* m, Detector* det,
                                       int detector_chan) {
    UNUSED(detChan);
    UNUSED(value);
    UNUSED(fs);
    UNUSED(detType);
    UNUSED(defs);
    UNUSED(preampGain);
    UNUSED(m);
    UNUSED(det);
    UNUSED(detector_chan);

    return XIA_SUCCESS;
}

/*
 * Updates the MCA start address cache.
 */
PSL_STATIC int psl__UpdateMCAAddressCache(int detChan, XiaDefaults* defs) {
    int status;

    parameter_t SPECTSTART;

    double spectStart;

    ASSERT(defs != NULL);

    status = pslGetParameter(detChan, "SPECTSTART", &SPECTSTART);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error getting MCA buffer start address for "
                "detChan %d",
                detChan);
        pslLogError("psl__UpdateMCAAddressCache", info_string, status);
        return status;
    }

    spectStart = (double) SPECTSTART;

    status = pslSetDefault("mca_start_address", &spectStart, defs);
    ASSERT(status == XIA_SUCCESS);

    sprintf(info_string, "Updated MCA start address cache to %#x for detChan %d",
            SPECTSTART, detChan);
    pslLogInfo("psl__UpdateMCAAddressCache", info_string);

    return XIA_SUCCESS;
}

/*
 * Set the baseline threshold.
 */
PSL_STATIC int psl__SetBThresh(int detChan, void* value, FirmwareSet* fs, char* detType,
                               XiaDefaults* defs, double preampGain, Module* m,
                               Detector* det, int detector_chan) {
    int status;

    double eVPerADC = 0.0;
    double dBASETHRESH = 0.0;
    double basethreshEV = 0.0;
    double disableAutoT = 0.0;

    parameter_t SLOWLEN = 0;
    parameter_t BASETHRESH = 0;

    UNUSED(m);
    UNUSED(det);
    UNUSED(fs);
    UNUSED(detType);
    UNUSED(preampGain);
    UNUSED(detector_chan);

    ASSERT(value != NULL);
    ASSERT(defs != NULL);

    /* Revert RUNTASK bit if the value passed in is 0. */
    basethreshEV = *((double*) value);

    if (basethreshEV == 0.0) {
        disableAutoT = 0.0;
    } else {
        disableAutoT = 1.0;
    }

    status = psl__SetRunTasks(detChan, &disableAutoT, DISABLE_AUTOT);
    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting RUNTASK to %d for detChan %d",
                (int) disableAutoT, detChan);
        pslLogError("psl__SetBThresh", info_string, status);
        return status;
    }

    status = psl__GetEVPerADC(defs, &eVPerADC);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting eV/ADC for detChan %d", detChan);
        pslLogError("psl__SetBThresh", info_string, status);
        return status;
    }

    status = dxp_get_one_dspsymbol(&detChan, "SLOWLEN", &SLOWLEN);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error getting SLOWLEN from detChan %d", detChan);
        pslLogError("psl__SetBThresh", info_string, status);
        return status;
    }

    dBASETHRESH = ((double) SLOWLEN * basethreshEV) / eVPerADC;
    BASETHRESH = (parameter_t) ROUND(dBASETHRESH);

    status = dxp_set_one_dspsymbol(&detChan, "BASETHRESH", &BASETHRESH);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error getting BASETHRESH from detChan %d", detChan);
        pslLogError("psl__SetBThresh", info_string, status);
        return status;
    }

    /* Re-"calculate" the actual threshold. This _is_ a deterministic process since the
     * specified value of the threshold is only modified due to rounding...
     */
    basethreshEV = ((double) BASETHRESH * eVPerADC) / ((double) SLOWLEN);
    *((double*) value) = basethreshEV;

    status = pslSetDefault("baseline_threshold", (void*) &basethreshEV, defs);
    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting baseline_threshold for detChan %d",
                detChan);
        pslLogError("psl__SetBThresh", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Read modify given bit in run task
 */
PSL_STATIC int psl__SetRunTasks(int detChan, void* value, int bit) {
    int status = 0;

    parameter_t RUNTASKS = 0;

    ASSERT(value != NULL);

    /* Set the proper bit of the RUNTASKS DSP parameter */
    /* First retrieve RUNTASKS from the DSP */
    status = dxp_get_one_dspsymbol(&detChan, "RUNTASKS", &RUNTASKS);
    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error getting RUNTASKS for detChan %d", detChan);
        pslLogError("psl__SetRunTasks", info_string, status);
        return status;
    }

    /* Set/reset the bit (masks are in saturn.h) */
    if (*((double*) value) == 1.0) {
        RUNTASKS |= (0x1 << bit);
    } else {
        RUNTASKS &= ~(0x1 << bit);
    }

    /* Finally write RUNTASKS back to the DSP */
    status = dxp_set_one_dspsymbol(&detChan, "RUNTASKS", &RUNTASKS);
    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error writing RUNTASKS for detChan %d", detChan);
        pslLogError("psl__SetRunTasks", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Calculate the eV/ADC value using the specified acquisition values
 */
PSL_STATIC int psl__GetEVPerADC(XiaDefaults* defs, double* eVPerADC) {
    int status;

    double calibEV = 0.0;
    double adcPercentRule = 0.0;

    ASSERT(defs != NULL);
    ASSERT(eVPerADC != NULL);

    status = pslGetDefault("adc_percent_rule", (void*) &adcPercentRule, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting adc_percent_rule.");
        pslLogError("psl__GetEVPerADC", info_string, status);
        return status;
    }

    status = pslGetDefault("calibration_energy", (void*) &calibEV, defs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting calibration_energy.");
        pslLogError("psl__GetEVPerADC", info_string, status);
        return status;
    }

    *eVPerADC = (double) (calibEV / ((adcPercentRule / 100.0) * NUM_BITS_ADC));

    /* *eVPerADC = calibEV / ((adcRule / 100.0) * ADC_RANGE); */

    return XIA_SUCCESS;
}

/*
 * Set the maximum width of the trigger filter pile-up inspection
 */
PSL_STATIC int psl__SetMaxWidth(int detChan, void* value, FirmwareSet* fs,
                                char* detType, XiaDefaults* defs, double preampGain,
                                Module* m, Detector* det, int detector_chan) {
    int status;

    parameter_t MAXWIDTH = 0;
    double maxWidth = 0;

    double clockSpeed;
    double clockTick;

    UNUSED(fs);
    UNUSED(det);
    UNUSED(m);
    UNUSED(detType);
    UNUSED(detector_chan);
    UNUSED(preampGain);

    ASSERT(value != NULL);

    status = pslGetClockSpeed(detChan, &clockSpeed);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting clock speed for detChan %d", detChan);
        pslLogError("psl__SetMaxWidth", info_string, status);
        return status;
    }

    clockTick = (1.0 / clockSpeed);
    MAXWIDTH = (parameter_t) ROUND(*((double*) value) / clockTick);

    if (MAXWIDTH < MIN_MAXWIDTH || MAXWIDTH > MAX_MAXWIDTH) {
        sprintf(info_string,
                "Requested max. width (%0.3f microseconds) is "
                "out-of-range (%0.3f, %0.3f) for detChan %d",
                *((double*) value), MIN_MAXWIDTH * clockTick, MAX_MAXWIDTH * clockTick,
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

    maxWidth = (double) MAXWIDTH * clockTick;
    *((double*) value) = maxWidth;

    status = pslSetDefault("maxwidth", (void*) &maxWidth, defs);
    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error setting maxwidth for detChan %d", detChan);
        pslLogError("psl__SetMaxWidth", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Set the minimum gap time for the slow filter.
 */
PSL_STATIC int psl__SetMinGapTime(int detChan, void* value, FirmwareSet* fs,
                                  char* detType, XiaDefaults* defs, double preampGain,
                                  Module* m, Detector* det, int detector_chan) {
    int status;

    double pt = 0.0;

    UNUSED(det);
    UNUSED(detType);
    UNUSED(detector_chan);

    ASSERT(value != NULL);

    status = pslSetDefault("minimum_gap_time", (void*) value, defs);

    /* Refs #1998 Reset gap_time whenever minimum_gap_time is set
     * To maintain backward compatibility, i.e. support for gap_time acq value
     */
    status = pslSetDefault("gap_time", (void*) value, defs);

    /* It feels a little odd to be pulling the peaking time out here, just to pass
    * it into a function that could it pull it out itself.
    */
    status = pslGetDefault("peaking_time", (void*) &pt, defs);
    ASSERT(status == XIA_SUCCESS);

    status = pslUpdateFilter(detChan, pt, defs, fs, preampGain, m);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error updating filter parameters after changing the "
                "slow filter minimum gap time for detChan %d",
                detChan);
        pslLogError("psl__SetMinGapTime", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Returns all of the statistics for the Saturn in a single array. @a
 * value is expected to be a double array capable of holding 7 values returned
 * in the following format:
 *
 * [runtime, trigger_livetime, energy_livetime, triggers, events, icr,
 * ocr]
 */
PSL_STATIC int psl__GetModuleStatistics(int detChan, void* value, XiaDefaults* defs) {
    int status;

    double* stats = NULL;

    unsigned long evts;
    unsigned long trigs;
    unsigned long unders;
    unsigned long overs;

    unsigned long dspParams[256];

    UNUSED(defs);

    ASSERT(value);

    pslLogWarning("psl__GetModuleStatistics",
                  "The current form of "
                  "'module_statistics' is deprecated and will be replaced "
                  "by the version in 'module_statistics_2' in the next "
                  "release.");

    stats = (double*) value;

    status = psl__GetDSPBlock(detChan, dspParams);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error getting all DSP parameters for detChan "
                "%d.",
                detChan);
        pslLogError("psl__GetModuleStatistics", info_string, status);
        return status;
    }

    status = psl__ExtractRealtime(detChan, dspParams, &stats[0]);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error extracting the realtime for detChan %d.", detChan);
        pslLogError("psl__GetModuleStatistics", info_string, status);
        return status;
    }

    status = psl__ExtractTLivetime(detChan, dspParams, &stats[1]);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error extracting the trigger livetime from the "
                "DSP parameter block for detChan %d.",
                detChan);
        pslLogError("psl__GetModuleStatistics", info_string, status);
        return status;
    }

    status = psl__ExtractELivetime(detChan, dspParams, &stats[2]);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error extracting the energy livetime from the "
                "DSP parameter block for detChan %d.",
                detChan);
        pslLogError("psl__GetModuleStatistics", info_string, status);
        return status;
    }

    status = psl__ExtractTriggers(detChan, dspParams, &trigs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error extracting the triggers from the "
                "DSP parameter block for detChan %d.",
                detChan);
        pslLogError("psl__GetModuleStatistics", info_string, status);
        return status;
    }

    stats[3] = (double) trigs;

    status = psl__ExtractEvents(detChan, dspParams, &evts);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error extracting the MCA event count from the "
                "DSP parameter block for detChan %d.",
                detChan);
        pslLogError("psl__GetModuleStatistics", info_string, status);
        return status;
    }

    stats[4] = (double) evts;

    status = psl__ExtractUnders(detChan, dspParams, &unders);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error extracting the underflows from the "
                "DSP parameter block for detChan %d.",
                detChan);
        pslLogError("psl__GetModuleStatistics", info_string, status);
        return status;
    }

    status = psl__ExtractOvers(detChan, dspParams, &overs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error extracting the overflows from the "
                "DSP parameter block for detChan %d.",
                detChan);
        pslLogError("psl__GetModuleStatistics", info_string, status);
        return status;
    }

    if (stats[1] > 0.0) {
        stats[5] = stats[3] / stats[1];
    } else {
        stats[5] = 0.0;
    }

    if (stats[0] > 0.0) {
        stats[6] = (stats[4] + unders + overs) / stats[0];
    } else {
        stats[6] = 0.0;
    }

    return XIA_SUCCESS;
}

PSL_STATIC int psl__GetELivetime(int detChan, void* value, XiaDefaults* defs) {
    int status;

    unsigned long dspParams[DSP_PARAM_MEM_LEN];

    UNUSED(defs);

    ASSERT(value);

    status = psl__GetDSPBlock(detChan, dspParams);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error getting all DSP parameters for detChan "
                "%d.",
                detChan);
        pslLogError("psl__GetELivetime", info_string, status);
        return status;
    }

    status = psl__ExtractELivetime(detChan, dspParams, (double*) value);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error extracting the energy livetime from the "
                "DSP parameter block for detChan %d.",
                detChan);
        pslLogError("psl__GetELivetime", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Returns all of the statistics for the Saturn in a single array. @a
 * value is expected to be a double array capable of holding 9 values returned
 * in the following format:
 *
 * [runtime, trigger_livetime, energy_livetime, triggers, events, icr,
 * ocr, underflows, overflows]
 */
PSL_STATIC int psl__GetModuleStatistics2(int detChan, void* value, XiaDefaults* defs) {
    int status;

    unsigned long trigs;
    unsigned long evts;
    unsigned long unders;
    unsigned long overs;

    double* stats;

    unsigned long dspParams[DSP_PARAM_MEM_LEN];

    UNUSED(defs);

    ASSERT(value);

    stats = (double*) value;

    status = psl__GetDSPBlock(detChan, dspParams);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error getting all DSP parameters for detChan "
                "%d.",
                detChan);
        pslLogError("psl__GetModuleStatistics2", info_string, status);
        return status;
    }

    status = psl__ExtractRealtime(detChan, dspParams, &stats[0]);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error extracting the realtime from the DSP "
                "parameter block for detChan %d.",
                detChan);
        pslLogError("psl__GetModuleStatistics2", info_string, status);
        return status;
    }

    status = psl__ExtractTLivetime(detChan, dspParams, &stats[1]);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error extracting the trigger livetime from the "
                "DSP parameter block for detChan %d.",
                detChan);
        pslLogError("psl__GetModuleStatistics2", info_string, status);
        return status;
    }

    status = psl__ExtractELivetime(detChan, dspParams, &stats[2]);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error extracting the energy livetime from the "
                "DSP parameter block for detChan %d.",
                detChan);
        pslLogError("psl__GetModuleStatistics2", info_string, status);
        return status;
    }

    status = psl__ExtractTriggers(detChan, dspParams, &trigs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error extracting the triggers from the "
                "DSP parameter block for detChan %d.",
                detChan);
        pslLogError("psl__GetModuleStatistics2", info_string, status);
        return status;
    }

    stats[3] = (double) trigs;

    status = psl__ExtractEvents(detChan, dspParams, &evts);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error extracting the MCA event count from the "
                "DSP parameter block for detChan %d.",
                detChan);
        pslLogError("psl__GetModuleStatistics2", info_string, status);
        return status;
    }

    stats[4] = (double) evts;

    status = psl__ExtractUnders(detChan, dspParams, &unders);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error extracting the underflows from the "
                "DSP parameter block for detChan %d.",
                detChan);
        pslLogError("psl__GetModuleStatistics2", info_string, status);
        return status;
    }

    stats[7] = (double) unders;

    status = psl__ExtractOvers(detChan, dspParams, &overs);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error extracting the overflows from the "
                "DSP parameter block for detChan %d.",
                detChan);
        pslLogError("psl__GetModuleStatistics2", info_string, status);
        return status;
    }

    stats[8] = (double) overs;

    /* ICR */
    if (stats[1] > 0.0) {
        stats[5] = stats[3] / stats[1];
    } else {
        stats[5] = 0.0;
    }

    /* OCR */
    if (stats[0] > 0.0) {
        stats[6] = (stats[4] + stats[7] + stats[8]) / stats[0];
    } else {
        stats[6] = 0.0;
    }

    return XIA_SUCCESS;
}

/*
 * Returns the current # of underflow events in value as an
 * unsigned long.
 */
PSL_STATIC int psl__GetUnderflows(int detChan, void* value, XiaDefaults* defs) {
    int status;

    unsigned long dspParams[DSP_PARAM_MEM_LEN];

    UNUSED(defs);

    ASSERT(value);

    status = psl__GetDSPBlock(detChan, dspParams);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error getting all DSP parameters for detChan "
                "%d.",
                detChan);
        pslLogError("psl__GetUnderflows", info_string, status);
        return status;
    }

    status = psl__ExtractUnders(detChan, dspParams, (unsigned long*) value);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error extracting the underflows from the DSP "
                "parameter block for detChan %d.",
                detChan);
        pslLogError("psl__GetUnderflows", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Returns the current # of overflow events in value as an
 * unsigned long.
 */
PSL_STATIC int psl__GetOverflows(int detChan, void* value, XiaDefaults* defs) {
    int status;

    unsigned long dspParams[DSP_PARAM_MEM_LEN];

    UNUSED(defs);

    ASSERT(value);

    status = psl__GetDSPBlock(detChan, dspParams);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error getting all DSP parameters for detChan "
                "%d.",
                detChan);
        pslLogError("psl__GetOverflows", info_string, status);
        return status;
    }

    status = psl__ExtractOvers(detChan, dspParams, (unsigned long*) value);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error extracting the overflows from the DSP "
                "parameter block for detChan %d.",
                detChan);
        pslLogError("psl__GetOverflows", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Returns the # of output events in the MCA (not including overflows
 * and underflows) via value as an unsigned long.
 */
PSL_STATIC int psl__GetMCAEvents(int detChan, void* value, XiaDefaults* defs) {
    int status;

    unsigned long dspParams[DSP_PARAM_MEM_LEN];

    UNUSED(defs);

    ASSERT(value);

    status = psl__GetDSPBlock(detChan, dspParams);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error getting all DSP parameters for detChan "
                "%d.",
                detChan);
        pslLogError("psl__GetMCAEvents", info_string, status);
        return status;
    }

    status = psl__ExtractEvents(detChan, dspParams, (unsigned long*) value);

    if (status != XIA_SUCCESS) {
        sprintf(info_string,
                "Error extracting the MCA event count from the "
                "DSP parameter block for detChan %d.",
                detChan);
        pslLogError("psl__GetMCAEvents", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Returns the entire DSP parameter memory in params.
 */
PSL_STATIC int psl__GetDSPBlock(int detChan, unsigned long* params) {
    int status;

    char* memStr = "data:0x0000:256";

    ASSERT(params);

    status = dxp_read_memory(&detChan, memStr, params);

    if (status != DXP_SUCCESS) {
        sprintf(info_string,
                "Error reading the DSP parameter memory for "
                "detChan %d. Xerxes reports status = %d.",
                detChan, status);
        pslLogError("psl__GetDSPBlock", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

PSL_STATIC int psl__ExtractRealtime(int detChan, unsigned long* params, double* rt) {
    int status;

    unsigned short idx;

    double spd;

    ASSERT(params);
    ASSERT(rt);

    status = pslGetClockSpeed(detChan, &spd);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting the clock speed for detChan %d.", detChan);
        pslLogError("psl__ExtractRealtime", info_string, status);
        return status;
    }

    GET_PARAM_IDX(detChan, "REALTIME0", idx, Realtime);
    *rt = ldexp((double) params[idx], 16);

    GET_PARAM_IDX(detChan, "REALTIME1", idx, Realtime);
    *rt += (double) params[idx];

    GET_PARAM_IDX(detChan, "REALTIME2", idx, Realtime);
    *rt += ldexp((double) params[idx], 32);

    *rt *= (16.0 / (spd * 1.0e6));

    return XIA_SUCCESS;
}

PSL_STATIC int psl__ExtractTLivetime(int detChan, unsigned long* params, double* tlt) {
    int status;

    double spd;

    unsigned short idx;

    ASSERT(params);
    ASSERT(tlt);

    status = pslGetClockSpeed(detChan, &spd);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting the clock speed for detChan %d.", detChan);
        pslLogError("psl__ExtractTLivetime", info_string, status);
        return status;
    }

    GET_PARAM_IDX(detChan, "LIVETIME0", idx, TLivetime);
    *tlt = ldexp((double) params[idx], 16);

    GET_PARAM_IDX(detChan, "LIVETIME1", idx, TLivetime);
    *tlt += (double) params[idx];

    GET_PARAM_IDX(detChan, "LIVETIME2", idx, TLivetime);
    *tlt += ldexp((double) params[idx], 32);

    *tlt *= (16.0 / (spd * 1.0e6));

    return XIA_SUCCESS;
}

PSL_STATIC int psl__ExtractELivetime(int detChan, unsigned long* params, double* elt) {
    int status;

    double spd;

    unsigned short idx;

    ASSERT(params);
    ASSERT(elt);

    status = pslGetClockSpeed(detChan, &spd);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error getting the clock speed for detChan %d.", detChan);
        pslLogError("psl__ExtractELivetime", info_string, status);
        return status;
    }

    GET_PARAM_IDX(detChan, "ELIVETIME0", idx, ELivetime);
    *elt = ldexp((double) params[idx], 16);

    GET_PARAM_IDX(detChan, "ELIVETIME1", idx, ELivetime);
    *elt += (double) params[idx];

    GET_PARAM_IDX(detChan, "ELIVETIME2", idx, ELivetime);
    *elt += ldexp((double) params[idx], 32);

    *elt *= (16.0 / (spd * 1.0e6));

    return XIA_SUCCESS;
}

PSL_STATIC int psl__ExtractTriggers(int detChan, unsigned long* params,
                                    unsigned long* trigs) {
    unsigned short idx;

    ASSERT(params);
    ASSERT(trigs);

    GET_PARAM_IDX(detChan, "FASTPEAKS0", idx, Triggers);
    *trigs = (unsigned long) ((unsigned long) params[idx] << 16);

    GET_PARAM_IDX(detChan, "FASTPEAKS1", idx, Triggers);
    *trigs += (unsigned long) params[idx];

    return XIA_SUCCESS;
}

PSL_STATIC int psl__ExtractEvents(int detChan, unsigned long* params,
                                  unsigned long* evts) {
    unsigned short idx;

    ASSERT(params);
    ASSERT(evts);

    GET_PARAM_IDX(detChan, "EVTSINRUN0", idx, Events);
    *evts = (unsigned long) ((unsigned long) params[idx] << 16);

    GET_PARAM_IDX(detChan, "EVTSINRUN1", idx, Events);
    *evts += (unsigned long) params[idx];

    return XIA_SUCCESS;
}

PSL_STATIC int psl__ExtractUnders(int detChan, unsigned long* params,
                                  unsigned long* unders) {
    unsigned short idx;

    ASSERT(params);
    ASSERT(unders);

    GET_PARAM_IDX(detChan, "UNDRFLOWS0", idx, Unders);
    *unders = (unsigned long) ((unsigned long) params[idx] << 16);

    GET_PARAM_IDX(detChan, "UNDRFLOWS1", idx, Unders);
    *unders += (unsigned long) params[idx];

    return XIA_SUCCESS;
}

PSL_STATIC int psl__ExtractOvers(int detChan, unsigned long* params,
                                 unsigned long* overs) {
    unsigned short idx;

    ASSERT(params);
    ASSERT(overs);

    GET_PARAM_IDX(detChan, "OVERFLOWS0", idx, Overs);
    *overs = (unsigned long) ((unsigned long) params[idx] << 16);

    GET_PARAM_IDX(detChan, "OVERFLOWS1", idx, Overs);
    *overs += (unsigned long) params[idx];

    return XIA_SUCCESS;
}
