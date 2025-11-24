/* Dynamic firmware configuration */

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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xerxes.h"
#include "xerxes_errors.h"
#include "xerxes_structures.h"

#include "xia_assert.h"
#include "xia_common.h"
#include "xia_handel.h"
#include "xia_handel_structures.h"

#include "handel_errors.h"
#include "handel_generic.h"
#include "handel_log.h"
#include "handeldef.h"
#include <util/xia_str_manip.h>

static int HANDEL_API xiaSetFirmwareItem(FirmwareSet* fs, Firmware* f, char* name,
                                         void* value);
static boolean_t HANDEL_API xiaIsPTRRFree(Firmware* firmware, unsigned short pttr);

HANDEL_EXPORT int HANDEL_API xiaNewFirmware(char* alias) {
    int status = XIA_SUCCESS;

    FirmwareSet* current = NULL;

    if (alias == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_ALIAS, "xiaNewFirmware", "alias cannot be NULL");
        return XIA_NULL_ALIAS;
    }

    if (!isHandelInit) {
        status = xiaInitHandel();
        if (status != XIA_SUCCESS) {
            fprintf(stderr, "FATAL ERROR: Unable to load libraries.\n");
            exit(XIA_INITIALIZE);
        }

        xiaLog(XIA_LOG_WARNING, "xiaNewFirmware", "Handel initialized silently");
    }

    if ((strlen(alias) + 1) > MAXALIAS_LEN) {
        xiaLog(XIA_LOG_ERROR, XIA_ALIAS_SIZE, "xiaFirmwareDetector",
               "Alias contains too many characters");
        return XIA_ALIAS_SIZE;
    }

    current = xiaFindFirmware(alias);
    if (current != NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_ALIAS_EXISTS, "xiaNewFirmware",
               "Alias %s already in use.", alias);
        return XIA_ALIAS_EXISTS;
    }

    xiaLog(XIA_LOG_DEBUG, "xiaNewFirmware", "create new firmware w/ alias = %s", alias);

    if (xiaFirmwareSetHead == NULL) {
        xiaFirmwareSetHead = (FirmwareSet*) handel_md_alloc(sizeof(FirmwareSet));
        current = xiaFirmwareSetHead;
    } else {
        current = xiaFirmwareSetHead;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = (FirmwareSet*) handel_md_alloc(sizeof(FirmwareSet));
        current = current->next;
    }

    if (current == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NOMEM, "xiaNewFirmware",
               "Unable to allocate memory for Firmware alias %s.", alias);
        return XIA_NOMEM;
    }

    current->alias = (char*) handel_md_alloc((strlen(alias) + 1) * sizeof(char));
    if (current->alias == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NOMEM, "xiaNewFirmware",
               "Unable to allocate memory for current->alias");
        return XIA_NOMEM;
    }

    strcpy(current->alias, alias);

    current->filename = NULL;
    current->keywords = NULL;
    current->numKeywords = 0;
    current->tmpPath = NULL;
    current->mmu = NULL;
    current->firmware = NULL;
    current->next = NULL;

    return XIA_SUCCESS;
}

