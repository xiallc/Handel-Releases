/*
 * This file contains the routines relating to control of the run
 * parameters, such as xiaSetAcquisitionValues and xiaGainOperation.
 */

/*
 * Copyright (c) 2002-2004 X-ray Instrumentation Associates
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

#include <ctype.h>

#include "xia_assert.h"
#include "xia_common.h"
#include "xia_handel.h"
#include "xia_handel_structures.h"
#include "xia_system.h"

#include "handel_errors.h"
#include "handel_log.h"
#include "handeldef.h"

static boolean_t HANDEL_API xiaIsUpperCase(char* string);

/*
 * Sets an acquisition value.
 *
 * detChan may be a single detChan or a detChan set.
 *
 * name may refer to an acquisition value or a DSP parameter.
 *
 * value must be a double *. value may be adjusted in translation. The
 * updated value is returned through the same parameter.
 */
HANDEL_EXPORT int HANDEL_API xiaSetAcquisitionValues(int detChan, char* name,
                                                     void* value) {
    int status;
    int elemType;
    int detector_chan;

    double savedValue;

    unsigned int modChan;

    char detectorType[MAXITEM_LEN];

    char* boardAlias;
    char* firmAlias;
    char* detectorAlias;

    char boardType[MAXITEM_LEN];

    boolean_t valueExists = FALSE_;

    DetChanElement* detChanElem = NULL;

    DetChanSetElem* detChanSetElem = NULL;

    XiaDefaults* defaults = NULL;

    XiaDaqEntry* entry = NULL;

    FirmwareSet* firmwareSet = NULL;

    Module* module = NULL;

    Detector* detector = NULL;

    CurrentFirmware* currentFirmware = NULL;

    PSLFuncs localFuncs;

    if (name == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_NAME, "xiaSetAcquisitionValues",
               "name cannot be NULL");
        return XIA_NULL_NAME;
    }

    if (value == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_VALUE, "xiaSetAcquisitionValues",
               "value cannot be NULL");
        return XIA_NULL_VALUE;
    }

    elemType = xiaGetElemType((unsigned int) detChan);

    switch (elemType) {
        case SINGLE:
            status = xiaGetBoardType(detChan, boardType);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaSetAcquisitionValues",
                       "Unable to get boardType for detChan %d", detChan);
                return status;
            }

            status = xiaLoadPSL(boardType, &localFuncs);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaSetAcquisitionValues",
                       "Unable to load PSL funcs for detChan %d", detChan);
                return status;
            }

            /*
             * I know that this part of the code is unbelievably weird. Ultimately,
             * the solution to this problem is to unify the dynamic config interface,
             * which we will do eventually. For now, we have to suffer though...
             */
            defaults = xiaGetDefaultFromDetChan((unsigned int) detChan);

            boardAlias = xiaGetAliasFromDetChan(detChan);
            module = xiaFindModule(boardAlias);
            modChan = xiaGetModChan((unsigned int) detChan);
            firmAlias = module->firmware[modChan];
            firmwareSet = xiaFindFirmware(firmAlias);
            /*
             * The preampGain is passed in directly since the detector alias also has
             * the detector channel information encoded into it.
             */
            detectorAlias = module->detector[modChan];
            detector_chan = module->detector_chan[modChan];
            detector = xiaFindDetector(detectorAlias);
            currentFirmware = &module->currentFirmware[modChan];

            switch (detector->type) {
                case XIA_DET_RESET:
                    strcpy(detectorType, "RESET");
                    break;
                case XIA_DET_RCFEED:
                    strcpy(detectorType, "RC");
                    break;
                default:
                case XIA_DET_UNKNOWN:
                    xiaLog(XIA_LOG_ERROR, XIA_MISSING_TYPE, "xiaSetAcquisitionValues",
                           "No detector type specified for detChan %d", detChan);
                    return XIA_MISSING_TYPE;
            }

            /* At this stage, xiaStartSystem() has been called so we
             * know that the required defaults are already present.
             * Therefore, any acq. value beyond this point is
             * assumed to be "special" and should be added if it
             * isn't present.
             */
            entry = defaults->entry;

            while (entry != NULL) {
                if (STREQ(name, entry->name)) {
                    valueExists = TRUE_;
                    break;
                }
                entry = entry->next;
            }

            if (!valueExists) {
                xiaLog(XIA_LOG_INFO, "xiaSetAcquisitionValues",
                       "Adding %s to defaults %s", name, defaults->alias);

                status = xiaAddDefaultItem(defaults->alias, name, value);

                if (status != XIA_SUCCESS) {
                    xiaLog(XIA_LOG_ERROR, status, "xiaSetAcquisitionValues",
                           "Error adding %s to defaults %s", name, defaults->alias);
                    return status;
                }
            }

            status = localFuncs.setAcquisitionValues(
                detChan, name, value, defaults, firmwareSet, currentFirmware,
                detectorType, detector, detector_chan, module, modChan);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaSetAcquisitionValues",
                       "Unable to set '%s' to %0.3f for detChan %d.", name,
                       *((double*) value), detChan);
                return status;
            }
            break;
        case SET:
            detChanElem = xiaGetDetChanPtr((unsigned int) detChan);
            detChanSetElem = detChanElem->data.detChanSet;

            /*
             * Save the user value, else it will be changed by the return value of the
             * setAcquisitionValues call. Use the last return value as the actual
             * return value
             */
            savedValue = *((double*) value);

            while (detChanSetElem != NULL) {
                *((double*) value) = savedValue;
                status =
                    xiaSetAcquisitionValues((int) detChanSetElem->channel, name, value);

                if (status != XIA_SUCCESS) {
                    xiaLog(XIA_LOG_ERROR, status, "xiaSetAcquisitionValues",
                           "Error setting acquisition values for detChan %d", detChan);
                    return status;
                }
                detChanSetElem = getListNext(detChanSetElem);
            }
            break;
        case 999:
            xiaLog(XIA_LOG_ERROR, XIA_INVALID_DETCHAN, "xiaSetAcquisitionValues",
                   "detChan number is not in the list of valid values ");
            return XIA_INVALID_DETCHAN;
        default:
            xiaLog(XIA_LOG_ERROR, XIA_UNKNOWN, "xiaSetAcquisitionValues",
                   "Should not be seeing this message");
            return XIA_UNKNOWN;
    }

    return XIA_SUCCESS;
}

