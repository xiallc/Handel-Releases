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

#include "xia_assert.h"
#include "xia_common.h"
#include "xia_handel.h"
#include "xia_system.h"

#include "handel_errors.h"
#include "handel_log.h"

#include "fdd.h"
#include "psl.h"

#include "xerxes.h"
#include "xerxes_errors.h"

/* Constants */
#define MAX_TYPE_LEN 32

/* Static routines */
static int HANDEL_API xiaValidateFirmwareSets(void);
static int HANDEL_API xiaValidateDetector(void);
static int HANDEL_API xiaValidateModule(PSLFuncs* funcs, unsigned int detChan);
static int HANDEL_API xiaValidateDetChan(DetChanElement* current);
static int HANDEL_API xiaValidateDetSet(DetChanElement* head);

static boolean_t HANDEL_API xiaIsFDFvsFirmValid(FirmwareSet* fSet);
static boolean_t HANDEL_API xiaArePTRsValid(Firmware** firmware);
static boolean_t HANDEL_API xiaAreFiPPIAndDSPValid(Firmware* firmware);
static boolean_t HANDEL_API xiaArePolaritiesValid(Detector* detector);
static boolean_t HANDEL_API xiaAreGainsValid(Detector* detector);
static boolean_t HANDEL_API xiaIsTypeValid(Detector* detector);

static int HANDEL_API _parseMemoryName(char* name, char* type, boolean_t* isRead,
                                       unsigned long* addr, unsigned long* len);

/*
 * Starts the system previously defined via .ini file or dynamic configuration.
 */
HANDEL_EXPORT int HANDEL_API xiaStartSystem(void) {
    /*
     * This routine does the following:
     * 1) Validates the information in HanDeL's data structures
     * 2) Builds XerXes data structures from its own
     * 3) Downloads firmware to specified detChans
     */

    int status;

    DetChanElement* current = NULL;
    xiaLog(XIA_LOG_INFO, "xiaStartSystem", "Starting system...");

    status = xiaValidateFirmwareSets();

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaStartSystem",
               "Error validating system-wide firmware sets.");
        return status;
    }

    status = xiaValidateDetector();

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaStartSystem",
               "Error validating system-wide detector configurations.");
        return status;
    }

    current = xiaGetDetChanHead();

    if (!current) {
        xiaLog(XIA_LOG_ERROR, XIA_NO_DETCHANS, "xiaStartSystem",
               "Unable to start system, no detChans "
               "are defined.");
        return XIA_NO_DETCHANS;
    }

    while (current != NULL) {
        switch (xiaGetElemType(current->detChan)) {
            case SET:
                xiaClearTags();
                status = xiaValidateDetSet(current);
                break;
            case SINGLE:
                status = xiaValidateDetChan(current);
                break;
            case 999:
                status = XIA_INVALID_DETCHAN;
                xiaLog(XIA_LOG_ERROR, status, "xiaStartSystem",
                       "detChan %d has an invalid type.", current->detChan);
                /* Let the check below do the "return" for us. */
                break;
            default:
                FAIL();
                break;
        }

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaStartSystem",
                   "Error validating detChan %d.", current->detChan);
            return status;
        }

        current = getListNext(current);
    }

    status = xiaBuildXerxesConfig();

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaStartSystem", "Error configuring Xerxes.");
        return status;
    }

    status = xiaUserSetup();

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaStartSystem",
               "Error performing user setup tasks.");
        return status;
    }

    xiaLog(XIA_LOG_INFO, "xiaStartSystem", "System started successfully.");
    return XIA_SUCCESS;
}

/*
 * Downloads firmware to a detector channel or set of detector channels.
 *
 * -1 is not allowed for the detChan.
 *
 * Allowed values for type are "dsp", "fippi", "user_dsp", "user_fippi",
 * and "system_fpga".
 *
 * The task of downloading firmware is handled by xiaStartSystem(), so
 * this routine should only be used for situations where special firmware is
 * required.
 */
