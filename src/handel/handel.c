/* Top-level Handel routines: initialization, exit, version. */

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

/* Include the VLD header for memory debugging options */
#ifdef __VLD_MEM_DBG__
#include <vld.h>
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xerxes.h"
#include "xerxes_errors.h"
#include "xerxes_structures.h"

#include "xia_assert.h"
#include "xia_common.h"
#include "xia_file.h"
#include "xia_handel.h"
#include "xia_handel_structures.h"
#include "xia_module.h"
#include "xia_system.h"
#include "xia_version.h"

#include "handel_errors.h"
#include "handel_generic.h"
#include "handel_log.h"
#include "handeldef.h"

#include "fdd.h"

static int HANDEL_API xiaInitMemory(void);
static int HANDEL_API xiaInitDetectorDS(void);
static int HANDEL_API xiaInitFirmwareSetDS(void);
static int HANDEL_API xiaInitXiaDefaultsDS(void);
static int HANDEL_API xiaInitDetChanDS(void);
static int HANDEL_API xiaInitModuleDS(void);

static int HANDEL_API xiaUnHook(void);

/*
 * Used to track if the library functions are initialized.
 */
boolean_t isHandelInit = FALSE_;

/*
 * Define the head of the Detector list
 */
Detector* xiaDetectorHead = NULL;

/*
 * Define the head of the Firmware Sets LL
 */
FirmwareSet* xiaFirmwareSetHead = NULL;

/*
 * Define the head of the XiaDefaults LL
 */
XiaDefaults* xiaDefaultsHead = NULL;

/*
 * Define the head of the Modules LL
 */
Module* xiaModuleHead = NULL;

/*
 * This is the head of the DetectorChannel LL
 */
DetChanElement* xiaDetChanHead = NULL;

HANDEL_EXPORT int HANDEL_API xiaInit(char* iniFile) {
    int status;
    int nFilesOpen;

    if (iniFile == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_BAD_NAME, "xiaInit", "iniFile was NULL");
        return XIA_BAD_NAME;
    }

    /* We need to clear and re-initialize Handel */
    status = xiaInitHandel();
    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaInit", "Error reinitializing Handel");
        return status;
    }

    /*
     * Verify that we currently don't have any file handles open. This is
     * not a legitimate error condition and indicates that we are not cleaning
     * up all of our handles somewhere else in the library.
     */
    nFilesOpen = xia_num_open_handles();

    if (nFilesOpen > 0) {
        xia_print_open_handles(stdout);
        FAIL();
    }

    status = xiaReadIniFile(iniFile);

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaInit", "unable to load %s", iniFile);
        return status;
    }

    return XIA_SUCCESS;
}

HANDEL_EXPORT int HANDEL_API xiaInitHandel(void) {
    int status = XIA_SUCCESS;

    /* Arbitrary length here */
    char version[120];

    if (!isHandelInit) {
        /*
         * Make sure everything is working on the Xerxes side of things.
         */
        status = dxp_init_library();

        if (status != DXP_SUCCESS) {
            /* Use printf() here since dxp_md_error() won't be properly initialized until
             * I assign the function pointers to the imported utils variable
             */
            printf("[ERROR] [%d] %s: %s\n", status, "xiaInitHandel",
                   "Unable to initialize XerXes libraries\n");
            return status;
        }

        /*
         * Make our function pointers equal to XerXes function pointers using the
         * imported utils variable
         */
        handel_md_log = utils->funcs->dxp_md_log;
        handel_md_output = utils->funcs->dxp_md_output;
        handel_md_enable_log = utils->funcs->dxp_md_enable_log;
        handel_md_suppress_log = utils->funcs->dxp_md_suppress_log;
        handel_md_set_log_level = utils->funcs->dxp_md_set_log_level;

        handel_md_alloc = utils->funcs->dxp_md_alloc;
        handel_md_free = utils->funcs->dxp_md_free;

        handel_md_puts = utils->funcs->dxp_md_puts;
        handel_md_wait = utils->funcs->dxp_md_wait;
        handel_md_fgets = utils->funcs->dxp_md_fgets;

        /* Init the FDD lib here */
        status = xiaFddInitialize();

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaInitHandel",
                   "Error initializing FDD layer");
            return status;
        }

        isHandelInit = TRUE_;
    } else {
        /*
         * Most user will be calling xiaInit after xiaInitHandel has already
         * executed from xiaSetLogLevel. To be safe the connection is always
         * re-initialized.
         */
        xiaLog(XIA_LOG_INFO, "xiaInitHandel", "Closing off existing connections.");
        status = xiaUnHook();
    }

    xiaLog(XIA_LOG_INFO, "xiaInitHandel", "Starting Handel");

    /*
     * Initialize the memory of both Handel and Xerxes.
     */
    status = xiaInitMemory();

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaInitHandel", "Unable to Initialize memory");
        return status;
    }

    xiaGetVersionInfo(NULL, NULL, NULL, version);
    xiaLog(XIA_LOG_INFO, "xiaInitHandel", "Successfully initialized Handel %s",
           version);

    /* Print out build configuration */
    xiaLog(XIA_LOG_INFO, "xiaInitHandel", "--- Supported interface ---");

