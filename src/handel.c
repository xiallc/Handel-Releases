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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "xerxes.h"
#include "xerxes_errors.h"
#include "xerxes_structures.h"

#include "xia_handel.h"
#include "xia_module.h"
#include "xia_system.h"
#include "xia_common.h"
#include "xia_assert.h"
#include "xia_version.h"
#include "xia_handel_structures.h"
#include "xia_file.h"

#include "handel_generic.h"
#include "handel_errors.h"
#include "handel_log.h"
#include "handeldef.h"

#include "fdd.h"


HANDEL_STATIC int HANDEL_API xiaInitMemory(void);
HANDEL_STATIC int HANDEL_API xiaInitDetectorDS(void);
HANDEL_STATIC int HANDEL_API xiaInitFirmwareSetDS(void);
HANDEL_STATIC int HANDEL_API xiaInitXiaDefaultsDS(void);
HANDEL_STATIC int HANDEL_API xiaInitDetChanDS(void);
HANDEL_STATIC int HANDEL_API xiaInitModuleDS(void);

HANDEL_STATIC int HANDEL_API xiaUnHook(void);

char info_string[4096];

/*
 * Used to track if the library functions are initialized.
 */
boolean_t isHandelInit = FALSE_;


/*
 * Define the head of the Detector list
 */
Detector *xiaDetectorHead = NULL;

/*
 * Define the head of the Firmware Sets LL
 */
FirmwareSet *xiaFirmwareSetHead = NULL;

/*
 * Define the head of the XiaDefaults LL
 */
XiaDefaults *xiaDefaultsHead = NULL;

/*
 * Define the head of the Modules LL
 */
Module *xiaModuleHead = NULL;

/*
 * This is the head of the DetectorChannel LL
 */
DetChanElement *xiaDetChanHead = NULL;