HANDEL_EXPORT int HANDEL_API xiaDownloadFirmware(int detChan, char* type) {
    int status;
    int elemType;

    unsigned int modChan;

    double peakingTime;

    char boardType[MAXITEM_LEN];
    char fileName[MAX_PATH_LEN];
    char rawFilename[MAXFILENAME_LEN];
    char detType[MAXITEM_LEN];

    char* alias;
    char* firmAlias;
    char* defAlias;

    FirmwareSet* firmwareSet = NULL;

    Module* module = NULL;

    Detector* detector = NULL;

    DetChanElement* detChanElem = NULL;

    DetChanSetElem* detChanSetElem = NULL;

    CurrentFirmware* currentFirmware = NULL;

    XiaDefaults* defs = NULL;

    PSLFuncs localFuncs;

    xiaLog(XIA_LOG_INFO, "xiaDownloadFirmware", "Downloading firmware");

    if (type == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_TYPE, "xiaDownloadFirmware",
               "name cannot be NULL");
        return XIA_NULL_TYPE;
    }

    elemType = xiaGetElemType((unsigned int) detChan);

    switch (elemType) {
        case SINGLE:
            alias = xiaGetAliasFromDetChan(detChan);
            /* Might want to check for NULL here but should be okay since I believe
             * that xiaGetElemType returns an error value if the detChan doesn't
             * exist.
             */
            module = xiaFindModule(alias);
            modChan = xiaGetModChan((unsigned int) detChan);
            detector = xiaFindDetector(module->detector[modChan]);
            firmAlias = module->firmware[modChan];
            defAlias = module->defaults[modChan];

            currentFirmware = &module->currentFirmware[modChan];

            peakingTime = xiaGetValueFromDefaults("peaking_time", defAlias);

            defs = xiaGetDefaultFromDetChan(detChan);
            ASSERT(defs != NULL);

            firmwareSet = xiaFindFirmware(firmAlias);

            if (firmwareSet->filename == NULL) {
                status =
                    xiaGetValueFromFirmware(firmAlias, peakingTime, type, fileName);
                if (status != XIA_SUCCESS) {
                    xiaLog(XIA_LOG_ERROR, status, "xiaDownloadFirmware",
                           "Error getting %s from %s", type, firmAlias);
                    return status;
                }

                /* For now use the filename as the rawFilename for the non-FDD case.
                 * I don't really think that it is a problem since the currentFirm.
                 * struct just wants a unique ID of some sort and the filenames in
                 * the Firmware struct are unique enough. (It's just that they
                 * aren't in the case of the FDD since the FDD DLL spits out files
                 * with basically the same name.
                 */
                strcpy(rawFilename, fileName);
            } else {
                switch (detector->type) {
                    case XIA_DET_RESET:
                        strcpy(detType, "RESET");
                        break;
                    case XIA_DET_RCFEED:
                        strcpy(detType, "RC_FEEDBACK");
                        break;
                    default:
                        xiaLog(XIA_LOG_ERROR, XIA_UNKNOWN, "xiaDownloadFirmware",
                               "Should not be seeing this message");
                        return XIA_UNKNOWN;
                }

                if (!firmwareSet->tmpPath) {
                    firmwareSet->tmpPath = utils->funcs->dxp_md_tmp_path();
                }

                /* Use the FDD here */
                status = xiaFddGetAndCacheFirmware(firmwareSet, type, peakingTime,
                                                   detType, fileName, rawFilename);

                if (status != XIA_SUCCESS) {
                    xiaLog(XIA_LOG_ERROR, status, "xiaDownloadFirmware",
                           "Error getting firmware from FDD");
                    return status;
                }
            }

            status = xiaGetBoardType(detChan, boardType);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaDownloadFirmware",
                       "Unable to get boardType for detChan %d", detChan);
                return status;
            }

            status = xiaLoadPSL(boardType, &localFuncs);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaDownloadFirmware",
                       "Unable to load PSL functions for boardType %s", boardType);
                return status;
            }

            status = localFuncs.downloadFirmware(detChan, type, fileName, module,
                                                 rawFilename, defs);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaDownloadFirmware",
                       "Unable to download Firmware for detChan %d", detChan);
                return status;
            }

            /* Sync up the current firmware structure here */
            if (STREQ(type, "fippi")) {
                strcpy(currentFirmware->currentFiPPI, rawFilename);
            } else if (STREQ(type, "dsp")) {
                strcpy(currentFirmware->currentDSP, rawFilename);
            } else if (STREQ(type, "user_fippi")) {
                strcpy(currentFirmware->currentUserFiPPI, rawFilename);
            } else if (STREQ(type, "user_dsp")) {
                strcpy(currentFirmware->currentUserDSP, rawFilename);
            } else if (STREQ(type, "system_fpga")) {
                strcpy(currentFirmware->currentSysFPGA, rawFilename);
            }
            break;
        case SET:
            /* We've already verified that there are no infinite loops in the detChan sets
             * by this point, so we don't need to check the isTagged field
             */
            detChanElem = xiaGetDetChanPtr((unsigned int) detChan);

            /* We know that it is a SET... */
            detChanSetElem = detChanElem->data.detChanSet;

            while (detChanSetElem != NULL) {
                status = xiaDownloadFirmware((int) detChanSetElem->channel, type);

                if (status != XIA_SUCCESS) {
                    xiaLog(XIA_LOG_ERROR, status, "xiaDownloadFirmware",
                           "Error downloading firmware to detChan %u",
                           detChanSetElem->channel);
                    return status;
                }

                detChanSetElem = getListNext(detChanSetElem);
            }
            break;
        case 999:
            xiaLog(XIA_LOG_ERROR, XIA_INVALID_DETCHAN, "xiaDownloadFirmware",
                   "detChan number is not in the list of valid values ");
            return XIA_INVALID_DETCHAN;
        default:
            xiaLog(XIA_LOG_ERROR, XIA_UNKNOWN, "xiaDownloadFirmware",
                   "Should not be seeing this message");
            return XIA_UNKNOWN;
    }

    xiaLog(XIA_LOG_INFO, "xiaDownloadFirmware", "Firmware downloaded successfully.");
    return XIA_SUCCESS;
}

