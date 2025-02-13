/* Windows platform-dependent layer */

/*
 * Copyright (c) 2002-2004 X-ray Instrumentation Associates
 * Copyright (c) 2005-2015 XIA LLC
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

/*
 * This file supports both native Win32 builds using MS Visual Studio and
 * Cygwin.
 */
#include <windows.h>

#ifdef CYGWIN32
#undef _WIN32
#endif /* CYGWIN32 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <ctype.h>

#include "xerxes_errors.h"
#include "xerxes_structures.h"
#include "md_win32.h"
#include "md_generic.h"
#include "xia_md.h"
#include "xia_common.h"
#include "xia_assert.h"

/*
 * Just make this a local global so that each routine that wants to write an
 * error message doesn't have to define a new variable.
 */
static char ERROR_STRING[XIA_LINE_LEN];

static unsigned int MAXBLK = 0;

/* Globals that don't depend on the communication protocol. */
static char* TMP_PATH = NULL;
static char* PATH_SEP = "\\";

/* The total # of hardware devices currently opened. */
static unsigned int numMod = 0;

#ifndef EXCLUDE_EPP
/*
 * The id variable stores an optional ID number associated with each module
 * (initially included for handling multiple EPP modules hanging off the same
 * EPP address)
 */
static int eppID[MAXMOD];
/* variables to store the IO channel information */
static char* eppName[MAXMOD];
/* Port stores the port number for each module, only used for the X10P/G200 */
static unsigned short port;
static unsigned short next_addr = 0;
/* Store the currentID used for Daisy Chain systems */
static int currentID = -1;
static unsigned int numEPP = 0;
#endif /* EXCLUDE_EPP */

#ifndef EXCLUDE_USB
/* variables to store the IO channel information */
static char* usbName[MAXMOD];
static long usb_addr = 0;
static unsigned int numUSB = 0;
#endif /* EXCLUDE_USB IO */

#ifndef EXCLUDE_USB2
#include "xia_usb2.h"
#include "xia_usb2_errors.h"

/* A string holding the device number in it. */
static char* usb2Names[MAXMOD];
/* The OS handle used for communication. */
static HANDLE usb2Handles[MAXMOD];
/* The cached target address for the next operation. */
static unsigned long usb2AddrCache[MAXMOD];
static unsigned int numUSB2 = 0;
#endif /* EXCLUDE_USB2 */

#ifndef EXCLUDE_SERIAL
#include "seriallib.h"

#define HEADER_SIZE 4

/* variables to store the IO channel information */
static char* serialName[MAXMOD];
static unsigned int numSerial = 0;

/* Serial port globals */
static int dxp_md_serial_read_header(unsigned short port_, unsigned short* bytes,
                                     unsigned short* buf);
static int dxp_md_serial_read_data(unsigned short port_, unsigned long baud,
                                   unsigned long size, unsigned short* buf);
static int dxp_md_serial_reset(unsigned short port_, unsigned long baud);
#endif /* EXCLUDE_SERIAL */

#ifndef EXCLUDE_PLX

#ifdef _WIN32
/* The PLX headers pulled in from plxlibapi.h include wtypes.h manually which
 * gets around WIN32_LEAN_AND_MEAN's exclusion of rpc.h, leading to the C4115
 * warning on VS builds, similarly the C4201 warning nonstandard extension
 * needs to be suppressed too
 */
#pragma warning(disable : 4115)
#pragma warning(disable : 4201)
#endif /* _WIN32 */

#include "plxlibapi.h"

#ifdef _WIN32
#pragma warning(default : 4201)
#pragma warning(default : 4115)
#endif /* _WIN32 */

#include "plxlib_errors.h"

static HANDLE pxiHandles[MAXMOD];
static char* pxiNames[MAXMOD];
static unsigned int numPLX = 0;
#endif /* EXCLUDE_PLX */

/*
 * Routine to create pointers to the MD utility routines
 *
 * Xia_Util_Functions *funcs;
 * char *type;
 */
XIA_MD_EXPORT int XIA_MD_API dxp_md_init_util(Xia_Util_Functions* funcs, char* type) {
    /*
     * Check to see if we are initializing the library with this call in addition
     * to assigning function pointers.
     */

    if (type != NULL) {
        if (STREQ(type, "INIT_LIBRARY")) {
            numMod = 0;
        }
    }

    funcs->dxp_md_alloc = dxp_md_alloc;
    funcs->dxp_md_free = dxp_md_free;
    funcs->dxp_md_puts = dxp_md_puts;
    funcs->dxp_md_wait = dxp_md_wait;

    funcs->dxp_md_error = dxp_md_error;
    funcs->dxp_md_warning = dxp_md_warning;
    funcs->dxp_md_info = dxp_md_info;
    funcs->dxp_md_debug = dxp_md_debug;
    funcs->dxp_md_output = dxp_md_output;
    funcs->dxp_md_suppress_log = dxp_md_suppress_log;
    funcs->dxp_md_enable_log = dxp_md_enable_log;
    funcs->dxp_md_set_log_level = dxp_md_set_log_level;
    funcs->dxp_md_log = dxp_md_log;
    funcs->dxp_md_set_priority = dxp_md_set_priority;
    funcs->dxp_md_fgets = dxp_md_fgets;
    funcs->dxp_md_tmp_path = dxp_md_tmp_path;
    funcs->dxp_md_clear_tmp = dxp_md_clear_tmp;
    funcs->dxp_md_path_separator = dxp_md_path_separator;

    md_md_alloc = dxp_md_alloc;
    md_md_free = dxp_md_free;

    return DXP_SUCCESS;
}

/*
 * Routine to create pointers to the MD utility routines
 *
 * Xia_Io_Functions *funcs;
 * char *type;
 */
XIA_MD_EXPORT int XIA_MD_API dxp_md_init_io(Xia_Io_Functions* funcs, char* type) {
    unsigned int i;

    for (i = 0; i < strlen(type); i++) {
        type[i] = (char) tolower((int) type[i]);
    }

#ifndef EXCLUDE_EPP
    if (STREQ(type, "epp")) {
        funcs->dxp_md_io = dxp_md_epp_io;
        funcs->dxp_md_initialize = dxp_md_epp_initialize;
        funcs->dxp_md_open = dxp_md_epp_open;
        funcs->dxp_md_close = dxp_md_epp_close;
    }
#endif /* EXCLUDE_EPP */

#ifndef EXCLUDE_USB
    if (STREQ(type, "usb")) {
        funcs->dxp_md_io = dxp_md_usb_io;
        funcs->dxp_md_initialize = dxp_md_usb_initialize;
        funcs->dxp_md_open = dxp_md_usb_open;
        funcs->dxp_md_close = dxp_md_usb_close;
    }
#endif /* EXCLUDE_USB */

#ifndef EXCLUDE_SERIAL
    if (STREQ(type, "serial")) {
        funcs->dxp_md_io = dxp_md_serial_io;
        funcs->dxp_md_initialize = dxp_md_serial_initialize;
        funcs->dxp_md_open = dxp_md_serial_open;
        funcs->dxp_md_close = dxp_md_serial_close;
    }
#endif /* EXCLUDE_SERIAL */

#ifndef EXCLUDE_PLX
    /*
     * Technically, the communications protocol is 'PXI', though the
     * driver is a PLX driver, which is why there are two different names
     * used here.
     */
    if (STREQ(type, "pxi")) {
        funcs->dxp_md_io = dxp_md_plx_io;
        funcs->dxp_md_initialize = dxp_md_plx_initialize;
        funcs->dxp_md_open = dxp_md_plx_open;
        funcs->dxp_md_close = dxp_md_plx_close;
    }
#endif /* EXCLUDE_PLX */

#ifndef EXCLUDE_USB2
    if (STREQ(type, "usb2")) {
        funcs->dxp_md_io = dxp_md_usb2_io;
        funcs->dxp_md_initialize = dxp_md_usb2_initialize;
        funcs->dxp_md_open = dxp_md_usb2_open;
        funcs->dxp_md_close = dxp_md_usb2_close;
    }
#endif /* EXCLUDE_USB2 */

    funcs->dxp_md_get_maxblk = dxp_md_get_maxblk;
    funcs->dxp_md_set_maxblk = dxp_md_set_maxblk;

    return DXP_SUCCESS;
}

