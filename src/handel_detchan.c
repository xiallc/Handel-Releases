/* detChan configuration */

/*
 * Copyright (c) 2002-2004 X-ray Instrumentation Associates
 *               2005-2016 XIA LLC
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

#include <stdlib.h>

#include "xia_handel.h"
#include "xia_handel_structures.h"
#include "xia_common.h"

#include "handel_errors.h"
#include "handel_log.h"

HANDEL_STATIC DetChanSetElem* HANDEL_API xiaGetDetSetTail(DetChanSetElem* head);

/*
 * This routine searches through the DetChanElement linked-list and returns
 * TRUE_ if the specified detChan # ISN'T used yet; FALSE_ otherwise.
 */
HANDEL_SHARED boolean_t HANDEL_API xiaIsDetChanFree(int detChan) {
    DetChanElement* current;

    current = xiaDetChanHead;

    if (isListEmpty(current)) {
        return TRUE_;
    }

    while (current != NULL) {
        if (current->detChan == detChan) {
            return FALSE_;
        }
        current = current->next;
    }

    return TRUE_;
}

/*
 * This routine adds a new (valid) DetChanElement. IT ASSUMES THAT THE DETCHAN
 * VALUE HAS ALREADY BEEN VALIDATED, PREFERABLY BY CALLING xiaDetChanFree().
 */
HANDEL_SHARED int HANDEL_API xiaAddDetChan(int type, unsigned int detChan, void* data) {
    int status;
    int len;

    DetChanSetElem* newSetElem = NULL;
    DetChanSetElem* tail = NULL;
    DetChanSetElem* masterTail = NULL;

    DetChanElement* current = NULL;
    DetChanElement* newDetChan = NULL;
    DetChanElement* masterDetChan = NULL;

    /*
     * The general strategy here is to walk to the end of the list and insert a
     * new element. This includes allocating memory and such.
     */

    if (data == NULL) {
        status = XIA_BAD_VALUE;
        sprintf(info_string, "detChan data is NULL");
        xiaLogError("xiaAddDetChan", info_string, status);
        return status;
    }

    if ((type != SINGLE) && (type != SET)) {
        status = XIA_BAD_TYPE;
        sprintf(info_string, "Specified DetChanElement type is invalid");
        xiaLogError("xiaAddDetChan", info_string, status);
        return status;
    }

    newDetChan = (DetChanElement*) handel_md_alloc(sizeof(DetChanElement));
    if (newDetChan == NULL) {
        status = XIA_NOMEM;
        sprintf(info_string, "Not enough memory to create detChan %u", detChan);
        xiaLogError("xiaAddDetChan", info_string, status);
        return status;
    }

    newDetChan->type = type;
    newDetChan->detChan = (int) detChan;
    newDetChan->isTagged = FALSE_;
    newDetChan->next = NULL;

    current = xiaDetChanHead;

    if (isListEmpty(current)) {
        xiaDetChanHead = newDetChan;
        current = xiaDetChanHead;
    } else {
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newDetChan;
    }

    switch (type) {
        case SINGLE:
            len = (int) strlen((char*) data);
            newDetChan->data.modAlias =
                (char*) handel_md_alloc((len + 1) * sizeof(char));
            if (newDetChan->data.modAlias == NULL) {
                status = XIA_NOMEM;
                xiaLogError("xiaAddDetChan",
                            "Not enough memory to create newDetChan->data.modAlias",
                            status);
                return status;
            }

            strcpy(newDetChan->data.modAlias, (char*) data);

            /*
             * Now add it to the -1 list: Does -1 already exist?
             */
            if (xiaIsDetChanFree(-1)) {
                xiaLogInfo("xiaAddDetChan", "Creating master detChan");

                masterDetChan =
                    (DetChanElement*) handel_md_alloc(sizeof(DetChanElement));

                if (masterDetChan == NULL) {
                    status = XIA_NOMEM;
                    xiaLogError("xiaAddDetChan",
                                "Not enough memory to create the master detChan list",
                                status);
                    return status;
                }

                masterDetChan->type = SET;
                masterDetChan->next = NULL;
                masterDetChan->isTagged = FALSE_;
                masterDetChan->data.detChanSet = NULL;
                masterDetChan->detChan = -1;

                /* List cannot be empty thanks to check above */
                while (current->next != NULL) {
                    current = current->next;
                }

                current->next = masterDetChan;

                sprintf(info_string, "(masterDetChan) current->next = %p",
                        current->next);
                xiaLogDebug("xiaAddDetChan", info_string);
            } else {
                masterDetChan = xiaGetDetChanPtr(-1);
            }

            newSetElem = (DetChanSetElem*) handel_md_alloc(sizeof(DetChanSetElem));

            if (newSetElem == NULL) {
                status = XIA_NOMEM;
                xiaLogError("xiaAddDetChan",
                            "Not enough memory to add channel to master detChan list",
                            status);
                return status;
            }

            newSetElem->next = NULL;
            newSetElem->channel = detChan;

            masterTail = xiaGetDetSetTail(masterDetChan->data.detChanSet);

            if (masterTail == NULL) {
                masterDetChan->data.detChanSet = newSetElem;
            } else {
                masterTail->next = newSetElem;
            }

            sprintf(info_string, "Added detChan %u with modAlias %s", detChan,
                    (char*) data);
            xiaLogDebug("xiaAddDetChan", info_string);
            break;
        case SET:
            newDetChan->data.detChanSet = NULL;

            newSetElem = (DetChanSetElem*) handel_md_alloc(sizeof(DetChanSetElem));
            if (newSetElem == NULL) {
                status = XIA_NOMEM;
                xiaLogError("xiaAddDetChan", "Not enough memory to create detChan set",
                            status);
                return status;
            }

            newSetElem->next = NULL;
            newSetElem->channel = *((unsigned int*) data);

            tail = xiaGetDetSetTail(newDetChan->data.detChanSet);

            /* May be the first element */
            if (tail == NULL) {
                newDetChan->data.detChanSet = newSetElem;
            } else {
                tail->next = newSetElem;
            }
            break;
        default:
            /* Should NEVER get here...but that's no excuse for not putting
             * the default case in.
             */
            status = XIA_BAD_TYPE;
            sprintf(
                info_string,
                "Specified DetChanElement type is invalid. Should not be seeing this!");
            xiaLogError("xiaAddDetChan", info_string, status);
            return status;
            break;
    }

    return XIA_SUCCESS;
}