/*
 * Loops over all the elements of the FirmwareSets LL and
 * checks that the data is valid. In the case of a misconfiguration, the
 * returned error code should indicate what part of the FirmwareSet is
 * invalid.
 *
 * The current logic is as follows:
 * 1) FirmwareSet must either define an FDF file OR a "Firmware" (LL)
 *
 * For the Firmware Elements within the FirmwareSet:
 * 1) The peaking time ranges may not overlap between different PTRRs. This
 *    creates an unacceptably ambiguous situation about which PTRR to use
 *    as the firmware definition for a given peaking time in the overlapped
 *    range.
 * 2) A (FiPPI OR User FiPPI) AND DSP must be defined for each element.
 */
static int HANDEL_API xiaValidateFirmwareSets(void) {
    int status;
    FirmwareSet* current = NULL;

    current = xiaGetFirmwareSetHead();

    while (current != NULL) {
        xiaLog(XIA_LOG_DEBUG, "xiaValidateFirmwareSets", "Validating firmware %s",
               current->alias);

        if (!xiaIsFDFvsFirmValid(current)) {
            status = XIA_FIRM_BOTH;
            xiaLog(XIA_LOG_WARNING, "xiaValidateFirmwareSets",
                   "Firmware alias %s contains both an FDF and Firmware definitions",
                   current->alias);
        }

        /* If we only have an FDF file then we don't need to do anything else */
        if (current->filename != NULL) {
            return XIA_SUCCESS;
        }

        if (!xiaArePTRsValid(&(current->firmware))) {
            xiaLog(XIA_LOG_ERROR, XIA_PTR_OVERLAP, "xiaValidateFirmwareSets",
                   "Firmware definitions in alias %s have overlapping peaking times",
                   current->alias);
            return XIA_PTR_OVERLAP;
        }

        if (!xiaAreFiPPIAndDSPValid(current->firmware)) {
            xiaLog(
                XIA_LOG_ERROR, XIA_MISSING_FIRM, "xiaValidateFirmwareSets",
                "Firmware definition(s) in alias %s is/are missing FiPPI and DSP files",
                current->alias);
            return XIA_MISSING_FIRM;
        }

        current = getListNext(current);
    }

    return XIA_SUCCESS;
}