#ifndef EXCLUDE_EPP
/*
 * Initialize the system.
 *
 * unsigned int *maxMod; Input: maximum number of dxp modules allowed
 * char *dllname; Input: name of the DLL
 */
XIA_MD_STATIC int XIA_MD_API dxp_md_epp_initialize(unsigned int* maxMod,
                                                   char* dllname) {
    int status = DXP_SUCCESS;
    int rstat = 0;

    /* EPP initialization */

    /* check if all the memory was allocated */
    if (*maxMod > MAXMOD) {
        status = DXP_NOMEM;
        sprintf(ERROR_STRING,
                "Calling routine requests %u maximum modules: only "
                "%u available.",
                *maxMod, MAXMOD);
        dxp_md_log_error("dxp_md_epp_initialize", ERROR_STRING, status);
        return status;
    }

    /* Zero out the number of modules currently in the system */
    numEPP = 0;

    /* Initialize the EPP port */
    rstat = sscanf(dllname, "%hx", &port);
    if (rstat != 1) {
        status = DXP_BAD_IONAME;
        dxp_md_log_error("dxp_md_epp_initialize", "Unable to read the EPP port address",
                         status);
        return status;
    }

    sprintf(ERROR_STRING, "EPP Port = %#hx", port);
    dxp_md_log_debug("dxp_md_epp_initialize", ERROR_STRING);

    /* Reset the currentID when the EPP interface is initialized */
    currentID = -1;

    return status;
}

/*
 * Routine is passed the user defined configuration string *name. This string
 * contains all the information needed to point to the proper IO channel by
 * future calls to dxp_md_io().
 *
 * char *ioname;	Input:  string used to specify this IO channel
 * int *camChan;	Output: returns a reference number for this module
 */
XIA_MD_STATIC int XIA_MD_API dxp_md_epp_open(char* ioname, int* camChan) {
    unsigned int i;
    int status = DXP_SUCCESS;
    int rstat = 0;

    sprintf(ERROR_STRING, "ioname = %s", ioname);
    dxp_md_log_debug("dxp_md_epp_open", ERROR_STRING);

    /*
     * First loop over the existing names to make sure this module
     * was not already configured?  Don't want/need to perform
     * this operation 2 times.
     */
    for (i = 0; i < numEPP; i++) {
        if (STREQ(eppName[i], ioname)) {
            status = DXP_SUCCESS;
            *camChan = i;
            return status;
        }
    }

    /*
     * Got a new one. Increase the number existing and assign the global information
     */
    if (eppName[numEPP] != NULL) {
        md_md_free(eppName[numEPP]);
    }
    eppName[numEPP] = (char*) md_md_alloc((strlen(ioname) + 1) * sizeof(char));

    strcpy(eppName[numEPP], ioname);

    /* See if this is a multi-module EPP chain, if not set its ID to -1 */
    if (ioname[0] == ':') {
        sscanf(ioname, ":%d", &(eppID[numEPP]));

        sprintf(ERROR_STRING, "ID = %d", eppID[numEPP]);
        dxp_md_log_debug("dxp_md_epp_open", ERROR_STRING);

        /* Initialize the port address first */
        rstat = DxpInitPortAddress((int) port);
        if (rstat != 0) {
            status = DXP_OPEN_EPP;
            sprintf(ERROR_STRING,
                    "Unable to initialize the EPP port address: "
                    "port = %#hx",
                    port);
            dxp_md_log_error("dxp_md_epp_open", ERROR_STRING, status);
            return status;
        }

        /* Call setID now to set up the port for Initialization */
        DxpSetID((unsigned short) eppID[numEPP]);
    } else {
        eppID[numEPP] = -1;
    }

    /* Call the EPPLIB.DLL routine */
    rstat = DxpInitEPP((int) port);

    /* Check for Success */
    if (rstat == 0) {
        status = DXP_SUCCESS;
    } else {
        status = DXP_OPEN_EPP;
        sprintf(ERROR_STRING, "Unable to initialize the EPP port: rstat = %d", rstat);
        dxp_md_log_error("dxp_md_epp_open", ERROR_STRING, status);
        return status;
    }

    *camChan = numEPP++;
    numMod++;

    return status;
}

/*
 * This routine performs the IO call to read or write data.  The pointer to
 * the desired IO channel is passed as camChan.  The address to write to is
 * specified by function and address.  The data length is specified by
 * length.  And the data itself is stored in data.
 *
 * int *camChan;				Input: pointer to IO channel to access
 * unsigned int *function;			Input: XIA EPP function definition
 * unsigned int *address;			Input: XIA EPP address definition
 * unsigned short *data;			I/O:  data read or written
 * unsigned int *length;			Input: how much data to read or write
 */