/*
 * Gets an acquisition value. Unless otherwise noted, all acquisition
 * values require value to be a double *, cast as void *.
 */
HANDEL_EXPORT int HANDEL_API xiaGetAcquisitionValues(int detChan, char* name,
                                                     void* value) {
    int status;
    int elemType;

    char boardType[MAXITEM_LEN];

    XiaDefaults* defaults = NULL;

    PSLFuncs localFuncs;

    if (name == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_NAME, "xiaGetAcquisitionValues",
               "name cannot be NULL");
        return XIA_NULL_NAME;
    }

    if (value == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_VALUE, "xiaGetAcquisitionValues",
               "value cannot be NULL");
        return XIA_NULL_VALUE;
    }

    elemType = xiaGetElemType((unsigned int) detChan);

    switch (elemType) {
        case SET:
            xiaLog(XIA_LOG_ERROR, XIA_BAD_TYPE, "xiaGetAcquisitionValues",
                   "Unable to retrieve values for a detChan SET");
            return XIA_BAD_TYPE;
        case SINGLE:
            status = xiaGetBoardType(detChan, boardType);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaGetAcquisitionValues",
                       "Unable to get boardType for detChan %d", detChan);
                return status;
            }
            status = xiaLoadPSL(boardType, &localFuncs);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaGetAcquisitionValues",
                       "Unable to load PSL funcs for detChan %d", detChan);
                return status;
            }

            defaults = xiaGetDefaultFromDetChan((unsigned int) detChan);

            status = localFuncs.getAcquisitionValues(detChan, name, value, defaults);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaGetAcquisitionValues",
                       "Unable to get acquisition values for detChan %d", detChan);
                return status;
            }
            break;
        case 999:
            xiaLog(XIA_LOG_ERROR, XIA_INVALID_DETCHAN, "xiaGetAcquisitionValues",
                   "detChan number is not in the list of valid values ");
            return XIA_INVALID_DETCHAN;
        default:
            xiaLog(XIA_LOG_ERROR, XIA_UNKNOWN, "xiaGetAcquisitionValues",
                   "Should not be seeing this message");
            return XIA_UNKNOWN;
    }

    return XIA_SUCCESS;
}