/*
 * Loops over all the elements of the Detector LL and checks
 * that the data is valid. In the case of misconfiguration, the returned error
 * code should indicate what part of the Detector is invalid.
 *
 * The current logic is as follows:
 * 1) Check that all polarities are valid from 0...nchan - 1
 * 2) Check that all gains are within a valid range from 0...nchan - 1
 * 3) Check that the type is defined beyond XIA_DET_UNKNOWN
 */
static int HANDEL_API xiaValidateDetector(void) {
    Detector* current = NULL;
    current = xiaGetDetectorHead();

    while (current != NULL) {
        if (!xiaArePolaritiesValid(current)) {
            xiaLog(XIA_LOG_ERROR, XIA_MISSING_POL, "xiaValidateDetector",
                   "Missing polarity in alias %s\n", current->alias);
            return XIA_MISSING_POL;
        }

        if (!xiaAreGainsValid(current)) {
            xiaLog(XIA_LOG_ERROR, XIA_MISSING_GAIN, "xiaValidateDetector",
                   "Missing gain in alias %s\n", current->alias);
            return XIA_MISSING_GAIN;
        }

        if (!xiaIsTypeValid(current)) {
            xiaLog(XIA_LOG_ERROR, XIA_MISSING_TYPE, "xiaValidateDetector",
                   "Missing type in alias %s\n", current->alias);
            return XIA_MISSING_TYPE;
        }

        current = getListNext(current);
    }

    return XIA_SUCCESS;
}

/*
 * This routine uses the product specific pointer to validate the module
 * data for the specified detChan. Basically, this routine just passes the
 * module information straight to the PSL layer since there is no point in
 * doing any "partial" verification here.
 */
static int HANDEL_API xiaValidateModule(PSLFuncs* funcs, unsigned int detChan) {
    int status;

    Module* current = NULL;

    XiaDefaults* defaults = NULL;

    current = xiaFindModule(xiaGetAliasFromDetChan(detChan));

    status = funcs->validateModule(current);

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaValidateModule", "Error validating module");
        return status;
    }

    defaults = xiaGetDefaultFromDetChan(detChan);

    status = funcs->validateDefaults(defaults);

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaValidateModule",
               "Error validating defaults for module");
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This routine contains the logic to check if and FDF and Firmware
 * definitions are defined in the FirmwareSet fSet.
 */
static boolean_t HANDEL_API xiaIsFDFvsFirmValid(FirmwareSet* fSet) {
    if ((fSet->filename != NULL) && (fSet->firmware != NULL)) {
        return FALSE_;
    }

    if ((fSet->filename == NULL) && (fSet->firmware == NULL)) {
        return FALSE_;
    }

    return TRUE_;
}

/*
 * Sorts the Firmware LL by min peaking time (which should
 * already be verified at the insertion point) and then checks the peaking
 * times for overlap. Assumes the firmware is non-NULL.
 */