HANDEL_EXPORT int HANDEL_API xiaAddFirmwareItem(char* alias, char* name, void* value) {
    int status = XIA_SUCCESS;
    char* strtemp = NULL;

    FirmwareSet* chosen = NULL;

    /* Have to keep a pointer for the current Firmware ptrr */
    static Firmware* current = NULL;

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

    /* Locate the FirmwareSet entry first */
    chosen = xiaFindFirmware(alias);
    if (chosen == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NO_ALIAS, "xiaAddFirmwareItem",
               "Alias %s has not been created.", alias);
        return XIA_NO_ALIAS;
    }

    strtemp = xia_lower(name);

    if (STREQ(strtemp, "filename")) {
        status = xiaSetFirmwareItem(chosen, NULL, strtemp, value);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaAddFirmwareItem",
                   "Failure to set Firmware data: %s", name);
            return status;
        }
    } else if (STREQ(strtemp, "ptrr")) {
        if (!xiaIsPTRRFree(chosen->firmware, *((unsigned short*) value))) {
            xiaLog(XIA_LOG_ERROR, XIA_BAD_PTRR, "xiaAddFirmwareItem",
                   "PTRR %u already exists", *((unsigned short*) value));
            return XIA_BAD_PTRR;
        }

        if (chosen->firmware == NULL) {
            chosen->firmware = (Firmware*) handel_md_alloc(sizeof(Firmware));
            current = chosen->firmware;
            current->prev = NULL;
        } else {
            current->next = (Firmware*) handel_md_alloc(sizeof(Firmware));
            current->next->prev = current;
            current = current->next;
        }

        if (current == NULL) {
            xiaLog(XIA_LOG_ERROR, XIA_NOMEM, "xiaAddFirmwareItem",
                   "Unable to allocate memory for firmware");
            return XIA_NOMEM;
        }

        current->ptrr = *((unsigned short*) value);
        current->max_ptime = 0.;
        current->min_ptime = 0.;
        current->user_fippi = NULL;
        current->user_dsp = NULL;
        current->system_fpga = NULL;
        current->dsp = NULL;
        current->fippi = NULL;
        current->next = NULL;
        current->numFilter = 0;
        current->filterInfo = NULL;
    } else {
        status = xiaSetFirmwareItem(chosen, current, strtemp, value);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, XIA_BAD_VALUE, "xiaAddFirmwareItem",
                   "Failure to set Firmware data: %s", name);
            return XIA_BAD_VALUE;
        }
    }

    return XIA_SUCCESS;
}

HANDEL_EXPORT int HANDEL_API xiaModifyFirmwareItem(char* alias, unsigned short ptrr,
                                                   char* name, void* value) {
    int status = XIA_SUCCESS;

    char* strtemp = NULL;

    FirmwareSet* chosen = NULL;

    Firmware* current = NULL;

    if (alias == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_ALIAS, "xiaModifyFirmwareItem",
               "value cannot be NULL");
        return XIA_NULL_ALIAS;
    }

    if (name == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_NAME, "xiaModifyFirmwareItem",
               "value cannot be NULL");
        return XIA_NULL_NAME;
    }

    if (value == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_VALUE, "xiaModifyFirmwareItem",
               "value cannot be NULL");
        return XIA_NULL_VALUE;
    }

    strtemp = xia_lower(name);

    /* Locate the FirmwareSet entry first */
    chosen = xiaFindFirmware(alias);
    if (chosen == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NO_ALIAS, "xiaModifyFirmwareItem",
               "Alias %s was not found.", alias);
        return XIA_NO_ALIAS;
    }

    /*
     * Check to see if the name is a ptrr-invariant name since some
     * users will probably set ptrr to NULL under these circumstances
     * which breaks the code if a ptrr check is performed
     */
    if (STREQ(name, "filename") || STREQ(name, "mmu") || STREQ(name, "fdd_tmp_path")) {
        status = xiaSetFirmwareItem(chosen, current, name, value);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaModifyFirmwareItem",
                   "Failure to set '%s' for '%s'", name, alias);
            return status;
        }

        return status;
    }

    /* Now find the ptrr only if the name being modified requires it */
    current = chosen->firmware;
    while ((current != NULL) && (current->ptrr != ptrr)) {
        current = current->next;
    }

    if (current == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_BAD_VALUE, "xiaModifyFirmwareItem",
               "ptrr (%u) not found.", ptrr);
        return XIA_BAD_VALUE;
    }

    /* Now modify the value */
    status = xiaSetFirmwareItem(chosen, current, strtemp, value);
    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaModifyFirmwareItem",
               "Failure to set Firmware data: %s", name);
        return status;
    }

    return status;
}

