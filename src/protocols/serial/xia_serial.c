/*
 * Copyright (c) 2002-2004 X-ray Instrumentation Associates
 * Copyright (c) 2005-2015 XIA LLC
 * All rights reserved
 *
 * NOT COVERED UNDER THE BSD LICENSE. NOT FOR RELEASE TO CUSTOMERS.
 */

#include <windows.h>

/* Headers for the COMM-DRV/Lib Library */
#define MSWIN
#define MSWIN32
#define MSWINDLL

#include "comm.h"

#include <stdio.h>

#include "Dlldefs.h"
#include "seriallib.h"
#include "xia_assert.h"

/* PROTOTYPES */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

XIA_EXPORT int XIA_API InitSerialPort(unsigned short port, unsigned long baud);
XIA_EXPORT serial_read_error_t* XIA_API ReadSerialPort(unsigned short port,
                                                       unsigned long size,
                                                       unsigned char* data);
XIA_EXPORT int XIA_API ReadSerialPortCS(unsigned short port, unsigned long size,
                                        char* data, serial_read_error_t* result);
XIA_EXPORT int XIA_API WriteSerialPort(unsigned short port, unsigned long size,
                                       unsigned char* data);
XIA_EXPORT int XIA_API CloseSerialPort(unsigned short port);
XIA_EXPORT int XIA_API CheckAndClearTransmitBuffer(unsigned short port);
XIA_EXPORT int XIA_API CheckAndClearReceiveBuffer(unsigned short port);
XIA_EXPORT int XIA_API SetTime(unsigned short port, int interval);
XIA_EXPORT int XIA_API SetTimeoutInMS(unsigned short port, double ms);
XIA_EXPORT int XIA_API CheckTime(unsigned short port);
XIA_EXPORT int XIA_API GetErrors(unsigned short port, unsigned short* blk);
XIA_EXPORT int XIA_API NumBytesAtSerialPort(unsigned short port,
                                            unsigned long* numBytes);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/* Globals */
static unsigned short timerBlk[10];

static serial_read_error_t err;

/*
 * Initialize the serial port located at COMx where x = port - 1.
 *
 * The baud parameter is currently unused as we hard-code 115.2 kbps
 * currently. We also hard-code the traditional 8N1 configuration.
 *
 * We always use the raw COM port value (port) as the identifier that is
 * passed to the CDRV serial port functions.
 */
XIA_EXPORT int XIA_API InitSerialPort(unsigned short port, unsigned long baud) {
    int status;
    int baudConstant;

    status =
        InitializePort((int) port, port - 1, 0, 0, CARD_WINAPI, 0, 32768, 32768, 0);

    if (status != RS232ERR_NONE) {
        printf("xia_serial.c InitSerialPort serial %d failed, status = %d\n", port - 1,
               status);
        return SERIAL_INIT;
    }

    /* Convert baud to corresponding baud constant */
    switch (baud) {
        case 110:
            baudConstant = BAUD110;
            break;
        case 150:
            baudConstant = BAUD150;
            break;
        case 300:
            baudConstant = BAUD300;
            break;
        case 600:
            baudConstant = BAUD600;
            break;
        case 1200:
            baudConstant = BAUD1200;
            break;
        case 2400:
            baudConstant = BAUD2400;
            break;
        case 4800:
            baudConstant = BAUD4800;
            break;
        case 9600:
            baudConstant = BAUD9600;
            break;
        case 19200:
            baudConstant = BAUD19200;
            break;
        case 38400:
            baudConstant = BAUD38400;
            break;
        case 57600:
            baudConstant = BAUD57600;
            break;
        case 115200:
            baudConstant = BAUD115200;
            break;
        case 14400:
            baudConstant = BAUD14400;
            break;
        case 28800:
            baudConstant = BAUD28800;
            break;
        default:
            baudConstant = BAUDUSER00; /* User baud rate */
            break;
    }

    /* Set custom baud value if it's not one of the predefined constants */
    if (baudConstant == BAUDUSER00) {
        status = SetCustomBaud((int) port, BAUDUSER00, baud);
    }

    if (status != RS232ERR_NONE) {
        CloseSerialPort(port);
        return SERIAL_PORT_SETUP;
    }

    status = SetPortCharacteristics((int) port, baudConstant, PAR_NONE, LENGTH_8,
                                    STOPBIT_1, PROT_NONNON);

    if (status != RS232ERR_NONE) {
        CloseSerialPort(port);
        return SERIAL_PORT_SETUP;
    }

    /* Timer resolution should be 1 ms ticks */
    status = CdrvSetTimerResolution((int) port, 1);

    if (status != RS232ERR_NONE) {
        CloseSerialPort(port);
        return SERIAL_PORT_SETUP;
    }

    return SERIAL_SUCCESS;
}

/*
 * This routine reads the number of bytes specified
 * in size from the currently open serial port.
 * InitSerialPort() must be called prior to using this
 * routine.
 */
XIA_EXPORT serial_read_error_t* XIA_API ReadSerialPort(unsigned short port,
                                                       unsigned long size,
                                                       unsigned char* data) {
    int actual = 0;

    ser_rs232_getpacket((int) port, 0, (unsigned char*) 0);

    actual = GetPacket((int) port, (int) size, (char*) data);

    err.actual = actual;
    err.expected = (int) size;

    if (actual != (int) size) {
        err.status = SERIAL_READ;
        err.bytes_in_recv_buf = BytesInReceiveBuffer((int) port);
        err.size_recv_buf = ReceiveBufferSize((int) port);
        err.is_hardware_overrun = IsOverrunError((int) port);
        err.is_software_overrun = IsInputOverrun((int) port);
    } else {
        err.status = SERIAL_SUCCESS;
    }

    return &err;
}