static boolean_t HANDEL_API xiaArePTRsValid(Firmware** firmware) {
    Firmware* current = NULL;
    Firmware* lookAhead = NULL;

    if (xiaInsertSort(firmware, xiaFirmComp) < 0) {
        return FALSE_;
    }

    current = *firmware;
    while (current != NULL) {
        /*
         * The basic theory here is that since the Firmware LL is sorted based on
         * min peaking time, we can check that the max peaking time for a given element
         * does not overlap with any of the other min peaking times that are "past" it
         * in the list.
         */
        lookAhead = getListNext(current);

        while (lookAhead != NULL) {
            if (current->max_ptime > lookAhead->min_ptime) {
                return FALSE_;
            }
            lookAhead = getListNext(lookAhead);
        }
        current = getListNext(current);
    }

    return TRUE_;
}

/*
 * This routine checks that a FiPPI and DSP are defined
 */
static boolean_t HANDEL_API xiaAreFiPPIAndDSPValid(Firmware* firmware) {
    if (firmware->dsp == NULL) {
        return FALSE_;
    }

    if ((firmware->fippi == NULL) && (firmware->user_fippi == NULL)) {
        return FALSE_;
    }

    return TRUE_;
}

/*
 * Verifies that all the polarities in detector have a valid
 * value. Essentially, all that this *really* checks is that all the
 * values in the polarity array are set to something. This is the case
 * because when the polarity is set via a call to xiaAddDetectorItem(), the
 * value is verified, therefore the value can only be out-of-range if it
 * hasn't been set yet.
 */
static boolean_t HANDEL_API xiaArePolaritiesValid(Detector* detector) {
    unsigned int i;

    for (i = 0; i < detector->nchan; i++) {
        if ((detector->polarity[i] != 1) && (detector->polarity[i] != 0)) {
            return FALSE_;
        }
    }

    return TRUE_;
}

/*
 * Verifies that all the gains in detector are within a valid
 * range. This routine has the same caveats as xiaArePolaritiesValid() in the
 * sense that the value should really only be out-of-range if it hasn't been
 * set yet.
 */
static boolean_t HANDEL_API xiaAreGainsValid(Detector* detector) {
    unsigned int i;

    for (i = 0; i < detector->nchan; i++) {
        if ((detector->gain[i] < XIA_GAIN_MIN) || (detector->gain[i] > XIA_GAIN_MAX)) {
            return FALSE_;
        }
    }

    return TRUE_;
}

/*
 * Verifies that the type isn't XIA_DET_UNKNOWN (since that is
 * what it is initialized to.
 */
static boolean_t HANDEL_API xiaIsTypeValid(Detector* detector) {
    if (detector->type == XIA_DET_UNKNOWN) {
        return FALSE_;
    }

    return TRUE_;
}

/*
 * Validates information for a SINGLE detChan.
 */
static int HANDEL_API xiaValidateDetChan(DetChanElement* current) {
    int status;

    char boardType[MAXITEM_LEN];

    PSLFuncs localFuncs;

    status = xiaGetBoardType(current->detChan, boardType);

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaValidateDetChan",
               "Error getting board type for specified detChan");
        return status;
    }

    status = xiaLoadPSL(boardType, &localFuncs);

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaValidateDetChan",
               "Error loading PSL functions");
        return status;
    }

    status = xiaValidateModule(&localFuncs, current->detChan);

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaValidateDetChan",
               "Error validating Module data for detChan %d", current->detChan);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Checks a detChanSet for infinite loops. Assumes that head is
 * a set.
 */
static int HANDEL_API xiaValidateDetSet(DetChanElement* head) {
    int status = XIA_SUCCESS;

    DetChanElement* current = NULL;
    DetChanSetElem* element = NULL;

    /*
     * We only want to tag detChans that are sets since multiple sets can
     * include references to SINGLE detChans.
     */
    head->isTagged = TRUE_;

    element = head->data.detChanSet;

    while (element != NULL) {
        current = xiaGetDetChanPtr(element->channel);
        switch (current->type) {
            case SINGLE:
                status = XIA_SUCCESS;
                break;
            case SET:
                if (current->isTagged) {
                    xiaLog(XIA_LOG_ERROR, XIA_INFINITE_LOOP, "xiaValidateDetSet",
                           "Infinite loop detected involving detChan %d",
                           current->detChan);
                    return XIA_INFINITE_LOOP;
                }
                status = xiaValidateDetSet(current);
                break;
            default:
                status = XIA_UNKNOWN;
                break;
        }

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaValidateDetSet",
                   "Error validating detChans");
            return status;
        }

        element = getListNext(element);
    }

    return status;
}

