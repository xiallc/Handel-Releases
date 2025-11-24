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

#include <limits.h>
#include <stdio.h>

#include "xerxes.h"
#include "xerxes_errors.h"

#include "xia_assert.h"
#include "xia_handel.h"
#include "xia_handel_structures.h"
#include "xia_system.h"

#include "handel_errors.h"
#include "handel_generic.h"
#include "handel_log.h"
#include "handel_xerxes.h"

#include "fdd.h"

static int xia__GetSystemFPGAName(Module* module, char* detType, char* sysFPGAName,
                                  char* rawFilename, boolean_t* found);
static int xia__GetSystemDSPName(Module* module, char* detType, char* sysDSPName,
                                 char* rawFilename, boolean_t* found);
static int xia__GetDSPName(Module* module, int channel, double peakingTime,
                           char* dspName, char* detectorType, char* rawFilename);
static int xia__GetFiPPIAName(Module* module, char* detType, char* sysDSPName,
                              char* rawFilename, boolean_t* found);
static int xia__GetFiPPIName(Module* module, int channel, double peakingTime,
                             char* fippiName, char* detectorType, char* rawFilename,
                             boolean_t* found);
static int xia__GetSystemFiPPIName(Module* module, char* detType, char* sysFipName,
                                   char* rawFilename, boolean_t* found);

static int xia__CopyInterfString(Module* m, char* interf);
static int xia__CopyMDString(Module* m, char* md);
static int xia__CopyChanString(Module* m, char* chan);

static int xia__AddXerxesSysItems(void);
static int xia__AddXerxesBoardType(Module* m);
static int xia__AddXerxesInterface(Module* m);
static int xia__AddXerxesModule(Module* m);

static int xia__AddSystemFPGA(Module* module, char* sysFPGAName, char* rawFilename);
static int xia__AddSystemDSP(Module* module, char* sysDSPName, char* rawFilename);
static int xia__AddFiPPIA(Module* module, char* sysDSPName, char* rawFilename);
static int xia__AddSystemFiPPI(Module* module, char* sysFipName, char* rawFilename);

static int xia__DoMMUConfig(Module* m);
static int xia__DoSystemFPGA(Module* m);
static int xia__DoSystemDSP(Module* m, boolean_t* found);
static int xia__DoDSP(Module* m);
static int xia__DoFiPPIA(Module* m, boolean_t* found);
static int xia__DoFiPPI(Module* m, boolean_t* found);
static int xia__DoSystemFiPPI(Module* m, boolean_t* found);

static int xia__GetDetStringFromDetChan(int detChan, Module* m, char* type);
static int xia__SetupSingleChan(Module* module, unsigned int detChan,
                                PSLFuncs* localFuncs);

/*
 * This routine calls XerXes routines in order to build a proper XerXes
 * configuration based on the HanDeL information.
 */
int HANDEL_API xiaBuildXerxesConfig(void) {
    int status;

    boolean_t found;
    boolean_t isSysFip;

    Module* current = NULL;

    status = dxp_init_ds();

    if (status != DXP_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaBuildXerxesConfig",
               "Error initializing Xerxes internal data structures");
        return status;
    }

    status = xia__AddXerxesSysItems();

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaBuildXerxesConfig",
               "Error adding system items to Xerxes configuration");
        return status;
    }

    /* Walk through the module list and make the proper calls to XerXes */
    current = xiaGetModuleHead();

    while (current != NULL) {
        status = xia__AddXerxesBoardType(current);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaBuildXerxesConfig",
                   "Error adding board type for alias = '%s'", current->alias);
            return status;
        }

        status = xia__AddXerxesInterface(current);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaBuildXerxesConfig",
                   "Error adding interface for alias = '%s'", current->alias);
            return status;
        }

        status = xia__AddXerxesModule(current);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaBuildXerxesConfig",
                   "Error adding module for alias = '%s'", current->alias);
            return status;
        }

        /*
         * This section begins the firmware configuration area. Not all firmwares
         * are required for each product, but we check to see what is available,
         * and we add it to the configuration if it is found.
         */

        status = xia__DoMMUConfig(current);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaBuildXerxesConfig",
                   "Error adding MMU for alias = '%s'", current->alias);
            return status;
        }

        status = xia__DoSystemFiPPI(current, &isSysFip);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaBuildXerxesConfig",
                   "Error adding System FiPPI for alias = '%s'", current->alias);
            return status;
        }

        status = xia__DoSystemFPGA(current);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaBuildXerxesConfig",
                   "Error adding System FPGA for alias = '%s'", current->alias);
            return status;
        }

        status = xia__DoSystemDSP(current, &found);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaBuildXerxesConfig",
                   "Error adding System DSP for alias = '%s'", current->alias);
            return status;
        }

        /*
         * If we didn't find a system DSP then we assume that the hardware supports
         * a single DSP for each channel.
         */
        if (!found && !isSysFip) {
            status = xia__DoDSP(current);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaBuildXerxesConfig",
                       "Error adding DSPs for alias = '%s'", current->alias);
                return status;
            }
        }

        status = xia__DoFiPPIA(current, &found);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaBuildXerxesConfig",
                   "Error adding FiPPI A for alias = '%s'", current->alias);
            return status;
        }

        if (!found && !isSysFip) {
            status = xia__DoFiPPI(current, &found);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaBuildXerxesConfig",
                       "Error adding FiPPIs for alias = '%s'", current->alias);
                return status;
            }
        }

        current = getListNext(current);
    }

    return XIA_SUCCESS;
}

/*
 * Essentially a wrapper around dxp_user_setup().
 */
