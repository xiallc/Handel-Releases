/*
 * Copyright (c) 2006-2016 XIA LLC
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


#include "windows.h"
#include "setupapi.h"

#ifdef CYGWIN32
#undef _WIN32
#endif /* CYGWIN32 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#ifdef _WIN32
#pragma warning(disable : 4214)
#pragma warning(disable : 4201)
#endif /* _WIN32 */

#include "cyioctl.h"

#ifdef _WIN32
#pragma warning(default : 4214)
#pragma warning(default : 4201)
#endif /* _WIN32 */

#include "xia_assert.h"

#include "xia_usb2_api.h"
#include "xia_usb2_errors.h"
#include "xia_usb2_private.h"


/* Prototypes */
static int xia_usb2__send_setup_packet(HANDLE h, unsigned long addr,
                                       unsigned long n_bytes, byte_t rw_flag);
static int xia_usb2__xfer(HANDLE h, byte_t ep, DWORD n_bytes, byte_t *buf,
                     DWORD *bytes_ret, DWORD timeout, boolean_t no_timeout_log);
static int xia_usb2__small_read_xfer(HANDLE h, DWORD n_bytes, byte_t *buf, DWORD *bytes_ret);
static int xia_usb2__set_max_packet_size(HANDLE h);
static void xia_usb2__log_error(const char* fmt, ...);
static void xia_usb2__flush_read_ep(HANDLE h);
static void print_hexbinary_lines(byte_t *buffer, int size, int line_length);
static void print_debug(const char* fmt, ...);

#ifdef XIA_USB2_DUMP_HELPERS
/* Currently we aren't using these routines for anything, but since they
 * were a pain to write (lots of boring details), I thought we should keep
 * them around in case we need to debug any of these data structures in the
 * future.
 */
static void xia_usb2__dump_config_desc(xia_usb2_configuration_descriptor_t *d);
static void xia_usb2__dump_interf_desc(xia_usb2_interface_descriptor_t *d);
static void xia_usb2__dump_ep_desc(xia_usb2_endpoint_descriptor_t *d);
#endif /* XIA_USB2_DUMP_HELPERS */


/* This is the Device setup class GUID supplied by Cypress to enumerate
 * USB devices by SetupDiGetClassDevs. The winapi constant
 * GUID_DEVINTERFACE_USB_DEVICE should also work.
 */
static GUID CYPRESS_GUID = {0xae18aa60, 0x7f6a, 0x11d4,
    {0x97, 0xdd, 0x0, 0x1, 0x2, 0x29, 0xb9, 0x59}
};

/* It is more efficient to transfer a complete buffer of whatever the
 * max packet size, even if the amount of bytes requested is less then that.
 */
static unsigned long XIA_USB2_SMALL_READ_PACKET_SIZE = 0;

/* The text of last printf info is reset every time the status code is updated
 * so that callers can retrieve it in case of an error status
 */
static char info_string[INFO_LEN];

/*
 * Opens the device with the specified number (dev) and returns
 * a valid HANDLE to the device or NULL if the device could not
 * be opened.
 */
