/* Dynamic detector configuration */

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xerxes.h"
#include "xerxes_errors.h"
#include "xerxes_structures.h"
#include "xia_handel.h"
#include "xia_handel_structures.h"

#include "handel_errors.h"
#include "handel_generic.h"
#include "handel_log.h"
#include "handeldef.h"

#include <util/xia_str_manip.h>

HANDEL_EXPORT int HANDEL_API xiaNewDetector(char* alias) {
    int status = XIA_SUCCESS;

    Detector* current = NULL;

    if (alias == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_ALIAS, "xiaNewDetector", "alias cannot be NULL");
        return XIA_NULL_ALIAS;
    }

    if (!isHandelInit) {
        status = xiaInitHandel();
        if (status != XIA_SUCCESS) {
            fprintf(stderr, "FATAL ERROR: Unable to load libraries.\n");
            exit(XIA_INITIALIZE);
        }
        xiaLog(XIA_LOG_WARNING, "xiaNewDetector", "Handel initialized silently");
    }

    if ((strlen(alias) + 1) > MAXALIAS_LEN) {
        xiaLog(XIA_LOG_ERROR, XIA_ALIAS_SIZE, "xiaNewDetector",
               "Alias contains too many characters");
        return XIA_ALIAS_SIZE;
    }

    current = xiaFindDetector(alias);
    if (current != NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_ALIAS_EXISTS, "xiaNewDetector",
               "Alias %s already in use.", alias);
        return XIA_ALIAS_EXISTS;
    }

    xiaLog(XIA_LOG_DEBUG, "xiaNewDetector", "create new detector w/ alias = %s", alias);

    if (xiaDetectorHead == NULL) {
        xiaDetectorHead = (Detector*) handel_md_alloc(sizeof(Detector));
        current = xiaDetectorHead;
    } else {
        current = xiaDetectorHead;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = (Detector*) handel_md_alloc(sizeof(Detector));
        current = current->next;
    }

    if (current == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NOMEM, "xiaNewDetector",
               "Unable to allocate memory for Detector alias %s.", alias);
        return XIA_NOMEM;
    }

    current->alias = (char*) handel_md_alloc((strlen(alias) + 1) * sizeof(char));
    if (current->alias == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NOMEM, "xiaNewDetector",
               "Unable to allocate memory for current->alias");
        return XIA_NOMEM;
    }

    strcpy(current->alias, alias);

    current->nchan = 0;
    current->polarity = NULL;
    current->gain = NULL;
    current->type = XIA_DET_UNKNOWN;
    current->typeValue = NULL;
    current->next = NULL;

    return XIA_SUCCESS;
}