int HANDEL_API xiaUserSetup(void) {
    int status;

    unsigned int i;
    int detChanInModule;

    DetChanElement* chan = NULL;

    Module* module = NULL;

    PSLFuncs localFuncs;

    XiaDefaults* defaults = NULL;

    status = dxp_user_setup();

    if (status != DXP_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaUserSetup",
               "Error downloading firmware via Xerxes.");
        return status;
    }

    module = xiaGetModuleHead();
    ASSERT(module);

    /*
     * The user setup function is iterated on modules first
     * this will allow insertion of per-module setup calls
     */
    while (module != NULL) {
        /*
         * Clear the isSetup flag which will be toggled after the first channel
         * in the module is set up.
         */
        module->isSetup = FALSE_;
        status = xiaLoadPSL(module->type, &localFuncs);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaUserSetup",
                   "Unable to load PSL funcs for module type %s.", module->type);
            return status;
        }

        /* Loop on channels in the module now. */
        chan = xiaGetDetChanHead();

        while (chan != NULL) {
            if (xiaGetElemType(chan->detChan) == SINGLE &&
                STREQ(chan->data.modAlias, module->alias)) {
                status = xia__SetupSingleChan(module, chan->detChan, &localFuncs);

                if (status != XIA_SUCCESS) {
                    xiaLog(XIA_LOG_ERROR, status, "xiaUserSetup",
                           "Unable to set up channel %u for module alias %s.",
                           chan->detChan, module->alias);
                    return status;
                }
            }
            chan = getListNext(chan);
        }

        /*
         * Module level setup function, need a detChan for the user functions
         * we could just use the first active channel in the module
         */
        for (i = 0; i < module->number_of_channels; i++) {
            if (module->channels[i] >= 0) {
                break;
            }
        }

        if (i == module->number_of_channels) {
            xiaLog(XIA_LOG_DEBUG, "xiaUserSetup",
                   "Skipping module setup for %s, module is disabled", module->alias);
        } else {
            detChanInModule = module->channels[i];

            defaults = xiaGetDefaultFromDetChan(detChanInModule);
            status = localFuncs.moduleSetup(detChanInModule, defaults, module);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaUserSetup",
                       "Unable to do module setup for module %s.", module->alias);
                return status;
            }
        }

        module = getListNext(module);
    }

    return XIA_SUCCESS;
}

/*
 * Set up a single channel in given module
 */
static int xia__SetupSingleChan(Module* module, unsigned int detChan,
                                PSLFuncs* localFuncs) {
    int status;

    int detector_chan;
    unsigned int modChan;

    char* detAlias;
    char* firmAlias;

    char detectorType[MAXITEM_LEN];

    FirmwareSet* firmwareSet = NULL;

    CurrentFirmware* currentFirmware = NULL;

    Detector* detector = NULL;

    XiaDefaults* defaults = NULL;

    ASSERT(module != NULL);
    ASSERT(localFuncs != NULL);

    modChan = xiaGetModChan(detChan);
    ASSERT(modChan < module->number_of_channels);

    firmAlias = module->firmware[modChan];
    firmwareSet = xiaFindFirmware(firmAlias);
    detAlias = module->detector[modChan];
    detector_chan = module->detector_chan[modChan];
    detector = xiaFindDetector(detAlias);
    currentFirmware = &module->currentFirmware[modChan];
    defaults = xiaGetDefaultFromDetChan(detChan);

    switch (detector->type) {
        case XIA_DET_RESET:
            strncpy(detectorType, "RESET", sizeof(detectorType));
            break;
        case XIA_DET_RCFEED:
            strncpy(detectorType, "RC", sizeof(detectorType));
            break;
        default:
        case XIA_DET_UNKNOWN:
            xiaLog(XIA_LOG_ERROR, XIA_MISSING_TYPE, "xia__SetupSingleChan",
                   "No detector type specified for detChan %u.", detChan);
            return XIA_MISSING_TYPE;
            break;
    }

    status =
        localFuncs->userSetup(detChan, defaults, firmwareSet, currentFirmware,
                              detectorType, detector, detector_chan, module, modChan);

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xia__SetupSingleChan",
               "Unable to complete user setup for detChan %u.", detChan);
        return status;
    }

    /* Having a single channel set up is enough for some per module operation */
    module->isSetup = TRUE_;

    /* Do any DSP parameters that are in the list */
    status = xiaUpdateUserParams(detChan);

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xia__SetupSingleChan",
               "Unable to update user parameters for detChan %u.", detChan);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Copy the interface specific information into the target string.
 *
 * Assumes that interf has already been allocated to the proper size. This
 * routine is meant to encapsulate all the product-specific logic in it.
 */
static int xia__CopyInterfString(Module* m, char* interf) {
    ASSERT(m != NULL);
    ASSERT(interf != NULL);

    switch (m->interface_info->type) {
        case XIA_INTERFACE_NONE:
        default:
            xiaLog(XIA_LOG_ERROR, XIA_MISSING_INTERFACE, "xia__CopyInterfString",
                   "No interface string specified for alias '%s'", m->alias);
            return XIA_MISSING_INTERFACE;
            break;

#ifndef EXCLUDE_EPP
        case XIA_EPP:
        case XIA_GENERIC_EPP:
            sprintf(interf, "%#x", m->interface_info->info.epp->epp_address);
            break;
#endif /* EXCLUDE_EPP */

#ifndef EXCLUDE_USB
        case XIA_USB:
            sprintf(interf, "%u", m->interface_info->info.usb->device_number);
            break;
#endif /* EXCLUDE_USB */

#ifndef EXCLUDE_USB2
        case XIA_USB2:
            sprintf(interf, "%u", m->interface_info->info.usb2->device_number);
            break;
#endif /* EXCLUDE_USB2 */

#ifndef EXCLUDE_SERIAL
        case XIA_SERIAL:
            if (m->interface_info->info.serial->device_file) {
                sprintf(interf, "%s", m->interface_info->info.serial->device_file);
            } else {
                sprintf(interf, "COM%u", m->interface_info->info.serial->com_port);
            }
            break;
#endif /* EXCLUDE_SERIAL */

#ifndef EXCLUDE_PLX
        case XIA_PLX:
            sprintf(interf, "pxi");
            break;
#endif /* EXCLUDE_PLX */
    }

    return XIA_SUCCESS;
}

/*
 * Builds the MD string required by Xerxes.
 *
 * Assumes that md has already been allocated. This routine is meant to
 * encapsulate all the hardware specific logic for building the MD string.
 */