HANDEL_EXPORT int HANDEL_API xiaInit(char *iniFile)
{
    int status;
    int nFilesOpen;

    /* We need to clear and re-initialize Handel */
    status = xiaInitHandel();

    if (status != XIA_SUCCESS) {
        xiaLogError("xiaInit", "Error reinitializing Handel", status);
        return status;
    }

    if (iniFile == NULL) {
        xiaLogError("xiaInit", ".INI file name must be non-NULL", XIA_BAD_NAME);
        return XIA_BAD_NAME;
    }

    /* Verify that we currently don't have any file handles open. This is
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
        sprintf(info_string, "Error reading in .INI file '%s'", iniFile);
        xiaLogError("xiaInit", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

HANDEL_EXPORT int HANDEL_API xiaInitHandel(void)
{
    int status = XIA_SUCCESS;

    /* Arbitrary length here */
    char version[120];

    if (!isHandelInit) {

        /* Make sure the everything is working on the Xerxes side of things.
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

        /* Make our function pointers equal to XerXes function pointers using the
         * imported utils variable
         */
        handel_md_log           = utils->funcs->dxp_md_log;
        handel_md_output        = utils->funcs->dxp_md_output;
        handel_md_enable_log    = utils->funcs->dxp_md_enable_log;
        handel_md_suppress_log  = utils->funcs->dxp_md_suppress_log;
        handel_md_set_log_level = utils->funcs->dxp_md_set_log_level;

        handel_md_alloc         = utils->funcs->dxp_md_alloc;
        handel_md_free          = utils->funcs->dxp_md_free;

        handel_md_puts          = utils->funcs->dxp_md_puts;
        handel_md_wait          = utils->funcs->dxp_md_wait;
        handel_md_fgets         = utils->funcs->dxp_md_fgets;

        /* Init the FDD lib here */
        status = xiaFddInitialize();

        if (status != XIA_SUCCESS) {
            xiaLogError("xiaInitHandel", "Error initializing FDD layer", status);
            return status;
        }

        isHandelInit = TRUE_;

    }
    else {

        /*
         * Most user will be calling xiaInit after xiaInitHandel has already
         * executed from xiaSetLogLevel. To be safe the connection is always
         * re-initialized.
         */
        xiaLogInfo("xiaInitHandel", "Closing off existing connections.");
        status = xiaUnHook();
    }

    xiaLogInfo("xiaInitHandel", "Starting Handel");

    /* Initialize the memory of both Handel and Xerxes.
     */
    status = xiaInitMemory();

    if (status != XIA_SUCCESS) {
        xiaLogError("xiaInitHandel", "Unable to Initialize memory", status);
        return status;
    }

    xiaGetVersionInfo(NULL, NULL, NULL, version);
    sprintf(info_string, "Successfully initialized Handel %s", version);
    xiaLogInfo("xiaInitHandel", info_string);

    /* Print out build configuration */
    xiaLogInfo("xiaInitHandel", "--- Supported interface ---");

#ifndef EXCLUDE_EPP
    xiaLogInfo("xiaInitHandel", "epp");
#endif /* EXCLUDE_EPP */

#ifndef EXCLUDE_USB
    xiaLogInfo("xiaInitHandel", "usb");
#endif /* EXCLUDE_USB */

#ifndef EXCLUDE_USB2
    xiaLogInfo("xiaInitHandel", "usb2");
#endif /* EXCLUDE_USB2 */

#ifndef EXCLUDE_SERIAL
    xiaLogInfo("xiaInitHandel", "serial");
#endif /* EXCLUDE_SERIAL */

#ifndef EXCLUDE_PLX
    xiaLogInfo("xiaInitHandel", "plx");
#endif /* EXCLUDE_PLX */

    xiaLogInfo("xiaInitHandel", "--- Supported board types ---");

#ifndef EXCLUDE_SATURN
    xiaLogInfo("xiaInitHandel", "saturn");
#endif

#ifndef EXCLUDE_UDXPS
    xiaLogInfo("xiaInitHandel", "udxps");
#endif

#ifndef EXCLUDE_UDXP
    xiaLogInfo("xiaInitHandel", "udxp");
#endif

#ifndef EXCLUDE_XMAP
    xiaLogInfo("xiaInitHandel", "xmap");
#endif /* EXCLUDE_XMAP */

#ifndef EXCLUDE_STJ
    xiaLogInfo("xiaInitHandel", "stj");
#endif /* EXCLUDE_STJ */

#ifndef EXCLUDE_MERCURY
    xiaLogInfo("xiaInitHandel", "mercury");
#endif /* EXCLUDE_MERCURY */

    return status;
}


/*
 * This routine initializes the data structures.
 */
HANDEL_STATIC int HANDEL_API xiaInitMemory(void)
{
    int status = XIA_SUCCESS;

    xiaLogInfo("xiaInitMemory", "Initializing Handel data structure.");

    status = xiaInitDetectorDS();
    if(status != XIA_SUCCESS) {
        xiaLogError("xiaInitMemory", "Unable to clear the Detector LL", status);
        return status;
    }

    status = xiaInitFirmwareSetDS();
    if(status != XIA_SUCCESS) {
        xiaLogError("xiaInitMemory", "Unable to clear the FirmwareSet LL", status);
        return status;
    }

    status = xiaInitModuleDS();
    if (status != XIA_SUCCESS) {
        xiaLogError("xiaInitMemory", "Unable to clear Module LL", status);
        return status;
    }

    status = xiaInitDetChanDS();
    if (status != XIA_SUCCESS) {
        xiaLogError("xiaInitMemory", "Unable to clear DetChan LL", status);
        return status;
    }

    status = xiaInitXiaDefaultsDS();
    if (status != XIA_SUCCESS) {
        xiaLogError("xiaInitMemory", "Unable to clear Defaults LL", status);
        return status;
    }

    return status;
}


/*
 * This routines disconnects from the hardware and cleans Handel's
 * internal data structures.
 */