/*
 * Removes an acquisition value from the channel. There is no
 * complementary Add routine, but you can add with
 * xiaSetAcquisitionValues.
 *
 * The only known use is as a hack to reset a standard acquisition
 * value to its default.
 *
 * Calls user setup to download all acquisition values after removing
 * the specified name, so for performance and consistency reasons it
 * is critical not to call this often or during data acquisition.
 */
HANDEL_EXPORT int HANDEL_API xiaRemoveAcquisitionValues(int detChan, char* name) {
    int status;
    int elemType;
    int modChan;

    char boardType[MAXITEM_LEN];
    char detType[MAXITEM_LEN];

    XiaDefaults* defaults = NULL;

    XiaDaqEntry* entry = NULL;
    XiaDaqEntry* previous = NULL;

    DetChanElement* detChanElem = NULL;

    DetChanSetElem* detChanSetElem = NULL;

    PSLFuncs localFuncs;

    FirmwareSet* fs = NULL;

    Module* m = NULL;

    Detector* det = NULL;

    char* alias = NULL;
    char* firmAlias = NULL;
    char* detAlias = NULL;

    if (name == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_NAME, "xiaRemoveAcquisitionValues",
               "Input name cannot be NULL");
        return XIA_NULL_NAME;
    }

    elemType = xiaGetElemType(detChan);

    switch (elemType) {
        case SINGLE:
            status = xiaGetBoardType(detChan, boardType);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaRemoveAcquisitionValues",
                       "Error getting board type for detChan %d", detChan);
                return status;
            }

            status = xiaLoadPSL(boardType, &localFuncs);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaRemoveAcquisitionValues",
                       "Unable to load PSL functions for detChan %d", detChan);
                return status;
            }

            defaults = xiaGetDefaultFromDetChan(detChan);

            entry = defaults->entry;

            while (entry != NULL) {
                if (STREQ(name, entry->name)) {
                    if (previous == NULL) {
                        defaults->entry = entry->next;
                    } else {
                        previous->next = entry->next;
                    }

                    handel_md_free((void*) entry->name);
                    handel_md_free((void*) entry);
                    break;
                }

                previous = entry;
                entry = entry->next;
            }

            /* Since we don't know what was removed, we better re-download all
             * the acquisition values again.
             */
            alias = xiaGetAliasFromDetChan(detChan);
            m = xiaFindModule(alias);
            modChan = xiaGetModChan(detChan);
            firmAlias = m->firmware[modChan];
            fs = xiaFindFirmware(firmAlias);
            detAlias = m->detector[modChan];
            det = xiaFindDetector(detAlias);
            /* Reset the defaults. */
            defaults = xiaGetDefaultFromDetChan(detChan);

            switch (det->type) {
                case XIA_DET_RESET:
                    strcpy(detType, "RESET");
                    break;
                case XIA_DET_RCFEED:
                    strcpy(detType, "RC");
                    break;
                default:
                case XIA_DET_UNKNOWN:
                    xiaLog(XIA_LOG_ERROR, XIA_MISSING_TYPE, "xiaSetAcquisitionValues",
                           "No detector type specified for detChan %d", detChan);
                    return XIA_MISSING_TYPE;
            }

            status = localFuncs.userSetup(detChan, defaults, fs,
                                          &(m->currentFirmware[modChan]), detType, det,
                                          m->detector_chan[modChan], m, modChan);

            if (status != XIA_SUCCESS) {
                xiaLog(
                    XIA_LOG_ERROR, status, "xiaRemoveAcquisitionValues",
                    "Error updating acquisition values after '%s' removed from list for detChan %d",
                    name, detChan);
                return status;
            }
            break;
        case SET:
            detChanElem = xiaGetDetChanPtr(detChan);
            detChanSetElem = detChanElem->data.detChanSet;
            while (detChanSetElem != NULL) {
                status = xiaRemoveAcquisitionValues(detChanSetElem->channel, name);
                if (status != XIA_SUCCESS) {
                    xiaLog(XIA_LOG_ERROR, status, "xiaRemoveAcquisitionValues",
                           "Error removing %s from detChan %u", name,
                           detChanSetElem->channel);
                    return status;
                }
                detChanSetElem = detChanSetElem->next;
            }
            break;
        case 999:
            xiaLog(XIA_LOG_ERROR, XIA_INVALID_DETCHAN, "xiaRemoveAcquisitionValues",
                   "detChan number is not in the list of valid values ");
            return XIA_INVALID_DETCHAN;
        default:
            xiaLog(XIA_LOG_ERROR, XIA_UNKNOWN, "xiaRemoveAcquisitionValues",
                   "Should not be seeing this message");
            return XIA_UNKNOWN;
    }

    return XIA_SUCCESS;
}