XIA_EXPORT int XIA_API xia_usb2_open(int dev, HANDLE *h)
{
    HDEVINFO dev_info;

    SP_DEVICE_INTERFACE_DATA intfc_data;

    SP_DEVINFO_DATA dev_info_data;

    PSP_INTERFACE_DEVICE_DETAIL_DATA intfc_detail_data;

    BOOL success;

    DWORD intfc_detail_size;
    DWORD err;

    int status;


    dev_info = SetupDiGetClassDevs(&CYPRESS_GUID, NULL, NULL,
                                   DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

    if (dev_info == NULL) {
        xia_usb2__log_error("No info returned from SetupDiGetClassDevs.");
        return XIA_USB2_GET_CLASS_DEVS;
    }

    intfc_data.cbSize = sizeof(intfc_data);

    success = SetupDiEnumDeviceInterfaces(dev_info, 0, &CYPRESS_GUID, dev,
                                          &intfc_data);

    if (!success) {
        err = GetLastError();
        SetupDiDestroyDeviceInfoList(dev_info);
        xia_usb2__log_error("SetupDiEnumDeviceInterfaces error = %#lx", err);
        return XIA_USB2_ENUM_DEV_INTFC;
    }

    /* Call this twice: once to get the size of the returned buffer and
     * once to fill the buffer.
     */
    success = SetupDiGetDeviceInterfaceDetail(dev_info, &intfc_data, NULL, 0,
                                              &intfc_detail_size, NULL);

    /* Per Microsoft's documentation, this function should return an
     * ERROR_INSUFFICIENT_BUFFER value.
     */
    if (success) {
        SetupDiDestroyDeviceInfoList(dev_info);
        xia_usb2__log_error("Status magically was true!");
        return XIA_USB2_DEV_INTFC_DETAIL;
    }

    if (GetLastError() != 0x7A) {
        SetupDiDestroyDeviceInfoList(dev_info);
        xia_usb2__log_error("Last error wasn't 0x7A!");
        return XIA_USB2_DEV_INTFC_DETAIL;
    }

    intfc_detail_data = malloc(intfc_detail_size);
    if (intfc_detail_data == NULL) {
        SetupDiDestroyDeviceInfoList(dev_info);
        xia_usb2__log_error("Error allocating memory!");
        return XIA_USB2_NO_MEM;
    }

    intfc_detail_data->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);

    dev_info_data.cbSize = sizeof(dev_info_data);

    success = SetupDiGetDeviceInterfaceDetail(dev_info, &intfc_data,
                                              intfc_detail_data,
                                              intfc_detail_size, NULL,
                                              &dev_info_data);

    if (!success) {
        err = GetLastError();
        free(intfc_detail_data);
        SetupDiDestroyDeviceInfoList(dev_info);
        xia_usb2__log_error("SetupDiGetDeviceInterfaceDetail error = %#lx", err);
        return XIA_USB2_DEV_INTFC_DETAIL;
    }

    *h = CreateFile(intfc_detail_data->DevicePath, GENERIC_WRITE | GENERIC_READ,
                    FILE_SHARE_WRITE | FILE_SHARE_READ, NULL,
                    OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

    free(intfc_detail_data);
    SetupDiDestroyDeviceInfoList(dev_info);

    if (*h == INVALID_HANDLE_VALUE) {
        err = GetLastError();
        xia_usb2__log_error("CreateFile error = %#lx", err);
        return XIA_USB2_INVALID_HANDLE;
    }

    status = xia_usb2__set_max_packet_size(*h);

    if (status != XIA_USB2_SUCCESS) {
        xia_usb2_close(*h);
        return status;
    }

    xia_usb2__flush_read_ep(*h);
    return XIA_USB2_SUCCESS;
}


/*
 * Closes a device handle (h) previously opened via. xia_usb2_open().
 */
XIA_EXPORT int XIA_API xia_usb2_close(HANDLE h)
{
    BOOL status;

    DWORD err;

    status = CloseHandle(h);

    if (!status) {
        err = GetLastError();
        xia_usb2__log_error("CloseHandle error = %#lx", err);
        return XIA_USB2_CLOSE_HANDLE;
    }

    return XIA_USB2_SUCCESS;
}


/*
 * Read the specified number of bytes from the specified address and
 * into the specified buffer.
 *
 * buf is expected to be allocated by the calling routine.
 */
XIA_EXPORT int XIA_API xia_usb2_read(HANDLE h, unsigned long addr,
                                     unsigned long n_bytes,
                                     byte_t *buf)
{
    unsigned long bytes_ret = 0;
    int status;

    status = xia_usb2_readn(h, addr, n_bytes, buf, &bytes_ret);
    if (status != XIA_USB2_SUCCESS)
        return status;

    /* Prior to adding xia_usb2_readn, the Windows driver never
     * checked the length, just whether there was an error. Just warn,
     * don't error, for backward compatiblity. Skip the common case where
     * an even number of bytes are allocated for odd number of expected
     * response.
     */
    if (bytes_ret != n_bytes && (bytes_ret != n_bytes - 1)) {
        print_debug("USB bulk read returned %lu bytes, expected %lu\n",
               bytes_ret, n_bytes);
    }

    return XIA_USB2_SUCCESS;
}

/*
 * Read the specified number of bytes from the specified address and
 * into the specified buffer.
 *
 * buf is expected to be allocated by the calling routine.
 *
 * Returns the actual number of bytes read in n_bytes_read. This can
 * be less than the number of bytes requested if the transfer succeeds
 * but the device returns fewer bytes for whatever reason, e.g.
 * microDXP not padding out a 512 byte transfer for smaller commands.
 */
XIA_EXPORT int XIA_API xia_usb2_readn(HANDLE h, unsigned long addr,
                                      unsigned long n_bytes,
                                      byte_t *buf, unsigned long *n_bytes_read)
{
    int status;
    DWORD bytes_ret;

    ASSERT(XIA_USB2_SMALL_READ_PACKET_SIZE == 512 ||
           XIA_USB2_SMALL_READ_PACKET_SIZE == 64);


    if (h == NULL) {
        return XIA_USB2_NULL_HANDLE;
    }

    if (n_bytes == 0) {
        return XIA_USB2_ZERO_BYTES;
    }

    if (buf == NULL) {
        return XIA_USB2_NULL_BUFFER;
    }

    if (n_bytes < XIA_USB2_SMALL_READ_PACKET_SIZE) {
        status = xia_usb2__send_setup_packet(h, addr,
                                             XIA_USB2_SMALL_READ_PACKET_SIZE,
                                             XIA_USB2_SETUP_FLAG_READ);

        if (status != XIA_USB2_SUCCESS) {
            return status;
        }

        status = xia_usb2__small_read_xfer(h, (DWORD)n_bytes, buf, &bytes_ret);

    } else {
        status = xia_usb2__send_setup_packet(h, addr, n_bytes,
                                             XIA_USB2_SETUP_FLAG_READ);

        if (status != XIA_USB2_SUCCESS) {
            return status;
        }

        status = xia_usb2__xfer(h, XIA_USB2_READ_EP, (DWORD)n_bytes, buf,
                                &bytes_ret, XIA_USB2_TIMEOUT, FALSE_);
    }

    if (status == XIA_USB2_SUCCESS) {
        *n_bytes_read = bytes_ret;
    }

    return status;
}


/*
 * Writes the requested buffer to the requested address.
 */
XIA_EXPORT int XIA_API xia_usb2_write(HANDLE h, unsigned long addr,
                                      unsigned long n_bytes, byte_t *buf)
{
    int status;
    DWORD bytes_ret;

    if (h == INVALID_HANDLE_VALUE) {
        return XIA_USB2_NULL_HANDLE;
    }

    if (n_bytes == 0) {
        return XIA_USB2_ZERO_BYTES;
    }

    if (buf == NULL) {
        return XIA_USB2_NULL_BUFFER;
    }

    status = xia_usb2__send_setup_packet(h, addr, n_bytes,
                                         XIA_USB2_SETUP_FLAG_WRITE);

    if (status != XIA_USB2_SUCCESS) {
        return status;
    }

    status = xia_usb2__xfer(h, XIA_USB2_WRITE_EP, (DWORD)n_bytes, buf,
                            &bytes_ret, XIA_USB2_TIMEOUT, FALSE_);

    return status;
}


/*
 * Sends an XIA-specific setup packet to the "setup" endpoint. This
 * is the first stage of our two-part process for transferring data to
 * and from the board.
 */
static int xia_usb2__send_setup_packet(HANDLE h, unsigned long addr,
                                       unsigned long n_bytes, byte_t rw_flag)
{
    int status;

    byte_t pkt[XIA_USB2_SETUP_PACKET_SIZE];
    DWORD bytes_ret;


    ASSERT(n_bytes != 0);
    ASSERT(rw_flag < 2);


    pkt[0] = (byte_t)(addr & 0xFF);
    pkt[1] = (byte_t)((addr >> 8) & 0xFF);
    pkt[2] = (byte_t)(n_bytes & 0xFF);
    pkt[3] = (byte_t)((n_bytes >> 8) & 0xFF);
    pkt[4] = (byte_t)((n_bytes >> 16) & 0xFF);
    pkt[5] = (byte_t)((n_bytes >> 24) & 0xFF);
    pkt[6] = rw_flag;
    pkt[7] = (byte_t)((addr >> 16) & 0xFF);
    pkt[8] = (byte_t)((addr >> 24) & 0xFF);

    status = xia_usb2__xfer(h, XIA_USB2_SETUP_EP, XIA_USB2_SETUP_PACKET_SIZE,
                            pkt, &bytes_ret, XIA_USB2_TIMEOUT, FALSE_);

    return status;
}


/*
 * Wrapper around the low-level transfer to the USB device. Handles
 * the configuration of the SINGLE_TRANSFER structure as required by the
 * Cypress driver.
 *
 * The timeout is implemented through setting a FILE_FLAG_OVERLAPPED flag
 * when calling CreateFile, passing the OVERLAPPED structure to DeviceIoControl,
 * Then calling WaitForSingleObject with a specified timeout value.
 *
 * A no_timeout_log option can be toggled to avoid logging timeout errors. This
 * should only used in combination with xia_usb2__flush_read_ep to avoid
 * printing out debug messages when there is not error.
 *
 */
static int xia_usb2__xfer(HANDLE h, byte_t ep, DWORD n_bytes, byte_t *buf,
                     DWORD *bytes_ret, DWORD timeout, boolean_t no_timeout_log)
{
    SINGLE_TRANSFER st;
    OVERLAPPED overlapped;

    DWORD err;
    DWORD wait;

    BOOL success;
    int status;

    ASSERT(n_bytes != 0);
    ASSERT(buf != NULL);

    memset(&st, 0, sizeof(st));
    st.ucEndpointAddress = ep;

    memset(&overlapped, 0, sizeof(overlapped));

    overlapped.hEvent = CreateEvent(NULL, TRUE_, FALSE_, NULL);

    if (overlapped.hEvent == INVALID_HANDLE_VALUE) {
        err = GetLastError();
        xia_usb2__log_error("Create overlapped event error = %#lx", err);
        return XIA_USB2_XFER;
    }

    success = DeviceIoControl(h, IOCTL_ADAPT_SEND_NON_EP0_DIRECT,
                              &st, sizeof(st), buf, n_bytes, bytes_ret,
                              &overlapped);

    /* In the unlikely event that the transfer completes immediately
     * there is no need to wait, otherwise we have to poll WaitForSingleObject
     */
    if (success) {
        CloseHandle(overlapped.hEvent);
        return XIA_USB2_SUCCESS;
    }

    err = GetLastError();

    if (err != ERROR_IO_PENDING) {
        CloseHandle(overlapped.hEvent);
        xia_usb2__log_error("DeviceIoControl last error code = %#lx", err);
        err = ERROR_FILE_NOT_FOUND ? XIA_USB2_DEVICE_NOT_FOUND : XIA_USB2_XFER;
        return err;
    }

    wait = WaitForSingleObject(overlapped.hEvent, timeout);

    switch (wait) {
    case WAIT_OBJECT_0: /* Success */
        break;
    case WAIT_TIMEOUT:  /* Timed out, need to cancel IO */
        if (!no_timeout_log)
            print_debug("Timed out waiting for transfer\n");
    default:
        err = GetLastError();
        if (!no_timeout_log)
            xia_usb2__log_error("Wait for transfer failed, wait = %#lx, "
            "error = %#lx. Cancelling...", wait, err);

        status = XIA_USB2_XFER;

        /* The cancel, wait, and check procedure is required to ensure the kernel
         * is done with our overlapped structure address and buffer. See
         * http://blogs.msdn.com/b/oldnewthing/archive/2011/02/02/10123392.aspx.
         *
         * Normally one would use CancelIo, but the Cypress driver does not seem
         * to support this API. The use of IOCTL_ADAPT_ABORT_PIPE was borrowed from
         * Cypress's .NET driver source.
         */
        success = DeviceIoControl(h, IOCTL_ADAPT_ABORT_PIPE, &st.ucEndpointAddress,
                                  1, NULL, 0, bytes_ret, NULL);

        if (!success) {
            err = GetLastError();

            if (err == ERROR_FILE_NOT_FOUND) {
                xia_usb2__log_error("IOCTL_ADAPT_ABORT_PIPE failed with ERROR_FILE_NOT_FOUND\n");
                status = XIA_USB2_DEVICE_NOT_FOUND;
            } else {
                print_debug("IOCTL_ADAPT_ABORT_PIPE failed, result = %#lx\n", err);
            }
        }

        WaitForSingleObject(overlapped.hEvent, INFINITE);
        CloseHandle(overlapped.hEvent);

        return status;
    }

    success = GetOverlappedResult(h, &overlapped, bytes_ret, FALSE_);

    if (!success) {
        err = GetLastError();
        CloseHandle(overlapped.hEvent);
        xia_usb2__log_error("Get overlapped result = %#lx", wait, err);
        return XIA_USB2_XFER;
    }

    CloseHandle(overlapped.hEvent);

    return XIA_USB2_SUCCESS;
}


/*
 * Performs a fast read of a small -- less than the max packet
 * size -- packet.
 *
 * Since the performance of USB2 with small packets is poor, it is faster
 * to read a larger block and extract the small number of bytes we actually
 * want.
 */
static int xia_usb2__small_read_xfer(HANDLE h, DWORD n_bytes, byte_t *buf, DWORD *bytes_ret)
{
    int status;

    byte_t *big_packet = NULL;

    ASSERT(XIA_USB2_SMALL_READ_PACKET_SIZE == 512 ||
           XIA_USB2_SMALL_READ_PACKET_SIZE == 64);
    ASSERT(n_bytes < XIA_USB2_SMALL_READ_PACKET_SIZE);
    ASSERT(n_bytes != 0);
    ASSERT(buf != NULL);


    big_packet = malloc(XIA_USB2_SMALL_READ_PACKET_SIZE);

    if (!big_packet) {
        return XIA_USB2_NO_MEM;
    }

    /* Initialize buffer to a fixed pattern to identify source of read errors
     * in case the buffer is not filled completely */
    memset(big_packet, 0xCD, XIA_USB2_SMALL_READ_PACKET_SIZE);

    status = xia_usb2__xfer(h, XIA_USB2_READ_EP, XIA_USB2_SMALL_READ_PACKET_SIZE,
                        big_packet, bytes_ret, XIA_USB2_TIMEOUT, FALSE_);

    if (status != XIA_USB2_SUCCESS) {
        free(big_packet);
        return status;
    }

    *bytes_ret = n_bytes;
    memcpy(buf, big_packet, n_bytes);
    free(big_packet);

    return XIA_USB2_SUCCESS;
}


/*
 * XIA USB2 devices transfer data to the host (this code) via EP6. It should
 * be sufficient to read the wMaxPacketSize field from the EP6 descriptor
 * and just use the largest packet size supported by the device which will
 * be either a full-speed or a high-speed device.
 */
static int xia_usb2__set_max_packet_size(HANDLE h)
{
    size_t total_desc_size = 0;
    size_t total_transfer_size = 0;

    int i;

    SINGLE_TRANSFER *transfer = NULL;

    BOOL success;

    DWORD bytes_ret;
    DWORD err;

    xia_usb2_endpoint_descriptor_t *ep = NULL;


    /* This size is XIA-specific: We care about the first configuration's
     * first interface and the endpoints that we use.
     */
    total_desc_size = sizeof(xia_usb2_configuration_descriptor_t) +
                      sizeof(xia_usb2_interface_descriptor_t) +
                      XIA_USB2_NUM_ENDPOINTS * sizeof(xia_usb2_endpoint_descriptor_t);

    total_transfer_size = sizeof(SINGLE_TRANSFER) + total_desc_size;

    transfer = (SINGLE_TRANSFER *)malloc(total_transfer_size);

    if (!transfer) {
        return XIA_USB2_NO_MEM;
    }

    memset(transfer, 0, total_transfer_size);

    transfer->SetupPacket.bmRequest = XIA_USB2_GET_DESCRIPTOR_REQTYPE;
    transfer->SetupPacket.bRequest  = XIA_USB2_GET_DESCRIPTOR_REQ;
    transfer->SetupPacket.wValue    =
        (unsigned short)(XIA_USB2_CONFIGURATION_DESCRIPTOR_TYPE << 8);
    transfer->SetupPacket.wIndex    = 0;
    transfer->SetupPacket.wLength   = (unsigned short)total_desc_size;
    transfer->SetupPacket.ulTimeOut = 1000;
    transfer->ucEndpointAddress     = XIA_USB2_CONTROL_EP;
    transfer->IsoPacketLength       = 0;
    transfer->BufferOffset          = (DWORD) sizeof(SINGLE_TRANSFER);
    transfer->BufferLength          = (DWORD) total_desc_size;

    success = DeviceIoControl(h, IOCTL_ADAPT_SEND_EP0_CONTROL_TRANSFER,
                              transfer, (DWORD) total_transfer_size, transfer,
                              (DWORD) total_transfer_size, &bytes_ret, NULL);

    if (!success) {
        free(transfer);
        err = GetLastError();
        xia_usb2__log_error("Setting up max packet size error = %#lx", err);
        return XIA_USB2_XFER;
    }

    for (i = 0; i < XIA_USB2_NUM_ENDPOINTS; i++) {
        /* Pick the endpoints out from the end of the configuration and
         * interface descriptors.
         */
        ep = (xia_usb2_endpoint_descriptor_t *)
             ((byte_t *)transfer + sizeof(SINGLE_TRANSFER) +
              sizeof(xia_usb2_configuration_descriptor_t) +
              sizeof(xia_usb2_interface_descriptor_t) +
              i * sizeof(xia_usb2_endpoint_descriptor_t));

        if (ep->bEndpointAddress == XIA_USB2_READ_EP) {
            XIA_USB2_SMALL_READ_PACKET_SIZE = ep->wMaxPacketSize;
            free(transfer);
            return XIA_USB2_SUCCESS;
        }
    }

    free(transfer);

    /* It is impossible for an XIA device to not have the read EP. */
    FAIL();

    /* Won't get here, but keeps the compiler happy. */
    return XIA_USB2_SUCCESS;
}

/*
 * Set the error info string before returning status code
 */
static void xia_usb2__log_error(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    vsprintf(info_string, fmt, args);
    info_string[sizeof(info_string) - 1] = '\0';

    printf(fmt, args);
    printf("\n");

    va_end(args);
}

/*
 * Return the content of the last info_string printed to screen
 * This is overwitten every time the api function is called
 */
XIA_EXPORT char* xia_usb2_get_last_error()
{
    return info_string;
}

#ifdef XIA_USB2_DUMP_HELPERS
/*
 * Debug dump of a configuration descriptor.
 */
static void xia_usb2__dump_config_desc(xia_usb2_configuration_descriptor_t *d)
{
    printf("\nConfiguration Descriptor\n");
    printf("bLength             = %hu\n", (unsigned short)d->bLength);
    printf("bDescriptorType     = %#hx\n", (unsigned short)d->bDescriptorType);
    printf("wTotalLength        = %hu\n", d->wTotalLength);
    printf("bNumInterfaces      = %hu\n", (unsigned short)d->bNumInterfaces);
    printf("bConfigurationValue = %hu\n",
           (unsigned short)d->bConfigurationValue);
    printf("iConfiguration      = %hu\n", (unsigned short)d->iConfiguration);
    printf("bmAttributes        = %#hx\n", (unsigned short)d->bmAttributes);
    printf("bMaxPower           = %hu mA\n",
           (unsigned short)(d->bMaxPower * 2));
    printf("\n");
}


/*
 * Debug dump of an interface descriptor.
 */
static void xia_usb2__dump_interf_desc(xia_usb2_interface_descriptor_t *d)
{
    printf("\nInterface Descriptor\n");
    printf("bLength            = %hu\n", (unsigned short)d->bLength);
    printf("bDescriptorType    = %#hx\n", (unsigned short)d->bDescriptorType);
    printf("bInterfaceNumber   = %hu\n", (unsigned short)d->bInterfaceNumber);
    printf("bAlternateSetting  = %hu\n", (unsigned short)d->bAlternateSetting);
    printf("bNumEndpoints      = %hu\n", (unsigned short)d->bNumEndpoints);
    printf("bInterfaceClass    = %#hx\n", (unsigned short)d->bInterfaceClass);
    printf("bInterfaceSubClass = %#hx\n",
           (unsigned short)d->bInterfaceSubClass);
    printf("bInterfaceProtocol = %#hx\n",
           (unsigned short)d->bInterfaceProtocol);
    printf("iInterface         = %hu\n", (unsigned short)d->iInterface);
    printf("\n");
}


/*
 * Debug dump of an endpoint descriptor.
 */
static void xia_usb2__dump_ep_desc(xia_usb2_endpoint_descriptor_t *d)
{
    printf("\nEndpoint Descriptor\n");
    printf("bLength          = %hu\n", (unsigned short)d->bLength);
    printf("bDescriptorType  = %#hx\n", (unsigned short)d->bDescriptorType);
    printf("bEndpointAddress = %#hx\n", (unsigned short)d->bEndpointAddress);
    printf("wMaxPacketSize   = %u\n", d->wMaxPacketSize);
    printf("bInterval        = %hu\n", (unsigned short)d->bInterval);
    printf("\n");
}
#endif /* XIA_USB2_DUMP_HELPERS */


/*
 * Occationally when user press CTRL+C to end a program, communication might be
 * broken off leaving unread data in device buffer, this would cause unexpected
 * responses to be sent back for subsequent connections. This function reads a
 * a large packet directly from XIA_USB2_READ_EP with a short timeout to clear
 * the buffer if  possible.
 */
static void xia_usb2__flush_read_ep(HANDLE h)
{
    int status;
    int loop = 0, maxloop = 64;
    int total_len = 0;
    DWORD rlen;

    int packet_size = XIA_USB2_SMALL_READ_PACKET_SIZE;
    byte_t big_packet[512];

    clock_t start = clock();
    double exec_seconds;

    memset(big_packet, 0xBC, packet_size);

    /* Use a very short timeout initially */
    status = xia_usb2__xfer(h, XIA_USB2_READ_EP, (DWORD)packet_size, big_packet,
                                &rlen, 10, TRUE_);

    while (rlen > 0) {
        print_debug("xia_usb2__flush_read_ep %d bytes\n", rlen);
#ifdef _DEBUG
        print_hexbinary_lines(big_packet, rlen, 0x20);
#endif
        total_len += (int)rlen;
        status = xia_usb2__xfer(h, XIA_USB2_READ_EP, (DWORD)packet_size, big_packet,
                                &rlen, 100, TRUE_);

        /* In theory we should only need to flush EP1 buffer 4 times,
         * one for each memory block on the device, we'll use a generous
         * maximum here.
         */
        loop++;
        if (loop > maxloop) {
            print_debug("xia_usb2__flush_read_ep usb2 flush exceeded maximum "
                        "%d", maxloop);
            break;
        }
    }

    exec_seconds = (double)(clock() - start) * 1000.0 / CLOCKS_PER_SEC;
    if (total_len >0)
        print_debug("xia_usb2__flush_read_ep exec time %.4f ms %d bytes\n",
                exec_seconds, total_len);

}

/*
 * Print binary data in hex format in lines of length line_length
 */
static void print_hexbinary_lines(byte_t *buffer, int size, int line_length)
{
    int j;
    for (j = 0; j < size; j++) {
        printf("%02X", buffer[j]);
        if ((j + 1) % line_length == 0) printf("\n");
    }
    if (size % line_length != 0) printf("\n");
}

/*
 * Print message to stdout for DEBUG builds
 */
static void print_debug(const char* fmt, ...)
{
#ifdef _DEBUG
    va_list args;
    va_start(args, fmt);

    vprintf(fmt, args);

    va_end(args);
#else
    UNUSED(fmt);
#endif
}
