/*
 * Copyright (c) 2002-2004 X-ray Instrumentation Associates
 * Copyright (c) 2005-2015 XIA LLC
 * All rights reserved
 *
 * NOT COVERED UNDER THE BSD LICENSE. NOT FOR RELEASE TO CUSTOMERS.
 */

#include <stdio.h>

#include "xia_mddef.h"
#include "xia_assert.h"

#include "md_generic.h"

#include "xerxesdef.h"
#include "xerxes_errors.h"
#include "xerxes_structures.h"

#include "udxp_common.h"
#include "udxp_command.h"

#ifdef XIA_ALPHA
#include "psl_udxp_alpha.h"
#endif

#define ESCAPE   0x1B
#define INFO_LEN 400

#define udxpc_log_error(x, y, z)    funcs.dxp_md_log(MD_ERROR, (x), (y), (z), __FILE__, __LINE__)
#define udxpc_log_debug(x, y)       funcs.dxp_md_log(MD_DEBUG, (x), (y),   0, __FILE__, __LINE__)
#define udxpc_md_alloc(x)           funcs.dxp_md_alloc((x))
#define udxpc_md_free(x)            funcs.dxp_md_free((x))


/* Import the necessary MD routine here */
XIA_MD_IMPORT int XIA_MD_API dxp_md_init_util(Xia_Util_Functions *funcs, char *type);

/* Helper functions */
int dxp_send_command(int ioChan, DXP_MD_IO udxp_io, unsigned long address,
                     byte_t cmd, unsigned int lenS, byte_t *send,
                     byte_t ioFlags);
int dxp_read_response(int ioChan, DXP_MD_IO udxp_io, unsigned long address,
                      unsigned int lenR, byte_t *receive,
                      byte_t ioFlags);
int dxp_verify_response(byte_t cmd, unsigned int lenS, byte_t *send,
                        unsigned int lenR, byte_t *receive);

int dxp_usb2_set_address_cache(int ioChan, DXP_MD_IO udxp_io, unsigned long address);

int dxp_usb2_reset_bus(int ioChan, int modChan, DXP_MD_IO udxp_io);

int dxp_usb2_reticulate_address(int modChan, byte_t *cmd, unsigned long *addr);

static int dxp__update_version_cache(int ioChan, DXP_MD_IO io,
                                     boolean_t is_usb,
                                     Xia_Util_Functions funcs);

/* Cache for the version number of a given microDXP ioChan. Since we need a way
 * to know if we have actively gathered this informaton or not, 0xFF will mean
 * that we have not fetched the version number yet.
 */
static byte_t VERSION_CACHE[MAXMOD][VERSION_NUMBERS];

#define UDXP_VERSION_NOT_READ 0xFF

#define IS_USB(x) ((boolean_t)(!(((x) & 0x40) == 0)))

#define UDXP_FREE(x)   udxpc_md_free((void *)(x)); \
                  (x) = NULL


static char INFO_STRING[INFO_LEN];


/*
 *Reset the state of all ioChans in the variant cache to "unread".
 */
XERXES_SHARED void dxp_init_pic_version_cache(void)
{
    memset(VERSION_CACHE, UDXP_VERSION_NOT_READ, sizeof(VERSION_CACHE));
}


/*
 * Computes a standard XOR checksum on the byte array that
 * is passed in using the following relation:
 * chksum_(i + 1) = chksum_(i) XOR data[i]
 */
XERXES_SHARED byte_t dxp_compute_chksum(unsigned int len, byte_t *data)
{
    unsigned int i;

    byte_t chkSum;


    for (i = 0, chkSum = 0x00; i < len; i++) {

        chkSum = (byte_t)(chkSum ^ data[i]);
    }

    return chkSum;
}


/*
 * Build a complete command string from the command,
 * the required data in the specification. The data
 * array should not contain the NData values. The
 * NData bytes and the checksum are automatically
 * added to the complete command string. cmdstr should
 * be allocated to a size of len + 4.
 */