/*
 * Downloads all user parameters, or DSP parameters set by calls to
 * xiaSetAcquisitionValues().
 */
HANDEL_EXPORT int HANDEL_API xiaUpdateUserParams(int detChan) {
    int status;
    int elemType;

    unsigned short param = 0;

    XiaDefaults* defaults = NULL;

    XiaDaqEntry* entry = NULL;

    DetChanElement* detChanElem = NULL;

    DetChanSetElem* detChanSetElem = NULL;

    xiaLog(XIA_LOG_DEBUG, "xiaUpdateUserParams",
           "Searching for user params to download");

    elemType = xiaGetElemType(detChan);

    switch (elemType) {
        case SINGLE:
            defaults = xiaGetDefaultFromDetChan(detChan);
            entry = defaults->entry;
            while (entry != NULL) {
                if (xiaIsUpperCase(entry->name)) {
                    param = (unsigned short) (entry->data);

                    xiaLog(XIA_LOG_DEBUG, "xiaUpdateUserParams", "Setting %s to %u",
                           entry->name, param);

                    status = xiaSetParameter(detChan, entry->name, param);

                    if (status != XIA_SUCCESS) {
                        xiaLog(XIA_LOG_ERROR, status, "xiaUpdateUserParams",
                               "Error setting parameter %s for detChan %d", entry->name,
                               detChan);
                        return status;
                    }
                }
                entry = entry->next;
            }
            break;
        case SET:
            detChanElem = xiaGetDetChanPtr(detChan);
            detChanSetElem = detChanElem->data.detChanSet;
            while (detChanSetElem != NULL) {
                status = xiaUpdateUserParams(detChanSetElem->channel);
                if (status != XIA_SUCCESS) {
                    xiaLog(XIA_LOG_ERROR, status, "xiaUpdateUserParams",
                           "Error setting user params for detChan %u",
                           detChanSetElem->channel);
                    return status;
                }
                detChanSetElem = detChanSetElem->next;
            }
            break;
        case 999:
            xiaLog(XIA_LOG_ERROR, XIA_INVALID_DETCHAN, "xiaUpdateUserParams",
                   "detChan number is not in the list of valid values ");
            return XIA_INVALID_DETCHAN;
        default:
            xiaLog(XIA_LOG_ERROR, XIA_UNKNOWN, "xiaUpdateUserParams",
                   "Shouldn't be seeing this message");
            return XIA_UNKNOWN;
    }

    return XIA_SUCCESS;
}