XIA_MD_STATIC int XIA_MD_API dxp_md_epp_io(int* camChan, unsigned int* function,
                                           unsigned long* address, void* data,
                                           unsigned int* length) {
    int rstat = 0;
    int status;

    int i;

    unsigned short* us_data = (unsigned short*) data;

    int ullength = (int) *length / 2;
    unsigned long* temp = NULL;

    if ((currentID != eppID[*camChan]) && (eppID[*camChan] != -1)) {
        DxpSetID((unsigned short) eppID[*camChan]);

        /* Update the currentID */
        currentID = eppID[*camChan];

        sprintf(ERROR_STRING, "calling SetID = %d, camChan = %d", eppID[*camChan],
                *camChan);
        dxp_md_log_debug("dxp_md_epp_io", ERROR_STRING);
    }

    /* Data*/
    if (*address == 0) {
        /* Perform short reads and writes if not in program address space */
        if (next_addr >= 0x4000) {
            if (*length > 1) {
                if (*function == MD_IO_READ) {
                    rstat = DxpReadBlock(next_addr, us_data, (int) *length);
                } else {
                    rstat = DxpWriteBlock(next_addr, us_data, (int) *length);
                }
            } else {
                if (*function == MD_IO_READ) {
                    rstat = DxpReadWord(next_addr, us_data);
                } else {
                    rstat = DxpWriteWord(next_addr, us_data[0]);
                }
            }
        } else {
            /* Perform long reads and writes if in program address space (24-bit) */
            /* Allocate memory */
            temp = (unsigned long*) dxp_md_alloc(sizeof(unsigned long) * ullength);

            if (!temp) {
                sprintf(ERROR_STRING, "Unable to allocate %zu bytes for temp",
                        sizeof(unsigned long) * ullength);
                dxp_md_log_error("dxp_md_epp_io", ERROR_STRING, DXP_NOMEM);
                return DXP_NOMEM;
            }

            if (*function == MD_IO_READ) {
                rstat = DxpReadBlocklong(next_addr, temp, ullength);
                /* reverse the byte order for the EPPLIB library */
                for (i = 0; i < ullength; i++) {
                    us_data[2 * i] = (unsigned short) (temp[i] & 0xFFFF);
                    us_data[2 * i + 1] = (unsigned short) ((temp[i] >> 16) & 0xFFFF);
                }
            } else {
                /* reverse the byte order for the EPPLIB library */
                for (i = 0; i < ullength; i++) {
                    temp[i] = ((us_data[2 * i] << 16) + us_data[2 * i + 1]);
                }
                rstat = DxpWriteBlocklong(next_addr, temp, ullength);
            }
            /* Free the memory */
            md_md_free(temp);
        }
        /* Address port*/
    } else if (*address == 1) {
        next_addr = (unsigned short) *us_data;
        /* Control port*/
    } else if (*address == 2) {
        /* Status port*/
    } else if (*address == 3) {
        /* do nothing */
    } else {
        sprintf(ERROR_STRING, "Unknown EPP address = %#lx", *address);
        status = DXP_MDIO;
        dxp_md_log_error("dxp_md_epp_io", ERROR_STRING, status);
        return status;
    }

    if (rstat != 0) {
        status = DXP_MDIO;
        sprintf(ERROR_STRING,
                "Problem Performing I/O to Function: %#x, address: "
                "%#lx",
                *function, *address);
        dxp_md_log_error("dxp_md_epp_io", ERROR_STRING, status);
        sprintf(ERROR_STRING, "Trying to write to internal address: %#hx, length %u",
                next_addr, *length);
        dxp_md_log_error("dxp_md_epp_io", ERROR_STRING, status);
        return status;
    }

    status = DXP_SUCCESS;

    return status;
}

/*
 * "Closes" the EPP connection, which means that it does nothing.
 */
XIA_MD_STATIC int XIA_MD_API dxp_md_epp_close(int* camChan) {
    UNUSED(camChan);
    return DXP_SUCCESS;
}

#endif /* EXCLUDE_EPP */

#ifndef EXCLUDE_USB
/*
 * Initialize the USB system.
 *
 * unsigned int *maxMod;					Input: maximum number of dxp modules allowed
 * char *dllname;							Input: name of the DLL
 */
XIA_MD_STATIC int XIA_MD_API dxp_md_usb_initialize(unsigned int* maxMod,
                                                   char* dllname) {
    int status = DXP_SUCCESS;

    UNUSED(dllname);

    /* USB initialization */

    /* check if all the memory was allocated */
    if (*maxMod > MAXMOD) {
        status = DXP_NOMEM;
        sprintf(ERROR_STRING,
                "Calling routine requests %u maximum modules: "
                "only %u available.",
                *maxMod, MAXMOD);
        dxp_md_log_error("dxp_md_usb_initialize", ERROR_STRING, status);
        return status;
    }

    /* Zero out the number of modules currently in the system */
    numUSB = 0;

    return status;
}

/*
 * Routine is passed the user defined configuration string *name.  This string
 * contains all the information needed to point to the proper IO channel by
 * future calls to dxp_md_io().
 */
XIA_MD_STATIC int XIA_MD_API dxp_md_usb_open(char* ioname, int* camChan) {
    int status = DXP_SUCCESS;
    unsigned int i;

    /* Temporary name so that we can get the length */
    char tempName[200];

    sprintf(ERROR_STRING, "ioname = %s", ioname);
    dxp_md_log_debug("dxp_md_usb_open", ERROR_STRING);

    /*
     * First loop over the existing names to make sure this module
     * was not already configured?  Don't want/need to perform
     * this operation 2 times.
     */
    for (i = 0; i < numUSB; i++) {
        if (STREQ(usbName[i], ioname)) {
            status = DXP_SUCCESS;
            *camChan = i;
            return status;
        }
    }

    /* Got a new one. Increase the number existing and assign the global information */
    sprintf(tempName, "\\\\.\\ezusb-%s", ioname);

    if (usbName[numUSB] != NULL) {
        md_md_free(usbName[numUSB]);
    }
    usbName[numUSB] = (char*) md_md_alloc((strlen(tempName) + 1) * sizeof(char));

    strcpy(usbName[numUSB], tempName);

    *camChan = numUSB++;
    numMod++;

    return status;
}

/*
 * This routine performs the IO call to read or write data.  The pointer to
 * the desired IO channel is passed as camChan.  The address to write to is
 * specified by function and address.  The data length is specified by
 * length.  And the data itself is stored in data.
 */
XIA_MD_STATIC int XIA_MD_API dxp_md_usb_io(int* camChan, unsigned int* function,
                                           unsigned long* address, void* data,
                                           unsigned int* length) {
    int rstat = 0;
    int status;

    unsigned short* us_data = (unsigned short*) data;

    if (*address == 0) {
        if (*function == MD_IO_READ) {
            rstat = xia_usb_read(usb_addr, (long) *length, usbName[*camChan], us_data);
        } else {
            rstat = xia_usb_write(usb_addr, (long) *length, usbName[*camChan], us_data);
        }
    } else if (*address == 1) {
        usb_addr = (long) us_data[0];
    }

    if (rstat != 0) {
        status = DXP_MDIO;
        sprintf(ERROR_STRING,
                "Problem Performing USB I/O to Function: %u, "
                "address: %#lx, Driver reported status %d",
                *function, *address, rstat);
        dxp_md_log_error("dxp_md_usb_io", ERROR_STRING, status);
        sprintf(ERROR_STRING,
                "Trying to write to internal address: %ld, "
                "length %u",
                usb_addr, *length);
        dxp_md_log_error("dxp_md_usb_io", ERROR_STRING, status);
        return status;
    }

    status = DXP_SUCCESS;

    return status;
}

/*
 * "Closes" the USB connection, which means that it does nothing.
 */
XIA_MD_STATIC int XIA_MD_API dxp_md_usb_close(int* camChan) {
    UNUSED(camChan);
    return DXP_SUCCESS;
}

#endif

#ifndef EXCLUDE_SERIAL

/*
 * Initialized any global structures. Only called once per library load.
 */
XIA_MD_STATIC int dxp_md_serial_initialize(unsigned int* maxMod, char* dllname) {
    int i;

    UNUSED(maxMod);
    UNUSED(dllname);

    numSerial = 0;

    for (i = 0; i < MAXMOD; i++) {
        serialName[i] = NULL;
    }

    return DXP_SUCCESS;
}

/*
 * Initializes a new COM port instance.
 *
 * This routine is called each time that a COM port is opened.
 * ioname encodes COM port and baud rate as "[port]:[baud_rate]".
 */