/*
 * Same as ReadSerialPort() but returns the result struct as an out parameter.
 * This is a convenience so .NET code can call this with a ref parameter instead
 * of catching a returned pointer (the latter would require compiling with
 * /unsafe).
 */
XIA_EXPORT int XIA_API ReadSerialPortCS(unsigned short port, unsigned long size,
                                        char* data, serial_read_error_t* result) {
    ASSERT(data != NULL);
    ASSERT(result != NULL);

    *result = *ReadSerialPort(port, size, (unsigned char*) data);
    return result->status;
}

/*
 * This routine writes the specified byte
 * array to the currently open serial port.
 * InitSerialPort() must be called prior
 * to using this routine.
 */
XIA_EXPORT int XIA_API WriteSerialPort(unsigned short port, unsigned long size,
                                       unsigned char* data) {
    int actual;

    actual = PutPacket((int) port, (int) size, (char*) data);

    ser_rs232_putpacket((int) port, 0, (unsigned char*) 0);

    if (actual != (int) size) {
        printf("actual(write) = %d\n", actual);
        return SERIAL_WRITE;
    }

    return SERIAL_SUCCESS;
}

XIA_EXPORT int XIA_API NumBytesAtSerialPort(unsigned short port,
                                            unsigned long* numBytes) {
    int cnt = 0;

    cnt = BytesInReceiveBuffer((int) port);
    *numBytes = (unsigned long) cnt;

    return SERIAL_SUCCESS;
}

XIA_EXPORT int XIA_API CloseSerialPort(unsigned short port) {
    int status;

    status = UnInitializePort((int) port);

    if (status != 0) {
        return SERIAL_UNINIT;
    }

    return SERIAL_SUCCESS;
}

/*
 * This routine checks the transmit buffer for any
 * spurious bytes and clears it.
 */
XIA_EXPORT int XIA_API CheckAndClearTransmitBuffer(unsigned short port) {
    int status;

    status = IsTransmitBufferEmpty((int) port);

    /* Need to clear out these bytes */
    if (status == 0) {
        status = FlushTransmitBuffer((int) port);
    }

    return SERIAL_SUCCESS;
}

/*
 * This routine checks the receive buffer for any
 * spurious bytes and clears it.
 */
XIA_EXPORT int XIA_API CheckAndClearReceiveBuffer(unsigned short port) {
    int status;

    status = IsReceiveBufferEmpty((int) port);

    /* Need to clear out these bytes */
    if (status == 0) {
        status = FlushReceiveBuffer((int) port);
    }

    return SERIAL_SUCCESS;
}

/*
 * Converts a timeout in ms, into ticks that the CDRV lib can use.
 */
XIA_EXPORT int XIA_API SetTimeoutInMS(unsigned short port, double ms) {
    int status;
    int ticks = (int) ms;

    status = SetTimeout((int) port, ticks);

    if (status != RS232ERR_NONE) {
        return SERIAL_COMM_TIMEOUTS;
    }

    return SERIAL_SUCCESS;
}

/*
 * Thin wrapper around the CDRV func.
 */
XIA_EXPORT int XIA_API SetTime(unsigned short port, int interval) {
    int status;

    status = CdrvSetTime((int) port, interval, timerBlk);

    if (status == -1) {
        return SERIAL_PORT_NOINIT;
    }

    return SERIAL_SUCCESS;
}

/*
 * Thin wrapper around the CDRV func. Should only
 * be called after SetTime() has been called. See
 * the COMM-DRV/Lib documentation for more
 * information.
 */
XIA_EXPORT int XIA_API CheckTime(unsigned short port) {
    int status;

    UNUSED(port);

    status = CdrvCheckTime(timerBlk);

    return status;
}

/*
 * This routine calls all of the possible error check functions
 * provided by COMM-DRV/Lib and packs the status into a 16-bit
 * word.
 */
XIA_EXPORT int XIA_API GetErrors(unsigned short port, unsigned short* blk) {
    int status;

    status = IsAllDataOut((int) port);
    *blk = (unsigned short) status;

    status = IsBreak((int) port);
    *blk = (unsigned short) (*blk | (status << 1));

    status = IsCarrierDetect((int) port);
    *blk = (unsigned short) (*blk | (status << 2));

    status = IsCts((int) port);
    *blk = (unsigned short) (*blk | (status << 3));

    status = IsDsr((int) port);
    *blk = (unsigned short) (*blk | (status << 4));

    status = IsFramingError((int) port);
    *blk = (unsigned short) (*blk | (status << 5));

    status = IsInputOverrun((int) port);
    *blk = (unsigned short) (*blk | (status << 6));

    status = IsOverrunError((int) port);
    *blk = (unsigned short) (*blk | (status << 7));

    status = IsParityError((int) port);
    *blk = (unsigned short) (*blk | (status << 8));

    status = IsPortAvailable((int) port);
    *blk = (unsigned short) (*blk | (status << 9));

    status = IsReceiveBufferEmpty((int) port);
    *blk = (unsigned short) (*blk | (status << 10));

    status = IsRing((int) port);
    *blk = (unsigned short) (*blk | (status << 11));

    status = IsTransmitBufferEmpty((int) port);
    *blk = (unsigned short) (*blk | (status << 12));

    return SERIAL_SUCCESS;
}
