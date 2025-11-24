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

#include "xia_common.h"
#include "xia_handel.h"
#include "xia_handel_structures.h"

#include "handel_errors.h"
#include "handel_log.h"

static DetChanSetElem* HANDEL_API xiaGetDetSetTail(DetChanSetElem* head);

/*
 * This routine searches through the DetChanElement linked-list and returns
 * TRUE_ if the specified detChan # ISN'T used yet; FALSE_ otherwise.
 */
boolean_t HANDEL_API xiaIsDetChanFree(int detChan) {
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
int HANDEL_API xiaAddDetChan(int type, unsigned int detChan, void* data) {
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
        xiaLog(XIA_LOG_ERROR, XIA_BAD_VALUE, "xiaAddDetChan", "detChan data is NULL");
        return XIA_BAD_VALUE;
    }

    if ((type != SINGLE) && (type != SET)) {
        xiaLog(XIA_LOG_ERROR, XIA_BAD_TYPE, "xiaAddDetChan",
               "DetChanElement type is invalid");
        return XIA_BAD_TYPE;
    }

    newDetChan = (DetChanElement*) handel_md_alloc(sizeof(DetChanElement));
    if (newDetChan == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NOMEM, "xiaAddDetChan",
               "Not enough memory to create detChan %u", detChan);
        return XIA_NOMEM;
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
                xiaLog(XIA_LOG_ERROR, XIA_NOMEM, "xiaAddDetChan",
                       "cannot create new DetChan alias");
                return XIA_NOMEM;
            }

            strcpy(newDetChan->data.modAlias, (char*) data);

            /*
             * Now add it to the -1 list: Does -1 already exist?
             */
            if (xiaIsDetChanFree(-1)) {
                xiaLog(XIA_LOG_INFO, "xiaAddDetChan", "Creating master detChan");

                masterDetChan =
                    (DetChanElement*) handel_md_alloc(sizeof(DetChanElement));

                if (masterDetChan == NULL) {
                    xiaLog(XIA_LOG_ERROR, XIA_NOMEM, "xiaAddDetChan",
                           "Not enough memory to create the master detChan list");
                    return XIA_NOMEM;
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

                xiaLog(XIA_LOG_DEBUG, "xiaAddDetChan",
                       "(masterDetChan) current->next = %p", current->next);
            } else {
                masterDetChan = xiaGetDetChanPtr(-1);
            }

            newSetElem = (DetChanSetElem*) handel_md_alloc(sizeof(DetChanSetElem));

            if (newSetElem == NULL) {
                xiaLog(XIA_LOG_ERROR, XIA_NOMEM, "xiaAddDetChan",
                       "Not enough memory to add channel to master detChan list");
                return XIA_NOMEM;
            }

            newSetElem->next = NULL;
            newSetElem->channel = detChan;

            masterTail = xiaGetDetSetTail(masterDetChan->data.detChanSet);

            if (masterTail == NULL) {
                masterDetChan->data.detChanSet = newSetElem;
            } else {
                masterTail->next = newSetElem;
            }

            xiaLog(XIA_LOG_DEBUG, "xiaAddDetChan", "Added detChan %u with modAlias %s",
                   detChan, (char*) data);
            break;
        case SET:
            newDetChan->data.detChanSet = NULL;

            newSetElem = (DetChanSetElem*) handel_md_alloc(sizeof(DetChanSetElem));
            if (newSetElem == NULL) {
                xiaLog(XIA_LOG_ERROR, XIA_NOMEM, "xiaAddDetChan",
                       "Not enough memory to create detChan set");
                return XIA_NOMEM;
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
            xiaLog(
                XIA_LOG_ERROR, XIA_BAD_TYPE, "xiaAddDetChan",
                "Specified DetChanElement type is invalid. Should not be seeing this!");
            return XIA_BAD_TYPE;
            break;
    }

    return XIA_SUCCESS;
}

/*
 * This routine removes an element from the DetChanElement LL. The detChan
 * value doesn't even need to be valid since (worst-case) the routine will
 * search the whole list and return an error if it doesn't find it.
 */
int HANDEL_API xiaRemoveDetChan(unsigned int detChan) {
    DetChanElement* current;
    DetChanElement* prev;
    DetChanElement* next;

    current = xiaDetChanHead;

    if (isListEmpty(current)) {
        xiaLog(XIA_LOG_ERROR, XIA_INVALID_DETCHAN, "xiaRemoveDetChan",
               "Specified detChan %u doesn't exist", detChan);
        return XIA_INVALID_DETCHAN;
    }

    prev = NULL;
    next = current->next;

    while (((int) detChan != current->detChan) && (next != NULL)) {
        prev = current;
        current = next;
        next = current->next;
    }

    if ((next == NULL) && ((int) detChan != current->detChan)) {
        xiaLog(XIA_LOG_ERROR, XIA_INVALID_DETCHAN, "xiaRemoveDetChan",
               "Specified detChan %u doesn't exist", detChan);
        return XIA_INVALID_DETCHAN;
    }

    xiaLog(XIA_LOG_INFO, "xiaRemoveDetChan", "Removing detChan %u", detChan);

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
            xiaLog(XIA_LOG_ERROR, XIA_BAD_TYPE, "xiaRemoveDetChan",
                   "Invalid type. Should not be seeing this!");
            return XIA_BAD_TYPE;
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
static DetChanSetElem* HANDEL_API xiaGetDetSetTail(DetChanSetElem* head) {
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
void HANDEL_API xiaFreeDetSet(DetChanSetElem* head) {
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
int HANDEL_API xiaGetElemType(int detChan) {
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
int HANDEL_API xiaGetBoardType(int detChan, char* boardType) {
    char* modAlias = NULL;

    int status;

    modAlias = xiaGetAliasFromDetChan(detChan);

    if (modAlias == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_INVALID_DETCHAN, "xiaGetBoardType",
               "detChan %d is not a valid module", detChan);
        return XIA_INVALID_DETCHAN;
    }

    status = xiaGetModuleItem(modAlias, "module_type", (void*) boardType);

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaGetBoardType",
               "Error getting board_type from module");
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This routine returns the module alias associated with a given detChan. If
 * the detChan doesn't exist or is a SET then NULL is returned.
 */
char* HANDEL_API xiaGetAliasFromDetChan(int detChan) {
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
DetChanElement* HANDEL_API xiaGetDetChanHead(void) {
    return xiaDetChanHead;
}

/*
 * This routine clears the isTagged fields from all the detChan elements.
 */
void HANDEL_API xiaClearTags(void) {
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
DetChanElement* HANDEL_API xiaGetDetChanPtr(int detChan) {
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
XiaDefaults* HANDEL_API xiaGetDefaultFromDetChan(unsigned int detChan) {
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