#ifndef EXCLUDE_EPP
    xiaLog(XIA_LOG_INFO, "xiaInitHandel", "epp");
#endif /* EXCLUDE_EPP */

#ifndef EXCLUDE_USB
    xiaLog(XIA_LOG_INFO, "xiaInitHandel", "usb");
#endif /* EXCLUDE_USB */

#ifndef EXCLUDE_USB2
    xiaLog(XIA_LOG_INFO, "xiaInitHandel", "usb2");
#endif /* EXCLUDE_USB2 */

#ifndef EXCLUDE_SERIAL
    xiaLog(XIA_LOG_INFO, "xiaInitHandel", "serial");
#endif /* EXCLUDE_SERIAL */

#ifndef EXCLUDE_PLX
    xiaLog(XIA_LOG_INFO, "xiaInitHandel", "plx");
#endif /* EXCLUDE_PLX */

    xiaLog(XIA_LOG_INFO, "xiaInitHandel", "--- Supported board types ---");

#ifndef EXCLUDE_SATURN
    xiaLog(XIA_LOG_INFO, "xiaInitHandel", "saturn");
#endif

#ifndef EXCLUDE_UDXPS
    xiaLog(XIA_LOG_INFO, "xiaInitHandel", "udxps");
#endif

#ifndef EXCLUDE_UDXP
    xiaLog(XIA_LOG_INFO, "xiaInitHandel", "udxp");
#endif

#ifndef EXCLUDE_XMAP
    xiaLog(XIA_LOG_INFO, "xiaInitHandel", "xmap");
#endif /* EXCLUDE_XMAP */

#ifndef EXCLUDE_STJ
    xiaLog(XIA_LOG_INFO, "xiaInitHandel", "stj");
#endif /* EXCLUDE_STJ */

#ifndef EXCLUDE_MERCURY
    xiaLog(XIA_LOG_INFO, "xiaInitHandel", "mercury");
#endif /* EXCLUDE_MERCURY */

    return status;
}

/*
 * This routine initializes the data structures.
 */
static int HANDEL_API xiaInitMemory(void) {
    int status = XIA_SUCCESS;

    xiaLog(XIA_LOG_INFO, "xiaInitMemory", "Initializing Handel data structure.");

    status = xiaInitDetectorDS();
    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaInitMemory",
               "Unable to clear the Detector LL");
        return status;
    }

    status = xiaInitFirmwareSetDS();
    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaInitMemory",
               "Unable to clear the FirmwareSet LL");
        return status;
    }

    status = xiaInitModuleDS();
    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaInitMemory", "Unable to clear Module LL");
        return status;
    }

    status = xiaInitDetChanDS();
    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaInitMemory", "Unable to clear DetChan LL");
        return status;
    }

    status = xiaInitXiaDefaultsDS();
    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaInitMemory", "Unable to clear Defaults LL");
        return status;
    }

    return status;
}