XERXES_SHARED int dxp_build_cmdstr(byte_t cmd, unsigned short len, byte_t *data, byte_t *cmdstr)
{
    int status;

    byte_t *preChksm = NULL;

    byte_t checksum = 0x00;

    unsigned short chksmIdx = (unsigned short)(len + 4);

    Xia_Util_Functions funcs;


    /* Initialize our function pointers
     * so that we can allocate memory
     * safely.
     */
    dxp_md_init_util(&funcs, NULL);

    /* Build data array so that the checksum can be
     * computed.
     */
    preChksm = (byte_t *)udxpc_md_alloc((len + 4) * sizeof(byte_t));

    if (preChksm == NULL) {

        status = DXP_NOMEM;
        udxpc_log_error("dxp_build_cmdstr", "Out-of-memory allocating space for preChksm",
                        status);
        return status;
    }

    preChksm[0] = (byte_t)ESCAPE;
    preChksm[1] = cmd;
    preChksm[2] = (byte_t)(len & 0xFF);
    preChksm[3] = (byte_t)((len >> 8) & 0xFF);

    memcpy(preChksm + 4, data, len);

    /* Compute the checksum, not including the
     * ESCAPE char.
     */
    checksum = dxp_compute_chksum(len + 3, preChksm + 1);
    memcpy(cmdstr, preChksm, len + 4);

    UDXP_FREE(preChksm);

    cmdstr[chksmIdx] = checksum;

    /*
      sprintf(INFO_STRING, "cmd = %u", cmdstr[1]);
      udxpc_log_debug("dxp_build_cmdstr", INFO_STRING);
      sprintf(INFO_STRING, "len (low) = %u", cmdstr[2]);
      udxpc_log_debug("dxp_build_cmdstr", INFO_STRING);
      sprintf(INFO_STRING, "len {high) = %u", cmdstr[3]);
      udxpc_log_debug("dxp_build_cmdstr", INFO_STRING);
      sprintf(INFO_STRING, "chksm = %u", cmdstr[chksmIdx]);
      udxpc_log_debug("dxp_build_cmdstr", INFO_STRING);
    */

    return DXP_SUCCESS;
}


/*
 * This routine sends the specified command
 * to the uDXP and waits for the response.
 * This routine is responsible for opening
 * and closing the serial port. Assumes that
 * the proper amount of memory has been allocated
 * for the send and receive data buffers. A simple
 * comparison is done of the checksums to make sure
 * that no gross errors have occurred. More complex
 * error checking is left to the calling routine.
 */
XERXES_SHARED int dxp_command(int ioChan, int modChan, DXP_MD_IO udxp_io, byte_t cmd,
                              unsigned int lenS, byte_t *send,
                              unsigned int lenR, byte_t *receive,
                              byte_t ioFlags)
{
    int status;

    unsigned long address = 0;

    Xia_Util_Functions funcs;

    ASSERT(lenR >= RECV_BASE);
    ASSERT(receive != NULL);


    dxp_md_init_util(&funcs, NULL);

    /* This should only be updated once per microDXP. */
    if (VERSION_CACHE[ioChan][PIC_VARIANT] == UDXP_VERSION_NOT_READ) {
        sprintf(INFO_STRING, "Initializing variant cache for ioChan %d", ioChan);
        udxpc_log_debug("dxp_command", INFO_STRING);

        status = dxp__update_version_cache(ioChan, udxp_io, ioFlags, funcs);

        if (status != DXP_SUCCESS) {
            sprintf(INFO_STRING, "Error updating the variant cache for ioChan %d",
                    ioChan);
            udxpc_log_error("dxp_command", INFO_STRING, status);
            return status;
        }
    }

    if (IS_USB(ioFlags)) {
        status = dxp_usb2_reticulate_address(modChan, &cmd, &address);

        if (status != DXP_SUCCESS) {
            sprintf(INFO_STRING, "Error updating the address for a USB transfer "
                    "for ioChan %d, modChan %d", ioChan, modChan);
            udxpc_log_error("dxp_command", INFO_STRING, status);
            return status;
        }
    }

    status = dxp_send_command(ioChan, udxp_io, address, cmd, lenS, send, ioFlags);

    if (status != DXP_SUCCESS) {
        sprintf(INFO_STRING, "Error sending command %#x to ioChan %d",
                cmd, ioChan);
        udxpc_log_error("dxp_command", INFO_STRING, status);
        return status;
    }

    status = dxp_read_response(ioChan, udxp_io, address, lenR, receive, ioFlags);

    if (status != DXP_SUCCESS) {
        sprintf(INFO_STRING, "Error reading response from ioChan %d", ioChan);
        udxpc_log_error("dxp_command", INFO_STRING, status);
        return status;
    }

    /* Verify response even for usb2 right now */
    status = dxp_verify_response(cmd, lenS, send, lenR, receive);

    if (status != DXP_SUCCESS) {
        sprintf(INFO_STRING, "Invalid response from ioChan %d", ioChan);
        udxpc_log_error("dxp_command", INFO_STRING, status);
        return status;
    }

    return DXP_SUCCESS;
}