static int xia__CopyMDString(Module* m, char* md) {
    UNUSED(md);
    ASSERT(m != NULL);
    ASSERT(md != NULL);

    switch (m->interface_info->type) {
        case XIA_INTERFACE_NONE:
        default:
            xiaLog(XIA_LOG_ERROR, XIA_MISSING_INTERFACE, "xia__CopyMDString",
                   "No interface string specified for alias '%s'", m->alias);
            return XIA_MISSING_INTERFACE;
            break;

#ifndef EXCLUDE_EPP
        case XIA_EPP:
        case XIA_GENERIC_EPP:
            /* If default then don't change anything, else tack on a : in front
             * of the string (tells XerXes to
             * treat this as a multi-module EPP chain
             */
            if (m->interface_info->info.epp->daisy_chain_id == UINT_MAX) {
                sprintf(md, "0");
            } else {
                sprintf(md, ":%u", m->interface_info->info.epp->daisy_chain_id);
            }
            break;
#endif /* EXCLUDE_EPP */
#ifndef EXCLUDE_USB
        case XIA_USB:
            sprintf(md, "%u", m->interface_info->info.usb->device_number);
            break;
#endif /* EXCLUDE_USB */
#ifndef EXCLUDE_USB2
        case XIA_USB2:
            sprintf(md, "%u", m->interface_info->info.usb2->device_number);
            break;
#endif /* EXCLUDE_USB2 */
#ifndef EXCLUDE_SERIAL
        case XIA_SERIAL:
            if (m->interface_info->info.serial->device_file) {
                sprintf(md, "%s:%u", m->interface_info->info.serial->device_file,
                        m->interface_info->info.serial->baud_rate);
            } else {
                sprintf(md, "%u:%u", m->interface_info->info.serial->com_port,
                        m->interface_info->info.serial->baud_rate);
            }
            break;
#endif /* EXCLUDE_SERIAL */
#ifndef EXCLUDE_PLX
        case XIA_PLX:
            sprintf(md, "%u:%u", m->interface_info->info.plx->bus,
                    m->interface_info->info.plx->slot);
            break;
#endif /* EXCLUDE_PLX */
    }

    return XIA_SUCCESS;
}

/*
 * Get the DSP name for the specified channel.
 *
 * If no FDD file is specified, then the DSP name is retrieved from the firmware
 * Structure. Otherwise, the DSP code is retrieved from the FDD file using
 * the FDD library.
 */
static int xia__GetDSPName(Module* module, int channel, double peakingTime,
                           char* dspName, char* detectorType, char* rawFilename) {
    char* firmAlias;
    int status;

    FirmwareSet* firmwareSet = NULL;

    firmAlias = module->firmware[channel];
    firmwareSet = xiaFindFirmware(firmAlias);

    if (firmwareSet->filename == NULL) {
        status = xiaGetDSPNameFromFirmware(firmAlias, peakingTime, dspName);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaGetDSPName",
                   "Error getting DSP code for firmware %s @ peaking time = %f",
                   firmAlias, peakingTime);
            return status;
        }
    } else {
        status = xiaFddGetAndCacheFirmware(firmwareSet, "dsp", peakingTime,
                                           detectorType, dspName, rawFilename);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaGetDSPName",
                   "Error getting DSP code from FDD file %s @ peaking time = %f",
                   firmwareSet->filename, peakingTime);
            return status;
        }
    }

    return XIA_SUCCESS;
}

/*
 * Gets the FiPPI name from the firmware configuration.
 */
static int xia__GetFiPPIName(Module* module, int channel, double peakingTime,
                             char* fippiName, char* detectorType, char* rawFilename,
                             boolean_t* found) {
    char* firmAlias;

    int status;

    FirmwareSet* firmwareSet = NULL;

    firmAlias = module->firmware[channel];

    firmwareSet = xiaFindFirmware(firmAlias);

    *found = FALSE_;

    if (firmwareSet->filename == NULL) {
        status = xiaGetFippiNameFromFirmware(firmAlias, peakingTime, fippiName);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xia__GetFiPPIName",
                   "Error getting FiPPI code for firmware %s @ peaking time = %f",
                   firmAlias, peakingTime);
            return status;
        }

        /* I think that we need this! */
        strcpy(rawFilename, fippiName);
    } else {
        status = xiaFddGetAndCacheFirmware(firmwareSet, "fippi", peakingTime,
                                           detectorType, fippiName, rawFilename);

        /* This is not necessarily an error. We will still pass the error value
         * up to the top-level but only use an informational message. For products
         * without system_fpga defined in their FDD files this message will always
         * appear, and we don't want them to be confused by spurious ERRORs.
         */
        if (status == XIA_FILEERR) {
            xiaLog(XIA_LOG_INFO, "xia__GetFiPPIName", "No fippi defined in %s",
                   firmwareSet->filename);
            return status;
        }

        *found = TRUE_;

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xia__GetFiPPIName",
                   "Error getting FiPPI code from FDD file %s @ peaking time = %f",
                   firmwareSet->filename, peakingTime);
            return status;
        }
    }

    return XIA_SUCCESS;
}

/*
 * Get the name of the MMU firmware, if it is defined.
 *
 * Returns status indicating if the MMU firmware was found or not.
 */
static int xia__GetMMUName(Module* m, int channel, char* mmuName, char* rawFilename) {
    char* firmAlias = NULL;

    FirmwareSet* firmware = NULL;

    ASSERT(m != NULL);
    ASSERT(m->firmware != NULL);
    ASSERT(m->firmware[channel] != NULL);

    firmAlias = m->firmware[channel];

    firmware = xiaFindFirmware(firmAlias);
    ASSERT(firmware != NULL);

    if (firmware->filename == NULL) {
        if (firmware->mmu == NULL) {
            return XIA_NO_MMU;
        }

        strcpy(mmuName, firmware->mmu);
        strcpy(rawFilename, firmware->mmu);
    } else {
        /* Do something with the FDD here */
        return XIA_NO_MMU;
    }

    return XIA_SUCCESS;
}

/*
 * Adds a system FPGA to the Xerxes configuration.
 */