/*
 * This routines disconnects from the hardware and cleans Handel's
 * internal data structures.
 */
HANDEL_EXPORT int HANDEL_API xiaExit(void) {
    int status;
    xiaLog(XIA_LOG_INFO, "xiaExit", "Exiting...");

    /*
     * Close down any communications that need to be shutdown.
     */
    status = xiaUnHook();

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaExit", "Error shutting down communications");
    }

    /* Other shutdown procedures go here */
    status = xiaInitMemory();
    status = dxp_init_ds();

    return XIA_SUCCESS;
}

/*
 * Returns various components of Handel's version information. These values
 * would typically be reassembled using a syntax such as 'maj'.'min'.'rel'.
 *
 * The optional pretty argument returns a string preformatted for
 * writing to a log or display. The 'pretty' string also contains an
 * extra tag of information indicating special build information (dev,
 * release, etc).
 */
HANDEL_EXPORT void HANDEL_API xiaGetVersionInfo(int* rel, int* min, int* maj,
                                                char* pretty) {
    if (rel && min && maj) {
        *rel = HANDEL_RELEASE_VERSION;
        *min = HANDEL_MINOR_VERSION;
        *maj = HANDEL_MAJOR_VERSION;
    }

    if (pretty) {
        sprintf(pretty, "v%d.%d.%d (%s)", HANDEL_MAJOR_VERSION, HANDEL_MINOR_VERSION,
                HANDEL_RELEASE_VERSION, VERSION_STRING);
    }
}

/*
 * Initializes the Detector data structures to an empty state.
 */
static int HANDEL_API xiaInitDetectorDS(void) {
    int status = XIA_SUCCESS;

    Detector* next = NULL;
    Detector* current = xiaDetectorHead;

    /* Search through the Detector LL and clear them out */
    while ((current != NULL) && (status == XIA_SUCCESS)) {
        next = current->next;

        status = xiaFreeDetector(current);
        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaInitDetectorDS",
                   "Error freeing detector");
            return status;
        }

        current = next;
    }

    xiaDetectorHead = NULL;

    return status;
}

/*
 * Frees the memory allocated to a Detector structure.
 *
 * Detector *detector;          Input: pointer to structure to free
 */
int HANDEL_API xiaFreeDetector(Detector* detector) {
    int status = XIA_SUCCESS;

    if (detector == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NOMEM, "xiaFreeDetector",
               "Detector object unallocated:  can not free");
        return XIA_NOMEM;
    }

    if (detector->alias != NULL) {
        handel_md_free(detector->alias);
    }
    if (detector->polarity != NULL) {
        handel_md_free(detector->polarity);
    }
    if (detector->gain != NULL) {
        handel_md_free(detector->gain);
    }

    handel_md_free(detector->typeValue);

    /* Free the Board_Info structure */
    handel_md_free(detector);
    detector = NULL;

    return status;
}

/*
 * Initializes the FirmwareSet data structures to an empty state.
 */
static int HANDEL_API xiaInitFirmwareSetDS(void) {
    int status = XIA_SUCCESS;

    FirmwareSet* next;
    FirmwareSet* current = xiaFirmwareSetHead;

    /* Search through the FirmwareSet LL and clear them out */
    while ((current != NULL) && (status == XIA_SUCCESS)) {
        next = current->next;

        status = xiaFreeFirmwareSet(current);
        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaInitFirmwareSetDS",
                   "Error freeing FirmwareSet");
            return status;
        }

        current = next;
    }

    xiaFirmwareSetHead = NULL;

    return status;
}

/*
 * Frees the memory allocated to a FirmwareSet structure.
 *
 * FirmwareSet *firmwareSet;        Input: pointer to structure to free
 */