/*
 * Read USB2 memory from the specified location
 * Only supported by USB2 dxp
 * This function can be accessed from the psl with dxp_read_memory
 */
XERXES_SHARED int dxp_usb_read_block(int ioChan, int modChan, DXP_MD_IO udxp_io,
                                     unsigned long addr, unsigned long n, unsigned short *data)
{

    int status;

    unsigned int serial_read  = 0;
    unsigned long a = DXP_A_IO;
    unsigned int dataWords = (unsigned int) n;

    unsigned long usbaddress = addr | ((unsigned long)modChan) << 16;
    int usbchan = ioChan;

    Xia_Util_Functions funcs;

    dxp_md_init_util(&funcs, NULL);
    ASSERT(data != NULL);


    status = dxp_usb2_set_address_cache(usbchan, udxp_io, usbaddress);

    if (status != DXP_SUCCESS) {
        sprintf(INFO_STRING, "Error setting address cahche for ioChan %d", ioChan);
        udxpc_log_error("dxp_read_block", INFO_STRING, status);
        return status;
    }

    status = udxp_io(&usbchan, &serial_read, &a, (void *)data, &dataWords);
    status += dxp_usb2_reset_bus(ioChan, modChan, udxp_io);

    if (status != DXP_SUCCESS) {
        sprintf(INFO_STRING, "Error reading %lu words from ioChan %d", n, ioChan);
        udxpc_log_error("dxp_usb_read_block", INFO_STRING, status);
        return status;
    }

    return DXP_SUCCESS;
}


XERXES_SHARED int dxp_usb_write_block(int ioChan, int modChan, DXP_MD_IO udxp_io,
                                      unsigned long addr, unsigned long n,
                                      unsigned short *data)
{
    int status;

    unsigned long a = DXP_A_IO;

    unsigned int serial_write = 1;
    unsigned int n_words;

    Xia_Util_Functions funcs;

    unsigned long usbaddress = addr | ((unsigned long)modChan) << 16;
    int usbchan = ioChan;

    ASSERT(udxp_io);
#ifdef XIA_ALPHA
    ASSERT((addr & ULTRA_CMD_USB_CONFIGURATION) > 0 ||
           (addr & ULTRA_CMD_UART2) > 0);
#endif
    ASSERT(n > 0);
    ASSERT(data);

    dxp_md_init_util(&funcs, NULL);

    status = dxp_usb2_set_address_cache(usbchan, udxp_io, usbaddress);

    if (status != DXP_SUCCESS) {
        sprintf(INFO_STRING, "Error setting USB2 address cache for "
                "ioChan = %d.", ioChan);
        udxpc_log_error("dxp_usb_write_block", INFO_STRING, status);
        return status;
    }

    n_words = (unsigned int)n;
    status = udxp_io(&usbchan, &serial_write, &a, data, &n_words);

    if (status != DXP_SUCCESS) {
        sprintf(INFO_STRING, "Error writing %u words (addr = %#lux) to "
                "ioChan = %d.", n_words, usbaddress, ioChan);
        udxpc_log_error("dxp_usb_write_block", INFO_STRING, status);
        return status;
    }

    /* Shouldn't need to reset the bus here as this routine currently
     * only supports acessing the USB chip directly. (If we ever allow
     * the upper address target to change from 0x04 to a microDXP
     * target, we may need to reset the bus after this access.
     */

    return DXP_SUCCESS;
}