/*
 * Initializes funcs to be of the proper type.
 */
int HANDEL_API xiaLoadPSL(char* boardType, PSLFuncs* funcs) {
    int status;
    UNUSED(funcs);

    /*
     * XXX TODO Use a list of function pointers to call these functions.
     */

    /*
     * We need this 'if' just so the conditionally compiled board types can use
     * 'else if'.
     */
    if (boardType == NULL) {
        status = XIA_UNKNOWN_BOARD;
#ifndef EXCLUDE_SATURN
    } else if (STREQ(boardType, "dxpx10p")) {
        status = saturn_PSLInit(funcs);
#endif /* EXCLUDE_SATURN */
#ifndef EXCLUDE_UDXPS
    } else if (STREQ(boardType, "udxps")) {
        status = udxps_PSLInit(funcs);
#endif /* EXCLUDE_UDXPS */
#ifndef EXCLUDE_UDXP
    } else if (STREQ(boardType, "udxp")) {
        status = udxp_PSLInit(funcs);
#endif /* EXCLUDE_UDXP */
#ifndef EXCLUDE_XMAP
    } else if (STREQ(boardType, "xmap")) {
        status = xmap_PSLInit(funcs);
#endif /* EXCLUDE_XMAP */
#ifndef EXCLUDE_STJ
    } else if (STREQ(boardType, "stj")) {
        status = stj_PSLInit(funcs);
#endif /* EXCLUDE_STJ */
#ifndef EXCLUDE_MERCURY
    } else if (STREQ(boardType, "mercury")) {
        status = mercury_PSLInit(funcs);
#endif /* EXCLUDE_MERCURY */
    } else {
        funcs = NULL;
        status = XIA_UNKNOWN_BOARD;
    }

    if (status == XIA_UNKNOWN_BOARD) {
        xiaLog(XIA_LOG_ERROR, status, "xiaLoadPSL",
               "Board type '%s' is not supported in this version of the library",
               boardType);
        return status;
    } else if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaLoadPSL", "Error initializing PSL functions");
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Performs non-persistent operations on the board.
 */
HANDEL_EXPORT int HANDEL_API xiaBoardOperation(int detChan, char* name, void* value) {
    int status;
    int elemType;

    char boardType[MAXITEM_LEN];

    XiaDefaults* defs = NULL;

    PSLFuncs localFuncs;

    if (name == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_NAME, "xiaBoardOperation",
               "name cannot be NULL");
        return XIA_NULL_NAME;
    }

    if (value == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_VALUE, "xiaBoardOperation",
               "value cannot be NULL");
        return XIA_NULL_VALUE;
    }

    elemType = xiaGetElemType((unsigned int) detChan);

    switch (elemType) {
        case SINGLE:
            status = xiaGetBoardType(detChan, boardType);
            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaBoardOperation",
                       "Unable to get boardType for detChan %d", detChan);
                return status;
            }

            defs = xiaGetDefaultFromDetChan(detChan);

            if (!defs) {
                xiaLog(XIA_LOG_ERROR, XIA_BAD_CHANNEL, "xiaBoardOperation",
                       "Error getting defaults for detChan %d", detChan);
                return XIA_BAD_CHANNEL;
            }

            status = xiaLoadPSL(boardType, &localFuncs);
            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaBoardOperation",
                       "Unable to load PSL functions for boardType %s", boardType);
                return status;
            }

            status = localFuncs.boardOperation(detChan, name, value, defs);
            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaBoardOperation",
                       "Unable to do board operation (%s) for detChan %d", name,
                       detChan);
                return status;
            }
            break;
        case SET:
            xiaLog(XIA_LOG_ERROR, XIA_BAD_TYPE, "xiaBoardOperation",
                   "This routine only supports single detChans");
            return XIA_BAD_TYPE;
        case 999:
            xiaLog(XIA_LOG_ERROR, XIA_INVALID_DETCHAN, "xiaBoardOperation",
                   "detChan number is not in the list of valid values ");
            return XIA_INVALID_DETCHAN;
        default:
            xiaLog(XIA_LOG_ERROR, XIA_UNKNOWN, "xiaBoardOperation",
                   "Should not be seeing this message");
            return XIA_UNKNOWN;
    }

    return XIA_SUCCESS;
}