/*
 * This routine removes an element from the DetChanElement LL. The detChan
 * value doesn't even need to be valid since (worst-case) the routine will
 * search the whole list and return an error if it doesn't find it.
 */
HANDEL_SHARED int HANDEL_API xiaRemoveDetChan(unsigned int detChan) {
    int status;

    DetChanElement* current;
    DetChanElement* prev;
    DetChanElement* next;

    current = xiaDetChanHead;

    if (isListEmpty(current)) {
        status = XIA_INVALID_DETCHAN;
        sprintf(info_string, "Specified detChan %u doesn't exist", detChan);
        xiaLogError("xiaRemoveDetChan", info_string, status);
        return status;
    }

    prev = NULL;
    next = current->next;

    while (((int) detChan != current->detChan) && (next != NULL)) {
        prev = current;
        current = next;
        next = current->next;
    }

    if ((next == NULL) && ((int) detChan != current->detChan)) {
        status = XIA_INVALID_DETCHAN;
        sprintf(info_string, "Specified detChan %u doesn't exist", detChan);
        xiaLogError("xiaRemoveDetChan", info_string, status);
        return status;
    }

    sprintf(info_string, "Removing detChan %u", detChan);
    xiaLogInfo("xiaRemoveDetChan", info_string);

    if (current == xiaDetChanHead) {
        xiaDetChanHead = current->next;
    } else {
        prev->next = next;
    }

    switch (current->type) {
        case SINGLE:
            handel_md_free((void*) current->data.modAlias);
            break;
        case SET:
            xiaFreeDetSet(current->data.detChanSet);
            break;
        default:
            status = XIA_BAD_TYPE;
            sprintf(info_string, "Invalid type. Should not be seeing this!");
            xiaLogError("xiaRemoveDetChan", info_string, status);
            return status;
            break;
    }

    handel_md_free((void*) current);

    return XIA_SUCCESS;
}

/*
 * This routine searches through a DetChanSetElem linked-list, starting at
 * head, for the end of the list, which is returned. Returns NULL if there are
 * any problems.
 */