XIA_MD_STATIC int XIA_MD_API dxp_md_serial_open(char* ioname, int* camChan) {
    int status;
    int len_ioname;

    unsigned int i;

    unsigned short port_;
    unsigned long baud;

    /* Check for an existing definition. */
    for (i = 0; i < numSerial; i++) {
        if (STREQ(serialName[i], ioname)) {
            *camChan = i;
            return DXP_SUCCESS;
        }
    }

    /* Parse in the COM port number and baud rate. */
    sscanf(ioname, "%hu:%lu", &port_, &baud);

    *camChan = numSerial++;

    status = InitSerialPort(port_, baud);

    if (status != SERIAL_SUCCESS) {
        sprintf(ERROR_STRING,
                "Error opening COM%hu, where the driver reports "
                "%d",
                port_, status);
        dxp_md_log_error("dxp_md_serial_open", ERROR_STRING, DXP_MDOPEN);
        return DXP_MDOPEN;
    }

    ASSERT(serialName[*camChan] == NULL);

    len_ioname = strlen(ioname) + 1;

    serialName[*camChan] = md_md_alloc(len_ioname);

    if (serialName[*camChan] == NULL) {
        sprintf(ERROR_STRING, "Unable to allocate %d bytes for serialName[%d]",
                len_ioname, *camChan);
        dxp_md_log_error("dxp_md_serial_open", ERROR_STRING, DXP_NOMEM);
        return DXP_NOMEM;
    }

    strcpy(serialName[*camChan], ioname);

    numMod++;

    return DXP_SUCCESS;
}

/*
 * This routine performs the read/write operations on the serial port.
 */
XIA_MD_STATIC int XIA_MD_API dxp_md_serial_io(int* camChan, unsigned int* function,
                                              unsigned long* wait_in_ms, void* data,
                                              unsigned int* length) {
    LARGE_INTEGER freq;
    LARGE_INTEGER before, after;

    int status;

    unsigned int i;

    unsigned short n_bytes = 0;
    unsigned short comPort;
    unsigned long baud;

    unsigned short* us_data = (unsigned short*) data;

    byte_t* buf = NULL;

    double to = (double) (*wait_in_ms);

    /*
     * Get the proper comPort information. Use this when we need to support multiple
     * serial ports.
     */
    sscanf(serialName[*camChan], "%hu:%lu", &comPort, &baud);

    QueryPerformanceFrequency(&freq);

    if (*function == MD_IO_READ) {
        QueryPerformanceCounter(&before);

        status = SetTimeoutInMS(comPort, to);

        if (status != SERIAL_SUCCESS) {
            sprintf(ERROR_STRING,
                    "Error setting timeout to %.3f milliseconds on COM%hu, "
                    "driver reported status %d",
                    to, comPort, status);
            dxp_md_log_error("dxp_md_serial_io", ERROR_STRING, DXP_MDIO);
            return DXP_MDIO;
        }

        status = dxp_md_serial_read_header(comPort, &n_bytes, us_data);

        if (status != DXP_SUCCESS) {
            dxp_md_log_error("dxp_md_serial_io", "Error reading header", status);
            return status;
        }

        QueryPerformanceCounter(&after);

        /*
         * Check the incoming data packet length against the size of the
         * user's buffer.
         */
        if ((unsigned int) (n_bytes + HEADER_SIZE) > *length) {
            sprintf(ERROR_STRING,
                    "Header reports ndata=%hu, larger than "
                    "requested length %u-%d.",
                    n_bytes, *length, HEADER_SIZE);
            dxp_md_log_error("dxp_md_serial_io", ERROR_STRING, DXP_MDSIZE);
            return DXP_MDSIZE;
        }

        /*
         * Calculate the timeout time based on a conservative transfer rate
         * of 5kb/s with a minimum of 1000 milliseconds.
         */
        to = (double) ((n_bytes / 5000.0) * 1000.0);

        if (to < 1000.0) {
            to = 1000.0;
        }

        status = SetTimeoutInMS(comPort, to);

        if (status != SERIAL_SUCCESS) {
            sprintf(ERROR_STRING,
                    "Error setting timeout to %.3f milliseconds"
                    "driver reported status %d",
                    to, status);
            dxp_md_log_error("dxp_md_serial_io", ERROR_STRING, DXP_MDIO);
            return DXP_MDIO;
        }

        status = dxp_md_serial_read_data(comPort, baud, n_bytes, us_data + HEADER_SIZE);

        if (status != DXP_SUCCESS) {
            dxp_md_log_error("dxp_md_serial_io", "Error reading data", status);
            return status;
        }
    } else if (*function == MD_IO_WRITE) {
        /* Write to the serial port */

        status = CheckAndClearTransmitBuffer(comPort);

        if (status != SERIAL_SUCCESS) {
            dxp_md_log_error("dxp_md_serial_io", "Error clearing transmit buffer",
                             status);
            return DXP_MDIO;
        }

        status = CheckAndClearReceiveBuffer(comPort);

        if (status != SERIAL_SUCCESS) {
            dxp_md_log_error("dxp_md_serial_io", "Error clearing receive buffer",
                             status);
            return DXP_MDIO;
        }

        buf = (byte_t*) md_md_alloc(*length * sizeof(byte_t));

        if (buf == NULL) {
            status = DXP_NOMEM;
            sprintf(ERROR_STRING, "Error allocating %u bytes for buf", *length);
            dxp_md_log_error("dxp_md_serial_io", ERROR_STRING, status);
            return status;
        }

        for (i = 0; i < *length; i++) {
            buf[i] = (byte_t) us_data[i];
        }

        status = WriteSerialPort(comPort, *length, buf);

        if (status != SERIAL_SUCCESS) {
            md_md_free((void*) buf);
            buf = NULL;

            sprintf(ERROR_STRING,
                    "Error writing %u bytes to COM%hu"
                    "driver reported status %d",
                    *length, comPort, status);
            dxp_md_log_error("dxp_md_serial_io", ERROR_STRING, status);
            return DXP_MDIO;
        }

        md_md_free((void*) buf);
        buf = NULL;
    } else if (*function == MD_IO_OPEN) {
        /* Do nothing */
    } else if (*function == MD_IO_CLOSE) {
        /* Do nothing */
    } else {
        status = DXP_MDUNKNOWN;
        sprintf(ERROR_STRING, "Unknown function: %u", *function);
        dxp_md_log_error("dxp_md_serial_io", ERROR_STRING, status);
        return status;
    }

    return DXP_SUCCESS;
}

/*
 * Closes and re-opens the port
 */
static int dxp_md_serial_reset(unsigned short port_, unsigned long baud) {
    int status;

    status = CloseSerialPort(port_);

    if (status != SERIAL_SUCCESS) {
        dxp_md_log_error("dxp_md_serial_reset", "Error closing port", status);
        return DXP_MDIO;
    }

    status = InitSerialPort(port_, baud);

    if (status != SERIAL_SUCCESS) {
        dxp_md_log_error("dxp_md_serial_reset", "Error re-initializing port", status);
        return DXP_MDIO;
    }

    return DXP_SUCCESS;
}

/*
 * Reads the header of the returned packet
 *
 * Reads the header of the returned packet and puts it into the user
 * specified buffer. Also calculates the number of bytes remaining
 * in the packet including the XOR checksum.
 */