/*
 * Performs product-specific special gain operations. value is
 * typically a double*, but theoretically could vary by name.
 */
HANDEL_EXPORT int HANDEL_API xiaGainOperation(int detChan, char* name, void* value) {
    int status;
    int elemType;

    unsigned int modChan;

    char boardType[MAXITEM_LEN];

    char* boardAlias;
    char* detectorAlias;

    DetChanElement* detChanElem = NULL;

    DetChanSetElem* detChanSetElem = NULL;

    Detector* detector = NULL;

    Module* module = NULL;

    XiaDefaults* defaults = NULL;

    PSLFuncs localFuncs;

    if (name == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_NAME, "xiaGainOperation", "name cannot be NULL");
        return XIA_NULL_NAME;
    }

    if (value == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_VALUE, "xiaGainOperation",
               "value cannot be NULL");
        return XIA_NULL_VALUE;
    }

    elemType = xiaGetElemType(detChan);

    switch (elemType) {
        case SINGLE:
            status = xiaGetBoardType(detChan, boardType);
            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaGainOperation",
                       "Unable to get boardType for detChan %d", detChan);
                return status;
            }

            status = xiaLoadPSL(boardType, &localFuncs);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaGainOperation",
                       "Unable to load PSL funcs for detChan %d", detChan);
                return status;
            }

            defaults = xiaGetDefaultFromDetChan((unsigned int) detChan);
            boardAlias = xiaGetAliasFromDetChan(detChan);
            module = xiaFindModule(boardAlias);
            modChan = xiaGetModChan((unsigned int) detChan);
            detectorAlias = module->detector[modChan];
            detector = xiaFindDetector(detectorAlias);

            status = localFuncs.gainOperation(detChan, name, value, detector, modChan,
                                              module, defaults);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaGainOperation",
                       "Error performing the gain Operation for detChan %d", detChan);
                return status;
            }
            break;
        case SET:
            detChanElem = xiaGetDetChanPtr(detChan);

            detChanSetElem = detChanElem->data.detChanSet;

            while (detChanSetElem != NULL) {
                status = xiaGainOperation(detChanSetElem->channel, name, value);

                if (status != XIA_SUCCESS) {
                    xiaLog(XIA_LOG_ERROR, status, "xiaGainOperation",
                           "Error changing the gain for detChan %d", detChan);
                    return status;
                }

                detChanSetElem = getListNext(detChanSetElem);
            }
            break;
        case 999:
            xiaLog(XIA_LOG_ERROR, XIA_INVALID_DETCHAN, "xiaGainOperation",
                   "detChan number is not in the list of valid values");
            return XIA_INVALID_DETCHAN;
        default:
            xiaLog(XIA_LOG_ERROR, XIA_UNKNOWN, "xiaGainOperation",
                   "Should not be seeing this message");
            return XIA_UNKNOWN;
    }

    return XIA_SUCCESS;
}

/*
 * Scales the channel's energy value by a constant factor.
 */