/*
 * Performs a raw memory operation on a module.
 *
 * This is an advanced routine that does some simple parsing
 * of the name and then passes the result down to Xerxes
 * directly.
 *
 * Name format: "[type]:[r|w]:[addr]:[len]"
 */
HANDEL_EXPORT int HANDEL_API xiaMemoryOperation(int detChan, char* name, void* value) {
    int status;

    char* nameX = NULL;

    char type[MAX_TYPE_LEN];

    boolean_t isRead = FALSE_;

    unsigned long addr = 0x00000000;
    unsigned long len = 0x00000000;

    if (name == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_NAME, "xiaMemoryOperation",
               "name cannot be NULL");
        return XIA_NULL_NAME;
    }

    if (value == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_VALUE, "xiaMemoryOperation",
               "value cannot be NULL");
        return XIA_NULL_VALUE;
    }

    xiaLog(XIA_LOG_DEBUG, "xiaMemoryOperation", "memory = %s", name);

    status = _parseMemoryName(name, type, &isRead, &addr, &len);

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaMemoryOperation",
               "Error parsing memory name");
        return status;
    }

    xiaLog(XIA_LOG_DEBUG, "xiaMemoryOperation",
           "type = '%s', isRead = '%u', addr = '%lx', len = '%lu'", type, isRead, addr,
           len);

    /*
     * The choice of malloc size may seem unusual, but we can easily promise that
     * the length of the Handel 'name' string is longer then the Xerxes equivalent
     * since the Xerxes string is subset of the Handel string.
     */
    nameX = (char*) handel_md_alloc(strlen(name) + 1);

    if (nameX == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NOMEM, "xiaMemoryOperation",
               "Out-of-memory creating nameX");
        return XIA_NOMEM;
    }

    sprintf(nameX, "%s:%lx:%lu", type, addr, len);

    if (isRead) {
        status = dxp_read_memory(&detChan, nameX, (unsigned long*) value);
    } else {
        status = dxp_write_memory(&detChan, nameX, (unsigned long*) value);
    }

    handel_md_free(nameX);

    if (status != DXP_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaMemoryOperation",
               "Error reading/writing memory ('%s') for detChan '%d'", name, detChan);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Executes a command directly on supported hardware.
 *
 * `send`: The data portion of the command. The rest of the command will
 * be added to this array by Handel.
 *
 * `recv`: The entire return command including any headers or checksums.
 */