HANDEL_EXPORT int HANDEL_API xiaGetFirmwareItem(char* alias, unsigned short ptrr,
                                                char* name, void* value) {
    int status = XIA_SUCCESS;

    unsigned short j;

    char* strtemp = NULL;

    unsigned short* filterInfo = NULL;

    FirmwareSet* chosen = NULL;

    Firmware* current = NULL;

    if (alias == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_ALIAS, "xiaGetFirmwareItem",
               "alias cannot be NULL");
        return XIA_NULL_ALIAS;
    }

    if (name == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_NAME, "xiaGetFirmwareItem",
               "name cannot be NULL");
        return XIA_NULL_NAME;
    }

    if (value == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_VALUE, "xiaGetFirmwareItem",
               "value cannot be NULL");
        return XIA_NULL_VALUE;
    }

    /* Find Firmware corresponding to alias */
    chosen = xiaFindFirmware(alias);

    if (chosen == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NO_ALIAS, "xiaGetFirmwareItem",
               "Alias %s has not been created", alias);
        return XIA_NO_ALIAS;
    }

    strtemp = xia_lower(name);

    /* Decide which value to return. Start with the ptrr-invariant values */
    if (STREQ(strtemp, "filename")) {
        /*
         * The new default behavior is to return a blank string in place of the
         * filename and to not error out.
         */
        if (chosen->filename == NULL) {
            xiaLog(XIA_LOG_INFO, "xiaGetFirmwareItem",
                   "No filename defined for firmware with alias %s", chosen->alias);

            strcpy((char*) value, "");
        } else {
            strcpy((char*) value, chosen->filename);
        }
    } else if (STREQ(strtemp, "fdd_tmp_path")) {
        if (chosen->filename == NULL) {
            xiaLog(XIA_LOG_ERROR, XIA_NO_FILENAME, "xiaGetFirmwareItem",
                   "No FDD file for '%s'", chosen->alias);
            return XIA_NO_FILENAME;
        }

        if (chosen->tmpPath == NULL) {
            xiaLog(XIA_LOG_ERROR, XIA_NO_TMP_PATH, "xiaGetFirmwareItem",
                   "FDD temporary file path never defined for '%s'", chosen->alias);
            return XIA_NO_TMP_PATH;
        }

        ASSERT((strlen(chosen->tmpPath) + 1) < MAXITEM_LEN);

        strcpy((char*) value, chosen->tmpPath);
    } else if (STREQ(strtemp, "mmu")) {
        if (chosen->mmu == NULL) {
            xiaLog(XIA_LOG_ERROR, XIA_NO_FILENAME, "xiaGetFirmwareItem",
                   "No MMU file defined for firmware with alias %s", chosen->alias);
            return XIA_NO_FILENAME;
        }

        strcpy((char*) value, chosen->mmu);
    } else {
        /*
         * Should be branching into names that require the ptrr value...if not
         * we'll still catch it at the end and everything will be fine
         */
        current = chosen->firmware;
        /*
         * Have to check to see if current is NULL as well. While this should
         * be a rare case and was discovered my a malicious test that I wrote, we need
         * to protect against it.
         */
        if (current == NULL) {
            xiaLog(XIA_LOG_ERROR, XIA_BAD_VALUE, "xiaGetFirmwareItem",
                   "No ptrr(s) defined for this alias: %s", alias);
            return XIA_BAD_VALUE;
        }

        while (current->ptrr != ptrr) {
            /* Check to see if we ran into the end of the list here... */
            current = current->next;

            if (current == NULL) {
                xiaLog(XIA_LOG_ERROR, XIA_BAD_PTRR, "xiaGetFirmwareItem",
                       "ptrr %u is not valid for this alias", ptrr);
                return XIA_BAD_PTRR;
            }
        }

        if (STREQ(strtemp, "min_peaking_time")) {
            *((double*) value) = current->min_ptime;
        } else if (STREQ(strtemp, "max_peaking_time")) {
            *((double*) value) = current->max_ptime;
        } else if (STREQ(strtemp, "fippi")) {
            strcpy((char*) value, current->fippi);
        } else if (STREQ(strtemp, "dsp")) {
            strcpy((char*) value, current->dsp);
        } else if (STREQ(strtemp, "user_fippi")) {
            strcpy((char*) value, current->user_fippi);
        } else if (STREQ(strtemp, "user_dsp")) {
            strcpy((char*) value, current->user_dsp);
        } else if (STREQ(strtemp, "num_filter")) {
            *((unsigned short*) value) = current->numFilter;
        } else if (STREQ(strtemp, "filter_info")) {
            /* Do a full copy here */
            filterInfo = (unsigned short*) value;
            for (j = 0; j < current->numFilter; j++) {
                filterInfo[j] = current->filterInfo[j];
            }
        } else {
            /* Bad name */
            xiaLog(XIA_LOG_ERROR, XIA_BAD_NAME, "xiaGetFirmwareItem",
                   "Invalid Name: %s", name);
            return XIA_BAD_NAME;
        }
        /*
         * Bad names propagate all the way to the end so there is no reason
         * to report an error message here.
         */
    }

    return status;
}

