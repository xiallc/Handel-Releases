/* Routines used to start, stop, get data from a run. */

/*
 * Copyright (c) 2002-2004 X-ray Instrumentation Associates
 * Copyright (c) 2005-2012 XIA LLC
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

#include <stdio.h>

#include "handeldef.h"
#include "xia_assert.h"
#include "xia_handel.h"
#include "xia_handel_structures.h"
#include "xia_system.h"

#include "handel_errors.h"
#include "handel_log.h"

/*
 * Starts a run on the specified detChan or detChan set. If resume is
 * set to 0, the MCA memory will be cleared prior to starting the run.
 * Pass 1 to resume without clearing the MCA.
 *
 * For some products, even if a single channel is specified, all channels for
 * that module will have a run started. This is an intrinsic property of the
 * hardware and there is no way to circumvent it in the software.
 */
HANDEL_EXPORT int HANDEL_API xiaStartRun(int detChan, unsigned short resume) {
    int status;
    int elemType;

    unsigned int chan = 0;

    char boardType[MAXITEM_LEN];

    char* alias = NULL;

    XiaDefaults* defaults = NULL;

    DetChanElement* detChanElem = NULL;

    DetChanSetElem* detChanSetElem = NULL;

    Module* module = NULL;

    PSLFuncs localFuncs;

    xiaLog(XIA_LOG_INFO, "xiaStartRun", "Starting a run on chan %d.", detChan);

    elemType = xiaGetElemType((unsigned int) detChan);

    switch (elemType) {
        case SINGLE:
            /* Check to see if this board is a multichannel board and,
             * if so, is a run already active on that channel set via.
             * the run broadcast...
             */
            alias = xiaGetAliasFromDetChan(detChan);
            module = xiaFindModule(alias);

            if (module->isMultiChannel) {
                status = xiaGetAbsoluteChannel(detChan, module, &chan);

                if (status != XIA_SUCCESS) {
                    xiaLog(XIA_LOG_ERROR, status, "xiaStartRun",
                           "detChan = %d not found in module '%s'", detChan, alias);
                    return status;
                }

                if (module->state->runActive[chan]) {
                    xiaLog(
                        XIA_LOG_INFO, "xiaStartRun",
                        "detChan %d is part of a multichannel module whose run was already started",
                        detChan);
                    break;
                }
            }

            status = xiaGetBoardType(detChan, boardType);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaStartRun",
                       "Unable to get boardType for detChan %d", detChan);
                return status;
            }

            status = xiaLoadPSL(boardType, &localFuncs);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaStartRun",
                       "Unable to load PSL funcs for detChan %d", detChan);
                return status;
            }

            defaults = xiaGetDefaultFromDetChan((unsigned int) detChan);

            status = localFuncs.startRun(detChan, resume, defaults, module);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaStartRun",
                       "Unable to start run for detChan %d", detChan);
                return status;
            }

            /*
             * Tag all the channels if this is a multichannel module.
             */
            if (module->isMultiChannel) {
                status = xiaTagAllRunActive(module, TRUE_);

                if (status != XIA_SUCCESS) {
                    xiaLog(XIA_LOG_ERROR, status, "xiaStartRun",
                           "Error setting channel state information: runActive");
                    return status;
                }
            }
            break;
        case SET:
            detChanElem = xiaGetDetChanPtr((unsigned int) detChan);

            detChanSetElem = detChanElem->data.detChanSet;

            while (detChanSetElem != NULL) {
                status = xiaStartRun((int) detChanSetElem->channel, resume);
                if (status != XIA_SUCCESS) {
                    xiaLog(XIA_LOG_ERROR, status, "xiaStartRun",
                           "Error starting run for detChan %d", detChan);
                    return status;
                }
                detChanSetElem = getListNext(detChanSetElem);
            }
            break;
        case 999:
            status = XIA_INVALID_DETCHAN;
            xiaLog(XIA_LOG_ERROR, status, "xiaStartRun",
                   "detChan number is not in the list of valid values ");
            return status;
        default:
            status = XIA_UNKNOWN;
            xiaLog(XIA_LOG_ERROR, status, "xiaStartRun",
                   "Should not be seeing this message");
            return status;
    }

    return XIA_SUCCESS;
}