HANDEL_EXPORT int HANDEL_API xiaCommandOperation(int detChan, byte_t cmd,
                                                 unsigned int lenS, byte_t* send,
                                                 unsigned int lenR, byte_t* recv) {
    int status;

    if (lenS > 0) {
        ASSERT(send != NULL);
    }

    if (lenR > 0) {
        ASSERT(recv != NULL);
    }

    status = dxp_cmd(&detChan, &cmd, &lenS, send, &lenR, recv);

    if (status != DXP_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaCommandOperation", "Error executing command");
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Sets the priority of the IO process.
 */
HANDEL_EXPORT int HANDEL_API xiaSetIOPriority(int pri) {
    int status;

    status = dxp_set_io_priority(&pri);

    if (status != DXP_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaSetIOPriority",
               "Error setting priority '%#x'", pri);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Parses in a memory string of the format defined for xiaMemoryOperation().
 */
static int HANDEL_API _parseMemoryName(char* name, char* type, boolean_t* isRead,
                                       unsigned long* addr, unsigned long* len) {
    int n_matched = 0;

    char* n = NULL;
    char* tok = NULL;
    char* delim = ":";

    if (name == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_NAME, "xiaSetIOPriority", "name cannot be null");
        return XIA_NULL_NAME;
    }

    if (type == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_TYPE, "xiaSetIOPriority", "name cannot be null");
        return XIA_NULL_TYPE;
    }

    if (isRead == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_VALUE, "xiaSetIOPriority",
               "isRead cannot be null");
        return XIA_NULL_VALUE;
    }

    if (addr == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_VALUE, "xiaSetIOPriority",
               "name cannot be null");
        return XIA_NULL_VALUE;
    }

    if (len == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_VALUE, "xiaSetIOPriority",
               "name cannot be null");
        return XIA_NULL_VALUE;
    }

    n = (char*) handel_md_alloc(strlen(name) + 1);

    if (n == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NOMEM, "_parseMemoryName",
               "Out-of-memory trying to create 'n' string");
        return XIA_NOMEM;
    }

    strncpy(n, name, strlen(name) + 1);

    tok = strtok(n, delim);

    if (tok == NULL) {
        handel_md_free(n);
        xiaLog(XIA_LOG_ERROR, XIA_INVALID_STR, "_parseMemoryName",
               "'%s' is not a valid memory string: missing 'type'", name);
        return XIA_INVALID_STR;
    }

    strncpy(type, tok, strlen(tok) + 1);

    tok = strtok(NULL, delim);

    if (tok == NULL) {
        handel_md_free(n);
        xiaLog(XIA_LOG_ERROR, XIA_INVALID_STR, "_parseMemoryName",
               "'%s' is not a valid memory string: missing 'r/w'", name);
        return XIA_INVALID_STR;
    }

    if (STREQ(tok, "r")) {
        *isRead = TRUE_;
    } else if (STREQ(tok, "w")) {
        *isRead = FALSE_;
    } else {
        handel_md_free(n);
        xiaLog(XIA_LOG_ERROR, XIA_INVALID_STR, "_parseMemoryName",
               "'%s' is not a valid r/w access specifier", tok);
        return XIA_INVALID_STR;
    }

    tok = strtok(NULL, delim);

    if (tok == NULL) {
        handel_md_free(n);
        xiaLog(XIA_LOG_ERROR, XIA_INVALID_STR, "_parseMemoryName",
               "'%s' is not a valid memory string: missing 'address'", name);
        return XIA_INVALID_STR;
    }

    n_matched = sscanf(tok, "%lx", addr);

    if (n_matched == 0) {
        handel_md_free(n);
        xiaLog(XIA_LOG_ERROR, XIA_INVALID_STR, "_parseMemoryName",
               "'%s' is not a valid memory string: bad address token '%s'", name, tok);
        return XIA_INVALID_STR;
    }

    tok = strtok(NULL, delim);

    if (tok == NULL) {
        handel_md_free(n);
        xiaLog(XIA_LOG_ERROR, XIA_INVALID_STR, "_parseMemoryName",
               "'%s' is not a valid memory string: missing 'length'", name);
        return XIA_INVALID_STR;
    }

    n_matched = sscanf(tok, "%lu", len);

    if (n_matched == 0) {
        handel_md_free(n);
        xiaLog(XIA_LOG_ERROR, XIA_INVALID_STR, "_parseMemoryName",
               "'%s' is not a valid memory string: bad length token '%s'", name, tok);
        return XIA_INVALID_STR;
    }

    handel_md_free(n);
    return XIA_SUCCESS;
}