static int xia__AddSystemFPGA(Module* module, char* sysFPGAName, char* rawFilename) {
    int status;

    char* sysFPGAStr[1];

    UNUSED(rawFilename);
    UNUSED(module);

    ASSERT(sysFPGAName != NULL);

    sysFPGAStr[0] = (char*) handel_md_alloc(strlen(sysFPGAName) + 1);

    if (!sysFPGAStr[0]) {
        xiaLog(XIA_LOG_ERROR, XIA_NOMEM, "xia__AddSystemFPGA",
               "Unable to allocate %zu bytes for 'sysFPGAStr[0]'",
               strlen(sysFPGAName) + 1);
        return XIA_NOMEM;
    }

    strncpy(sysFPGAStr[0], sysFPGAName, strlen(sysFPGAName) + 1);

    status = dxp_add_board_item("system_fpga", (char**) sysFPGAStr);

    handel_md_free(sysFPGAStr[0]);

    if (status != DXP_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xia__AddSystemFPGA",
               "Error adding 'system_fpga' board item");
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Retrieves the system FPGA name from the firmware, if it has
 * been defined.
 *
 * Currently, this routine only returns success if a system FPGA is
 * defined in an FDD file. All other conditions return false, including
 * the case where an FDD file is not used.
 *
 * Future versions of this routine should also include calling
 * parameters for the length of sysFPGAName and rawFilename to prevent
 * buffer overflows from occurring.
 *
 * The FDD file associates a peaking time range with each entry, but some
 * firmware types, such as the system FPGA, are global so we pass in a fake
 * peaking time instead.
 *
 * The System FPGA is always assumed to be defined in the firmware for
 * channel 0 in the module.
 */
static int xia__GetSystemFPGAName(Module* module, char* detType, char* sysFPGAName,
                                  char* rawFilename, boolean_t* found) {
    int status;

    double fake_pt = 1.0;

    FirmwareSet* fs = NULL;

    ASSERT(module != NULL);
    ASSERT(detType != NULL);
    ASSERT(sysFPGAName != NULL);
    ASSERT(rawFilename != NULL);
    ASSERT(found != NULL);

    *found = FALSE_;

    fs = xiaFindFirmware(module->firmware[0]);

    if (!fs || !fs->filename) {
        xiaLog(XIA_LOG_INFO, "xia__GetSystemFPGAName",
               "No firmware set defined for alias '%s'", module->firmware[0]);
        return XIA_NULL_FIRMWARE;
    }

    status = xiaFddGetAndCacheFirmware(fs, "system_fpga", fake_pt, detType, sysFPGAName,
                                       rawFilename);

    /* This is not necessarily an error. We will still pass the error value
     * up to the top-level but only use an informational message. For products
     * without system_fpga defined in their FDD files this message will always
     * appear, and we don't want them to be confused by spurious ERRORs.
     */
    if (status == XIA_FILEERR) {
        xiaLog(XIA_LOG_INFO, "xia__GetSystemFPGAName", "No system_fpga defined in %s",
               fs->filename);
        return status;
    }

    *found = TRUE_;

    /* These are "real" errors, not just missing file problems. */
    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xia__GetSystemFPGAName",
               "Error finding system_fpga in %s", fs->filename);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Get the system DSP name from the FDD file, if either exists.
 *
 * Currently, this routine only returns success if a system DSP is
 * defined in an FDD file. All other conditions return false, including
 * the case where an FDD file is not used.
 *
 * Future versions of this routine should also include calling parameters
 * for the length of sysDSPName and rawFilename to prevent buffer
 * overflows from occurring.
 *
 * XXX: This routine contains a hard-coded peaking time since most FDDs only
 * contain one DSP for the entire range of peaking times. This is a hack and
 * must be fixed in future versions.
 *
 * The System DSP is always assumed to be defined in the firmware for
 * channel 0 in the module.
 */
static int xia__GetSystemDSPName(Module* module, char* detType, char* sysDSPName,
                                 char* rawFilename, boolean_t* found) {
    int status;

    double fake_pt = 1.0;
    FirmwareSet* fs = NULL;

    ASSERT(module != NULL);
    ASSERT(detType != NULL);
    ASSERT(sysDSPName != NULL);
    ASSERT(rawFilename != NULL);
    ASSERT(found != NULL);

    *found = FALSE_;

    fs = xiaFindFirmware(module->firmware[0]);

    if (!fs || !fs->filename) {
        xiaLog(XIA_LOG_INFO, "xia__GetSystemDSPName",
               "No firmware set defined for alias '%s'", module->firmware[0]);
        return XIA_NULL_FIRMWARE;
    }

    status = xiaFddGetAndCacheFirmware(fs, "system_dsp", fake_pt, detType, sysDSPName,
                                       rawFilename);

    /*
     * This is not necessarily an error. We will still pass the error value
     * up to the top-level but only use an informational message. For products
     * without system_dsp defined in their FDD files this message will always
     * appear, and we don't want them to be confused by spurious ERRORs.
     */
    if (status == XIA_FILEERR) {
        xiaLog(XIA_LOG_INFO, "xia__GetSystemDSPName", "No system_dsp defined in %s",
               fs->filename);
        return status;
    }

    *found = TRUE_;

    /* These are "real" errors, not just missing file problems. */
    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xia__GetSystemDSPName",
               "Cannot find system_dsp in %s", fs->filename);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Add the specified system DSP to the Xerxes configuration.
 */
static int xia__AddSystemDSP(Module* module, char* sysDSPName, char* rawFilename) {
    int status;

    char* sysDSPStr[1];

    UNUSED(rawFilename);
    UNUSED(module);

    ASSERT(sysDSPName != NULL);

    sysDSPStr[0] = (char*) handel_md_alloc(strlen(sysDSPName) + 1);

    if (!sysDSPStr[0]) {
        xiaLog(XIA_LOG_ERROR, XIA_NOMEM, "xiaAddSystemDSP",
               "Unable to allocate %zu bytes for 'sysDSPStr[0]'",
               strlen(sysDSPName) + 1);
        return XIA_NOMEM;
    }

    strncpy(sysDSPStr[0], sysDSPName, strlen(sysDSPName) + 1);

    status = dxp_add_board_item("system_dsp", (char**) sysDSPStr);

    handel_md_free(sysDSPStr[0]);

    if (status != DXP_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaAddSystemDSP",
               "Error adding 'system_dsp' board item");
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Get the FiPPI A name from the FDD file, if either exists.
 *
 * Currently, this routine only returns success if a FiPPI A is
 * defined in an FDD file. All other conditions return false, including
 * the case where an FDD file is not used.
 *
 * Future versions of this routine should also include calling parameters
 * for the length of sysFippiAName and rawFilename to prevent buffer
 * overflows from occurring.
 *
 * The Fippi A is always assumed to be defined in the firmware for
 * channel 0 in the module.
 */
static int xia__GetFiPPIAName(Module* module, char* detType, char* sysFippiAName,
                              char* rawFilename, boolean_t* found) {
    int status;

    double fake_pt = 1.0;

    FirmwareSet* fs = NULL;

    ASSERT(module != NULL);
    ASSERT(detType != NULL);
    ASSERT(sysFippiAName != NULL);
    ASSERT(rawFilename != NULL);
    ASSERT(found != NULL);

    *found = FALSE_;

    fs = xiaFindFirmware(module->firmware[0]);

    if (!fs || !fs->filename) {
        xiaLog(XIA_LOG_INFO, "xia__GetFiPPIAName",
               "No firmware set defined for alias '%s'", module->firmware[0]);
        return XIA_NULL_FIRMWARE;
    }

    status = xiaFddGetAndCacheFirmware(fs, "fippi_a", fake_pt, detType, sysFippiAName,
                                       rawFilename);

    /*
     * This is not necessarily an error. We will still pass the error value
     * up to the top-level but only use an informational message. For products
     * without fippi_a defined in their FDD files this message will always
     * appear, and we don't want them to be confused by spurious ERRORs.
     */
    if (status == XIA_FILEERR) {
        xiaLog(XIA_LOG_INFO, "xia__GetFiPPIAName", "No fippi_a defined in %s",
               fs->filename);
        return status;
    }

    *found = TRUE_;

    /* These are "real" errors, not just missing file problems. */
    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xia__GetFiPPIAName", "Cannot find fippi_a in %s",
               fs->filename);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Add FiPPI A to the Xerxes configuration.
 */
static int xia__AddFiPPIA(Module* module, char* sysFippiAName, char* rawFilename) {
    int status;

    char* sysFippiAStr[1];

    UNUSED(rawFilename);
    UNUSED(module);

    ASSERT(sysFippiAName != NULL);

    sysFippiAStr[0] = (char*) handel_md_alloc(strlen(sysFippiAName) + 1);

    if (!sysFippiAStr[0]) {
        xiaLog(XIA_LOG_ERROR, XIA_NOMEM, "xiaAddSystemFippiA",
               "Unable to allocate %zu bytes for 'sysFippiAStr[0]'",
               strlen(sysFippiAName) + 1);
        return XIA_NOMEM;
    }

    strncpy(sysFippiAStr[0], sysFippiAName, strlen(sysFippiAName) + 1);

    status = dxp_add_board_item("fippi_a", (char**) sysFippiAStr);

    handel_md_free(sysFippiAStr[0]);

    if (status != DXP_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaAddSystemFippiA",
               "Error adding 'fippi_a' board item");
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Add all the system items to the Xerxes configuration
 */
static int xia__AddXerxesSysItems(void) {
    unsigned long i;
    int status;

    /*
     * Xerxes requires us to add all of allowed/known board types to the
     * system. The list of known boards is controlled via preprocessor macros
     * passed in to the build. For more information, see the top-level Makefile.
     */
    for (i = 0; i < N_KNOWN_BOARDS; i++) {
        status = dxp_add_system_item(BOARD_LIST[i], (char**) SYS_NULL);

        if (status != DXP_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xia__AddXerxesSysItems",
                   "Error adding Xerxes system item '%s'", BOARD_LIST[i]);
            return status;
        }
    }

    return XIA_SUCCESS;
}

/*
 * Add the board type of the specified module to the Xerxes
 * configuration.
 */
static int xia__AddXerxesBoardType(Module* m) {
    int status;

    char** type = NULL;

    ASSERT(m != NULL);

    type = (char**) handel_md_alloc(sizeof(char*));

    if (!type) {
        xiaLog(XIA_LOG_ERROR, XIA_NOMEM, "xia__AddXerxesBoardType",
               "Error allocating %zu bytes for 'type'", sizeof(char*));
        return XIA_NOMEM;
    }

    type[0] = (char*) handel_md_alloc(strlen(m->type) + 1);

    if (!type[0]) {
        handel_md_free(type);

        xiaLog(XIA_LOG_ERROR, XIA_NOMEM, "xia__AddXerxesBoardType",
               "Error allocating %zu bytes for 'type[0]'", strlen(m->type) + 1);
        return XIA_NOMEM;
    }

    strcpy(type[0], m->type);

    status = dxp_add_board_item("board_type", type);

    handel_md_free(type[0]);
    handel_md_free(type);

    if (status != DXP_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xia__AddXerxesBoardType",
               "Error adding board_type '%s' to Xerxes for alias '%s'", m->type,
               m->alias);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Add the required interface string to the Xerxes configuration
 */
static int xia__AddXerxesInterface(Module* m) {
    int status;

    size_t interfLen = 0;

    char* interf[2];

    ASSERT(m != NULL);

    interfLen = strlen(INTERF_LIST[m->interface_info->type]) + 1;

    interf[0] = (char*) handel_md_alloc(interfLen);

    if (!interf[0]) {
        xiaLog(XIA_LOG_ERROR, XIA_NOMEM, "xia__AddXerxesInterface",
               "Error allocating %zu bytes for 'interf[0]'", interfLen);
        return XIA_NOMEM;
    }

    interf[1] = (char*) handel_md_alloc(MAX_INTERF_LEN);

    if (!interf[1]) {
        handel_md_free(interf[0]);
        xiaLog(XIA_LOG_ERROR, XIA_NOMEM, "xia___AddXerxesInterface",
               "Error allocating %d bytes for 'interf[1]'", MAX_INTERF_LEN);
        return XIA_NOMEM;
    }

    strcpy(interf[0], INTERF_LIST[m->interface_info->type]);

    xiaLog(XIA_LOG_DEBUG, "xia__AddXerxesInterface", "type = %u, name = '%s'",
           m->interface_info->type, INTERF_LIST[m->interface_info->type]);

    status = xia__CopyInterfString(m, interf[1]);

    if (status != XIA_SUCCESS) {
        handel_md_free(interf[0]);
        handel_md_free(interf[1]);

        xiaLog(XIA_LOG_ERROR, status, "xia__AddXerxesInterface",
               "Error getting interface string for alias '%s'", m->alias);
        return status;
    }

    status = dxp_add_board_item("interface", (char**) interf);

    if (status != DXP_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xia__AddXerxesInterface",
               "Error adding interface '%s, %s' to Xerxes for alias '%s'", interf[0],
               interf[1], m->alias);

        handel_md_free(interf[0]);
        handel_md_free(interf[1]);

        return status;
    }

    handel_md_free(interf[0]);
    handel_md_free(interf[1]);

    return XIA_SUCCESS;
}

/*
 * Add the module board item to the Xerxes configuration.
 */
static int xia__AddXerxesModule(Module* m) {
    int status;

    unsigned int i;
    unsigned int j;

    char** modStr = NULL;

    char mdStr[MAX_MD_LEN];
    char chanStr[MAX_NUM_CHAN_LEN];

    ASSERT(m != NULL);

    modStr = (char**) handel_md_alloc((m->number_of_channels + 2) * sizeof(char*));

    if (!modStr) {
        xiaLog(XIA_LOG_ERROR, XIA_NOMEM, "xia__AddXerxesModule",
               "Error allocating %zu bytes for 'modStr'",
               (m->number_of_channels + 2) * sizeof(char*));
        return XIA_NOMEM;
    }

    /*
     * The first two module string elements always refer to the MD string and
     * the "number of channels" for this module string.
     */
    modStr[0] = &(mdStr[0]);
    modStr[1] = &(chanStr[0]);

    status = xia__CopyMDString(m, modStr[0]);

    if (status != XIA_SUCCESS) {
        handel_md_free(modStr);
        xiaLog(XIA_LOG_ERROR, status, "xia__AddXerxesModule",
               "Error copying MD string to modules string for alias '%s'", m->alias);
        return status;
    }

    status = xia__CopyChanString(m, modStr[1]);

    if (status != XIA_SUCCESS) {
        handel_md_free(modStr);

        xiaLog(XIA_LOG_ERROR, status, "xia__AddXerxesModule",
               "Error copying channel string to modules string for alias '%s'",
               m->alias);
        return status;
    }

    for (i = 0; i < m->number_of_channels; i++) {
        modStr[i + 2] = (char*) handel_md_alloc(MAX_CHAN_LEN);

        if (!modStr[i + 2]) {
            /* Need to unwind the previous allocations */
            for (j = 0; j < i; j++) {
                handel_md_free(modStr[j + 2]);
            }

            handel_md_free(modStr);

            xiaLog(XIA_LOG_ERROR, XIA_NOMEM, "xia__AddXerxesModule",
                   "Error allocating %d bytes for 'modStr[%d]'", MAX_CHAN_LEN,
                   (int) (i + 2));
            return XIA_NOMEM;
        }

        sprintf(modStr[i + 2], "%d", m->channels[i]);
    }

    status = dxp_add_board_item("module", modStr);

    for (i = 0; i < m->number_of_channels; i++) {
        handel_md_free(modStr[i + 2]);
    }

    handel_md_free(modStr);

    if (status != DXP_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xia__AddXerxesModule",
               "Error adding module to Xerxes for alias '%s'", m->alias);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Copies the "number of channels" string in the format required by Xerxes.
 * Assumes that chan is already allocated.
 */
static int xia__CopyChanString(Module* m, char* chan) {
    ASSERT(m != NULL);
    ASSERT(chan != NULL);

    sprintf(chan, "%u", m->number_of_channels);

    return XIA_SUCCESS;
}

/*
 * Checks for the presence of the MMU firmware and configures
 * Xerxes with it -- if found.
 */
static int xia__DoMMUConfig(Module* m) {
    int status;

    char name[MAXFILENAME_LEN];
    char rawName[MAXFILENAME_LEN];

    char** mmu = NULL;

    ASSERT(m != NULL);

    status = xia__GetMMUName(m, 0, name, rawName);

    if (status != XIA_NO_MMU) {
        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xia__DoMMUConfig",
                   "Error trying to get MMU nume for alias '%s'", m->alias);
            return status;
        }

        strcpy(m->currentFirmware[0].currentMMU, rawName);

        mmu = (char**) handel_md_alloc(sizeof(char**));

        if (!mmu) {
            xiaLog(XIA_LOG_ERROR, XIA_NOMEM, "xia__DoMMUConfig",
                   "Error allocating %zu bytes for 'mmu'", sizeof(char**));
            return XIA_NOMEM;
        }

        mmu[0] = (char*) handel_md_alloc(strlen(name) + 1);

        if (!mmu[0]) {
            handel_md_free(mmu);

            xiaLog(XIA_LOG_ERROR, XIA_NOMEM, "xia__DoMMUConfig",
                   "Error allocating %zu bytes for 'mmu[0]'", strlen(name) + 1);
            return XIA_NOMEM;
        }

        strcpy(mmu[0], name);

        status = dxp_add_board_item("mmu", mmu);

        handel_md_free(mmu[0]);
        handel_md_free(mmu);

        if (status != DXP_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xia__DoMMUConfig",
                   "Error adding MMU to Xerxes for alias '%s'", m->alias);
            return status;
        }
    }

    return XIA_SUCCESS;
}