static int dxp_md_serial_read_header(unsigned short port_, unsigned short* bytes,
                                     unsigned short* buf) {
    int i;

    byte_t lo;
    byte_t hi;
    byte_t b;

    byte_t header[HEADER_SIZE];

    serial_read_error_t* err = NULL;

    ASSERT(bytes != NULL);
    ASSERT(buf != NULL);

    for (i = 0; i < HEADER_SIZE; i++) {
        err = ReadSerialPort(port_, 1, &b);

        if (err->status != SERIAL_SUCCESS) {
            sprintf(ERROR_STRING,
                    "Error reading header from COM%hu: actual = %d, "
                    "expected = %d, bytes_in_recv_buf = %d, size_recv_buf = %d, "
                    "driver reported status %d",
                    port_, err->actual, err->expected, err->bytes_in_recv_buf,
                    err->size_recv_buf, err->status);
            dxp_md_log_error("dxp_md_serial_read_header", ERROR_STRING, DXP_MDIO);
            return DXP_MDIO;
        }

        header[i] = b;
    }

    /*
     * XXX Is any error checking done elsewhere to verify the initial, non-size bytes
     * in the header? Perhaps in dxp_cmd()/dxp_command()?
     */
    lo = header[2];
    hi = header[3];

    /* Include the XOR checksum in this calculation */
    *bytes = (unsigned short) (((unsigned short) lo | ((unsigned short) hi << 8)) + 1);

    if (*bytes == 1) {
        dxp_md_log_debug("dxp_md_serial_read_header",
                         "Number of data bytes = 1 in header");
        for (i = 0; i < HEADER_SIZE; i++) {
            sprintf(ERROR_STRING, "header[%d] = %#hx", i, (unsigned short) header[i]);
            dxp_md_log_debug("dxp_md_serial_read_header", ERROR_STRING);
        }
    }

    /*
     * Copy header into data buffer. Can't use memcpy() since we are
     * going from byte_t -> unsigned short.
     */
    for (i = 0; i < HEADER_SIZE; i++) {
        buf[i] = (unsigned short) header[i];
    }

    return DXP_SUCCESS;
}

/*
 * Reads the specified number of bytes from the port and copies them to the buffer.
 */
static int dxp_md_serial_read_data(unsigned short port_, unsigned long baud,
                                   unsigned long size, unsigned short* buf) {
    int status;

    unsigned long i;

    byte_t* b = NULL;

    serial_read_error_t* err = NULL;

    ASSERT(buf != NULL);

    b = (byte_t*) md_md_alloc(size * sizeof(byte_t));

    if (b == NULL) {
        sprintf(ERROR_STRING, "Error allocating %lu bytes for 'b'", size);
        dxp_md_log_error("dxp_md_serial_read_data", ERROR_STRING, DXP_NOMEM);
        return DXP_NOMEM;
    }

    err = ReadSerialPort(port_, size, b);

    if (err->status != SERIAL_SUCCESS) {
        md_md_free(b);
        sprintf(ERROR_STRING, "bytes_in_recv_buf = %d, size_recv_buf = %d",
                err->bytes_in_recv_buf, err->size_recv_buf);
        dxp_md_log_debug("dxp_md_serial_read_data", ERROR_STRING);
        sprintf(ERROR_STRING, "hardware_overrun = %s, software_overrun = %s",
                err->is_hardware_overrun > 0 ? "true" : "false",
                err->is_software_overrun > 0 ? "true" : "false");
        dxp_md_log_debug("dxp_md_serial_read_data", ERROR_STRING);
        sprintf(ERROR_STRING,
                "Error reading data from COM%hu: "
                "expected = %d, actual = %d",
                port_, err->expected, err->actual);
        dxp_md_log_error("dxp_md_serial_read_data", ERROR_STRING, DXP_MDIO);

        status = dxp_md_serial_reset(port_, baud);

        if (status != DXP_SUCCESS) {
            sprintf(ERROR_STRING,
                    "Error attempting to reset COM%hu in response to a "
                    "communications failure",
                    port_);
            dxp_md_log_error("dxp_md_serial_read_data", ERROR_STRING, status);
        }

        return DXP_MDIO;
    }

    for (i = 0; i < size; i++) {
        buf[i] = (unsigned short) b[i];
    }

    md_md_free(b);

    return DXP_SUCCESS;
}

/*
 * Closes the serial port connection so that we don't
 * crash due to interrupt vectors left lying around.
 */
XIA_MD_STATIC int XIA_MD_API dxp_md_serial_close(int* camChan) {
    int status;

    unsigned short comPort;

    ASSERT(camChan != NULL);

    sprintf(ERROR_STRING, "Called with camChan %d", *camChan);
    dxp_md_log_debug("dxp_md_serial_close", ERROR_STRING);

    if (serialName[*camChan] != NULL) {
        sscanf(serialName[*camChan], "%hu", &comPort);

        sprintf(ERROR_STRING, "Preparing to close camChan %d, comPort %hu", *camChan,
                comPort);
        dxp_md_log_debug("dxp_md_serial_close", ERROR_STRING);

        status = CloseSerialPort(comPort);

        if (status != 0) {
            return DXP_MDCLOSE;
        }

        md_md_free(serialName[*camChan]);
        serialName[*camChan] = NULL;

        numSerial--;
    }

    return DXP_SUCCESS;
}

#endif /* EXCLUDE_SERIAL */

/*
 * Routine to get the maximum number of words that can be block transferred at
 * once. This can change from system to system and from controller to
 * controller. A maxblk size of 0 means to write all data at once, regardless
 * of transfer size.
 */
XIA_MD_STATIC int XIA_MD_API dxp_md_get_maxblk(void) {
    return MAXBLK;
}

/*
 * Routine to set the maximum number of words that can be block transferred at
 * once.  This can change from system to system and from controller to
 * controller.  A maxblk size of 0 means to write all data at once, regardless
 * of transfer size.
 *
 * unsigned int *blksiz;
 */
XIA_MD_STATIC int XIA_MD_API dxp_md_set_maxblk(unsigned int* blksiz) {
    int status;

    if (*blksiz > 0) {
        MAXBLK = *blksiz;
    } else {
        status = DXP_NEGBLOCKSIZE;
        dxp_md_log_error("dxp_md_set_maxblk", "Block size must be positive or zero",
                         status);
        return status;
    }
    return DXP_SUCCESS;
}

/*
 * Routine to wait a specified time in seconds.  This allows the user to call
 * routines that are as precise as required for the purpose at hand.
 *
 * float *time;							Input: Time to wait in seconds
 */
XIA_MD_STATIC int XIA_MD_API dxp_md_wait(float* time) {
    DWORD wait = (DWORD) (1000.0 * (*time));
    Sleep(wait);
    return DXP_SUCCESS;
}

/*
 * Routine to allocate memory.  The calling structure is the same as the
 * ANSI C standard routine malloc().
 *
 * size_t length; Input: length of the memory to allocate in units of size_t (defined
 *                       to be a byte on most systems)
 */
XIA_MD_SHARED void* XIA_MD_API dxp_md_alloc(size_t length) {
    return malloc(length);
}