HANDEL_EXPORT int HANDEL_API xiaGetNumFirmwareSets(unsigned int* numFirmware) {
    unsigned int count = 0;

    FirmwareSet* current = xiaFirmwareSetHead;

    if (numFirmware == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_VALUE, "xiaGetNumFirmwareSets",
               "numFirmware cannot be NULL");
        return XIA_NULL_VALUE;
    }

    while (current != NULL) {
        count++;
        current = getListNext(current);
    }

    *numFirmware = count;

    return XIA_SUCCESS;
}

HANDEL_EXPORT int HANDEL_API xiaGetFirmwareSets(char* firmwares[]) {
    int i;
    FirmwareSet* current = xiaFirmwareSetHead;

    if (firmwares == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_VALUE, "xiaGetFirmwareSets",
               "firmwares array is NULL");
        return XIA_NULL_VALUE;
    }

    for (i = 0; current != NULL; current = getListNext(current), i++) {
        if (firmwares[i] == NULL) {
            xiaLog(XIA_LOG_ERROR, XIA_NULL_VALUE, "xiaGetFirmwareSets",
                   "firmware[%i] is NULL", i);
            return XIA_NULL_VALUE;
        }
        strcpy(firmwares[i], current->alias);
    }

    return XIA_SUCCESS;
}

HANDEL_EXPORT int HANDEL_API xiaGetFirmwareSets_VB(unsigned int index, char* alias) {
    unsigned int curIdx;

    FirmwareSet* current = xiaFirmwareSetHead;

    for (curIdx = 0; current != NULL; current = getListNext(current), curIdx++) {
        if (curIdx == index) {
            strcpy(alias, current->alias);
            return XIA_SUCCESS;
        }
    }

    xiaLog(XIA_LOG_ERROR, XIA_BAD_INDEX, "xiaGetFirmwareSets_VB",
           "Index = %u is out of range for the firmware set list", index);
    return XIA_BAD_INDEX;
}

HANDEL_EXPORT int HANDEL_API xiaGetNumPTRRs(char* alias, unsigned int* numPTRR) {
    unsigned int count = 0;

    FirmwareSet* chosen = NULL;

    Firmware* current = NULL;

    if (alias == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_ALIAS, "xiaGetNumPTRRs", "alias cannot be NULL");
        return XIA_NULL_ALIAS;
    }

    if (numPTRR == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_VALUE, "xiaGetNumPTRRs",
               "numPTRR cannot be NULL");
        return XIA_NULL_VALUE;
    }

    chosen = xiaFindFirmware(alias);

    if (chosen == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NO_ALIAS, "xiaGetNumPTRRs",
               "Alias %s has not been created yet", alias);
        return XIA_NO_ALIAS;
    }

    if (chosen->filename != NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_LOOKING_PTRR, "xiaGetNumPTRRs",
               "Looking for PTRRs and found an FDD file for alias %s", alias);
        return XIA_LOOKING_PTRR;
    }

    current = chosen->firmware;

    while (current != NULL) {
        count++;
        current = getListNext(current);
    }

    *numPTRR = count;

    return XIA_SUCCESS;
}

/*
 * This routine modifies information about a Firmware Item entry
 */