int HANDEL_API xiaFreeFirmwareSet(FirmwareSet* firmwareSet) {
    int status = XIA_SUCCESS;

    unsigned int i;

    Firmware *next, *current;

    if (firmwareSet == NULL) {
        status = XIA_NOMEM;
        xiaLog(XIA_LOG_ERROR, status, "xiaFreeFirmwareSet",
               "FirmwareSet object unallocated:  can not free");
        return status;
    }

    if (firmwareSet->alias != NULL) {
        handel_md_free(firmwareSet->alias);
    }

    if (firmwareSet->filename != NULL) {
        handel_md_free(firmwareSet->filename);
    }

    if (firmwareSet->mmu != NULL) {
        handel_md_free(firmwareSet->mmu);
    }

    if (firmwareSet->tmpPath != NULL) {
        handel_md_free(firmwareSet->tmpPath);
    }

    /* Loop over the Firmware information, deallocating memory */
    current = firmwareSet->firmware;

    while (current != NULL) {
        next = current->next;
        status = xiaFreeFirmware(current);
        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaFreeFirmwareSet",
                   "Error freeing firmware");
            return status;
        }
        current = next;
    }

    for (i = 0; i < firmwareSet->numKeywords; i++) {
        handel_md_free((void*) firmwareSet->keywords[i]);
    }

    handel_md_free((void*) firmwareSet->keywords);

    /* Free the FirmwareSet structure */
    handel_md_free(firmwareSet);
    firmwareSet = NULL;

    return status;
}

/*
 * Frees the memory allocated to a Firmware structure.
 *
 * Firmware *firmware;        Input: pointer to structure to free
 */
int HANDEL_API xiaFreeFirmware(Firmware* firmware) {
    int status = XIA_SUCCESS;

    if (firmware == NULL) {
        status = XIA_NOMEM;
        xiaLog(XIA_LOG_ERROR, status, "xiaFreeFirmware",
               "Firmware object unallocated:  can not free");
        return status;
    }

    if (firmware->dsp != NULL) {
        handel_md_free((void*) firmware->dsp);
    }

    if (firmware->fippi != NULL) {
        handel_md_free((void*) firmware->fippi);
    }

    if (firmware->user_fippi != NULL) {
        handel_md_free((void*) firmware->user_fippi);
    }

    if (firmware->user_dsp != NULL) {
        handel_md_free((void*) firmware->user_dsp);
    }

    if (firmware->system_fpga != NULL) {
        handel_md_free((void*) firmware->system_fpga);
    }

    if (firmware->filterInfo != NULL) {
        handel_md_free((void*) firmware->filterInfo);
    }

    handel_md_free((void*) firmware);
    firmware = NULL;

    return status;
}

/*
 * Initializes the XiaDefaults structure.
 */
static int HANDEL_API xiaInitXiaDefaultsDS(void) {
    int status = XIA_SUCCESS;

    XiaDefaults* next;
    XiaDefaults* current = xiaDefaultsHead;

    /* Search through the XiaDefaults LL and clear them out */
    while ((current != NULL) && (status == XIA_SUCCESS)) {
        next = current->next;

        status = xiaFreeXiaDefaults(current);
        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaInitXiaDefaultDS",
                   "Error freeing default");
            return status;
        }

        current = next;
    }
    /* And clear out the pointer to the head of the list(the memory is freed) */
    xiaDefaultsHead = NULL;

    return status;
}

/*
 * Frees the memory allocated to an XiaDefaults structure.
 */
int HANDEL_API xiaFreeXiaDefaults(XiaDefaults* xiaDefaults) {
    int status = XIA_SUCCESS;

    XiaDaqEntry* next = NULL;
    XiaDaqEntry* current = NULL;

    if (xiaDefaults == NULL) {
        status = XIA_NOMEM;
        xiaLog(XIA_LOG_ERROR, status, "xiaFreeXiaDefaults",
               "XiaDefaults object unallocated:  can not free");
        return status;
    }

    if (xiaDefaults->alias != NULL) {
        handel_md_free(xiaDefaults->alias);
    }

    /* Loop over the xiaDaqEntry information, deallocating memory */
    current = xiaDefaults->entry;
    while (current != NULL) {
        next = current->next;
        status = xiaFreeXiaDaqEntry(current);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaFreeXiaDefaults",
                   "Error freeing DAQ entry");
            return status;
        }
        current = next;
    }

    /* Free the XiaDefaults structure */
    handel_md_free(xiaDefaults);
    xiaDefaults = NULL;

    return status;
}