/*
 * Routine to free memory.  Same calling structure as the ANSI C routine
 * free().
 *
 * void *array;	Input: pointer to the memory to free
 */
XIA_MD_SHARED void XIA_MD_API dxp_md_free(void* array) {
    free(array);
}

/*
 * Routine to print a string to the screen or log. No direct calls to
 * printf() are performed by XIA libraries. All output is directed either
 * through the dxp_md_error() or dxp_md_puts() routines.
 *
 * char *s;	Input: string to print or log
 */
XIA_MD_STATIC int XIA_MD_API dxp_md_puts(char* s) {
    return printf("%s", s);
}

/*
 * Safe version of fgets() that can handle both UNIX and DOS line-endings.
 *
 * If the trailing two characters are '\\r' + '\\n', they are replaced by a
 * single '\\n'.
 */
XIA_MD_STATIC char* dxp_md_fgets(char* s, int length, FILE* stream) {
    char* cstatus = NULL;

    size_t ret_len = 0;

    ASSERT(s != NULL);
    ASSERT(stream != NULL);
    ASSERT(length > 0);

    cstatus = fgets(s, length - 1, stream);

    if (!cstatus) {
        if (ferror(stream)) {
            dxp_md_log_warning("dxp_md_fgets", "Error detected reading from "
                                               "stream.");
        }
        return NULL;
    }

    ret_len = strlen(s);

    if ((ret_len > 1) && (s[ret_len - 2] == '\r') && (s[ret_len - 1] == '\n')) {
        s[ret_len - 2] = '\n';
        s[ret_len - 1] = '\0';
    }

    return s;
}

/*
 * Get a safe temporary directory path.
 *
 * If the temporary path is greater than the length of that passed in
 * string, an error is returned.
 *
 * The search order is defined at
 * http://msdn.microsoft.com/library/default.asp? \
 * url=/library/en-us/fileio/fs/gettemppath.asp
 *
 * NOTE: The temporary path is only computed once and the result is
 * cached. If we ever make Handel thread-safe, we won't be able to cache
 * the data as a global anymore.
 */
XIA_MD_STATIC char* dxp_md_tmp_path(void) {
    DWORD tmp_path_len = 0;
    DWORD chars_written = 0;

    LPSTR tmp_path;

    if (TMP_PATH) {
        return TMP_PATH;
    }

    /*
     * According to the docs for GetTempPath(), if the actual length is longer
     * then the passed in length, the actual size is returned. We can then
     * use the actual length to allocate the correct amount of memory.
     */
    tmp_path_len = GetTempPathA(0, NULL);

    if (tmp_path_len == 0) {
        dxp_md_log_error("dxp_md_tmp_path",
                         "Windows returned an error trying to "
                         "determine the temporary file path.",
                         DXP_WIN32_API);
        return NULL;
    }

    tmp_path = dxp_md_alloc(tmp_path_len + 1);

    if (!tmp_path) {
        sprintf(ERROR_STRING, "Unable to allocate %lu bytes for 'tmp_path'.",
                tmp_path_len + 1);
        dxp_md_log_error("dxp_md_tmp_path", ERROR_STRING, DXP_NOMEM);
        return NULL;
    }

    chars_written = GetTempPathA(tmp_path_len, tmp_path);

    if (chars_written == 0) {
        dxp_md_log_error("dxp_md_tmp_path",
                         "Windows returned an error trying to "
                         "determine the temporary file path.",
                         DXP_WIN32_API);
        return NULL;
    }

    TMP_PATH = (char*) tmp_path;

    return TMP_PATH;
}

/*
 * Clears the temporary path cache.
 *
 * Caution: This will free the cache if it is already allocated. Any pointers
 * that pointed at the old cache will probably segfault if you access them again
 * -- if you're lucky. This routine, mainly, exists for testing dxp_md_tmp_path()
 * and for cleaning up globally allocated resources on exit.
 */
XIA_MD_STATIC void dxp_md_clear_tmp(void) {
    if (TMP_PATH) {
        dxp_md_free(TMP_PATH);
    }
    TMP_PATH = NULL;
}

/*
 * Returns the path separator for Win32 platform.
 */
XIA_MD_STATIC char* dxp_md_path_separator(void) {
    return PATH_SEP;
}

/*
 * Sets the priority of the current process using the Win32 API
 */
XIA_MD_STATIC int XIA_MD_API dxp_md_set_priority(int* priority) {
    BOOL status;

    DWORD pri;

    HANDLE h;

    char pri_str[5];

    h = GetCurrentProcess();

    switch (*priority) {
        default:
            sprintf(ERROR_STRING, "Invalid priority type: %d", *priority);
            dxp_md_log_error("dxp_md_set_priority", ERROR_STRING,
                             DXP_MDINVALIDPRIORITY);
            return DXP_MDINVALIDPRIORITY;
            break;
        case MD_IO_PRI_NORMAL:
            pri = NORMAL_PRIORITY_CLASS;
            strncpy(pri_str, "NORM", 5);
            break;
        case MD_IO_PRI_HIGH:
            pri = REALTIME_PRIORITY_CLASS;
            strncpy(pri_str, "HIGH", 5);
            break;
    }

    status = SetPriorityClass(h, pri);

    if (!status) {
        sprintf(ERROR_STRING, "Error setting priority class (%s) for current process",
                pri_str);
        dxp_md_log_error("dxp_md_set_priority", ERROR_STRING, DXP_MDPRIORITY);
        return DXP_MDPRIORITY;
    }

    return DXP_SUCCESS;
}

#ifndef EXCLUDE_PLX

/*
 * Initializes any global structures.
 *
 * This routine is called once per overall library session and, therefore,
 * its job is to initialize the global structures used to manage the
 * PXI handles.
 */
XIA_MD_STATIC int dxp_md_plx_initialize(unsigned int* maxMod, char* dllname) {
    UNUSED(maxMod);
    UNUSED(dllname);

    /*
     * All the other routines check maxMod against MAXMOD, but as near
     * as I can tell it doesn't seem to matter. For now, I will skip this
     * check.
     */
    numPLX = 0;

    return DXP_SUCCESS;
}

/*
 * Binds a PXI module to a PLX driver handle.
 *
 * Two unique pieces of information are required to open the PCI device:
 * The slot number and the bus number. These values are typically found
 * in the .INI file that comes with the crate (pxisys.ini). ioname encodes
 * these two pieces of information as "[bus]:[slot]".
 */