HANDEL_EXPORT int HANDEL_API xiaAddDetectorItem(char* alias, char* name, void* value) {
    unsigned int slen;

    long chan = 0;

    unsigned short j;

    char* strtemp;

    Detector* chosen = NULL;

    if (alias == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_ALIAS, "xiaAddDetectorItem",
               "alias cannot be NULL");
        return XIA_NULL_ALIAS;
    }

    if (name == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_NAME, "xiaAddDetectorItem",
               "name cannot be NULL");
        return XIA_NULL_NAME;
    }

    if (value == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_VALUE, "xiaAddDetectorItem",
               "value cannot be NULL");
        return XIA_NULL_VALUE;
    }

    /* Locate the Detector entry first */
    chosen = xiaFindDetector(alias);
    if (chosen == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NO_ALIAS, "xiaAddDetectorItem",
               "Alias %s has not been created.", alias);
        return XIA_NO_ALIAS;
    }

    strtemp = xia_lower(name);

    if (STREQ(strtemp, "number_of_channels")) {
        chosen->nchan = *((unsigned short*) value);
        chosen->polarity =
            (unsigned short*) handel_md_alloc(chosen->nchan * sizeof(unsigned short));
        chosen->gain = (double*) handel_md_alloc(chosen->nchan * sizeof(double));
        chosen->typeValue = (double*) handel_md_alloc(sizeof(double) * chosen->nchan);

        if ((chosen->polarity == NULL) || (chosen->gain == NULL) ||
            (chosen->typeValue == NULL)) {
            if (chosen->polarity) {
                handel_md_free(chosen->polarity);
                chosen->polarity = NULL;
            }
            if (chosen->gain) {
                handel_md_free(chosen->gain);
                chosen->gain = NULL;
            }
            if (chosen->typeValue) {
                handel_md_free(chosen->typeValue);
                chosen->typeValue = NULL;
            }

            xiaLog(XIA_LOG_ERROR, XIA_NOMEM, "xiaAddDetectorItem",
                   "Unable to allocate memory for detector info");
            return XIA_NOMEM;
        }

        return XIA_SUCCESS;
    }

    /*
     * The number of channels must be set first before we can do any of the
     * other values. Why? Because memory is allocated once the number of channels
     * is known.
     */
    if (chosen->nchan == 0) {
        xiaLog(XIA_LOG_ERROR, XIA_NO_CHANNELS, "xiaAddDetectorItem",
               "Detector '%s' must set its number of channels before setting '%s'",
               chosen->alias, strtemp);
        return XIA_NO_CHANNELS;
    }

    if (STREQ(strtemp, "type")) {
        if (STREQ((char*) value, "reset")) {
            chosen->type = XIA_DET_RESET;
        } else if (STREQ((char*) value, "rc_feedback")) {
            chosen->type = XIA_DET_RCFEED;
        } else {
            xiaLog(XIA_LOG_ERROR, XIA_BAD_VALUE, "xiaAddDetectorItem",
                   "Error setting detector type for %s", chosen->alias);
            return XIA_BAD_VALUE;
        }
    } else if (STREQ(strtemp, "type_value")) {
        /*
         * This constrains us to a "single" det type value for now.
         * If that isn't good enough for some customers, we can upgrade it at a
         * later date to allow for each channel to be set individually.
         */
        for (j = 0; j < chosen->nchan; j++) {
            chosen->typeValue[j] = *((double*) value);
        }
    } else if (strncmp(strtemp, "channel", 7) == 0) {
        /* Determine the quantity to define: gain or polarity */
        slen = (unsigned int) strlen(strtemp);
        if (STREQ(strtemp + slen - 5, "_gain")) {
            strtemp[slen - 5] = ' ';
            chan = atol(strtemp + 7);
            /* Now store the value for the gain if the channel number is value */
            if (chan >= (long) chosen->nchan) {
                xiaLog(XIA_LOG_ERROR, XIA_BAD_VALUE, "xiaAddDetectorItem",
                       "Channel number invalid for %s.", name);
                return XIA_BAD_VALUE;
            }
            chosen->gain[chan] = *((double*) value);
        } else if (STREQ(strtemp + slen - 9, "_polarity")) {
            /* Now get the channel number, insert space, then use the standard C routine */
            strtemp[slen - 9] = ' ';
            chan = atol(strtemp + 7);
            /* Now store the value for the gain if the channel number is value */
            if (chan >= (long) chosen->nchan) {
                xiaLog(XIA_LOG_ERROR, XIA_BAD_VALUE, "xiaAddDetectorItem",
                       "Channel number invalid for %s.", name);
                return XIA_BAD_VALUE;
            }

            if ((STREQ((char*) value, "pos")) || (STREQ((char*) value, "+")) ||
                (STREQ((char*) value, "positive"))) {
                chosen->polarity[chan] = 1;
            } else if ((STREQ((char*) value, "neg")) || (STREQ((char*) value, "-")) ||
                       (STREQ((char*) value, "negative"))) {
                chosen->polarity[chan] = 0;
            } else {
                xiaLog(XIA_LOG_ERROR, XIA_BAD_VALUE, "xiaAddDetectorItem",
                       "Invalid polarity %s.", (char*) value);
                return XIA_BAD_VALUE;
            }
        } else {
            xiaLog(XIA_LOG_ERROR, XIA_BAD_NAME, "xiaAddDetectorItem",
                   "Invalid name %s.", name);
            return XIA_BAD_NAME;
        }
    } else {
        xiaLog(XIA_LOG_ERROR, XIA_BAD_NAME, "xiaAddDetectorItem", "Invalid name %s.",
               name);
        return XIA_BAD_NAME;
    }

    return XIA_SUCCESS;
}