/*
 * Checks for the presence of a system FPGA and adds it to the
 * configuration.
 */
static int xia__DoSystemFPGA(Module* m) {
    int status;
    int detChan = 0;

    unsigned int i;

    char detType[MAXITEM_LEN];
    char sysFPGAName[MAX_PATH_LEN];
    char rawName[MAXFILENAME_LEN];

    boolean_t found = FALSE_;

    ASSERT(m != NULL);

    /* Assume that the first enabled detChan defines the correct detector type. */
    for (i = 0; i < m->number_of_channels; i++) {
        detChan = m->channels[i];

        if (detChan != -1) {
            break;
        }
    }

    status = xia__GetDetStringFromDetChan(detChan, m, detType);

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xia__DoSystemFPGA",
               "Error getting detector type string for alias '%s'", m->alias);
        return status;
    }

    status = xia__GetSystemFPGAName(m, detType, sysFPGAName, rawName, &found);

    if ((status != XIA_SUCCESS) && found) {
        xiaLog(XIA_LOG_ERROR, status, "xia__DoSystemFPGA",
               "Error getting System FPGA for alias '%s'", m->alias);
        return status;
    }

    if (found) {
        for (i = 0; i < m->number_of_channels; i++) {
            strcpy(m->currentFirmware[i].currentSysFPGA, rawName);
        }

        status = xia__AddSystemFPGA(m, sysFPGAName, rawName);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xia__DoSystemFPGA",
                   "Error adding System FPGA '%s' to Xerxes", sysFPGAName);
            return status;
        }
    }

    return XIA_SUCCESS;
}