/*
 * Frees the memory allocated to an XiaDaqEntry structure.
 */
int HANDEL_API xiaFreeXiaDaqEntry(XiaDaqEntry* entry) {
    int status = XIA_SUCCESS;

    if (entry == NULL) {
        status = XIA_NOMEM;
        xiaLog(XIA_LOG_ERROR, status, "xiaFreeXiaDaqEntry",
               "XiaDaqEntry object unallocated:  can not free");
        return status;
    }

    if (entry->name != NULL) {
        handel_md_free(entry->name);
    }

    /* Free the XiaDaqEntry structure */
    handel_md_free(entry);
    entry = NULL;

    return status;
}

/*
 * Frees a previously allocated module and all if its subcomponents.
 *
 * Assumes module has been allocated. Does _not_ assume that all of
 * module's subcomponents have.
 */
int HANDEL_API xiaFreeModule(Module* module) {
    int status;

    unsigned int i;
    unsigned int j;
    unsigned int k;
    unsigned int numDetStr;
    unsigned int numFirmStr;
    unsigned int numDefStr;

    PSLFuncs f;

    if (module == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_VALUE, "xiaAddFirmwareItem", "module is NULL");
        return XIA_NULL_VALUE;
    }

    switch (module->interface_info->type) {
        default:
            /* Impossible */
            FAIL();
            break;
        case XIA_INTERFACE_NONE:
            /* Only free the top-level struct */
            ASSERT(module->interface_info != NULL);
            handel_md_free(module->interface_info);
            break;
        case XIA_PLX:
            handel_md_free(module->interface_info->info.plx);
            handel_md_free(module->interface_info);
            break;
        case XIA_EPP:
        case XIA_GENERIC_EPP:
            handel_md_free((void*) module->interface_info->info.epp);
            handel_md_free((void*) module->interface_info);
            break;
        case XIA_SERIAL:
            if (module->interface_info->info.serial->device_file)
                handel_md_free(module->interface_info->info.serial->device_file);
            handel_md_free((void*) module->interface_info->info.serial);
            handel_md_free((void*) module->interface_info);
            break;
        case XIA_USB:
            handel_md_free((void*) module->interface_info->info.usb);
            handel_md_free((void*) module->interface_info);
            break;
        case XIA_USB2:
            handel_md_free((void*) module->interface_info->info.usb2);
            handel_md_free((void*) module->interface_info);
            break;
    }
    module->interface_info = NULL;

    for (i = 0; i < module->number_of_channels; i++) {
        if (module->channels[i] != -1) {
            status = xiaRemoveDetChan((unsigned int) module->channels[i]);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaFreeModule",
                       "Error removing detChan member");
                /*
                 * Should this continue since we'll leak memory if we return
                 * prematurely?
                 */
                return status;
            }
        }
    }

    handel_md_free(module->channels);
    module->channels = NULL;

    if (module->detector != NULL) {
        numDetStr = module->number_of_channels;

        for (i = 0; i < numDetStr; i++) {
            handel_md_free((void*) module->detector[i]);
        }
    }

    handel_md_free((void*) module->detector);
    handel_md_free((void*) module->detector_chan);

    if (module->firmware != NULL) {
        numFirmStr = module->number_of_channels;

        for (j = 0; j < numFirmStr; j++) {
            handel_md_free((void*) module->firmware[j]);
        }
    }

    handel_md_free((void*) module->firmware);

    if (module->defaults != NULL) {
        numDefStr = module->number_of_channels;

        for (k = 0; k < numDefStr; k++) {
            /* Remove the defaults from the system */
            status = xiaRemoveDefault(module->defaults[k]);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaFreeModule",
                       "Error removing values associated with modChan %u", k);
                return status;
            }
            handel_md_free((void*) module->defaults[k]);
        }
    }

    handel_md_free((void*) module->defaults);
    handel_md_free((void*) module->currentFirmware);

    /* Free up any multichannel info that was allocated */
    if (module->isMultiChannel) {
        handel_md_free(module->state->runActive);
        handel_md_free(module->state);
    }

    /*
     * If the type isn't set, then there is no
     * chance that any of the type-specific data is set
     * like the SCA data.
     */
    if (module->type != NULL) {
        status = xiaLoadPSL(module->type, &f);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaFreeModule", "Error loading PSL for '%s'",
                   module->alias);
            return status;
        }

        if (module->ch != NULL) {
            for (i = 0; i < module->number_of_channels; i++) {
                status = f.freeSCAs(module, i);

                if (status != XIA_SUCCESS) {
                    xiaLog(XIA_LOG_ERROR, status, "xiaFreeModule",
                           "Error removing SCAs from modChan '%u', alias '%s'", i,
                           module->alias);
                    return status;
                }
            }
            handel_md_free(module->ch);
        }

        handel_md_free(module->type);
        module->type = NULL;
    }

    handel_md_free(module->alias);
    module->alias = NULL;

    /*
     * This (freeing the module) was previously absent, which I think was causing a
     * small (16-bit) memory leak.
     */
    handel_md_free(module);
    module = NULL;

    /*
     * XXX If this is the last module, i.e. the # of detChans besides "-1" and
     * any "SET"s is 0, then release the rest of the detChan list.
     */

    return XIA_SUCCESS;
}