HANDEL_EXPORT int HANDEL_API xiaModifyDetectorItem(char* alias, char* name,
                                                   void* value) {
    int status = XIA_SUCCESS;

    unsigned int slen;
    char* strtemp = NULL;

    if (alias == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_ALIAS, "xiaModifyDetectorItem",
               "alias cannot be NULL");
        return XIA_NULL_ALIAS;
    }

    if (name == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_NAME, "xiaModifyDetectorItem",
               "name cannot be NULL");
        return XIA_NULL_NAME;
    }

    if (value == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_VALUE, "xiaModifyDetectorItem",
               "value can not be NULL");
        return XIA_NULL_VALUE;
    }

    strtemp = xia_lower(name);

    if (strncmp(strtemp, "channel", 7) == 0) {
        /*
         * Make sure that the name is either gain or polarity, no others are allowed
         * modification. Addendum: type_value is also allowed now. See BUG ID #58.
         */
        slen = (unsigned int) strlen(strtemp);
        if ((STREQ(strtemp + slen - 5, "_gain")) ||
            (STREQ(strtemp + slen - 9, "_polarity"))) {
            /* Try to modify the value */
            status = xiaAddDetectorItem(alias, name, value);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaModifyDetectorItem",
                       "Unable to modify detector value");
                return status;
            }
            return status;
        }
    }

    if (STREQ(strtemp, "type_value")) {
        status = xiaAddDetectorItem(alias, name, value);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaModifyDetectorItem",
                   "Unable to modify detector value");
            return status;
        }
        return XIA_SUCCESS;
    }

    /* Not a valid name to modify */
    xiaLog(XIA_LOG_ERROR, XIA_BAD_NAME, "xiaModifyDetectorItem",
           "Cannot modify the name:%s", name);
    return XIA_BAD_NAME;
}

HANDEL_EXPORT int HANDEL_API xiaGetDetectorItem(char* alias, char* name, void* value) {
    int status = XIA_SUCCESS;
    int idx;

    char* sidx;

    char* strtemp = NULL;

    Detector* chosen = NULL;

    if (alias == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_ALIAS, "xiaAddFirmwareItem",
               "alias cannot be NULL");
        return XIA_NULL_ALIAS;
    }

    if (name == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_NAME, "xiaAddFirmwareItem",
               "name cannot be NULL");
        return XIA_NULL_NAME;
    }

    if (value == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_VALUE, "xiaAddFirmwareItem",
               "value cannot be NULL");
        return XIA_NULL_VALUE;
    }

    /* Try and find the alias */
    chosen = xiaFindDetector(alias);
    if (chosen == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NO_ALIAS, "xiaGetDetectorItem",
               "Alias %s has not been created.", alias);
        return XIA_NO_ALIAS;
    }

    strtemp = xia_lower(name);

    if (STREQ(name, "number_of_channels")) {
        *((unsigned short*) value) = chosen->nchan;
    } else if (STREQ(name, "type")) {
        switch (chosen->type) {
            case XIA_DET_RESET:
                strcpy((char*) value, "reset");
                break;
            case XIA_DET_RCFEED:
                strcpy((char*) value, "rc_feedback");
                break;
            default:
            case XIA_DET_UNKNOWN:
                xiaLog(XIA_LOG_ERROR, XIA_BAD_VALUE, "xiaGetDetectorItem",
                       "Detector %s currently is not assigned a valid type",
                       chosen->alias);
                return XIA_BAD_VALUE;
        }
    } else if (STREQ(name, "type_value")) {
        /*
         * Since all values are the same for the typeValue, this is an acceptable
         * thing to do.
         */
        *((double*) value) = chosen->typeValue[0];
    } else if (strncmp(name, "channel", 7) == 0) {
        /* Is it a gain or a polarity? */
        /* The #-may-be-greater-then-one-digit hack */
        sidx = strchr(strtemp, '_');
        sidx[0] = '\0';
        idx = atoi(&strtemp[7]);

        /* Sanity Check: This *is* a valid channel, right?? */
        if (idx >= (int) chosen->nchan) {
            xiaLog(XIA_LOG_ERROR, XIA_BAD_VALUE, "xiaGetDetectorItem",
                   "Channel #: %d is invalid for %s", idx, name);
            return XIA_BAD_VALUE;
        }

        if (STREQ(sidx + 1, "gain")) {
            *((double*) value) = chosen->gain[idx];
        } else if (STREQ(sidx + 1, "polarity")) {
            switch (chosen->polarity[idx]) {
                default:
                    /* Bad Trouble */
                    xiaLog(XIA_LOG_ERROR, XIA_BAD_VALUE, "xiaGetDetectorItem",
                           "Internal polarity value inconsistent");
                    return XIA_BAD_VALUE;
                case 0:
                    strcpy((char*) value, "neg");
                    break;
                case 1:
                    strcpy((char*) value, "pos");
                    break;
            }
        } else {
            xiaLog(XIA_LOG_ERROR, XIA_BAD_NAME, "xiaGetDetectorItem",
                   "Invalid name: %s", name);
            return XIA_BAD_NAME;
        }
    } else {
        xiaLog(XIA_LOG_ERROR, XIA_BAD_NAME, "xiaGetDetectorItem", "Invalid name: %s",
               name);
        return XIA_BAD_NAME;
    }

    return status;
}