XIA_MD_STATIC int dxp_md_plx_open(char* ioname, int* camChan) {
    int i;
    int status;
    int n_scanned = 0;
    size_t len_ioname = 0;

    unsigned short id = 0xFFFF;
    unsigned short bus;
    unsigned short slot;

    ASSERT(ioname != NULL);
    ASSERT(camChan != NULL);

    /* Search for an existing definition */
    for (i = 0; i < (int) numPLX; i++) {
        if (STREQ(pxiNames[i], ioname)) {
            *camChan = i;
            return DXP_SUCCESS;
        }
    }

    /* Parse in and open a new slot */
    n_scanned = sscanf(ioname, "%hu:%hu", &bus, &slot);

    if (n_scanned != 2) {
        sprintf(ERROR_STRING,
                "Error parsing ioname = '%s'. The proper format is "
                "'bus:slot'.",
                ioname);
        dxp_md_log_error("dxp_md_plx_open", ERROR_STRING, DXP_MDINVALIDNAME);
        return DXP_MDINVALIDNAME;
    }

    *camChan = numPLX++;

    ASSERT(bus < 256);
    ASSERT(slot < 256);

    status = plx_open_slot(id, (byte_t) bus, (byte_t) slot, &pxiHandles[*camChan]);

    if (status != PLX_SUCCESS) {
        plx_print_error(status, ERROR_STRING);
        dxp_md_log_warning("dxp_md_plx_open", ERROR_STRING);
        sprintf(ERROR_STRING,
                "Error opening slot '%hu' on bus '%hu', where the "
                "driver reports a status of %d",
                slot, bus, status);
        dxp_md_log_error("dxp_md_plx_open", ERROR_STRING, DXP_MDOPEN);
        return DXP_MDOPEN;
    }

    ASSERT(pxiNames[*camChan] == NULL);

    len_ioname = strlen(ioname) + 1;

    pxiNames[*camChan] = (char*) md_md_alloc(len_ioname);

    if (!pxiNames[*camChan]) {
        sprintf(ERROR_STRING, "Unable to allocate %zu bytes for pxiNames[%d]",
                len_ioname, *camChan);
        dxp_md_log_error("dxp_md_plx_open", ERROR_STRING, DXP_NOMEM);
        return DXP_NOMEM;
    }

    strncpy(pxiNames[*camChan], ioname, len_ioname);

    numMod++;

    return DXP_SUCCESS;
}

/*
 * Performs the specified I/O operation on the hardware.
 *
 * The PLX driver supports 3 different I/O operations: read/write single words
 * and burst reads. The I/O operation type is controlled via function, where
 * 0 corresponds to a single write, 1 corresponds to a single read and 2
 * corresponds to a burst read.
 */
static int dxp_md_plx_io(int* camChan, unsigned int* function, unsigned long* addr,
                         void* data, unsigned int* length) {
    int status;

    HANDLE h;

    unsigned long* buf = (unsigned long*) data;

    UNUSED(function);
    UNUSED(length);

    ASSERT(camChan != NULL);
    ASSERT(data != NULL);

    h = pxiHandles[*camChan];

    /* How do we check if the handle is valid? These aren't pointers... */

    switch (*function) {
        case 0:
            /* Single word write */
#ifdef XIA_PLX_WRITE_DEBUG
            sprintf(ERROR_STRING, "XIA_PLX_WRITE_DEBUG: %#lx = %#lx", *addr, *buf);
            dxp_md_log_debug("dxp_md_plx_io", ERROR_STRING);
#endif /* XIA_PLX_WRITE_DEBUG */
            status = plx_write_long(h, *addr, *buf);
            break;
        case 1:
            /* Single word read */
            status = plx_read_long(h, *addr, buf);
            break;
        case 2:
            /* Burst read, normal, 2 dead words */
            status = plx_read_block(h, *addr, *length, 2, buf);
            break;
        default:
            /* This should never occur */
            status = DXP_MDUNKNOWN;
            FAIL();
            break;
    }

    if (status != PLX_SUCCESS) {
        plx_print_error(status, ERROR_STRING);
        dxp_md_log_error("dxp_md_plx_io", ERROR_STRING, DXP_MDIO);
        sprintf(ERROR_STRING,
                "Unable to perform requested I/O: func = %u, addr = "
                "%#lx, len = %u, (PLX driver reports status = %d)",
                *function, *addr, *length, status);
        dxp_md_log_error("dxp_md_plx_io", ERROR_STRING, DXP_MDIO);
        return DXP_MDIO;
    }

    return DXP_SUCCESS;
}

/*
 * Closes the port.
 */
static int dxp_md_plx_close(int* camChan) {
    HANDLE h;

    int status;

    ASSERT(camChan != NULL);

    h = pxiHandles[*camChan];

    if (h) {
        status = plx_close_slot(h);

        if (status != PLX_SUCCESS) {
            plx_print_error(status, ERROR_STRING);
            dxp_md_log_debug("dxp_md_plx_open", ERROR_STRING);
            sprintf(ERROR_STRING,
                    "Error closing PXI slot '%s', handle = %p,"
                    " (PLX driver reports status = %d)",
                    pxiNames[*camChan], &h, status);
            dxp_md_log_error("dxp_md_plx_close", ERROR_STRING, DXP_MDCLOSE);
            return DXP_MDCLOSE;
        }

        ASSERT(pxiNames[*camChan] != NULL);

        md_md_free(pxiNames[*camChan]);
        pxiNames[*camChan] = NULL;

        numPLX--;

        sprintf(ERROR_STRING, "Closed the PLX HANDLE at camChan = %d", *camChan);
        dxp_md_log_debug("dxp_md_plx_close", ERROR_STRING);

        /* Is there a way to mark the handle as unused now? */
        pxiHandles[*camChan] = NULL;
    } else {
        sprintf(ERROR_STRING, "Skipping previously closed camChan = %d", *camChan);
        dxp_md_log_info("dxp_md_plx_close", ERROR_STRING);
    }

    return DXP_SUCCESS;
}

#endif /* EXCLUDE_PLX */
#ifndef EXCLUDE_USB2

/*
 * Perform any one-time initialization tasks for the USB2 driver.
 */
XIA_MD_STATIC int dxp_md_usb2_initialize(unsigned int* maxMod, char* dllname) {
    UNUSED(maxMod);
    UNUSED(dllname);
    numUSB2 = 0;
    return DXP_SUCCESS;
}

/*
 * Open the requested USB2 device.
 */
XIA_MD_STATIC int dxp_md_usb2_open(char* ioname, int* camChan) {
    int i;
    int dev;
    int status;
    size_t len;

    ASSERT(ioname != NULL);
    ASSERT(camChan != NULL);

    for (i = 0; i < (int) numUSB2; i++) {
        if (STREQ(usb2Names[i], ioname)) {
            *camChan = i;
            return DXP_SUCCESS;
        }
    }

    sscanf(ioname, "%d", &dev);

    *camChan = numUSB2++;

    status = xia_usb2_open(dev, &usb2Handles[*camChan]);

    if (status != XIA_USB2_SUCCESS) {
        sprintf(ERROR_STRING, "Error opening USB device '%d'", dev);
        dxp_md_log_error("dxp_md_usb2_open", ERROR_STRING, DXP_MDOPEN);
        /* Error format %.100s is based on ERROR_STRING length at 132 */
        sprintf(ERROR_STRING, "USB2 driver status=%d error: %.100s", status,
                xia_usb2_get_last_error());
        dxp_md_log_error("dxp_md_usb2_open", ERROR_STRING, DXP_MDOPEN);
        return DXP_MDOPEN;
    }

    ASSERT(usb2Handles[*camChan] != INVALID_HANDLE_VALUE);
    ASSERT(usb2Names[*camChan] == NULL);

    len = strlen(ioname) + 1;

    usb2Names[*camChan] = md_md_alloc(len);

    if (usb2Names[*camChan] == NULL) {
        sprintf(ERROR_STRING, "Unable to allocate %zu bytes for usb2Names[%d]", len,
                *camChan);
        dxp_md_log_error("dxp_md_usb2_open", ERROR_STRING, DXP_NOMEM);
        return DXP_NOMEM;
    }

    strcpy(usb2Names[*camChan], ioname);

    usb2AddrCache[*camChan] = MD_INVALID_ADDR;

    numMod++;

    return DXP_SUCCESS;
}