/*
 * For the specified command, modify the transfer address for USB
 * targets. In general the device ID (modChan) goes in the high word
 * and a given UART is selected in the low word by command type.
 */
int dxp_usb2_reticulate_address(int modChan, byte_t *cmd, unsigned long *addr)
{
    /* Encode the device ID in Byte 2. */
    unsigned long high_word = ((unsigned long) modChan) << 16;

    ASSERT(addr != NULL);

    switch(*cmd) {
#ifdef XIA_ALPHA

    case CMD_GET_ALPHA_HV:
    case CMD_SET_ALPHA_HV:
    case CMD_TILT_IO:
    case CMD_GET_MB_ID:
        /* for I2C bus access not only the addresses need to be changed
           but also the commands, we are going to abandon this scheme when
           the current serial Alpha boards become obsolete, and we no longer
           need to support the commands through serial connection */
        *cmd       = CMD_ACCESS_I2C;
        *addr      = high_word | 0x03000000;
        break;

    case CMD_ALPHA_PULSER_ENABLE_DISABLE:
    case CMD_ALPHA_PULSER_CONFIG_1:
    case CMD_ALPHA_PULSER_CONFIG_2:
    case CMD_ALPHA_PULSER_SET_MODE:
    case CMD_ALPHA_PULSER_CONFIG_VETO:
    case CMD_ALPHA_PULSER_ENABLE_DISABLE_VETO:
    case CMD_ALPHA_PULSER_CONTROL:
        /* Pulser commands are redirected to UART 2. */
        *addr      = high_word | 0x02000000;
        break;

#endif /* XIA_ALPHA */

    default:
        /* for default UART operation add UART 1 to Byte 3 */
        *addr      = high_word | 0x01000000;
        break;
    }

    return DXP_SUCCESS;
}


/*
 * USB2-UART interface only
 * Give back control of the bus to the PIC by writing to address
 * This is needed after access to uDXP memory via parallel bus
 * Address range 0x0000 to 0x8000
 * e.g. reading of spectrum memory, alpha buffer memory with dxp_usb_read_block
 */
int dxp_usb2_reset_bus(int ioChan, int modChan, DXP_MD_IO udxp_io)
{

    int status;

    unsigned int usbaddress = RELEASE_IDMA_BUS_ADDR | ((unsigned long)modChan) << 16;
    int usbchan = ioChan;

    unsigned int serial_write = 1;
    unsigned long a = DXP_A_IO;

    unsigned int dummyWordLength = 1;
    unsigned short *dummyWord = NULL;

    Xia_Util_Functions funcs;


    dxp_md_init_util(&funcs, NULL);

    status = dxp_usb2_set_address_cache(usbchan, udxp_io, usbaddress);

    if (status != DXP_SUCCESS) {
        sprintf(INFO_STRING, "Error setting address cache for ioChan %d, modChan %d",
                ioChan, modChan);
        udxpc_log_error("dxp_usb2_reset_bus", INFO_STRING, status);
        return status;
    }

    dummyWord = (unsigned short *)udxpc_md_alloc(dummyWordLength * sizeof(unsigned short));
    dummyWord[0] = 0;

    status = udxp_io(&usbchan, &serial_write, &a, (void *)dummyWord,
                     &dummyWordLength);
    UDXP_FREE(dummyWord);

    if (status != DXP_SUCCESS) {
        sprintf(INFO_STRING, "Error writing to ioChan %d to reset bus", ioChan);
        udxpc_log_error("dxp_usb2_reset_bus", INFO_STRING, status);
        return status;
    }

    return DXP_SUCCESS;
}