/*
 * Gets the appropriate detector type string for the specified detChan.
 * Assumes that memory for type has already been allocated.
 */
static int xia__GetDetStringFromDetChan(int detChan, Module* m, char* type) {
    int modChan;

    char* detAlias = NULL;

    Detector* det = NULL;

    /* If a disabled channel is passed in, we just use module channel 0. */
    if (detChan != -1) {
        modChan = xiaGetModChan((unsigned int) detChan);
    } else {
        modChan = 0;
    }

    detAlias = m->detector[modChan];
    det = xiaFindDetector(detAlias);

    ASSERT(det != NULL);

    switch (det->type) {
        case XIA_DET_RESET:
            strcpy(type, "RESET");
            break;
        case XIA_DET_RCFEED:
            strcpy(type, "RC");
            break;
        default:
        case XIA_DET_UNKNOWN:
            xiaLog(XIA_LOG_ERROR, XIA_MISSING_TYPE, "xia__GetDetStringFromDetChan",
                   "No detector type specified for detChan %d", detChan);
            return XIA_MISSING_TYPE;
            break;
    }

    return XIA_SUCCESS;
}

/*
 * Checks for the presence of a system DSP and adds it to the
 * configuration.
 */
static int xia__DoSystemDSP(Module* m, boolean_t* found) {
    int detChan = -1;
    int status;

    unsigned int i;

    char detType[MAXITEM_LEN];
    char sysDSPName[MAX_PATH_LEN];
    char rawName[MAXFILENAME_LEN];

    ASSERT(m != NULL);
    ASSERT(found != NULL);

    for (i = 0; i < m->number_of_channels; i++) {
        detChan = m->channels[i];
        if (detChan != -1) {
            break;
        }
    }

    status = xia__GetDetStringFromDetChan(detChan, m, detType);

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xia__DoSystemDSP",
               "Error getting detector type string for alias '%s'", m->alias);
        return status;
    }

    status = xia__GetSystemDSPName(m, detType, sysDSPName, rawName, found);

    if ((status != XIA_SUCCESS) && *found) {
        xiaLog(XIA_LOG_ERROR, status, "xia__DoSystemDSP",
               "Error getting System DSP for alias '%s'", m->alias);
        return status;
    }

    if (*found) {
        status = xia__AddSystemDSP(m, sysDSPName, rawName);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xia__DoSystemDSP",
                   "Error adding System DSP '%s' to Xerxes", sysDSPName);
            return status;
        }
    }

    return XIA_SUCCESS;
}