HANDEL_EXPORT int HANDEL_API xiaExit(void)
{
    int status;
    xiaLogInfo("xiaExit", "Exiting...");

    /* Close down any communications that need to be shutdown.
     */
    status = xiaUnHook();

    if (status != XIA_SUCCESS) {
        xiaLogError("xiaExit", "Error shutting down communications", status);
        /* Ignore this status since we're on the way out */
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
HANDEL_EXPORT void HANDEL_API xiaGetVersionInfo(int *rel, int *min, int *maj,
                                                char *pretty)
{
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
HANDEL_STATIC int HANDEL_API xiaInitDetectorDS(void)
{
    int status = XIA_SUCCESS;

    Detector *next    = NULL;
    Detector *current = xiaDetectorHead;

    /* Search thru the Detector LL and clear them out */
    while ((current != NULL) && (status == XIA_SUCCESS)) {
        next = current->next;

        status = xiaFreeDetector(current);
        if (status != XIA_SUCCESS) {
            xiaLogError("xiaInitDetectorDS", "Error freeing detector", status);
            return status;
        }

        current = next;
    }

    xiaDetectorHead = NULL;

    return status;
}

/*
 * Frees the memory allocated to a Detector structure.
 */
HANDEL_SHARED int HANDEL_API xiaFreeDetector(Detector *detector)
/* Detector *detector;          Input: pointer to structure to free */
{
    int status = XIA_SUCCESS;

    if (detector == NULL) {
        status = XIA_NOMEM;
        sprintf(info_string,"Detector object unallocated:  can not free");
        xiaLogError("xiaFreeDetector", info_string, status);
        return status;
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
HANDEL_STATIC int HANDEL_API xiaInitFirmwareSetDS(void)
{
    int status = XIA_SUCCESS;

    FirmwareSet *next;
    FirmwareSet *current = xiaFirmwareSetHead;

    /* Search thru the FirmwareSet LL and clear them out */
    while ((current != NULL) && (status == XIA_SUCCESS)) {
        next = current->next;

        status = xiaFreeFirmwareSet(current);
        if (status != XIA_SUCCESS) {
            xiaLogError("xiaInitFirmwareSetDS", "Error freeing FirmwareSet", status);
            return status;
        }

        current = next;
    }

    xiaFirmwareSetHead = NULL;

    return status;
}

/*
 * Frees the memory allocated to a FirmwareSet structure.
 */
HANDEL_SHARED int HANDEL_API xiaFreeFirmwareSet(FirmwareSet *firmwareSet)
/* FirmwareSet *firmwareSet;        Input: pointer to structure to free */
{
    int status = XIA_SUCCESS;

    unsigned int i;

    Firmware *next, *current;

    if (firmwareSet == NULL) {
        status = XIA_NOMEM;
        sprintf(info_string,"FirmwareSet object unallocated:  can not free");
        xiaLogError("xiaFreeFirmwareSet", info_string, status);
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
            xiaLogError("xiaFreeFirmwareSet", "Error freeing firmware", status);
            return status;
        }

        current = next;
    }

    for (i = 0; i < firmwareSet->numKeywords; i++) {
        handel_md_free((void *)firmwareSet->keywords[i]);
    }

    handel_md_free((void *)firmwareSet->keywords);

    /* Free the FirmwareSet structure */
    handel_md_free(firmwareSet);
    firmwareSet = NULL;

    return status;
}

/*
 * Frees the memory allocated to a Firmware structure.
 */
HANDEL_SHARED int HANDEL_API xiaFreeFirmware(Firmware *firmware)
/* Firmware *firmware;        Input: pointer to structure to free */
{
    int status = XIA_SUCCESS;

    if (firmware == NULL) {
        status = XIA_NOMEM;
        sprintf(info_string,"Firmware object unallocated:  can not free");
        xiaLogError("xiaFreeFirmware", info_string, status);
        return status;
    }

    if (firmware->dsp != NULL) {
        handel_md_free((void *)firmware->dsp);
    }

    if (firmware->fippi != NULL) {
        handel_md_free((void *)firmware->fippi);
    }

    if (firmware->user_fippi != NULL) {
        handel_md_free((void *)firmware->user_fippi);
    }

    if (firmware->user_dsp != NULL) {
        handel_md_free((void *)firmware->user_dsp);
    }

    if (firmware->system_fpga != NULL) {
        handel_md_free((void *)firmware->system_fpga);
    }

    if (firmware->filterInfo != NULL) {
        handel_md_free((void *)firmware->filterInfo);
    }

    handel_md_free((void *)firmware);
    firmware = NULL;

    return status;
}


/*
 * Initializes the XiaDefaults structure.
 */
HANDEL_STATIC int HANDEL_API xiaInitXiaDefaultsDS(void)
{
    int status = XIA_SUCCESS;

    XiaDefaults *next;
    XiaDefaults *current = xiaDefaultsHead;

    /* Search thru the XiaDefaults LL and clear them out */
    while ((current != NULL) && (status == XIA_SUCCESS)) {
        next = current->next;

        status = xiaFreeXiaDefaults(current);
        if (status != XIA_SUCCESS) {
            xiaLogError("xiaInitXiaDefaultDS", "Error freeing default", status);
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
HANDEL_SHARED int HANDEL_API xiaFreeXiaDefaults(XiaDefaults *xiaDefaults)
{
    int status = XIA_SUCCESS;

    XiaDaqEntry *next = NULL;
    XiaDaqEntry *current = NULL;

    if (xiaDefaults == NULL) {
        status = XIA_NOMEM;
        sprintf(info_string,"XiaDefaults object unallocated:  can not free");
        xiaLogError("xiaFreeXiaDefaults", info_string, status);
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
            xiaLogError("xiaFreeXiaDefaults", "Error freeing DAQ entry", status);
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
HANDEL_SHARED int HANDEL_API xiaFreeXiaDaqEntry(XiaDaqEntry *entry)
{
    int status = XIA_SUCCESS;

    if (entry == NULL) {
        status = XIA_NOMEM;
        sprintf(info_string,"XiaDaqEntry object unallocated:  can not free");
        xiaLogError("xiaFreeXiaDaqEntry", info_string, status);
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
 * modules's subcomponents have.
 */
HANDEL_SHARED int HANDEL_API xiaFreeModule(Module *module)
{
    int status;

    unsigned int i;
    unsigned int j;
    unsigned int k;
    unsigned int numDetStr;
    unsigned int numFirmStr;
    unsigned int numDefStr;

    PSLFuncs f;


    ASSERT(module != NULL);


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
        handel_md_free((void *)module->interface_info->info.epp);
        handel_md_free((void *)module->interface_info);
        break;

    case XIA_SERIAL:
        if (module->interface_info->info.serial->device_file)
            handel_md_free(module->interface_info->info.serial->device_file);
        handel_md_free((void *)module->interface_info->info.serial);
        handel_md_free((void *)module->interface_info);
        break;

    case XIA_USB:
        handel_md_free((void *)module->interface_info->info.usb);
        handel_md_free((void *)module->interface_info);
        break;

    case XIA_USB2:
        handel_md_free((void *)module->interface_info->info.usb2);
        handel_md_free((void *)module->interface_info);
        break;

    }
    module->interface_info = NULL;

    for (i = 0; i < module->number_of_channels; i++) {
        if (module->channels[i] != -1) {
            status = xiaRemoveDetChan((unsigned int)module->channels[i]);

            if (status != XIA_SUCCESS) {
                sprintf(info_string, "Error removing detChan member");
                xiaLogError("xiaFreeModule", info_string, status);
                /* Should this continue since we'll leak memory if we return
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
            handel_md_free((void *)module->detector[i]);
        }
    }

    handel_md_free((void *)module->detector);
    handel_md_free((void *)module->detector_chan);

    if (module->firmware != NULL) {
        numFirmStr = module->number_of_channels;

        for (j = 0; j < numFirmStr; j++) {
            handel_md_free((void *)module->firmware[j]);
        }
    }

    handel_md_free((void *)module->firmware);

    if (module->defaults != NULL) {
        numDefStr = module->number_of_channels;

        for (k = 0; k < numDefStr; k++) {
            /* Remove the defaults from the system */
            status = xiaRemoveDefault(module->defaults[k]);

            if (status != XIA_SUCCESS) {

                sprintf(info_string, "Error removing values associated with modChan %u", k);
                xiaLogError("xiaFreeModule", info_string, status);
                return status;
            }

            handel_md_free((void *)module->defaults[k]);
        }
    }

    handel_md_free((void *)module->defaults);
    handel_md_free((void *)module->currentFirmware);

    /* Free up any multichannel info that was allocated */
    if (module->isMultiChannel) {
        handel_md_free(module->state->runActive);
        handel_md_free(module->state);
    }

    /* If the type isn't set, then there is no
     * chance that any of the type-specific data is set
     * like the SCA data.
     */
    if (module->type != NULL) {
        status = xiaLoadPSL(module->type, &f);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error loading PSL for '%s'", module->alias);
            xiaLogError("xiaFreeModule", info_string, status);
            return status;
        }

        if (module->ch != NULL) {
            for (i = 0; i < module->number_of_channels; i++) {
                status = f.freeSCAs(module, i);

                if (status != XIA_SUCCESS) {
                    sprintf(info_string, "Error removing SCAs from modChan '%u', alias '%s'",
                            i, module->alias);
                    xiaLogError("xiaFreeModule", info_string, status);
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



    /* This (freeing the module) was previously absent, which I think was causing a
     * small (16-bit) memory leak.
     */
    handel_md_free(module);
    module = NULL;

    /* XXX If this is the last module, i.e. the # of detChans besides "-1" and
     * any "SET"s is 0, then release the rest of the detChan list.
     */

    return XIA_SUCCESS;
}

/*
 * Initializes the DetChanElement data structure.
 */
HANDEL_STATIC int HANDEL_API xiaInitDetChanDS(void)
{
    DetChanElement *current = NULL;
    DetChanElement *head    = xiaDetChanHead;

    current = head;

    while (current != NULL) {
        head = current;
        current = current->next;

        switch (head->type) {
        case SINGLE:
            handel_md_free((void *)head->data.modAlias);
            break;

        case SET:
            xiaFreeDetSet(head->data.detChanSet);
            break;

        default:
            break;
        }

        handel_md_free((void *)head);
    }

    xiaDetChanHead = NULL;

    return XIA_SUCCESS;
}


/*
 * Frees the Module data structure.
 */
HANDEL_STATIC int HANDEL_API xiaInitModuleDS(void)
{
    int status;

    Module *current = NULL;
    Module *head    = xiaModuleHead;

    current = head;

    while (current != NULL) {
        head = current;
        current = current->next;

        status = xiaFreeModule(head);
        if (status != XIA_SUCCESS) {
            xiaLogError("xiaInitModuleDS", "Error freeing module(s)", status);
            return status;
        }
    }

    xiaModuleHead = NULL;

    return XIA_SUCCESS;
}


/*
 * Shuts down communication on each module.
 */
HANDEL_STATIC int HANDEL_API xiaUnHook(void)
{
    int status;

    char boardType[MAXITEM_LEN];

    DetChanElement *current = xiaDetChanHead;

    PSLFuncs localFuncs;


    while (current != NULL) {
        /* Only do the single channels since sets
         * are made up of single channels and would
         * make the whole thing a little too redundant.
         */
        if (current->type == SINGLE) {
            status = xiaGetBoardType(current->detChan, boardType);

            if (status != XIA_SUCCESS) {
                sprintf(info_string, "Unable to get boardType for detChan %d",
                        current->detChan);
                xiaLogError("xiaUnHook", info_string, status);
                return status;
            }

            status = xiaLoadPSL(boardType, &localFuncs);

            if (status != XIA_SUCCESS) {
                sprintf(info_string, "Unable to load PSL functions for boardType %s",
                        boardType);
                xiaLogError("xiaUnHook", info_string, status);
                return status;
            }

            status = localFuncs.unHook(current->detChan);

            if (status != XIA_SUCCESS) {
                sprintf(info_string, "Unable to close communications for boardType %s",
                        boardType);
                xiaLogError("xiaUnHook", info_string, status);
                return status;
            }
        }

        current = current->next;
    }

    return XIA_SUCCESS;
}