/*
 * Send the command to ioChan
 */
int dxp_send_command(int ioChan, DXP_MD_IO udxp_io, unsigned long address,
                     byte_t cmd, unsigned int lenS, byte_t *send, byte_t ioFlags)
{
    int status;

    unsigned int i;

    unsigned int serial_write = 1;

    unsigned long a = DXP_A_IO;

    unsigned int totalCmdLen  = (unsigned int)(lenS + 5);
    unsigned int cmdWords = 0;

    byte_t *byteCmd = NULL;
    byte_t *byteCmdPtr = NULL;
    unsigned short *shortCmd = NULL;

    Xia_Util_Functions funcs;


    if (lenS > 0) {
        ASSERT(send != NULL);
    }

    dxp_md_init_util(&funcs, NULL);

    /* Build the command string to pass to the board. */
    byteCmd = (byte_t *)udxpc_md_alloc(totalCmdLen * sizeof(byte_t));

    if (!byteCmd) {
        sprintf(INFO_STRING, "Out-of-memory allocating %u bytes for 'byteCmd'",
                totalCmdLen);
        udxpc_log_error("dxp_send_command", INFO_STRING, DXP_NOMEM);
        return DXP_NOMEM;
    }

    status = dxp_build_cmdstr(cmd, (unsigned short)lenS, send, byteCmd);

    if (status != DXP_SUCCESS) {
        UDXP_FREE(byteCmd);
        sprintf(INFO_STRING, "Error building command: cmd = %u", cmd);
        udxpc_log_error("dxp_send_command", INFO_STRING, status);
        return status;
    }

    /* The MD layer requires the data array to be of size unsigned short */
    if (IS_USB(ioFlags)) {
        cmdWords = (unsigned int) (((float) totalCmdLen + 1.) / 2.);
    } else {
        cmdWords = totalCmdLen;
    }

    shortCmd = (unsigned short *)udxpc_md_alloc(cmdWords *
                                                sizeof(unsigned short));
    if (!shortCmd) {
        UDXP_FREE(byteCmd);
        status = DXP_NOMEM;
        sprintf(INFO_STRING, "Out-of-memory allocating %u bytes for 'shortCmd'",
                cmdWords * sizeof(unsigned short));
        udxpc_log_error("dxp_send_command", INFO_STRING, DXP_NOMEM);
        return DXP_NOMEM;
    }

    if (IS_USB(ioFlags)) {
        memcpy(shortCmd, byteCmd, totalCmdLen);
        /* Pad the last byte of shortCmd if sending an odd number of bytes */
        if (totalCmdLen < cmdWords * 2) {
            byteCmdPtr = (byte_t *)shortCmd;
            byteCmdPtr[totalCmdLen] = 0;
        }
    } else {
        for (i = 0; i < totalCmdLen; i++) {
            shortCmd[i] = (unsigned short)byteCmd[i];
        }
    }

    UDXP_FREE(byteCmd);

    if ((ioFlags & IO_NORMAL) != 0) {

        /* set address cache for USB2 */
        if (IS_USB(ioFlags)) {

            status = dxp_usb2_set_address_cache(ioChan, udxp_io, address);

            if (status != DXP_SUCCESS) {
                UDXP_FREE(shortCmd);
                sprintf(INFO_STRING, "Error setting address cahche for ioChan %d", ioChan);
                udxpc_log_error("dxp_send_command", INFO_STRING, status);
                return status;
            }
        }

        /* Send the command */
        status = udxp_io(&ioChan, &serial_write, &a, (void *)shortCmd,
                         &cmdWords);

        if (status != DXP_SUCCESS) {
            UDXP_FREE(shortCmd);
            sprintf(INFO_STRING, "Error sending command %#x to ioChan %d",
                    cmd, ioChan);
            udxpc_log_error("dxp_send_command", INFO_STRING, status);
            return status;
        }
    }

    UDXP_FREE(shortCmd);

    return DXP_SUCCESS;
}