/*
 * Adds the DSP info for each channel to the Xerxes configuration.
 */
static int xia__DoDSP(Module* m) {
    int status;

    int detChan;

    unsigned int i;

    double pt = 0.0;

    char detType[MAXITEM_LEN];
    char dspName[MAX_PATH_LEN];
    char rawName[MAXFILENAME_LEN];
    char chan[MAX_CHAN_LEN];

    char* dspStr[2];

    ASSERT(m != NULL);

    /* We skip channels that are disabled (detChan = -1) */
    for (i = 0; i < m->number_of_channels; i++) {
        detChan = m->channels[i];

        if (detChan == -1) {
            continue;
        }

        status = xia__GetDetStringFromDetChan(detChan, m, detType);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xia__DoDSP",
                   "Error getting detector type string for alias '%s'", m->alias);
            return status;
        }

        pt = xiaGetValueFromDefaults("peaking_time", m->defaults[i]);

        status = xia__GetDSPName(m, i, pt, dspName, detType, rawName);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xia__DoDSP",
                   "Error getting DSP name for alias '%s'", m->alias);
            return status;
        }

        /* Update the current firmware cache */
        strcpy(m->currentFirmware[i].currentDSP, rawName);

        dspStr[0] = &(chan[0]);
        dspStr[1] = &(dspName[0]);

        sprintf(dspStr[0], "%u", i);

        status = dxp_add_board_item("dsp", (char**) dspStr);

        if (status != DXP_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xia__DoDSP",
                   "Error adding 'dsp' for alias '%s'", m->alias);
            return status;
        }
    }

    return XIA_SUCCESS;
}

/*
 * Checks for the presence of FiPPI A and adds it to the configuration.
 */
static int xia__DoFiPPIA(Module* m, boolean_t* found) {
    int status;
    int detChan = 0;

    unsigned int i;

    char detType[MAXITEM_LEN];
    char fippiAName[MAX_PATH_LEN];
    char rawName[MAXFILENAME_LEN];

    ASSERT(m != NULL);
    ASSERT(found != NULL);

    for (i = 0; i < m->number_of_channels; i++) {
        detChan = m->channels[i];

        if (detChan != -1) {
            break;
        }
    }

    status = xia__GetDetStringFromDetChan(detChan, m, detType);

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xia__DoFiPPIA",
               "Error getting detector type string for alias '%s'", m->alias);
        return status;
    }

    status = xia__GetFiPPIAName(m, detType, fippiAName, rawName, found);

    if ((status != XIA_SUCCESS) && *found) {
        xiaLog(XIA_LOG_ERROR, status, "xia__DoFiPPIA",
               "Error getting FiPPI A for alias '%s'", m->alias);
        return status;
    }

    /* Set FiPPIA as the FiPPI for all of the channels */
    for (i = 0; i < m->number_of_channels; i++) {
        strcpy(m->currentFirmware[i].currentFiPPI, rawName);
    }

    if (*found) {
        status = xia__AddFiPPIA(m, fippiAName, rawName);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xia__DoFiPPIA",
                   "Error adding FiPPI A '%s' to Xerxes for alias '%s'", fippiAName,
                   m->alias);
            return status;
        }
    }

    return XIA_SUCCESS;
}

/*
 * Adds the FiPPI data for each enabled channel to the Xerxes configuration.
 */