/*
 * Initializes the DetChanElement data structure.
 */
static int HANDEL_API xiaInitDetChanDS(void) {
    DetChanElement* current = NULL;
    DetChanElement* head = xiaDetChanHead;

    current = head;

    while (current != NULL) {
        head = current;
        current = current->next;

        switch (head->type) {
            case SINGLE:
                handel_md_free((void*) head->data.modAlias);
                break;
            case SET:
                xiaFreeDetSet(head->data.detChanSet);
                break;
            default:
                break;
        }
        handel_md_free((void*) head);
    }

    xiaDetChanHead = NULL;

    return XIA_SUCCESS;
}

/*
 * Frees the Module data structure.
 */
static int HANDEL_API xiaInitModuleDS(void) {
    int status;

    Module* current = NULL;
    Module* head = xiaModuleHead;

    current = head;

    while (current != NULL) {
        head = current;
        current = current->next;

        status = xiaFreeModule(head);
        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaInitModuleDS", "Error freeing module(s)");
            return status;
        }
    }

    xiaModuleHead = NULL;

    return XIA_SUCCESS;
}

/*
 * Shuts down communication on each module.
 */
static int HANDEL_API xiaUnHook(void) {
    int status;

    char boardType[MAXITEM_LEN];

    DetChanElement* current = xiaDetChanHead;

    PSLFuncs localFuncs;

    while (current != NULL) {
        /*
         * Only do the single channels since sets
         * are made up of single channels and would
         * make the whole thing a little too redundant.
         */
        if (current->type == SINGLE) {
            status = xiaGetBoardType(current->detChan, boardType);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaUnHook",
                       "Unable to get boardType for detChan %d", current->detChan);
                return status;
            }

            status = xiaLoadPSL(boardType, &localFuncs);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaUnHook",
                       "Unable to load PSL functions for boardType %s", boardType);
                return status;
            }

            status = localFuncs.unHook(current->detChan);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaUnHook",
                       "Unable to close communications for boardType %s", boardType);
                return status;
            }
        }

        current = current->next;
    }

    return XIA_SUCCESS;
}