static int HANDEL_API xiaSetFirmwareItem(FirmwareSet* fs, Firmware* f, char* name,
                                         void* value) {
    int status = XIA_SUCCESS;

    unsigned int i;
    unsigned int len;

    char** oldKeywords = NULL;

    parameter_t* oldFilterInfo = NULL;

    /* Specify the mmu */
    if (STREQ(name, "mmu")) {
        /* If allocated already, then free */
        if (fs->mmu != NULL) {
            handel_md_free((void*) fs->mmu);
        }
        /* Allocate memory for the filename */
        len = (unsigned int) strlen((char*) value);
        fs->mmu = (char*) handel_md_alloc((len + 1) * sizeof(char));
        if (fs->mmu == NULL) {
            xiaLog(XIA_LOG_ERROR, XIA_NOMEM, "xiaSetFirmwareItem",
                   "Unable to allocate memory for fs->mmu");
            return XIA_NOMEM;
        }

        /* Copy the filename into the firmware structure */
        strcpy(fs->mmu, (char*) value);
    } else if (STREQ(name, "filename")) {
        if (fs->filename != NULL) {
            handel_md_free((void*) fs->filename);
        }
        len = (unsigned int) strlen((char*) value);
        fs->filename = (char*) handel_md_alloc((len + 1) * sizeof(char));
        if (fs->filename == NULL) {
            xiaLog(XIA_LOG_ERROR, XIA_NOMEM, "xiaSetFirmwareItem",
                   "Unable to allocate memory for fs->filename");
            return XIA_NOMEM;
        }
        strcpy(fs->filename, (char*) value);
    } else if (STREQ(name, "fdd_tmp_path")) {
        len = (unsigned int) strlen((char*) value);

        fs->tmpPath = handel_md_alloc(len + 1);

        if (!fs->tmpPath) {
            xiaLog(XIA_LOG_ERROR, XIA_NOMEM, "xiaSetFirmwareItem",
                   "Unable to allocated %u bytes for 'fs->tmpPath' with alias = '%s'",
                   len + 1, fs->alias);
            return XIA_NOMEM;
        }

        strcpy(fs->tmpPath, (char*) value);
    } else if (STREQ(name, "keyword")) {
        /*
         * Check to see if the filename exists, because if it doesn't, then there is
         * no reason to be entering keywords. N.b. There is no _real_ reason that the
         * user should have to enter the filename first...it's just that it makes sense
         * from a "logic" standpoint. This restriction _may_ be eased in the future.
         *
         * This should work, even if the number of keywords is zero
         */
        oldKeywords = (char**) handel_md_alloc(fs->numKeywords * sizeof(char*));

        for (i = 0; i < fs->numKeywords; i++) {
            oldKeywords[i] =
                (char*) handel_md_alloc((strlen(fs->keywords[i]) + 1) * sizeof(char));
            strcpy(oldKeywords[i], fs->keywords[i]);
            handel_md_free((void*) (fs->keywords[i]));
        }

        handel_md_free((void*) fs->keywords);
        fs->keywords = NULL;

        /* The original keywords array should be free
         * (if it previously held keywords)
         */
        fs->numKeywords++;
        fs->keywords = (char**) handel_md_alloc(fs->numKeywords * sizeof(char*));
        if (fs->keywords == NULL) {
            xiaLog(XIA_LOG_ERROR, XIA_NOMEM, "xiaSetFirmwareItem",
                   "Unable to allocate memory for fs->keywords");
            return XIA_NOMEM;
        }

        for (i = 0; i < (fs->numKeywords - 1); i++) {
            fs->keywords[i] =
                (char*) handel_md_alloc((strlen(oldKeywords[i]) + 1) * sizeof(char));
            if (fs->keywords[i] == NULL) {
                xiaLog(XIA_LOG_ERROR, XIA_NOMEM, "xiaSetFirmwareItem",
                       "Unable to allocate memory for fs->keywords[i]");
                return XIA_NOMEM;
            }

            strcpy(fs->keywords[i], oldKeywords[i]);
            handel_md_free((void*) oldKeywords[i]);
        }

        handel_md_free((void*) oldKeywords);

        len = (unsigned int) strlen((char*) value);
        fs->keywords[fs->numKeywords - 1] =
            (char*) handel_md_alloc((len + 1) * sizeof(char));
        if (fs->keywords[fs->numKeywords - 1] == NULL) {
            xiaLog(XIA_LOG_ERROR, XIA_NOMEM, "xiaSetFirmwareItem",
                   "Unable to allocate memory for fs->keywords[fs->numKeywords - 1]");
            return XIA_NOMEM;
        }
        strcpy(fs->keywords[fs->numKeywords - 1], (char*) value);
    } else {
        /* Check to make sure a valid Firmware structure exists */
        if (f == NULL) {
            xiaLog(XIA_LOG_ERROR, XIA_BAD_VALUE, "xiaAddFirmwareItem",
                   "PTRR not specified, no Firmware object exists");
            return XIA_BAD_VALUE;
        }

        if (STREQ(name, "min_peaking_time")) {
            /*
             * HanDeL doesn't have enough information to validate the peaking
             * time values here. It only has enough information to check that they
             * are correct relative to one another. We consider "0" to signify that
             * the min/max_peaking_times are not defined yet and, therefore, we only
             * check when one of them is changed and they are both non-zero.
             * In the future I will try and simplify this logic a little bit. For now,
             * brute force!
             */
            f->min_ptime = *((double*) value);
            if ((f->min_ptime != 0) && (f->max_ptime != 0)) {
                if (f->min_ptime > f->max_ptime) {
                    xiaLog(XIA_LOG_ERROR, XIA_BAD_VALUE, "xiaSetFirmwareItem",
                           "Min. peaking time = %f not smaller then max. peaking time",
                           f->min_ptime);
                    return XIA_BAD_VALUE;
                }
            }
        } else if (STREQ(name, "max_peaking_time")) {
            f->max_ptime = *((double*) value);
            if ((f->min_ptime != 0) && (f->max_ptime != 0)) {
                if (f->max_ptime < f->min_ptime) {
                    xiaLog(XIA_LOG_ERROR, XIA_BAD_VALUE, "xiaSetFirmwareItem",
                           "Max. peaking time = %f not larger then min. peaking time",
                           f->max_ptime);
                    return XIA_BAD_VALUE;
                }
            }
        } else if (STREQ(name, "fippi")) {
            /* If allocated already, then free */
            if (f->fippi != NULL) {
                handel_md_free(f->fippi);
            }
            /* Allocate memory for the filename */
            len = (unsigned int) strlen((char*) value);
            f->fippi = (char*) handel_md_alloc((len + 1) * sizeof(char));
            if (f->fippi == NULL) {
                xiaLog(XIA_LOG_ERROR, XIA_NOMEM, "xiaSetFirmwareItem",
                       "Unable to allocate memory for f->fippi");
                return XIA_NOMEM;
            }
            strcpy(f->fippi, (char*) value);
        } else if (STREQ(name, "user_fippi")) {
            if (f->user_fippi != NULL) {
                handel_md_free(f->user_fippi);
            }

            len = (unsigned int) strlen((char*) value);
            f->user_fippi = (char*) handel_md_alloc((len + 1) * sizeof(char));
            if (f->user_fippi == NULL) {
                xiaLog(XIA_LOG_ERROR, XIA_NOMEM, "xiaSetFirmwareItem",
                       "Unable to allocate memory for f->user_fippi");
                return XIA_NOMEM;
            }

            strcpy(f->user_fippi, (char*) value);
        } else if (STREQ(name, "dsp")) {
            if (f->dsp != NULL) {
                handel_md_free(f->dsp);
            }

            /* Allocate memory for the filename */
            len = (unsigned int) strlen((char*) value);
            f->dsp = (char*) handel_md_alloc((len + 1) * sizeof(char));
            if (f->dsp == NULL) {
                xiaLog(XIA_LOG_ERROR, XIA_NOMEM, "xiaSetFirmwareItem",
                       "Unable to allocate memory for f->dsp");
                return XIA_NOMEM;
            }

            strcpy(f->dsp, (char*) value);
        } else if (STREQ(name, "filter_info")) {
            oldFilterInfo =
                (parameter_t*) handel_md_alloc(f->numFilter * sizeof(parameter_t));

            for (i = 0; i < f->numFilter; i++) {
                oldFilterInfo[i] = f->filterInfo[i];
            }

            handel_md_free((void*) f->filterInfo);
            f->filterInfo = NULL;

            f->numFilter++;
            f->filterInfo =
                (parameter_t*) handel_md_alloc(f->numFilter * sizeof(parameter_t));

            for (i = 0; i < (unsigned short) (f->numFilter - 1); i++) {
                f->filterInfo[i] = oldFilterInfo[i];
            }
            f->filterInfo[f->numFilter - 1] = *((parameter_t*) value);

            handel_md_free((void*) oldFilterInfo);
            oldFilterInfo = NULL;
        } else {
            xiaLog(XIA_LOG_ERROR, XIA_BAD_NAME, "xiaSetFirmwareItem",
                   "Invalid name %s.", name);
            return XIA_BAD_NAME;
        }
    }

    return status;
}