static int xia__DoFiPPI(Module* m, boolean_t* found) {
    int status;

    int detChan;

    unsigned int i;

    double pt = 0.0;

    char detType[MAXITEM_LEN];
    char fippiName[MAX_PATH_LEN];
    char rawName[MAXFILENAME_LEN];
    char chan[MAX_CHAN_LEN];

    char* fippiStr[2];

    ASSERT(m != NULL);

    /* We skip channels that are disabled (detChan = -1) */
    for (i = 0; i < m->number_of_channels; i++) {
        detChan = m->channels[i];

        if (detChan == -1) {
            continue;
        }

        status = xia__GetDetStringFromDetChan(detChan, m, detType);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xia__DoFiPPI",
                   "Error getting detector type string for alias '%s'", m->alias);
            return status;
        }

        pt = xiaGetValueFromDefaults("peaking_time", m->defaults[i]);

        status = xia__GetFiPPIName(m, i, pt, fippiName, detType, rawName, found);

        if (status != XIA_SUCCESS && *found) {
            xiaLog(XIA_LOG_ERROR, status, "xia__DoFiPPI",
                   "Error getting FiPPI name for alias '%s'", m->alias);
            return status;
        }

        /* Update the current firmware cache */
        strcpy(m->currentFirmware[i].currentFiPPI, rawName);

        fippiStr[0] = &(chan[0]);
        fippiStr[1] = &(fippiName[0]);

        sprintf(fippiStr[0], "%u", i);

        if (*found) {
            status = dxp_add_board_item("fippi", (char**) fippiStr);

            if (status != DXP_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xia__DoFiPPI",
                       "Error adding 'fippi' for alias '%s'", m->alias);
                return status;
            }
        }
    }

    return XIA_SUCCESS;
}

/*
 * Checks for the presence of a System FiPPI and adds it to the configuration.
 */
static int xia__DoSystemFiPPI(Module* m, boolean_t* found) {
    int detChan = -1;
    int status;

    unsigned int i;

    char detType[MAXITEM_LEN];
    char sysFipName[MAX_PATH_LEN];
    char rawName[MAXFILENAME_LEN];

    ASSERT(m != NULL);

    *found = FALSE_;

    /* Find the first enabled channel and use that to get the detector type. */
    for (i = 0; i < m->number_of_channels; i++) {
        detChan = m->channels[i];
        if (detChan != -1) {
            break;
        }
    }

    status = xia__GetDetStringFromDetChan(detChan, m, detType);

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xia__DoSystemFiPPI",
               "Error getting detector type string for alias '%s'", m->alias);
        return status;
    }

    status = xia__GetSystemFiPPIName(m, detType, sysFipName, rawName, found);

    if ((status != XIA_SUCCESS) && *found) {
        xiaLog(XIA_LOG_ERROR, status, "xia__DoSystemFiPPI",
               "Error getting System FiPPI for alias '%s'", m->alias);
        return status;
    }

    if (*found) {
        for (i = 0; i < m->number_of_channels; i++) {
            strcpy(m->currentFirmware[i].currentSysFiPPI, rawName);
        }

        status = xia__AddSystemFiPPI(m, sysFipName, rawName);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xia__DoSystemFiPPI",
                   "Error adding System FiPPI '%s' to Xerxes", sysFipName);
            return status;
        }
    }

    return XIA_SUCCESS;
}

/*
 * Retrieves the System FiPPI name from the firmware, if it exists.
 *
 * found is only set to TRUE if a System FiPPI is defined in the FDD file. If
 * the module does not use an FDD file, FALSE will be returned.
 *
 * System FiPPIs are used on hardware that only have a single FPGA and no
 * DSP. We use a fake peaking time since the System FiPPI will be defined over
 * the entire peaking time range of the product.
 */
static int xia__GetSystemFiPPIName(Module* m, char* detType, char* sysFipName,
                                   char* rawFilename, boolean_t* found) {
    int status;

    double fakePT = 1.0;

    FirmwareSet* fs = NULL;

    ASSERT(m != NULL);
    ASSERT(detType != NULL);
    ASSERT(sysFipName != NULL);
    ASSERT(rawFilename != NULL);
    ASSERT(found != NULL);
    ASSERT(m->firmware[0] != NULL);

    *found = FALSE_;

    fs = xiaFindFirmware(m->firmware[0]);

    if (!fs || !fs->filename) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_FIRMWARE, "xia__GetSystemFiPPIName",
               "No firmware set defined for alias '%s'", m->firmware[0]);
        return XIA_NULL_FIRMWARE;
    }

    status = xiaFddGetAndCacheFirmware(fs, "system_fippi", fakePT, detType, sysFipName,
                                       rawFilename);

    /*
     * This is not necessarily an error. We will still pass the error value
     * up to the top-level but only use an informational message. For products
     * without system_fippis defined in their FDD files this message will always
     * appear, and we don't want them to be confused by spurious ERRORs.
     */
    if (status == XIA_FILEERR) {
        xiaLog(XIA_LOG_INFO, "xia__GetSystemFiPPIName",
               "No system_fippi defined in '%s'", fs->filename);
        return XIA_FILEERR;
    }

    *found = TRUE_;

    /* These are "real" errors, not just missing file problems. */
    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xia__GetSystemFiPPIName",
               "Error finding system_fippi in '%s'", fs->filename);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Adds a System FiPPI to the Xerxes configuration.
 */
static int xia__AddSystemFiPPI(Module* m, char* sysFipName, char* rawFilename) {
    int status;

    /* Xerxes requires items as lists of strings. */
    char* sysFipStr[1];

    UNUSED(rawFilename);
    UNUSED(m);

    ASSERT(sysFipName != NULL);

    sysFipStr[0] = handel_md_alloc(strlen(sysFipName) + 1);

    if (sysFipStr[0] == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NOMEM, "xia__AddSystemFiPPI",
               "Unable to allocated %zu bytes for 'sysFipStr[0]'",
               strlen(sysFipName) + 1);
        return XIA_NOMEM;
    }

    strcpy(sysFipStr[0], sysFipName);

    status = dxp_add_board_item("system_fippi", (char**) sysFipStr);

    handel_md_free(sysFipStr[0]);

    if (status != DXP_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xia__AddSystemFiPPI",
               "Error adding 'system_fippi', '%s', board item to Xerxes configuration",
               sysFipName);
        return status;
    }

    return XIA_SUCCESS;
}