/*
 * Stops a run on the detChan or detChan set. For some products this
 * will stop a run on all the channels in detChan's module.
 */
HANDEL_EXPORT int HANDEL_API xiaStopRun(int detChan) {
    int status;
    int elemType;

    unsigned int chan = 0;

    char boardType[MAXITEM_LEN];

    char* alias = NULL;

    DetChanElement* detChanElem = NULL;

    DetChanSetElem* detChanSetElem = NULL;

    Module* module = NULL;

    PSLFuncs localFuncs;

    xiaLog(XIA_LOG_INFO, "xiaStopRun", "Stopping a run on chan %d...", detChan);

    elemType = xiaGetElemType((unsigned int) detChan);

    switch (elemType) {
        case SINGLE:
            alias = xiaGetAliasFromDetChan(detChan);
            module = xiaFindModule(alias);

            if (module->isMultiChannel) {
                status = xiaGetAbsoluteChannel(detChan, module, &chan);

                if (status != XIA_SUCCESS) {
                    xiaLog(XIA_LOG_ERROR, status, "xiaStopRun",
                           "detChan = %d not found in module '%s'", detChan, alias);
                    return status;
                }

                if (!module->state->runActive[chan]) {
                    xiaLog(
                        XIA_LOG_INFO, "xiaStopRun",
                        "detChan %d is part of a multichannel module whose run was already stopped",
                        detChan);
                    break;
                }
            }

            status = xiaGetBoardType(detChan, boardType);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaStopRun",
                       "Unable to get boardType for detChan %d", detChan);
                return status;
            }

            status = xiaLoadPSL(boardType, &localFuncs);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaStopRun",
                       "Unable to load PSL funcs for detChan %d", detChan);
                return status;
            }

            status = localFuncs.stopRun(detChan, module);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaStopRun",
                       "Unable to stop run for detChan %d", detChan);
                return status;
            }

            if (module->isMultiChannel) {
                status = xiaTagAllRunActive(module, FALSE_);

                if (status != XIA_SUCCESS) {
                    xiaLog(XIA_LOG_ERROR, status, "xiaStopRun",
                           "Error setting channel state information: runActive");
                    return status;
                }
            }
            break;
        case SET:
            detChanElem = xiaGetDetChanPtr((unsigned int) detChan);

            detChanSetElem = detChanElem->data.detChanSet;

            while (detChanSetElem != NULL) {
                /* Recursive loop here to stop run for all channels */
                status = xiaStopRun((int) detChanSetElem->channel);

                if (status != XIA_SUCCESS) {
                    xiaLog(XIA_LOG_ERROR, status, "xiaStopRun",
                           "Error stopping run for detChan %d", detChan);
                    return status;
                }

                detChanSetElem = getListNext(detChanSetElem);
            }
            break;
        case 999:
            xiaLog(XIA_LOG_ERROR, XIA_INVALID_DETCHAN, "xiaStopRun",
                   "detChan number is not in the list of valid values ");
            return XIA_INVALID_DETCHAN;
        default:
            xiaLog(XIA_LOG_ERROR, XIA_UNKNOWN, "xiaStopRun",
                   "Should not be seeing this message");
            return XIA_UNKNOWN;
    }

    return XIA_SUCCESS;
}

/*
 * Returns run data for the channel. detChan must be single channel only.
 */