/*
 * Read a response from ioChan
 */
int dxp_read_response(int ioChan, DXP_MD_IO udxp_io, unsigned long address,
                      unsigned int lenR, byte_t *receive, byte_t ioFlags)
{
    int status;

    unsigned int i;

    unsigned int serial_read  = 0;

    unsigned int retWords     = 0;

    unsigned long WAIT_IN_MS = 10000;
    unsigned long a;

    unsigned short *shortRet = NULL;

    Xia_Util_Functions funcs;

    dxp_md_init_util(&funcs, NULL);

    if (lenR > 0) {
        ASSERT(receive != NULL);
    }

    if (IS_USB(ioFlags)) {
        retWords = (unsigned int) (((float) lenR + 1.) / 2.);
        a = DXP_A_IO;
    } else {
        retWords = lenR;
        /* NOTE that this parameter is *addr in dxp_md_usb2_io
         * And wait_in_ms in dxp_md_serial_io (timeout only used in MD_IO_READ)
         */
        a = WAIT_IN_MS;
    }

    shortRet = (unsigned short *)udxpc_md_alloc(retWords * sizeof(unsigned short));

    if (!shortRet) {
        status = DXP_NOMEM;
        sprintf(INFO_STRING, "Out-of-memory allocating %u bytes for 'shortRet'",
                lenR * sizeof(unsigned short));
        udxpc_log_error("dxp_read_response", INFO_STRING, DXP_NOMEM);
        return DXP_NOMEM;
    }


    if ((ioFlags & IO_NORMAL) != 0) {

        /* Set address cache for USB 2 */
        if (IS_USB(ioFlags)) {
            status = dxp_usb2_set_address_cache(ioChan, udxp_io, address);

            if (status != DXP_SUCCESS) {
                UDXP_FREE(shortRet);
                sprintf(INFO_STRING, "Error setting address cahche for ioChan %d", ioChan);
                udxpc_log_error("dxp_read_response", INFO_STRING, status);
                return status;
            }
        }

        /* Receive the response */
        status = udxp_io(&ioChan, &serial_read, &a, (void *)shortRet, &retWords);

        if (status != DXP_SUCCESS) {
            UDXP_FREE(shortRet);
            sprintf(INFO_STRING, "Error reading response from ioChan %d", ioChan);
            udxpc_log_error("dxp_read_response", INFO_STRING, status);
            return status;
        }
    }

    /* Unpack from ushort for USB2 */
    if (IS_USB(ioFlags)) {

        memcpy(receive, shortRet, lenR);

    } else {

        for (i = 0; i < lenR; i++) {
            receive[i] = (byte_t)(shortRet[i] & 0xFF);
        }

    }

    UDXP_FREE(shortRet);

    return DXP_SUCCESS;
}


/*
 * Simple verification of the response
 */