HANDEL_EXPORT int HANDEL_API xiaGainCalibrate(int detChan, double deltaGain) {
    int status;
    int elemType;

    unsigned int modChan;

    char boardType[MAXITEM_LEN];

    char* detectorAlias;
    char* boardAlias;

    Detector* detector = NULL;

    XiaDefaults* defaults = NULL;

    Module* module = NULL;

    DetChanElement* detChanElem = NULL;

    DetChanSetElem* detChanSetElem = NULL;

    PSLFuncs localFuncs;

    elemType = xiaGetElemType((unsigned int) detChan);

    switch (elemType) {
        case SINGLE:
            status = xiaGetBoardType(detChan, boardType);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaGainCalibrate",
                       "Unable to get boardType for detChan %d", detChan);
                return status;
            }

            status = xiaLoadPSL(boardType, &localFuncs);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaGainCalibrate",
                       "Unable to load PSL funcs for detChan %d", detChan);
                return status;
            }

            defaults = xiaGetDefaultFromDetChan((unsigned int) detChan);
            boardAlias = xiaGetAliasFromDetChan(detChan);
            module = xiaFindModule(boardAlias);
            modChan = xiaGetModChan((unsigned int) detChan);
            detectorAlias = module->detector[modChan];
            detector = xiaFindDetector(detectorAlias);

            status = localFuncs.gainCalibrate(detChan, detector, modChan, module,
                                              defaults, deltaGain);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaGainCalibrate",
                       "Error calibrating the gain for detChan %d", detChan);
                return status;
            }
            break;
        case SET:
            detChanElem = xiaGetDetChanPtr((unsigned int) detChan);
            detChanSetElem = detChanElem->data.detChanSet;
            while (detChanSetElem != NULL) {
                status = xiaGainCalibrate((int) detChanSetElem->channel, deltaGain);
                if (status != XIA_SUCCESS) {
                    xiaLog(XIA_LOG_ERROR, status, "xiaGainCalibrate",
                           "Error calibrating the gain for detChan %d", detChan);
                    return status;
                }
                detChanSetElem = getListNext(detChanSetElem);
            }
            break;
        case 999:
            xiaLog(XIA_LOG_ERROR, XIA_INVALID_DETCHAN, "xiaGainCalibrate",
                   "detChan number is not in the list of valid values ");
            return XIA_INVALID_DETCHAN;
        default:
            xiaLog(XIA_LOG_ERROR, XIA_UNKNOWN, "xiaGainCalibrate",
                   "Should not be seeing this message");
            return XIA_UNKNOWN;
    }

    return XIA_SUCCESS;
}

/*
 * Retrieves the value of the DSP parameter name from the specified
 * detChan.
 */
HANDEL_EXPORT int HANDEL_API xiaGetParameter(int detChan, const char* name,
                                             unsigned short* value) {
    int status;
    int elemType;

    char boardType[MAXITEM_LEN];

    PSLFuncs localFuncs;

    if (name == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_NAME, "xiaGetParameter", "name cannot be NULL");
        return XIA_NULL_NAME;
    }

    if (value == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_VALUE, "xiaGetParameter",
               "value cannot be NULL");
        return XIA_NULL_VALUE;
    }

    elemType = xiaGetElemType((unsigned int) detChan);

    /* We only support SINGLE chans... */
    switch (elemType) {
        case SINGLE:
            status = xiaGetBoardType(detChan, boardType);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaGetParameter",
                       "Unable to get boardType for detChan %d", detChan);
                return status;
            }

            status = xiaLoadPSL(boardType, &localFuncs);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaGetParameter",
                       "Unable to load PSL funcs for detChan %d", detChan);
                return status;
            }

            status = localFuncs.getParameter(detChan, name, value);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaGetParameter",
                       "Error getting parameter %s from detChan %d", name, detChan);
                return status;
            }
            break;
        case SET:
            xiaLog(XIA_LOG_ERROR, XIA_BAD_TYPE, "xiaGetParameter",
                   "detChan SETs are not supported for this routine");
            return XIA_BAD_TYPE;
        case 999:
            xiaLog(XIA_LOG_ERROR, XIA_INVALID_DETCHAN, "xiaGetParameter",
                   "detChan number is not in the list of valid values ");
            return XIA_INVALID_DETCHAN;
        default:
            xiaLog(XIA_LOG_ERROR, XIA_UNKNOWN, "xiaGetParameter",
                   "Should not be seeing this message");
            return XIA_UNKNOWN;
    }

    return XIA_SUCCESS;
}

/*
 * Sets the value of DSP parameter name for detChan.
 */