HANDEL_STATIC DetChanSetElem* HANDEL_API xiaGetDetSetTail(DetChanSetElem* head) {
    DetChanSetElem* current;

    current = head;

    if (isListEmpty(current)) {
        return NULL;
    }

    while (current->next != NULL) {
        current = current->next;
    }

    return current;
}

/*
 * This routine frees a DetChanSetElem linked-list by walking through the list
 * and freeing elements in it's wake.
 */
HANDEL_SHARED void HANDEL_API xiaFreeDetSet(DetChanSetElem* head) {
    DetChanSetElem* current;

    current = head;

    while (current != NULL) {
        head = current;
        current = current->next;
        handel_md_free((void*) head);
    }
}

/*
 * This routine returns the value in the type field of the specified detChan.
 */
HANDEL_SHARED int HANDEL_API xiaGetElemType(int detChan) {
    DetChanElement* current = NULL;

    current = xiaDetChanHead;

    while (current != NULL) {
        if (current->detChan == detChan) {
            return current->type;
        }
        current = current->next;
    }

    /* This isn't an error code, this is just an "invalid" type */
    return 999;
}

/*
 * This routine returns a string representing the module->board_type field.
 */
HANDEL_SHARED int HANDEL_API xiaGetBoardType(int detChan, char* boardType) {
    char* modAlias = NULL;

    int status;

    modAlias = xiaGetAliasFromDetChan(detChan);

    if (modAlias == NULL) {
        status = XIA_INVALID_DETCHAN;
        sprintf(info_string, "detChan %d is not a valid module", detChan);
        xiaLogError("xiaGetBoardType", info_string, status);
        return status;
    }

    status = xiaGetModuleItem(modAlias, "module_type", (void*) boardType);

    if (status != XIA_SUCCESS) {
        xiaLogError("xiaGetBoardType", "Error getting board_type from module", status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This routine returns the module alias associated with a given detChan. If
 * the detChan doesn't exist or is a SET then NULL is returned.
 */
HANDEL_SHARED char* HANDEL_API xiaGetAliasFromDetChan(int detChan) {
    DetChanElement* current = NULL;

    if (xiaIsDetChanFree(detChan)) {
        return NULL;
    }

    current = xiaDetChanHead;

    while ((current->next != NULL) && (current->detChan != detChan)) {
        current = current->next;
    }

    if ((current->next == NULL) && (current->detChan != detChan)) {
        return NULL;
    }

    if (current->type == SET) {
        return NULL;
    }

    return current->data.modAlias;
}

/*
 * This routine returns a pointer to the head of the DetChan LL. I know that
 * some people would prefer to just access the global directly, but I'd
 * rather try to implement some sort of encapsulation.
 */
HANDEL_SHARED DetChanElement* HANDEL_API xiaGetDetChanHead(void) {
    return xiaDetChanHead;
}

/*
 * This routine clears the isTagged fields from all the detChan elements.
 */
HANDEL_SHARED void HANDEL_API xiaClearTags(void) {
    DetChanElement* current = NULL;
    current = xiaDetChanHead;
    while (current != NULL) {
        current->isTagged = FALSE_;
        current = getListNext(current);
    }
}

/*
 * This routine returns a pointer to the detChan element denoted by the
 * detChan value.
 */
HANDEL_SHARED DetChanElement* HANDEL_API xiaGetDetChanPtr(int detChan) {
    DetChanElement* current = NULL;

    current = xiaDetChanHead;

    while (current != NULL) {
        if (current->detChan == detChan) {
            return current;
        }

        current = getListNext(current);
    }

    return NULL;
}

/*
 * This routine returns a pointer to the XiaDefaults item associated with
 * the specified detChan. This is pretty much a "convenience" routine.
 */
HANDEL_SHARED XiaDefaults* HANDEL_API xiaGetDefaultFromDetChan(unsigned int detChan) {
    unsigned int modChan;

    int status;

    char tmpStr[MAXITEM_LEN];
    char defaultStr[MAXALIAS_LEN];

    char* alias = NULL;

    modChan = xiaGetModChan(detChan);
    alias = xiaGetAliasFromDetChan((int) detChan);
    sprintf(tmpStr, "default_chan%u", modChan);

    status = xiaGetModuleItem(alias, tmpStr, (void*) defaultStr);

    if (status != XIA_SUCCESS) {
        return NULL;
    }

    return xiaFindDefault(defaultStr);
}