HANDEL_EXPORT int HANDEL_API xiaGetRunData(int detChan, char* name, void* value) {
    int status;
    int elemType;

    char* alias = NULL;

    char boardType[MAXITEM_LEN];

    PSLFuncs localFuncs;

    XiaDefaults* defaults = NULL;

    Module* m = NULL;

    if (name == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_NAME, "xiaGetRunData", "name cannot be NULL");
        return XIA_NULL_NAME;
    }

    if (value == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_VALUE, "xiaGetRunData", "value cannot be NULL");
        return XIA_NULL_VALUE;
    }

    elemType = xiaGetElemType((unsigned int) detChan);

    switch (elemType) {
        case SINGLE:
            status = xiaGetBoardType(detChan, boardType);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaGetRunData",
                       "Unable to get boardType for detChan %d", detChan);
                return status;
            }

            status = xiaLoadPSL(boardType, &localFuncs);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaGetRunData",
                       "Unable to load PSL funcs for detChan %d", detChan);
                return status;
            }

            defaults = xiaGetDefaultFromDetChan((unsigned int) detChan);
            alias = xiaGetAliasFromDetChan(detChan);
            m = xiaFindModule(alias);

            /* A NULL module would indicate that something is broken
             * internally since the value returned by xiaGetAliasFromDetChan()
             * _must_ be a real alias.
             */
            ASSERT(m != NULL);

            status = localFuncs.getRunData(detChan, name, value, defaults, m);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaGetRunData",
                       "Unable get run data %s for detChan %d", name, detChan);
                return status;
            }
            break;
        case SET:
            /*
             * Don't allow SETs since there is no way to handle the potential of
             * multidimensional data.
             */
            xiaLog(XIA_LOG_ERROR, XIA_BAD_TYPE, "xiaGetRunData",
                   "Unable to get run data for a detChan SET");
            return XIA_BAD_TYPE;
        case 999:
            xiaLog(XIA_LOG_ERROR, XIA_INVALID_DETCHAN, "xiaGetRunData",
                   "detChan number is not in the list of valid values ");
            return XIA_INVALID_DETCHAN;
        default:
            xiaLog(XIA_LOG_ERROR, XIA_UNKNOWN, "xiaGetRunData",
                   "Should not be seeing this message");
            return XIA_UNKNOWN;
    }

    return XIA_SUCCESS;
}

/*
 * Starts and stops a special run.
 *
 * Blocks until the special run is complete or an internal timeout
 * occurs. Internal timeouts vary between processor and special run
 * type.
 */
HANDEL_EXPORT int HANDEL_API xiaDoSpecialRun(int detChan, char* name, void* info) {
    int status;
    int elemType;

    char boardType[MAXITEM_LEN];

    DetChanElement* detChanElem = NULL;

    DetChanSetElem* detChanSetElem = NULL;

    XiaDefaults* defaults;

    PSLFuncs localFuncs;

    /* The following declarations are used to retrieve the preampGain. */
    Module* module = NULL;
    Detector* detector = NULL;
    char* boardAlias;
    char* detectorAlias;
    int detector_chan;
    unsigned int modChan;

    if (name == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_NAME, "xiaDoSpecialRun", "name cannot be NULL");
        return XIA_NULL_NAME;
    }

    if (info == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_INFO, "xiaDoSpecialRun", "info cannot be NULL");
        return XIA_NULL_INFO;
    }

    xiaLog(XIA_LOG_INFO, "xiaDoSpecialRun", "Starting special run %s on chan %d...",
           name, detChan);

    elemType = xiaGetElemType((unsigned int) detChan);

    switch (elemType) {
        case SINGLE:
            status = xiaGetBoardType(detChan, boardType);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaDoSpecialRun",
                       "Unable to get boardType for detChan %d", detChan);
                return status;
            }

            status = xiaLoadPSL(boardType, &localFuncs);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaDoSpecialRun",
                       "Unable to load PSL funcs for detChan %d", detChan);
                return status;
            }

            /* Load the defaults */
            defaults = xiaGetDefaultFromDetChan((unsigned int) detChan);

            /* Retrieve the preampGain for the specialRun routine */
            boardAlias = xiaGetAliasFromDetChan(detChan);
            module = xiaFindModule(boardAlias);
            modChan = xiaGetModChan((unsigned int) detChan);
            detectorAlias = module->detector[modChan];
            detector_chan = module->detector_chan[modChan];
            detector = xiaFindDetector(detectorAlias);

            status = localFuncs.doSpecialRun(detChan, name, info, defaults, detector,
                                             detector_chan);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaDoSpecialRun",
                       "Unable to perform special run for detChan %d", detChan);
                return status;
            }
            break;
        case SET:
            detChanElem = xiaGetDetChanPtr((unsigned int) detChan);

            detChanSetElem = detChanElem->data.detChanSet;

            while (detChanSetElem != NULL) {
                status = xiaDoSpecialRun((int) detChanSetElem->channel, name, info);

                if (status != XIA_SUCCESS) {
                    xiaLog(XIA_LOG_ERROR, status, "xiaDoSpecialRun",
                           "Error performing special run for detChan %d", detChan);
                    return status;
                }

                detChanSetElem = getListNext(detChanSetElem);
            }
            break;
        case 999:
            xiaLog(XIA_LOG_ERROR, XIA_INVALID_DETCHAN, "xiaDoSpecialRun",
                   "detChan number is not in the list of valid values");
            return XIA_INVALID_DETCHAN;
        default:
            xiaLog(XIA_LOG_ERROR, XIA_UNKNOWN, "xiaDoSpecialRun",
                   "Should not be seeing this message");
            return XIA_UNKNOWN;
    }

    return XIA_SUCCESS;
}