int dxp_verify_response(byte_t cmd,
                        unsigned int lenS, byte_t *send,
                        unsigned int lenR, byte_t *receive)
{
    unsigned int j;

    unsigned int retLen  = 0;

    byte_t retChksm     = 0x00;
    byte_t calcChksm    = 0x00;

    Xia_Util_Functions funcs;

    dxp_md_init_util(&funcs, NULL);

    if (lenS > 0) ASSERT(send != NULL);
    if (lenR > 0) ASSERT(receive != NULL);

    /* Check for the presence of the escape character */
    if (receive[0] != 0x1B) {
        sprintf(INFO_STRING, "cmd = %#x, lenS = %u, send = %p, lenR = %u, "
                "receive = %p", cmd, lenS, send, lenR, receive);
        udxpc_log_debug("dxp_read_response", INFO_STRING);

        sprintf(INFO_STRING, "receive[0] = %#x, receive[1] = %#x, receive[2] = %#x",
                receive[0], receive[1], receive[2]);
        udxpc_log_debug("dxp_read_response", INFO_STRING);

        sprintf(INFO_STRING, "Escape character (0x1B) is missing from response");
        udxpc_log_error("dxp_read_response", INFO_STRING, DXP_MISSING_ESC);
        return DXP_MISSING_ESC;
    }

    retLen = receive[2] | ((unsigned int)(receive[3]) << 8);
    retLen += RECV_BASE;

    retChksm = receive[retLen - 1];
    calcChksm = dxp_compute_chksum((unsigned short)(retLen - 2), receive + 1);

    if (retChksm != calcChksm) {
        /* This is a rare enough error that we should write the entire buffer and
         * all other relevant data to the log.
         */
        for (j = 0; j < lenS; j++) {
            sprintf(INFO_STRING, "send[%u] = %#x", j, send[j]);
            udxpc_log_debug("dxp_read_response", INFO_STRING);
        }

        for (j = 0; j < lenR; j++) {
            sprintf(INFO_STRING, "receive[%u] = %#x", j, receive[j]);
            udxpc_log_debug("dxp_read_response", INFO_STRING);
        }

        sprintf(INFO_STRING, "Checksum mismatch: retChksm = %u, calcChksm = %u",
                retChksm, calcChksm);
        udxpc_log_error("dxp_read_response", INFO_STRING, DXP_CHECKSUM);
        return DXP_CHECKSUM;
    }

    /* Hardware reported an error here */
    if (receive[4] != 0) {
        sprintf(INFO_STRING, "Hardware reported status = %u", receive[4]);
        udxpc_log_error("dxp_read_response", INFO_STRING, DXP_UDXP);

        /* Dump out the commands here */
        for (j = 0; j < lenS; j++) {
            sprintf(INFO_STRING, "send[%u] = %#x", j, send[j]);
            udxpc_log_debug("dxp_read_response", INFO_STRING);
        }

        for (j = 0; j < lenR; j++) {
            sprintf(INFO_STRING, "receive[%u] = %#x", j, receive[j]);
            udxpc_log_debug("dxp_read_response", INFO_STRING);
        }

        return DXP_UDXP;
    }

    return DXP_SUCCESS;
}


/*
 * USB2-UART interface  only
 * Set the USB2 IO address prior to a USB2 IO call
 */
int dxp_usb2_set_address_cache(int ioChan, DXP_MD_IO udxp_io, unsigned long address)
{
    int status;

    unsigned int f;
    unsigned int len;

    unsigned long a;

    Xia_Util_Functions funcs;

    /* Initialize our function pointers
     * so that we can allocate memory
     * safely.
     */
    dxp_md_init_util(&funcs, NULL);


    /* Write the address to the cache. */
    a   = DXP_A_ADDR;
    f   = DXP_F_IGNORE;
    len = 0;

    status = udxp_io(&ioChan, &f, &a, &address, &len);

    if (status != DXP_SUCCESS) {
        sprintf(INFO_STRING, "Error setting usb address to %#lux", address);
        udxpc_log_error("dxp_set_usb2_address_cache", INFO_STRING, status);
        return status;
    }

    return DXP_SUCCESS;
}


/*
 * This routine converts a byte array
 * (which is generally made up of unsigned chars)
 * into a full-on string (made up of chars).
 */
XERXES_SHARED int dxp_byte_to_string(unsigned char *bytes, unsigned int len, char *string)
{
    unsigned int i;


    for (i = 0; i < len; i++) {
        string[i] = (char)bytes[i];
    }

    /* In case the original string
     * is malformed...
     */
    string[len - 1] = '\0';

    return DXP_SUCCESS;
}


/*
 * Read the board information and store the variant information in the
 * cache.
 */