HANDEL_EXPORT int HANDEL_API xiaRemoveFirmware(char* alias) {
    int status = XIA_SUCCESS;

    char* strtemp = NULL;

    FirmwareSet* found = NULL;
    FirmwareSet* prev = NULL;
    FirmwareSet* current = NULL;
    FirmwareSet* next = NULL;

    if (alias == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_ALIAS, "xiaRemoveFirmware",
               "alias cannot be NULL");
        return XIA_NULL_ALIAS;
    }

    found = xiaFindFirmware(alias);
    if (isListEmpty(xiaFirmwareSetHead) || found == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NO_ALIAS, "xiaRemoveFirmware",
               "Alias %s does not exist", alias);
        return XIA_NO_ALIAS;
    }

    strtemp = xia_lower(alias);

    current = xiaFirmwareSetHead;
    next = current->next;
    while (!STREQ(strtemp, current->alias)) {
        prev = current;
        current = next;
        next = current->next;
    }

    if (current == xiaFirmwareSetHead) {
        xiaFirmwareSetHead = current->next;
    } else {
        if (prev != NULL) {
            prev->next = current->next;
        } else {
            xiaLog(XIA_LOG_ERROR, XIA_BAD_INDEX, "xiaRemoveFirmware",
                   "no previous element in fw list");
            return XIA_BAD_INDEX;
        }
    }

    xiaFreeFirmwareSet(current);

    return status;
}