HANDEL_EXPORT int HANDEL_API xiaSetParameter(int detChan, const char* name,
                                             unsigned short value) {
    int status;
    int elemType;

    char boardType[MAXITEM_LEN];

    DetChanSetElem* detChanSetElem = NULL;

    DetChanElement* detChanElem = NULL;

    PSLFuncs localFuncs;

    if (name == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_NAME, "xiaSetParameter",
               "Input name cannot be NULL");
        return XIA_NULL_NAME;
    }

    elemType = xiaGetElemType((unsigned int) detChan);

    switch (elemType) {
        case SINGLE:
            status = xiaGetBoardType(detChan, boardType);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaSetParameter",
                       "Unable to get boardType for detChan %d", detChan);
                return status;
            }

            status = xiaLoadPSL(boardType, &localFuncs);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaSetParameter",
                       "Unable to load PSL funcs for detChan %d", detChan);
                return status;
            }

            status = localFuncs.setParameter(detChan, name, value);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaSetParameter",
                       "Error setting parameter %s from detChan %d", name, detChan);
                return status;
            }
            break;
        case SET:
            detChanElem = xiaGetDetChanPtr((unsigned int) detChan);

            detChanSetElem = detChanElem->data.detChanSet;

            while (detChanSetElem != NULL) {
                status = xiaSetParameter((int) detChanSetElem->channel, name, value);

                if (status != XIA_SUCCESS) {
                    xiaLog(XIA_LOG_ERROR, status, "xiaSetParameter",
                           "Error setting parameter %s for detChan %d", name, detChan);
                    return status;
                }

                detChanSetElem = getListNext(detChanSetElem);
            }
            break;
        case 999:
            xiaLog(XIA_LOG_ERROR, XIA_INVALID_DETCHAN, "xiaSetParameter",
                   "detChan number is not in the list of valid values");
            return XIA_INVALID_DETCHAN;
        default:
            xiaLog(XIA_LOG_ERROR, XIA_UNKNOWN, "xiaSetParameter",
                   "Should not be seeing this message");
            return XIA_UNKNOWN;
    }

    return XIA_SUCCESS;
}

/*
 * Returns the number of DSP parameters for the channel.
 */
HANDEL_EXPORT int HANDEL_API xiaGetNumParams(int detChan, unsigned short* value) {
    int status;
    int elemType;

    char boardType[MAXITEM_LEN];

    PSLFuncs localFuncs;

    if (value == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_VALUE, "xiaGetNumParams",
               "Input value cannot be NULL");
        return XIA_NULL_VALUE;
    }

    elemType = xiaGetElemType((unsigned int) detChan);

    /* We only support SINGLE chans... */
    switch (elemType) {
        case SINGLE:
            status = xiaGetBoardType(detChan, boardType);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaGetNumParams",
                       "Unable to get boardType for detChan %d", detChan);
                return status;
            }

            status = xiaLoadPSL(boardType, &localFuncs);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaGetNumParams",
                       "Unable to load PSL funcs for detChan %d", detChan);
                return status;
            }

            status = localFuncs.getNumParams(detChan, value);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaGetNumParams",
                       "Error getting number of DSP params from detChan %d", detChan);
                return status;
            }
            break;
        case SET:
            xiaLog(XIA_LOG_ERROR, XIA_BAD_TYPE, "xiaGetNumParams",
                   "detChan SETs are not supported for this routine");
            return XIA_BAD_TYPE;
        case 999:
            xiaLog(XIA_LOG_ERROR, XIA_INVALID_DETCHAN, "xiaGetNumParams",
                   "detChan number is not in the list of valid values ");
            return XIA_INVALID_DETCHAN;
        default:
            xiaLog(XIA_LOG_ERROR, XIA_UNKNOWN, "xiaGetNumParams",
                   "Should not be seeing this message");
            return XIA_UNKNOWN;
    }

    return XIA_SUCCESS;
}

/*
 * See if a string is only composed of upper case
 */
static boolean_t HANDEL_API xiaIsUpperCase(char* string) {
    size_t len, i;
    len = strlen(string);
    for (i = 0; i < len; i++) {
        if (!isupper(CTYPE_CHAR(string[i])) && !isdigit(CTYPE_CHAR(string[i]))) {
            return FALSE_;
        }
    }
    return TRUE_;
}