HANDEL_EXPORT int HANDEL_API xiaGetNumDetectors(unsigned int* numDetectors) {
    unsigned int count = 0;

    if (numDetectors == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_VALUE, "xiaGetNumDetectors",
               "numDetectors cannot be NULL");
        return XIA_NULL_VALUE;
    }

    Detector* current = xiaDetectorHead;

    while (current != NULL) {
        count++;
        current = getListNext(current);
    }

    *numDetectors = count;

    return XIA_SUCCESS;
}

HANDEL_EXPORT int HANDEL_API xiaGetDetectors(char* detectors[]) {
    int i;

    Detector* current = xiaDetectorHead;

    if (detectors == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_VALUE, "xiaGetDetectors",
               "detectors array is NULL");
        return XIA_NULL_VALUE;
    }

    for (i = 0; current != NULL; current = getListNext(current), i++) {
        if (detectors[i] == NULL) {
            xiaLog(XIA_LOG_ERROR, XIA_NULL_VALUE, "xiaGetDetectors",
                   "detector[%i] is NULL", i);
            return XIA_NULL_VALUE;
        }
        strcpy(detectors[i], current->alias);
    }

    return XIA_SUCCESS;
}

HANDEL_EXPORT int HANDEL_API xiaGetDetectors_VB(unsigned int index, char* alias) {
    unsigned int curIdx;

    Detector* current = xiaDetectorHead;

    for (curIdx = 0; current != NULL; current = getListNext(current), curIdx++) {
        if (curIdx == index) {
            strcpy(alias, current->alias);

            return XIA_SUCCESS;
        }
    }

    xiaLog(XIA_LOG_ERROR, XIA_BAD_INDEX, "xiaGetDetectors_VB",
           "Index = %u is out of range for the detectors list", index);
    return XIA_BAD_INDEX;
}

HANDEL_EXPORT int HANDEL_API xiaRemoveDetector(char* alias) {
    int status = XIA_SUCCESS;
    char* strtemp = NULL;

    Detector* found = NULL;
    Detector* prev = NULL;
    Detector* current = NULL;
    Detector* next = NULL;

    if (alias == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_ALIAS, "xiaRemoveDetector",
               "alias cannot be NULL");
        return XIA_NULL_ALIAS;
    }

    found = xiaFindDetector(alias);
    if (isListEmpty(xiaDetectorHead) || found == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NO_ALIAS, "xiaRemoveDetector",
               "Alias %s does not exist", alias);
        return XIA_NO_ALIAS;
    }

    strtemp = xia_lower(alias);

    current = xiaDetectorHead;
    next = current->next;
    while (!STREQ(strtemp, current->alias)) {
        prev = current;
        current = next;
        next = current->next;
    }

    if (current == xiaDetectorHead) {
        xiaDetectorHead = current->next;
    } else {
        if (prev != NULL) {
            prev->next = current->next;
        } else {
            xiaLog(XIA_LOG_ERROR, XIA_BAD_INDEX, "xiaRemoveDetector",
                   "no previous element in det list");
            return XIA_BAD_INDEX;
        }
    }

    xiaFreeDetector(current);

    return status;
}

/*
 * This routine returns the entry of the Detector linked list that matches
 * the alias.  If NULL is returned, then no match was found.
 */
Detector* HANDEL_API xiaFindDetector(char* alias) {
    char* strtemp = NULL;
    Detector* current = NULL;

    strtemp = xia_lower(alias);

    /* First check if this alias exists already? */
    current = xiaDetectorHead;
    while (current != NULL) {
        if (STREQ(strtemp, current->alias)) {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

/*
 * This routine returns the head of the Detector LL
 */
Detector* HANDEL_API xiaGetDetectorHead(void) {
    return xiaDetectorHead;
}