static int dxp__update_version_cache(int ioChan, DXP_MD_IO io, byte_t flags,
                                     Xia_Util_Functions funcs)
{
    int status;

    unsigned long addr = 0;

    DEFINE_CMD_ZERO_SEND(CMD_GET_BOARD_INFO, 27);


    if (IS_USB(flags)) {
        status = dxp_usb2_reticulate_address(ioChan, &cmd, &addr);

        if (status != DXP_SUCCESS) {
            sprintf(INFO_STRING, "Error updating transfer address for a USB "
                    "board while attempting to update the variant cache "
                    "for ioChan %d", ioChan);
            udxpc_log_error("dxp__update_version_cache", INFO_STRING, status);
            return status;
        }
    }

    status = dxp_send_command(ioChan, io, addr, cmd, lenS, NULL, flags);

    if (status != DXP_SUCCESS) {
        sprintf(INFO_STRING, "Error sending board information command to "
                "get the variant for ioChan %d", ioChan);
        udxpc_log_error("dxp__update_version_cache", INFO_STRING, status);
        return status;
    }

    status = dxp_read_response(ioChan, io, addr, lenR, receive, flags);

    if (status != DXP_SUCCESS) {
        sprintf(INFO_STRING, "Error reading response to board information "
                "command for getting the variant for ioChan %d", ioChan);
        udxpc_log_error("dxp__update_version_cache", INFO_STRING, status);
        return status;
    }

    status = dxp_verify_response(cmd, lenS, NULL, lenR, receive);

    if (status != DXP_SUCCESS) {
        sprintf(INFO_STRING, "Error validating response to board information "
                "command for getting the variant for ioChan %d", ioChan);
        udxpc_log_error("dxp__update_version_cache", INFO_STRING, status);
        return status;
    }

    ASSERT(VERSION_CACHE[ioChan][PIC_VARIANT] == UDXP_VERSION_NOT_READ);
    VERSION_CACHE[ioChan][PIC_VARIANT] = receive[5];
    VERSION_CACHE[ioChan][PIC_MAJOR] = receive[6];
    VERSION_CACHE[ioChan][PIC_MINOR] = receive[7];
    VERSION_CACHE[ioChan][DSP_VARIANT] = receive[8];
    VERSION_CACHE[ioChan][DSP_MAJOR] = receive[9];
    VERSION_CACHE[ioChan][DSP_MINOR] = receive[10];

    return DXP_SUCCESS;
}


XERXES_SHARED boolean_t dxp_has_direct_mca_readout(int ioChan)
{
#ifdef XIA_ALPHA
    UNUSED(ioChan);
    return TRUE_;
#else
    ASSERT(VERSION_CACHE[ioChan][PIC_VARIANT] != UDXP_VERSION_NOT_READ);
    return (VERSION_CACHE[ioChan][PIC_VARIANT] & (1 << 5)) > 0;
#endif
}

XERXES_SHARED boolean_t dxp_is_supermicro(int ioChan)
{
#ifdef XIA_ALPHA
    UNUSED(ioChan);
    return FALSE_;
#else
    ASSERT(VERSION_CACHE[ioChan][PIC_MAJOR] != UDXP_VERSION_NOT_READ);
    return VERSION_CACHE[ioChan][PIC_MAJOR] > 2;
#endif
}

XERXES_SHARED boolean_t dxp_has_direct_trace_readout(int ioChan)
{
#ifdef XIA_ALPHA
    UNUSED(ioChan);
    return FALSE_;
#else
    byte_t *version = VERSION_CACHE[ioChan];
    ASSERT(version[PIC_MAJOR] != UDXP_VERSION_NOT_READ);
    return (version[PIC_MAJOR] == 3 && version[PIC_MINOR] >= 6) ||
           version[PIC_MAJOR] > 3;
#endif
}

XERXES_SHARED unsigned long dxp_dsp_coderev(int ioChan)
{

#ifdef XIA_ALPHA
    UNUSED(ioChan);
    return 0;
#else
    byte_t *version = VERSION_CACHE[ioChan];
    unsigned long dsp_version = 0;
    ASSERT(VERSION_CACHE[ioChan][DSP_MAJOR] != UDXP_VERSION_NOT_READ);
    dsp_version = ((unsigned long)version[DSP_MAJOR] << 8) +
                  (unsigned long)version[DSP_MINOR];
    return dsp_version;
#endif

}