/*
 * Wraps both read/write USB2 operations.
 *
 * The I/O type (read/write) is specified in function. The allowed types
 * are the same as the USB driver so that we can use USB2 and USB interchangeably
 * as communication protocols.
 */
XIA_MD_STATIC int dxp_md_usb2_io(int* camChan, unsigned int* function,
                                 unsigned long* addr, void* data, unsigned int* len) {
    int status;

    unsigned int i;

    unsigned long cache_addr;

    unsigned short* buf = NULL;

    byte_t* byte_buf = NULL;

    unsigned long n_bytes = 0;

    ASSERT(addr != NULL);
    ASSERT(function != NULL);
    ASSERT(data != NULL);
    ASSERT(camChan != NULL);
    ASSERT(len != NULL);

    buf = (unsigned short*) data;

    n_bytes = (unsigned long) (*len) * 2;

    /*
     * Unlike some of our other communication types, we require that the
     * target address be set as a separate operation. This value will be saved
     * until the real I/O is requested. In a perfect world, we wouldn't require
     * this type of operation anymore, but we have to maintain backwards
     * compatibility for products that want to use USB2 but originally used
     * other protocols that did require the address be set separately.
     */
    switch (*addr) {
        case 0:
            if (usb2AddrCache[*camChan] == MD_INVALID_ADDR) {
                sprintf(ERROR_STRING, "No target address set for camChan %d", *camChan);
                dxp_md_log_error("dxp_md_usb2_io", ERROR_STRING, DXP_MD_TARGET_ADDR);
                return DXP_MD_TARGET_ADDR;
            }

            cache_addr = (unsigned long) usb2AddrCache[*camChan];

            switch (*function) {
                case MD_IO_READ:
                    /* The data comes from the calling routine as an unsigned short, so
                     * we need to convert it to a byte array for the USB2 driver.
                     */
                    byte_buf = md_md_alloc(n_bytes);

                    if (!byte_buf) {
                        sprintf(ERROR_STRING,
                                "Error allocating %lu bytes for 'byte_buf' for "
                                "camChan %d",
                                n_bytes, *camChan);
                        dxp_md_log_error("dxp_md_usb2_io", ERROR_STRING, DXP_NOMEM);
                        return DXP_NOMEM;
                    }

                    /* Initialize buffer to a fixed pattern to identify source of read
                     * errors in case the buffer is not filled completely
                     */
                    memset(byte_buf, 0xAB, n_bytes);

                    status = xia_usb2_read(usb2Handles[*camChan], cache_addr, n_bytes,
                                           byte_buf);

                    if (status != XIA_USB2_SUCCESS) {
                        md_md_free(byte_buf);
                        sprintf(ERROR_STRING,
                                "Error reading %lu bytes from %#lx for "
                                "camChan %d",
                                n_bytes, cache_addr, *camChan);
                        dxp_md_log_error("dxp_md_usb2_io", ERROR_STRING, DXP_MDIO);
                        sprintf(ERROR_STRING, "USB2 driver status=%d error: %.100s",
                                status, xia_usb2_get_last_error());
                        dxp_md_log_error("dxp_md_usb2_open", ERROR_STRING, DXP_MDIO);
                        return DXP_MDIO;
                    }

                    for (i = 0; i < *len; i++) {
                        buf[i] = (unsigned short) (byte_buf[i * 2] |
                                                   (byte_buf[(i * 2) + 1] << 8));
                    }

                    md_md_free(byte_buf);
                    break;
                case MD_IO_WRITE:
                    /* The data comes from the calling routine as an unsigned short, so
                     * we need to convert it to a byte array for the USB2 driver.
                     */
                    byte_buf = md_md_alloc(n_bytes);

                    if (byte_buf == NULL) {
                        sprintf(ERROR_STRING,
                                "Error allocating %lu bytes for 'byte_buf' for "
                                "camChan %d",
                                n_bytes, *camChan);
                        dxp_md_log_error("dxp_md_usb2_io", ERROR_STRING, DXP_NOMEM);
                        return DXP_NOMEM;
                    }

                    for (i = 0; i < *len; i++) {
                        byte_buf[i * 2] = (byte_t) (buf[i] & 0xFF);
                        byte_buf[(i * 2) + 1] = (byte_t) ((buf[i] >> 8) & 0xFF);
                    }

                    status = xia_usb2_write(usb2Handles[*camChan], cache_addr, n_bytes,
                                            byte_buf);

                    md_md_free(byte_buf);

                    if (status != XIA_USB2_SUCCESS) {
                        sprintf(ERROR_STRING,
                                "Error writing %lu bytes to %#lx for "
                                "camChan %d.",
                                n_bytes, cache_addr, *camChan);
                        dxp_md_log_error("dxp_md_usb2_io", ERROR_STRING, DXP_MDIO);
                        sprintf(ERROR_STRING, "USB2 driver status=%d error: %.100s",
                                status, xia_usb2_get_last_error());
                        dxp_md_log_error("dxp_md_usb2_open", ERROR_STRING, DXP_MDIO);
                        return DXP_MDIO;
                    }
                    break;
                default:
                    FAIL();
                    break;
            }
            break;
        case 1:
            /* Even though we aren't entirely thread-safe yet, it doesn't hurt to
             * store the address as a function of camChan, instead of as a global
             * variable like dxp_md_usb_io().
             */
            usb2AddrCache[*camChan] = *((unsigned long*) data);
            break;
        default:
            FAIL();
            break;
    }

    return DXP_SUCCESS;
}

/*
 * Closes a device previously opened with dxp_md_usb2_open().
 */
XIA_MD_STATIC int dxp_md_usb2_close(int* camChan) {
    int status;

    HANDLE h;

    ASSERT(camChan != NULL);

    h = usb2Handles[*camChan];

    if (h == NULL) {
        sprintf(ERROR_STRING, "Skipping previously closed camChan = %d", *camChan);
        dxp_md_log_info("dxp_md_usb2_close", ERROR_STRING);
        return DXP_SUCCESS;
    }

    status = xia_usb2_close(h);

    if (status != XIA_USB2_SUCCESS) {
        sprintf(ERROR_STRING, "Error closing camChan = %d", *camChan);
        dxp_md_log_error("dxp_md_usb2_close", ERROR_STRING, DXP_MDCLOSE);
        sprintf(ERROR_STRING, "USB2 driver status=%d error: %.100s", status,
                xia_usb2_get_last_error());
        dxp_md_log_error("dxp_md_usb2_open", ERROR_STRING, DXP_MDCLOSE);
        return DXP_MDCLOSE;
    }

    usb2Handles[*camChan] = NULL;

    ASSERT(usb2Names[*camChan] != NULL);

    md_md_free(usb2Names[*camChan]);
    usb2Names[*camChan] = NULL;

    numUSB2--;

    return DXP_SUCCESS;
}

#endif /* EXCLUDE_USB2 */