/*
 * Returns run data from a special run. detChan must be single channel
 * only.
 *
 * For most special runs this also stops the run that was started by
 * xiaDoSpecialRun(). For some types of runs, this routine must be called
 * after xiaDoSpecialRun(). See the special run table for information on which
 * special runs require the data to be read out and for the names of the
 * special run data to read out.
 */
HANDEL_EXPORT int HANDEL_API xiaGetSpecialRunData(int detChan, char* name,
                                                  void* value) {
    int status;
    int elemType;

    XiaDefaults* defaults;

    char boardType[MAXITEM_LEN];

    DetChanElement* detChanElem = NULL;

    DetChanSetElem* detChanSetElem = NULL;

    PSLFuncs localFuncs;

    if (name == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_NAME, "xiaGetSpecialRunData",
               "name cannot be NULL");
        return XIA_NULL_NAME;
    }

    if (value == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_VALUE, "xiaGetSpecialRunData",
               "value cannot be NULL");
        return XIA_NULL_VALUE;
    }

    elemType = xiaGetElemType((unsigned int) detChan);

    switch (elemType) {
        case SINGLE:
            status = xiaGetBoardType(detChan, boardType);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaGetSpecialRunData",
                       "Unable to get boardType for detChan %d", detChan);
                return status;
            }

            status = xiaLoadPSL(boardType, &localFuncs);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaGetSpecialRunData",
                       "Unable to load PSL funcs for detChan %d", detChan);
                return status;
            }

            /* Load the defaults */
            defaults = xiaGetDefaultFromDetChan((unsigned int) detChan);

            status = localFuncs.getSpecialRunData(detChan, name, value, defaults);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaGetSpecialRunData",
                       "Unable to get special run data for detChan %d", detChan);
                return status;
            }
            break;
        case SET:
            detChanElem = xiaGetDetChanPtr((unsigned int) detChan);

            detChanSetElem = detChanElem->data.detChanSet;

            while (detChanSetElem != NULL) {
                status =
                    xiaGetSpecialRunData((int) detChanSetElem->channel, name, value);

                if (status != XIA_SUCCESS) {
                    xiaLog(XIA_LOG_ERROR, status, "xiaGetSpecialRunData",
                           "Error getting special run data for detChan %d", detChan);
                    return status;
                }

                detChanSetElem = getListNext(detChanSetElem);
            }
            break;
        case 999:
            xiaLog(XIA_LOG_ERROR, XIA_INVALID_DETCHAN, "xiaGetSpecialRunData",
                   "detChan number is not in the list of valid values ");
            return XIA_INVALID_DETCHAN;
        default:
            xiaLog(XIA_LOG_ERROR, XIA_UNKNOWN, "xiaGetSpecialRunData",
                   "Should not be seeing this message");
            return XIA_UNKNOWN;
    }

    return XIA_SUCCESS;
}