/*
 * This routine returns the entry of the Firmware linked list that matches
 * the alias. If NULL is returned, then no match was found.
 */
FirmwareSet* HANDEL_API xiaFindFirmware(char* alias) {
    char* strtemp = NULL;

    FirmwareSet* current = NULL;

    strtemp = xia_lower(alias);

    /* First check if this alias exists already? */
    current = xiaFirmwareSetHead;
    while (current != NULL) {
        /* If the alias matches, return a pointer to the detector */
        if (STREQ(strtemp, current->alias)) {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

/*
 * Searches the Firmware LL and returns TRUE if the specified PTTR already
 * is in the list.
 */
static boolean_t HANDEL_API xiaIsPTRRFree(Firmware* firmware, unsigned short pttr) {
    while (firmware != NULL) {
        if (firmware->ptrr == pttr) {
            return FALSE_;
        }
        firmware = firmware->next;
    }

    return TRUE_;
}

/*
 * Returns the head of the FirmwareSet LL
 */
FirmwareSet* HANDEL_API xiaGetFirmwareSetHead(void) {
    return xiaFirmwareSetHead;
}

/*
 * This routine returns the number of firmware in a Firmware LL
 */
int HANDEL_API xiaGetNumFirmware(Firmware* firmware) {
    int numFirm;

    Firmware* current = NULL;

    current = firmware;
    numFirm = 0;

    while (current != NULL) {
        numFirm++;
        current = current->next;
    }

    return numFirm;
}

/*
 * This routine compares two Firmware elements (actually, the min peaking time
 * value of each) and returns 1 if key1 > key2, 0 if key1 == key2, and -1 if
 * key1 < key2.
 */
int xiaFirmComp(const void* key1, const void* key2) {
    Firmware* key1_f = (Firmware*) key1;
    Firmware* key2_f = (Firmware*) key2;

    if (key1_f->min_ptime > key2_f->min_ptime) {
        return 1;
    } else if (key1_f->min_ptime == key2_f->min_ptime) {
        return 0;
    } else if (key1_f->min_ptime < key2_f->min_ptime) {
        return -1;
    }

    /* Compiler wants this... */
    return 0;
}

/*
 * This routine returns the name of the dsp code associated with the alias
 * and peakingTime.
 */
int HANDEL_API xiaGetDSPNameFromFirmware(char* alias, double peakingTime,
                                         char* dspName) {
    FirmwareSet* current = NULL;
    Firmware* firmware = NULL;

    current = xiaFindFirmware(alias);
    if (current == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NO_ALIAS, "xiaGetDSPNameFromFirmware",
               "Unable to find firmware %s", alias);
        return XIA_NO_ALIAS;
    }

    firmware = current->firmware;
    while (firmware != NULL) {
        if ((peakingTime >= firmware->min_ptime) &&
            (peakingTime <= firmware->max_ptime)) {
            strcpy(dspName, firmware->dsp);
            return XIA_SUCCESS;
        }

        firmware = getListNext(firmware);
    }

    xiaLog(XIA_LOG_ERROR, XIA_BAD_VALUE, "xiaGetDSPNameFromFirmware",
           "peakingTime %f does not match any of the PTRRs in %s", peakingTime, alias);
    return XIA_BAD_VALUE;
}

/*
 * This returns the name of the FiPPI code associated with the alias and
 * peaking time.
 */
int HANDEL_API xiaGetFippiNameFromFirmware(char* alias, double peakingTime,
                                           char* fippiName) {
    FirmwareSet* current = NULL;
    Firmware* firmware = NULL;

    current = xiaFindFirmware(alias);
    if (current == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NO_ALIAS, "xiaGetFippiNameFromFirmware",
               "Unable to find firmware %s", alias);
        return XIA_NO_ALIAS;
    }

    firmware = current->firmware;

    while (firmware != NULL) {
        if ((peakingTime >= firmware->min_ptime) &&
            (peakingTime <= firmware->max_ptime)) {
            strcpy(fippiName, firmware->fippi);
            return XIA_SUCCESS;
        }

        firmware = getListNext(firmware);
    }

    xiaLog(XIA_LOG_ERROR, XIA_BAD_VALUE, "xiaGetFippiNameFromFirmware",
           "peakingTime %f does not match any of the PTRRs in %s", peakingTime, alias);
    return XIA_BAD_VALUE;
}

/*
 * This routine looks to replace the get*FromFirmware() routines by using
 * a generic search based on peakingTime.
 */
int HANDEL_API xiaGetValueFromFirmware(char* alias, double peakingTime, char* name,
                                       char* value) {
    FirmwareSet* current = NULL;
    Firmware* firmware = NULL;

    current = xiaFindFirmware(alias);
    if (current == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NO_ALIAS, "xiaGetValueFromFirmware",
               "Unable to find firmware %s", alias);
        return XIA_NO_ALIAS;
    }

    if (STREQ(name, "mmu")) {
        if (current->mmu == NULL) {
            xiaLog(XIA_LOG_ERROR, XIA_BAD_VALUE, "xiaGetValueFromFirmware",
                   "MMU is NULL");
            return XIA_BAD_VALUE;
        }

        strcpy(value, current->mmu);
        return XIA_SUCCESS;
    }

    /*
     * Hacky way of dealing with the
     * special uDXP FiPPI types.
     */
    if (STREQ(name, "fippi0") || STREQ(name, "fippi1") || STREQ(name, "fippi2")) {
        strcpy(value, name);
        return XIA_SUCCESS;
    }

    firmware = current->firmware;
    while (firmware != NULL) {
        if ((peakingTime >= firmware->min_ptime) &&
            (peakingTime <= firmware->max_ptime)) {
            if (STREQ(name, "fippi")) {
                if (firmware->fippi == NULL) {
                    xiaLog(XIA_LOG_ERROR, XIA_BAD_VALUE, "xiaGetValueFromFirmware",
                           "FiPPI is NULL");
                    return XIA_BAD_VALUE;
                }

                strcpy(value, firmware->fippi);

                return XIA_SUCCESS;
            } else if (STREQ(name, "user_fippi")) {
                if (firmware->user_fippi == NULL) {
                    xiaLog(XIA_LOG_ERROR, XIA_BAD_VALUE, "xiaGetValueFromFirmware",
                           "User FiPPI is NULL");
                    return XIA_BAD_VALUE;
                }

                strcpy(value, firmware->user_fippi);

                return XIA_SUCCESS;
            } else if (STREQ(name, "dsp")) {
                if (firmware->dsp == NULL) {
                    xiaLog(XIA_LOG_ERROR, XIA_BAD_VALUE, "xiaGetValueFromFirmware",
                           "DSP is NULL");
                    return XIA_BAD_VALUE;
                }

                strcpy(value, firmware->dsp);

                return XIA_SUCCESS;
            } else if (STREQ(name, "user_dsp")) {
                if (firmware->user_dsp == NULL) {
                    xiaLog(XIA_LOG_ERROR, XIA_BAD_VALUE, "xiaGetValueFromFirmware",
                           "User DSP is NULL");
                    return XIA_BAD_VALUE;
                }
                strcpy(value, firmware->user_dsp);
                return XIA_SUCCESS;
            }
        }

        firmware = getListNext(firmware);
    }

    xiaLog(XIA_LOG_ERROR, XIA_BAD_VALUE, "xiaGetValueFromFirmware", "Error getting %s from %s", name, alias);
    return XIA_BAD_VALUE;
}