/*
 * Returns DSP symbol names, DSP values, etc... Assumes that the
 * proper amount of memory has been allocated based on the number of
 * parameters and the required type of value.
 */
HANDEL_EXPORT int HANDEL_API xiaGetParamData(int detChan, char* name, void* value) {
    int status;
    int elemType;

    char boardType[MAXITEM_LEN];

    PSLFuncs localFuncs;

    if (name == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_NAME, "xiaGetParamData", "name cannot be NULL");
        return XIA_NULL_NAME;
    }

    if (value == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_VALUE, "xiaGetParamData",
               "value cannot be NULL");
        return XIA_NULL_VALUE;
    }

    elemType = xiaGetElemType((unsigned int) detChan);

    /* We only support SINGLE chans... */
    switch (elemType) {
        case SINGLE:
            status = xiaGetBoardType(detChan, boardType);
            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaGetParamData",
                       "Unable to get boardType for detChan %d", detChan);
                return status;
            }

            status = xiaLoadPSL(boardType, &localFuncs);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaGetParamData",
                       "Unable to load PSL funcs for detChan %d", detChan);
                return status;
            }

            status = localFuncs.getParamData(detChan, name, value);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaGetParamData",
                       "Error getting DSP param data from detChan %d", detChan);
                return status;
            }
            break;
        case SET:
            xiaLog(XIA_LOG_ERROR, XIA_BAD_TYPE, "xiaGetParamData",
                   "detChan SETs are not supported for this routine");
            return XIA_BAD_TYPE;
        case 999:
            xiaLog(XIA_LOG_ERROR, XIA_INVALID_DETCHAN, "xiaGetParamData",
                   "detChan number is not in the list of valid values ");
            return XIA_INVALID_DETCHAN;
        default:
            xiaLog(XIA_LOG_ERROR, XIA_UNKNOWN, "xiaGetParamData",
                   "Should not be seeing this message");
            return XIA_UNKNOWN;
    }

    return XIA_SUCCESS;
}

/*
 * Returns the DSP symbol name located at the specified index in the
 * symbol name list. This routine exists since VB can't pass a string array into
 * a DLL and, therefore, VB users must get a single string at a time. Assumes
 * that name has the proper amount of memory allocated.
 */
HANDEL_EXPORT int HANDEL_API xiaGetParamName(int detChan, unsigned short index,
                                             char* name) {
    int status;
    int elemType;

    char boardType[MAXITEM_LEN];

    PSLFuncs localFuncs;

    if (name == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_NAME, "xiaGetParamName", "name cannot be NULL");
        return XIA_NULL_NAME;
    }

    elemType = xiaGetElemType((unsigned int) detChan);

    /* We only support SINGLE chans... */
    switch (elemType) {
        case SINGLE:
            status = xiaGetBoardType(detChan, boardType);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaGetParamName",
                       "Unable to get boardType for detChan %d", detChan);
                return status;
            }

            status = xiaLoadPSL(boardType, &localFuncs);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaGetParamName",
                       "Unable to load PSL funcs for detChan %d", detChan);
                return status;
            }

            status = localFuncs.getParamName(detChan, index, name);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaGetParamName",
                       "Error getting DSP params from detChan %d", detChan);
                return status;
            }
            break;
        case SET:
            xiaLog(XIA_LOG_ERROR, XIA_BAD_TYPE, "xiaGetParamName",
                   "detChan SETs are not supported for this routine");
            return XIA_BAD_TYPE;
        case 999:
            xiaLog(XIA_LOG_ERROR, XIA_INVALID_DETCHAN, "xiaGetParamName",
                   "detChan number is not in the list of valid values ");
            return XIA_INVALID_DETCHAN;
        default:
            xiaLog(XIA_LOG_ERROR, XIA_UNKNOWN, "xiaGetParamName",
                   "Should not be seeing this message");
            return XIA_UNKNOWN;
    }

    return XIA_SUCCESS;
}
