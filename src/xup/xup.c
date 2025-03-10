/*
 * Copyright (c) 2002-2004 X-ray Instrumentation Associates
 * Copyright (c) 2005-2012 XIA LLC
 * All rights reserved
 *
 * NOT COVERED UNDER THE BSD LICENSE. NOT FOR RELEASE TO CUSTOMERS.
 */

#include <math.h>
#include <stdio.h>
#include <time.h>

#include "xia_assert.h"
#include "xia_common.h"
#include "xia_handel.h"
#include "xia_xup.h"

#include "psl_common.h"

#include "handel_errors.h"

#include "xerxes.h"
#include "xerxes_errors.h"

#include "udxp_command.h"

#include "xup_writer.h"

static void xupInitKeyRing(void);

static byte_t xupDecryptByte(byte_t idx, byte_t cipher);
static byte_t xupEncryptByte(byte_t idx, byte_t plain);
static unsigned short xupByteToUS(byte_t lo, byte_t hi);
static unsigned long xupByteToLong(byte_t lo0, byte_t lo1, byte_t hi0, byte_t hi1);

static int xupDownload(int detChan, byte_t offset, unsigned long size, byte_t data[]);

static int xupDoFiPPI(int detChan, unsigned short fipNum, unsigned long size,
                      byte_t data[]);
static int xupDoFiPPIGen(int detChan, unsigned short fipNum, unsigned long size,
                         byte_t data[]);

static int xupDoI2C(int detChan, unsigned short addr, unsigned long size,
                    byte_t data[]);

static int xupLoadFlashImage(int detChan, byte_t* img);

static int xupReadI2CToBuffer(int detChan, byte_t* i2c);
static int xupReadFlashToBuffer(int detChan, byte_t* flash);

static int xupWriteBackupXUP(byte_t* i2c, byte_t* flash, byte_t* sn,
                             unsigned short* chksum, struct tm* tstr);
static byte_t xupXORChksum(unsigned long size, byte_t* data);

static int xupReadSerialNumber(int detChan, byte_t* sn);

static int xupWriteBackupAccessFile(struct tm* tstr, byte_t* sn, unsigned short chksum);

static void xupCalculateAccessCode(byte_t* sn, unsigned short chksum, byte_t* acode);

static int xupReadFlash(int detChan, unsigned short addr, unsigned long size,
                        byte_t* data);
static int xupWriteFlash(int detChan, unsigned short addr, unsigned long size,
                         byte_t* data);

/* Decoder routines */
static int decode001(char* xup, int detChan);

decodeFP decoders[] = {NULL, decode001};

/* Downloading routines */
static int download01(int detChan, unsigned long size, byte_t buffer[]);
static int download03(int detChan, unsigned long size, byte_t buffer[]);
static int download06(int detChan, unsigned long size, byte_t buffer[]);
static int download07(int detChan, unsigned long size, byte_t buffer[]);
static int download09(int detChan, unsigned long size, byte_t buffer[]);
static int download0A(int detChan, unsigned long size, byte_t buffer[]);
static int download0C(int detChan, unsigned long size, byte_t buffer[]);
static int download0D(int detChan, unsigned long size, byte_t buffer[]);
static int download0E(int detChan, unsigned long size, byte_t buffer[]);
static int download10(int detChan, unsigned long size, byte_t buffer[]);
static int download11(int detChan, unsigned long size, byte_t buffer[]);
static int download12(int detChan, unsigned long size, byte_t buffer[]);
static int download14(int detChan, unsigned long size, byte_t buffer[]);
static int download16(int detChan, unsigned long size, byte_t buffer[]);
static int download17(int detChan, unsigned long size, byte_t buffer[]);
static int download18(int detChan, unsigned long size, byte_t buffer[]);

downloadFP downloaders[] = {
    NULL,       download01, NULL,       download03, NULL,       NULL,       download06,
    download07, NULL,       download09, download0A, NULL,       download0C, download0D,
    download0E, NULL,       download10, download11, download12, NULL,       download14,
    NULL,       download16, download17, download18,
};

/* Macros */
#define NUM_DECODERS ((sizeof(decoders) / sizeof(decoders[0])) - 1)

#define BAK_NAME_LEN 11

Key_t keyRing[1];

unsigned int OFFSET[3] = {0x0800, 0x5A80, 0xAD00};

/* The backupPath, if defined, is used by the backup code to determined
 * where the backup files should be written.
 */
char* backupPath = NULL;

/*
 * This routine processes a binary XUP payload
 * See the XUP spec. for more information. This routine
 * is flexible enough to handle multiple versions and
 * formats of the XUP file.
 */
int xupProcess(int detChan, char* xup) {
    int status;

    byte_t ver = 0x00;

    byte_t gid[2];

    FILE* fp = NULL;

    UNUSED(detChan);

    xupInitKeyRing();

    fp = fopen(xup, "rb");
    if (!fp) {
        status = XIA_OPEN_FILE;
        sprintf(info_string, "Error opening %s for processing", xup);
        pslLogError("xupProcess", info_string, status);
        return status;
    }

    fread(gid, 1, sizeof(byte_t) * 2, fp);

    /* Decode what version of the XUP spec this
     * package was created with.
     */
    fread(&ver, 1, sizeof(byte_t), fp);

    fclose(fp);

    /* Vector to the proper decoding routine
     * based on the version number.
     */
    if (ver > NUM_DECODERS) {
        status = XIA_XUP_VERSION;
        pslLogError("xupProcess", "XUP version is not supported", status);
        return status;
    }

    status = decoders[ver](xup, detChan);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error decoding XUP (%s), version = %#x", xup, ver);
        pslLogError("xupProcess", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * The backups are, themselves, written out as XUPs w/ a matching
 * access file. The XUP needs to be written out with the I2C section
 * first, followed by a reboot command, followed by the flash section.
 */
int xupWriteBackups(int detChan, char* xup) {
    int status;

    unsigned short chksum = 0x0000;
    unsigned short gid = 0x0000;

    byte_t sn[SERIAL_NUM_LEN];
    byte_t i2c[I2C_MEMORY_SIZE_BYTES];
    byte_t flash[FLASH_MEMORY_SIZE_BYTES];

    /* The time structure is important so that the XUP file
     * name and the access file name use the same time value.
     */
    struct tm* tstr = NULL;

    time_t current;

    FILE* fp = NULL;

    UNUSED(xup);

    xupInitKeyRing();

    time(&current);
    tstr = localtime(&current);

    if (xup != NULL) {
        /* Check to see if the GID is
         * the "backup" GID. If it is,
         * then we want to skip the rest of
         * this function since we'll just be
         * backing up the backup, which is
         * pretty unnecessary.
         */
        fp = fopen(xup, "rb");

        if (fp == NULL) {
            status = XIA_OPEN_FILE;
            pslLogError("xupWriteBackups", "Error checking XUP", status);
            return status;
        }

        fread(&gid, 2, 1, fp);
        fclose(fp);

        if (gid == 0xFFFF) {
            pslLogInfo("xupWriteBackups", "Skipping backup phase");
            return XIA_SUCCESS;
        }
    }

    /* Read I2C and Flash into buffers */
    status = xupReadI2CToBuffer(detChan, i2c);

    if (status != XIA_SUCCESS) {
        pslLogError("xupWriteBackups", "Error reading out buffers", status);
        return status;
    }

    status = xupReadFlashToBuffer(detChan, flash);

    if (status != XIA_SUCCESS) {
        pslLogError("xupWriteBackups", "Error reading out buffers", status);
        return status;
    }

    /* We need the serial number for both the backup XUP
     * filename and the access file.
     */
    status = xupReadSerialNumber(detChan, sn);

    if (status != XIA_SUCCESS) {
        pslLogError("xupWriteBackups", "Error reading out data from memory", status);
        return status;
    }

    /* Build backup sections */
    status = xupWriteBackupXUP(i2c, flash, sn, &chksum, tstr);

    if (status != XIA_SUCCESS) {
        pslLogError("xupWriteBackups", "Error writing backup information", status);
        return status;
    }

    /* Build access file */
    status = xupWriteBackupAccessFile(tstr, sn, chksum);

    if (status != XIA_SUCCESS) {
        pslLogError("xupWriteBackups", "Error writing access file", status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Writes information about an XUP to the
 * History sector of the board.
 */
int xupWriteHistory(int detChan, char* xup) {
    int status;

    unsigned short nEntries = 0;
    unsigned short gid = 0;
    unsigned short tstamp = 0x0000;

    unsigned int i;
    unsigned int j;
    unsigned int addr = 0;
    unsigned int lenS = 3;
    unsigned int lenR = RECV_BASE + 1 + (2 * MAX_FLASH_READ);

    unsigned long idx = 0;

    byte_t cmd = CMD_READ_FLASH;

    byte_t send[3];
    byte_t quadrant[QUADRANT_SIZE + 3];
    byte_t receive[RECV_BASE + 1 + (2 * MAX_FLASH_READ)];
    byte_t receiveW[RECV_BASE + 1];
    byte_t sector[SECTOR_SIZE_BYTES];

    time_t current;

    struct tm* timeStr = NULL;

    FILE* fp = NULL;

    for (i = 0, addr = XUP_HISTORY_ADDR;
         addr < (unsigned int) ((SECTOR_SIZE_BYTES / 2) + XUP_HISTORY_ADDR);
         i++, addr += MAX_FLASH_READ) {
        send[0] = (byte_t) (addr & 0xFF);
        send[1] = (byte_t) ((addr >> 8) & 0xFF);
        send[2] = (byte_t) MAX_FLASH_READ;

        status = dxp_cmd(&detChan, &cmd, &lenS, send, &lenR, receive);

        if (status != DXP_SUCCESS) {
            pslLogError("xupWriteHistory", "Error reading history information", status);
            return status;
        }

        memcpy(sector + (i * MAX_FLASH_READ * 2), receive + RECV_BASE,
               MAX_FLASH_READ * 2);
    }

    /* The history sector is laid out as follows:
     * word[0] = # of used entries
     * word[1] = GID entry #1
     * word[2] = Timestamp entry #1
     * etc...
     */
    nEntries = xupByteToUS(sector[0], sector[1]);

    /* Read out GID */
    fp = fopen(xup, "rb");
    if (!fp) {
        status = XIA_OPEN_FILE;
        pslLogError("xupWriteHistory", "Error reading history from board", status);
        return status;
    }

    fread(&gid, 1, sizeof(unsigned short), fp);

    fclose(fp);

    /* Create timestamp */
    time(&current);
    timeStr = localtime(&current);

    /* Not sure if it is safe to manipulate the
     * struct tm components directly, but here
     * goes nothing. It looks like the C Std defines
     * the elements of struct tm across implementations
     * ...
     */
    tstamp = (unsigned short) (timeStr->tm_year - 100);
    tstamp |= (unsigned short) ((timeStr->tm_mday) << 7);
    tstamp |= (unsigned short) ((timeStr->tm_mon + 1) << 12);

    /* Write new entry */
    idx = (unsigned long) ((nEntries * 4) + 2);
    sector[idx++] = (byte_t) (gid & 0xFF);
    sector[idx++] = (byte_t) ((gid >> 8) & 0xFF);
    sector[idx++] = (byte_t) (tstamp & 0xFF);
    sector[idx++] = (byte_t) ((tstamp >> 8) & 0xFF);
    sector[0] = (byte_t) (++nEntries & 0xFF);
    sector[1] = (byte_t) ((nEntries >> 8) & 0xFF);

    /* Write sector back to the board */
    cmd = CMD_WRITE_FLASH;
    lenS = QUADRANT_SIZE + 3;
    lenR = RECV_BASE + 1;

    for (j = 0; j < NUM_QUADRANTS; j++) {
        quadrant[0] = (byte_t) j;
        quadrant[1] = (byte_t) (XUP_HISTORY_ADDR & 0xFF);
        quadrant[2] = (byte_t) ((XUP_HISTORY_ADDR >> 8) & 0xFF);
        memcpy(quadrant + 3, sector + (j * QUADRANT_SIZE), QUADRANT_SIZE);

        status = dxp_cmd(&detChan, &cmd, &lenS, quadrant, &lenR, receiveW);

        if (status != DXP_SUCCESS) {
            pslLogError("xupWriteHistory", "Error writing history information", status);
            return status;
        }
    }

    return XIA_SUCCESS;
}

/*
 * Decodes version 1 XUP files
 */
static int decode001(char* xup, int detChan) {
    int status;

    unsigned long pSize = 0x00000000;
    unsigned long j;

    unsigned short i;
    unsigned short pSections = 0x0000;
    /*    unsigned short pSize     = 0x0000; */

    byte_t offset = 0x00;
    byte_t sectionsLo = 0x00;
    byte_t sectionsHi = 0x00;
    byte_t size0 = 0x00;
    byte_t size1 = 0x00;
    byte_t size2 = 0x00;
    byte_t size3 = 0x00;
    byte_t bbyte = 0x00;

    byte_t header[6];
    byte_t sections[2];
    byte_t size[4];

    byte_t* buffer = NULL;

    FILE* fp = NULL;

    sprintf(info_string, "Downloading: %s", xup);
    pslLogDebug("<XUP>", info_string);

    fp = fopen(xup, "rb");
    if (!fp) {
        status = XIA_OPEN_FILE;
        sprintf(info_string, "Error opening %s", xup);
        pslLogError("<XUP>", info_string, status);
        return status;
    }

    fread(header, 1, sizeof(byte_t) * 6, fp);

    fread(sections, 1, sizeof(byte_t) * 2, fp);

    sectionsLo = xupDecryptByte(0, sections[0]);
    sectionsHi = xupDecryptByte(0, sections[1]);
    pSections = xupByteToUS(sectionsLo, sectionsHi);

    for (i = 0; i < pSections; i++) {
        fread(&offset, 1, sizeof(byte_t), fp);

        offset = xupDecryptByte(0, offset);

        fread(size, 1, sizeof(byte_t) * 4, fp);

        size0 = xupDecryptByte(0, size[0]);
        size1 = xupDecryptByte(0, size[1]);
        size2 = xupDecryptByte(0, size[2]);
        size3 = xupDecryptByte(0, size[3]);
        pSize = xupByteToLong(size0, size1, size2, size3);

        if (pSize != 0) {
            buffer = (byte_t*) MALLOC(pSize * sizeof(byte_t));
            if (!buffer) {
                fclose(fp);
                status = XIA_NOMEM;
                pslLogError("<XUP>", "Out-of-memory creating buffer", status);
                return status;
            }

            for (j = 0; j < pSize; j++) {
                fread(&bbyte, 1, sizeof(byte_t), fp);
                buffer[j] = xupDecryptByte(0, bbyte);
            }

            status = xupDownload(detChan, offset, pSize, buffer);

            if (status != XIA_SUCCESS) {
                utils->funcs->dxp_md_free(buffer);
                fclose(fp);
                sprintf(info_string, "Error downloading XUP to detChan %d", detChan);
                pslLogError("<XUP>", info_string, status);
                return status;
            }

            utils->funcs->dxp_md_free(buffer);
            buffer = NULL;
        }
    }

    fclose(fp);

    return XIA_SUCCESS;
}

static void xupInitKeyRing(void) {
    /* Version 001 */
    keyRing[0].size = 8;
    keyRing[0].ptr = 0;
    keyRing[0].key[0] = 0xF6;
    keyRing[0].key[1] = 0x37;
    keyRing[0].key[2] = 0xAC;
    keyRing[0].key[3] = 0xDD;
    keyRing[0].key[4] = 0x05;
    keyRing[0].key[5] = 0xC2;
    keyRing[0].key[6] = 0x1F;
    keyRing[0].key[7] = 0x61;
}

static byte_t xupDecryptByte(byte_t idx, byte_t cipher) {
    byte_t plain = 0x00;
    byte_t p = keyRing[idx].ptr;

    plain = (byte_t) (cipher ^ keyRing[idx].key[p]);

    if (p == keyRing[idx].size - 1) {
        keyRing[idx].ptr = 0;
    } else {
        keyRing[idx].ptr++;
    }

    return plain;
}

/*
 * This is the same as xupDecryptByte(), but only because
 * we are using a simple XOR cipher right now. In the future
 * if we choose to use something more complicated then we
 * will probably need separate routines that don't do the
 * same thing.
 */
static byte_t xupEncryptByte(byte_t idx, byte_t plain) {
    byte_t cipher = 0x00;
    byte_t p = keyRing[idx].ptr;

    cipher = (byte_t) (plain ^ keyRing[idx].key[p]);

    if (p == keyRing[idx].size - 1) {
        keyRing[idx].ptr = 0;
    } else {
        keyRing[idx].ptr++;
    }

    return cipher;
}

/*
 * This routine turns 2 bytes
 * into an unsigned short.
 */
static unsigned short xupByteToUS(byte_t lo, byte_t hi) {
    unsigned short result;

    result = (unsigned short) (lo | ((unsigned short) hi << 8));
    return result;
}

/*
 * This routine dispatched the data to the
 * proper downloading routine.
 */
static int xupDownload(int detChan, byte_t offset, unsigned long size, byte_t data[]) {
    int status;

    status = (downloaders[offset])(detChan, size, data);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Unable to download XUP to detChan %d", detChan);
        pslLogError("<XUP>", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This routine downloads buffer to the
 * FiPPI0 data location.
 */
static int download0C(int detChan, unsigned long size, byte_t buffer[]) {
    int status;

    status = xupDoFiPPI(detChan, 0, size, buffer);

    if (status != XIA_SUCCESS) {
        pslLogError("<XUP>", "Error downloading XUP", status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Since all of the FiPPI downloading is
 * essentially the same, modulo a memory address,
 * I am creating this routine as a choke-point for
 * downloading FiPPI data to the board.
 */
static int xupDoFiPPI(int detChan, unsigned short fipNum, unsigned long size,
                      byte_t data[]) {
    int status;

    unsigned int i;
    unsigned int j;
    unsigned int addr = (unsigned int) (OFFSET[fipNum] + DATA_ADDR_OFFSET);
    unsigned int lenS = QUADRANT_SIZE + 3;
    unsigned int lenR = RECV_BASE + 1;

    byte_t cmd = CMD_WRITE_FLASH;

    byte_t fippi[NUM_SECTORS_FOR_FIPPI * BYTES_PER_SECTOR];
    byte_t send[QUADRANT_SIZE + 3];
    byte_t receive[RECV_BASE + 1];

    /* The deal here is that the actual transfer is a pain if
     * your buffer doesn't break on a sector boundary, so we
     * don't try to be clever about only transferring "size" amount
     * of FiPPI data and then 0x00 for everything else. Instead, we
     * just pre-build a buffer equal to the max. size of the
     * FiPPI data section and set it to zero, then we write the
     * data into the buffer and transfer the whole thing.
     */
    memset(fippi, 0x0000, sizeof(fippi) / sizeof(fippi[0]));
    memcpy(fippi, data, size);

    for (i = 0; i < NUM_SECTORS_FOR_FIPPI; i++, addr += WORDS_PER_SECTOR) {
        for (j = 0; j < NUM_QUADRANTS; j++) {
            send[0] = (byte_t) j;
            send[1] = (byte_t) (addr & 0xFF);
            send[2] = (byte_t) ((addr >> 8) & 0xFF);
            memcpy(send + 3, fippi + ((j * QUADRANT_SIZE) + (i * BYTES_PER_SECTOR)),
                   QUADRANT_SIZE);

            status = dxp_cmd(&detChan, &cmd, &lenS, send, &lenR, receive);

            if (status != DXP_SUCCESS) {
                pslLogError("<XUP>", "Transfer error loading XUP", status);
                return status;
            }
        }
    }

    return XIA_SUCCESS;
}

/*
 */
static int download06(int detChan, unsigned long size, byte_t buffer[]) {
    int status;

    /* At this point, the buffer should be nothing more
     * then a flash image. Let's do a sanity check to
     * make sure that everything is consistent...
     */
    if (size != FLASH_MEMORY_SIZE_BYTES) {
        status = XIA_SIZE_MISMATCH;
        pslLogError("<XUP>", "Size mismatch", status);
        return status;
    }

    status = xupLoadFlashImage(detChan, buffer);

    if (status != XIA_SUCCESS) {
        pslLogError("<XUP>", "Error loading data into memory", status);
        return status;
    }

    return XIA_SUCCESS;
}

static int download07(int detChan, unsigned long size, byte_t buffer[]) {
    int status;

    status = xupWriteFlash(detChan, 0x0400, size, buffer);

    if (status != XIA_SUCCESS) {
        pslLogError("<XUP>", "Error loading data into memory", status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Download the FiPPI0 General Data information.
 */
static int download09(int detChan, unsigned long size, byte_t buffer[]) {
    int status;

    status = xupDoFiPPIGen(detChan, 0, size, buffer);

    if (status != XIA_SUCCESS) {
        pslLogError("<XUP>", "Error downloading XUP", status);
        return status;
    }

    return XIA_SUCCESS;
}

static int download0A(int detChan, unsigned long size, byte_t buffer[]) {
    int status;

    status = xupWriteFlash(detChan, 0x0980, size, buffer);

    if (status != XIA_SUCCESS) {
        pslLogError("<XUP>", "Error writing data into memory", status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Downloads the FiPPI general data info for
 * the specified FiPPI. Obviously, since this
 * code is the same for all 3 FiPPIs, it has
 * been isolated in its own routine.
 */
static int xupDoFiPPIGen(int detChan, unsigned short fipNum, unsigned long size,
                         byte_t data[]) {
    int status;

    unsigned int j;
    unsigned int addr = (unsigned int) OFFSET[fipNum];
    unsigned int lenS = QUADRANT_SIZE + 3;
    unsigned int lenR = RECV_BASE + 1;

    byte_t cmd = CMD_WRITE_FLASH;

    byte_t header[BYTES_PER_SECTOR];
    byte_t send[QUADRANT_SIZE + 3];
    byte_t receive[RECV_BASE + 1];

    /* The deal here is that the actual transfer is a pain if
     * your buffer doesn't break on a sector boundary, so we
     * don't try to be clever about only transferring "size" amount
     * of FiPPI data and then 0x00 for everything else. Instead, we
     * just pre-build a buffer equal to the max. size of the
     * FiPPI data section and set it to zero, then we write the
     * data into the buffer and transfer the whole thing.
     */
    memset(header, 0x0000, sizeof(header) / sizeof(header[0]));
    memcpy(header, data, size);

    for (j = 0; j < NUM_QUADRANTS; j++) {
        send[0] = (byte_t) j;
        send[1] = (byte_t) (addr & 0xFF);
        send[2] = (byte_t) ((addr >> 8) & 0xFF);
        memcpy(send + 3, header + (j * QUADRANT_SIZE), QUADRANT_SIZE);

        status = dxp_cmd(&detChan, &cmd, &lenS, send, &lenR, receive);

        if (status != DXP_SUCCESS) {
            pslLogError("<XUP>", "Transfer error loading XUP", status);
            return status;
        }
    }

    return XIA_SUCCESS;
}

/*
 * This routine calculates the actual checksum
 * of the XUP file and then compares it to the
 * checksum supplied in the file. If the values
 * agree then the routine returns TRUE_, if not
 * FALSE_ is returned.
 */
boolean_t xupIsChecksumValid(char* xup) {
    int status;

    FILE* fp = NULL;

    fp = fopen(xup, "rb");
    if (!fp) {
        status = XIA_OPEN_FILE;
        pslLogError("<XUP>", "Error loading XUP", status);
        return FALSE_;
    }

    /* XXX Not sure what to about this
     * yet. The _right_ way to do this
     * is to parse the whole file into
     * some sort of nice structure, but...
     */

    fclose(fp);

    return TRUE_;
}

/*
 * Download FiPPI 1 general data
 */
static int download0D(int detChan, unsigned long size, byte_t buffer[]) {
    int status;

    status = xupDoFiPPIGen(detChan, 1, size, buffer);

    if (status != XIA_SUCCESS) {
        pslLogError("<XUP>", "Error downloading XUP", status);
        return status;
    }

    return XIA_SUCCESS;
}

static int download0E(int detChan, unsigned long size, byte_t buffer[]) {
    int status;

    status = xupWriteFlash(detChan, 0x5C00, size, buffer);

    if (status != XIA_SUCCESS) {
        pslLogError("<XUP>", "Error downloading data to memory", status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Download FiPPI 1 data block
 */
static int download10(int detChan, unsigned long size, byte_t buffer[]) {
    int status;

    status = xupDoFiPPI(detChan, 1, size, buffer);

    if (status != XIA_SUCCESS) {
        pslLogError("<XUP>", "Error downloading XUP", status);
        return status;
    }

    return XIA_SUCCESS;
}

static int download11(int detChan, unsigned long size, byte_t buffer[]) {
    int status;

    status = xupDoFiPPIGen(detChan, 2, size, buffer);

    if (status != XIA_SUCCESS) {
        pslLogError("<XUP>", "Error downloading XUP", status);
        return status;
    }

    return XIA_SUCCESS;
}

static int download12(int detChan, unsigned long size, byte_t buffer[]) {
    int status;

    status = xupWriteFlash(detChan, 0xAE80, size, buffer);

    if (status != XIA_SUCCESS) {
        pslLogError("<XUP>", "Error downloading data to memory", status);
        return status;
    }

    return XIA_SUCCESS;
}

static int download14(int detChan, unsigned long size, byte_t buffer[]) {
    int status;

    status = xupDoFiPPI(detChan, 2, size, buffer);

    if (status != XIA_SUCCESS) {
        pslLogError("<XUP>", "Error downloading XUP", status);
        return status;
    }

    return XIA_SUCCESS;
}

static int download01(int detChan, unsigned long size, byte_t buffer[]) {
    int status;

    status = xupDoI2C(detChan, I2C_PREAM_OFFSET, size, buffer);

    if (status != XIA_SUCCESS) {
        pslLogError("<XUP>", "Error downloading XUP", status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Download the DSP block of the I2C memory
 */
static int download03(int detChan, unsigned long size, byte_t buffer[]) {
    int status;

    float wait = 3.0;

    status = xupDoI2C(detChan, I2C_DSP_OFFSET, size, buffer);

    if (status != XIA_SUCCESS) {
        pslLogError("<XUP>", "Error downloading XUP", status);
        return status;
    }

    utils->funcs->dxp_md_wait(&wait);

    return XIA_SUCCESS;
}

/*
 * This command actually just reboots the board.
 */
static int download16(int detChan, unsigned long size, byte_t buffer[]) {
    int status;

    UNUSED(size);
    UNUSED(buffer);

    status = xupReboot(detChan);

    if (status != XIA_SUCCESS) {
        pslLogError("<XUP>", "Error doing XUP operation", status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Download a GLOBSET to the board
 */
static int download17(int detChan, unsigned long size, byte_t buffer[]) {
    int status;

    status = xupWriteFlash(detChan, 0x680, size, buffer);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error downloading data to memory for detChan %d",
                detChan);
        pslLogError("<XUP>", info_string, status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Clears the backup flag section of the I2C preamble.
 */
static int download18(int detChan, unsigned long size, byte_t buffer[]) {
    int status;

    byte_t b[] = {0x00};

    UNUSED(buffer);
    UNUSED(size);

    status = xupDoI2C(detChan, BACKUP_FLAG_OFFSET, 1, b);

    if (status != XIA_SUCCESS) {
        pslLogError("<XUP>", "Error downloading XUP", status);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Write to an arbitrary address in the I2C memory.
 */
static int xupDoI2C(int detChan, unsigned short addr, unsigned long size,
                    byte_t data[]) {
    /*FILE *fp = NULL;*/

    int status;

    unsigned int i;
    unsigned int nTransfers = (unsigned int) (size / MAX_I2C_WRITE_BYTES);
    unsigned int extra = 0;
    unsigned int lenS = 2 + MAX_I2C_WRITE_BYTES;
    unsigned int lenR = 1 + RECV_BASE;

    byte_t cmd = CMD_WRITE_I2C;

    byte_t send[2 + MAX_I2C_WRITE_BYTES];
    byte_t receive[1 + RECV_BASE];

    for (i = 0; i < nTransfers; i++, addr += MAX_I2C_WRITE_BYTES) {
        send[0] = (byte_t) (addr & 0xFF);
        send[1] = (byte_t) ((addr >> 8) & 0xFF);
        memcpy(send + 2, data + (i * MAX_I2C_WRITE_BYTES), MAX_I2C_WRITE_BYTES);

        status = dxp_cmd(&detChan, &cmd, &lenS, send, &lenR, receive);

        if (status != DXP_SUCCESS) {
            sprintf(info_string,
                    "Transfer error loading I2C loop: %u, send: %p, lenS: %u", i, send,
                    lenS);
            pslLogError("<XUP>", info_string, status);
            return status;
        }
    }

    /* If not an even multiple of MAX_I2C_WRITE_BYTES then
     * we need to do one more transfer.
     */
    extra = size % MAX_I2C_WRITE_BYTES;

    if (extra != 0) {
        /* A note of sloppiness here: send[] will now
         * be larger (size-wise) then extra.
         */

        lenS = extra + 2;

        send[0] = (byte_t) (addr & 0xFF);
        send[1] = (byte_t) ((addr >> 8) & 0xFF);
        memcpy(send + 2, data + (size - extra), extra);

        status = dxp_cmd(&detChan, &cmd, &lenS, send, &lenR, receive);

        if (status != DXP_SUCCESS) {
            pslLogError("<XUP>", "Transfer error loading I2C", status);
            return status;
        }
    }

    return XIA_SUCCESS;
}

/*
 * Reboot the board
 */
int xupReboot(int detChan) {
    int status;

    unsigned int lenS = 4;
    unsigned int lenR = 1 + RECV_BASE;

    byte_t cmd = CMD_REBOOT;

    byte_t send[4];
    byte_t receive[1 + RECV_BASE];

    float wait = 5.0;

    /* Reboot the board */
    send[0] = 0xAA;
    send[1] = 0x55;
    send[2] = 0xAA;
    send[3] = 0x55;
    status = dxp_cmd(&detChan, &cmd, &lenS, send, &lenR, receive);

    if (status != DXP_SUCCESS) {
        pslLogError("xupReboot", "Error rebooting board", status);
        return status;
    }

    utils->funcs->dxp_md_wait(&wait);

    return XIA_SUCCESS;
}

/*
 * Loads a flash image onto the board using the
 * same procedure as found in udxps_psl.c
 */
static int xupLoadFlashImage(int detChan, byte_t* img) {
    int status;

    unsigned int numSectors = 512;
    unsigned int numQuadrants = 4;
    unsigned int i;
    unsigned int j;
    unsigned int lenS = 67;
    unsigned int lenR = RECV_BASE + 1;

    unsigned short addr = 0x0000;

    byte_t cmd = CMD_WRITE_FLASH;

    byte_t send[67];
    byte_t receive[RECV_BASE + 1];

    for (i = 0; i < numSectors; i++, addr += 128) {
        for (j = 0; j < numQuadrants; j++) {
            send[0] = (byte_t) j;
            send[1] = (byte_t) (addr & 0xFF);
            send[2] = (byte_t) ((addr >> 8) & 0xFF);
            memcpy(send + 3, img + ((j * 64) + (addr * 2)), 64);

            status = dxp_cmd(&detChan, &cmd, &lenS, send, &lenR, receive);

            if (status != DXP_SUCCESS) {
                pslLogError("<XUP>", "Unknown I/O Error", status);
                return status;
            }
        }
    }

    return XIA_SUCCESS;
}

/*
 * Set the global backup path
 */
int xupSetBackupPath(char* path) {
    size_t pathLen = 0;

    boolean_t addSlash = FALSE_;

    ASSERT(path != NULL);

    if (backupPath) {
        utils->funcs->dxp_md_free(backupPath);
        backupPath = NULL;
    }

    ASSERT(backupPath == NULL);

    pathLen = strlen(path) + 1;

    /* Later uses of the backup path expect it to terminate with a slash */
    if (path[pathLen - 1] != '\\') {
        pathLen++;
        addSlash = TRUE_;
    }

    backupPath = (char*) MALLOC(pathLen);

    if (!backupPath) {
        sprintf(info_string, "Error allocating %zd bytes for 'backupPath'",
                strlen(path) + 1);
        pslLogError("xupSetBackupPath", info_string, XIA_NOMEM);
        return XIA_NOMEM;
    }

    strcpy(backupPath, path);

    if (addSlash) {
        strcat(backupPath, "\\");
    }

    return XIA_SUCCESS;
}

/*
 * Reads out the I2C memory into the supplied buffer.
 */
static int xupReadI2CToBuffer(int detChan, byte_t* i2c) {
    int status;

    unsigned int lenS = 4;
    unsigned int lenR = RECV_BASE + 1 + (2 * MAX_I2C_READ);

    unsigned long addr = 0;

    byte_t cmd = CMD_READ_I2C;

    byte_t send[4];
    byte_t receive[RECV_BASE + 1 + (2 * MAX_I2C_READ)];

    for (addr = 0; addr < I2C_MEMORY_SIZE_BYTES;
         addr += (unsigned int) (MAX_I2C_READ * 2)) {
        send[0] = (byte_t) (addr & 0xFF);
        send[1] = (byte_t) ((addr >> 8) & 0xFF);
        send[2] = (byte_t) MAX_I2C_READ * 2;
        send[3] = (byte_t) 0x00;

        status = dxp_cmd(&detChan, &cmd, &lenS, send, &lenR, receive);

        if (status != DXP_SUCCESS) {
            pslLogError("<XUP>", "Error reading I2C memory", status);
            return status;
        }

        memcpy(i2c + addr, receive + RECV_BASE, MAX_I2C_READ * 2);
    }

    return XIA_SUCCESS;
}

/*
 * Reads out the flash memory into the specified buffer.
 */
static int xupReadFlashToBuffer(int detChan, byte_t* flash) {
    int status;

    unsigned int lenS = 3;
    unsigned int lenR = RECV_BASE + 1 + (2 * MAX_FLASH_READ);

    unsigned long addr = 0;

    byte_t cmd = CMD_READ_FLASH;

    byte_t send[3];
    byte_t receive[RECV_BASE + 1 + (2 * MAX_FLASH_READ)];

    for (addr = 0; addr < (unsigned long) (FLASH_MEMORY_SIZE_BYTES / 2);
         addr += MAX_FLASH_READ) {
        send[0] = (byte_t) (addr & 0xFF);
        send[1] = (byte_t) ((addr >> 8) & 0xFF);
        send[2] = (byte_t) MAX_FLASH_READ;

        status = dxp_cmd(&detChan, &cmd, &lenS, send, &lenR, receive);

        if (status != DXP_SUCCESS) {
            pslLogError("xupDumpFlash", "Error reading Flash memory", status);
            return status;
        }

        memcpy(flash + (addr * 2), receive + RECV_BASE, MAX_FLASH_READ * 2);
    }

    return XIA_SUCCESS;
}

/*
 * Write the backup XUP and return the computed chksum so that the
 * access file can be generated.
 */
static int xupWriteBackupXUP(byte_t* i2c, byte_t* flash, byte_t* sn,
                             unsigned short* chksum, struct tm* tstr) {
    int status;
    int r;
    size_t pathSize = 0;

    unsigned long i;

    unsigned short gid = 0xFFFF;
    unsigned short ver = XUP_CURRENT_VERSION;
    unsigned short usChksum = 0x0000;
    unsigned short nSections = 3;

    byte_t b;
    byte_t access = 0x00;

    byte_t sections[I2C_MEMORY_SIZE_BYTES + 5 + FLASH_MEMORY_SIZE_BYTES + 5 + 5];

    /* The 32 isn't magic...the backup files have the following name:
     * backup_sssss_xxxxyyzzhhmmss.xup
     */
    char name[32];

    char* completePath = NULL;

    FILE* fp = NULL;

    /* Create un-encrypted array for purposes
     * of calculating the chksum and then transfer
     * to the final payload.
     */

    /* I2C sections first */
    sections[0] = (byte_t) 0x01;
    sections[1] = (byte_t) (I2C_MEMORY_SIZE_BYTES & 0xFF);
    sections[2] = (byte_t) ((I2C_MEMORY_SIZE_BYTES >> 8) & 0xFF);
    sections[3] = (byte_t) ((I2C_MEMORY_SIZE_BYTES >> 16) & 0xFF);
    sections[4] = (byte_t) ((I2C_MEMORY_SIZE_BYTES >> 24) & 0xFF);
    memcpy(sections + 5, i2c, I2C_MEMORY_SIZE_BYTES);
    /* Now the reboot command */
    sections[I2C_MEMORY_SIZE_BYTES + 5] = (byte_t) 0x16;
    sections[I2C_MEMORY_SIZE_BYTES + 6] = (byte_t) 0x00;
    sections[I2C_MEMORY_SIZE_BYTES + 7] = (byte_t) 0x00;
    sections[I2C_MEMORY_SIZE_BYTES + 8] = (byte_t) 0x00;
    sections[I2C_MEMORY_SIZE_BYTES + 9] = (byte_t) 0x00;
    /* Now the Flash memory */
    sections[I2C_MEMORY_SIZE_BYTES + 10] = (byte_t) 0x06;
    sections[I2C_MEMORY_SIZE_BYTES + 11] = (byte_t) (FLASH_MEMORY_SIZE_BYTES & 0xFF);
    sections[I2C_MEMORY_SIZE_BYTES + 12] =
        (byte_t) ((FLASH_MEMORY_SIZE_BYTES >> 8) & 0xFF);
    sections[I2C_MEMORY_SIZE_BYTES + 13] =
        (byte_t) ((FLASH_MEMORY_SIZE_BYTES >> 16) & 0xFF);
    sections[I2C_MEMORY_SIZE_BYTES + 14] =
        (byte_t) ((FLASH_MEMORY_SIZE_BYTES >> 24) & 0xFF);
    memcpy(sections + I2C_MEMORY_SIZE_BYTES + 15, flash, FLASH_MEMORY_SIZE_BYTES);

    usChksum = (unsigned short) xupXORChksum(
        I2C_MEMORY_SIZE_BYTES + FLASH_MEMORY_SIZE_BYTES + 15, sections);
    *chksum = usChksum;

    sprintf(name, "backup_%c%c%c%c%c_%02hu%02hu%02hu%02hu%02hu%02hu.xup", sn[11],
            sn[12], sn[13], sn[14], sn[15], (unsigned short) (tstr->tm_year - 100),
            (unsigned short) (tstr->tm_mon + 1), (unsigned short) tstr->tm_mday,
            (unsigned short) tstr->tm_hour, (unsigned short) tstr->tm_min,
            (unsigned short) tstr->tm_sec);

    /* If the backup path has been set then we want to prepend it to the
     * backup XUP name.
     */
    if (backupPath) {
        pathSize = strlen(backupPath) + strlen(name) + 1;
    } else {
        pathSize = strlen(name) + 1;
    }

    completePath = (char*) MALLOC(pathSize);

    if (!completePath) {
        sprintf(info_string, "Error allocating %zu bytes for 'completePath'", pathSize);
        pslLogError("<XUP>", info_string, XIA_NOMEM);
        return XIA_NOMEM;
    }

    if (backupPath) {
        strcpy(completePath, backupPath);
        strncat(completePath, name, strlen(name) + 1);
    } else {
        strncpy(completePath, name, pathSize);
    }

    sprintf(info_string, "completePath = %s", completePath);
    pslLogDebug("<XUP>", info_string);

    fp = fopen(completePath, "wb");

    FREE(completePath);

    if (fp == NULL) {
        status = XIA_OPEN_FILE;
        pslLogError("<XUP>", "Unable to open backup XUP", status);
        return status;
    }

    srand((unsigned) time(NULL));

    /* Need to generate an access code
     * that isn't the "no access file required"
     * code.
     */
    do {
        r = rand();
        access = (byte_t) ((r / RAND_MAX) * 256);
    } while (access == 0x32);

    fwrite(&gid, 2, 1, fp);
    fwrite(&ver, 1, 1, fp);

    fwrite(&access, 1, 1, fp);
    fwrite(&usChksum, 2, 1, fp);

    /* The rest of this data is encrypted */
    b = xupEncryptByte(0, (byte_t) (nSections & 0xFF));
    fwrite(&b, sizeof(b), 1, fp);
    b = xupEncryptByte(0, (byte_t) ((nSections >> 8) & 0xFF));
    fwrite(&b, sizeof(b), 1, fp);

    for (i = 0; i < I2C_MEMORY_SIZE_BYTES + FLASH_MEMORY_SIZE_BYTES + 15; i++) {
        b = xupEncryptByte(0, sections[i]);
        fwrite(&b, sizeof(b), 1, fp);
    }

    fclose(fp);

    return XIA_SUCCESS;
}

/*
 * Computes a byte-wide XOR checksum from the supplied data.
 */
static byte_t xupXORChksum(unsigned long size, byte_t* data) {
    unsigned long i;
    byte_t chksum = 0x00;

    for (i = 0; i < size; i++) {
        chksum = (byte_t) (chksum ^ data[i]);
    }

    return chksum;
}

/*
 * Reads the serial number from the hardware.
 */
static int xupReadSerialNumber(int detChan, byte_t* sn) {
    int status;

    unsigned int lenS = 0;
    unsigned int lenR = RECV_BASE + 2 + SERIAL_NUM_LEN;

    byte_t cmd = CMD_GET_SERIAL_NUMBER;

    byte_t receive[RECV_BASE + 2 + SERIAL_NUM_LEN];

    status = dxp_cmd(&detChan, &cmd, &lenS, NULL, &lenR, receive);

    if (status != DXP_SUCCESS) {
        pslLogError("<XUP>", "Error reading memory", status);
        return status;
    }

    memcpy(sn, receive + 5, SERIAL_NUM_LEN);

    return XIA_SUCCESS;
}

/*
 * Write an access file out to match up with the
 */
static int xupWriteBackupAccessFile(struct tm* tstr, byte_t* sn,
                                    unsigned short chksum) {
    int status;
    size_t pathSize = 0;

    char* completePath = NULL;

    char name[32];

    byte_t cs = 0x00;

    byte_t preChk[10];
    byte_t acode[8];

    FILE* fp = NULL;

    sprintf(name, "backup_%c%c%c%c%c_%02hu%02hu%02hu%02hu%02hu%02hu.acf", sn[11],
            sn[12], sn[13], sn[14], sn[15], (unsigned short) (tstr->tm_year - 100),
            (unsigned short) (tstr->tm_mon + 1), (unsigned short) tstr->tm_mday,
            (unsigned short) tstr->tm_hour, (unsigned short) tstr->tm_min,
            (unsigned short) tstr->tm_sec);

    xupCalculateAccessCode(sn, chksum, acode);

    /* Prebuild the file since we have to calculate the
     * checksum anyways...
     */
    preChk[0] = (byte_t) 0x01;
    preChk[1] = (byte_t) 0x00;
    memcpy(preChk + 2, acode, 8);

    cs = xupXORChksum(10, preChk);

    /* If the backup path has been set then we want to prepend it to the
     * backup XUP name.
     */
    if (backupPath) {
        pathSize = strlen(backupPath) + strlen(name) + 1;
    } else {
        pathSize = strlen(name) + 1;
    }

    completePath = (char*) MALLOC(pathSize);

    if (!completePath) {
        sprintf(info_string, "Error allocating %zu bytes for 'completePath'", pathSize);
        pslLogError("<XUP>", info_string, XIA_NOMEM);
        return XIA_NOMEM;
    }

    if (backupPath) {
        strcpy(completePath, backupPath);
        strncat(completePath, name, strlen(name) + 1);
    } else {
        strncpy(completePath, name, pathSize);
    }

    sprintf(info_string, "completePath = %s", completePath);
    pslLogDebug("<XUP>", info_string);

    fp = fopen(completePath, "wb");

    FREE(completePath);

    if (fp == NULL) {
        status = XIA_OPEN_FILE;
        pslLogError("<XUP>", "Error opening file", status);
        return status;
    }

    fwrite(preChk, 10, 1, fp);
    fwrite(&cs, 1, 1, fp);

    fclose(fp);

    return XIA_SUCCESS;
}

/*
 * Calculates an access code from a serial number and checksum.
 * The procedures here are explained in more detail in the
 * specification: microDXP Software Security Model.
 */
static void xupCalculateAccessCode(byte_t* sn, unsigned short chksum, byte_t* acode) {
    unsigned short i;
    unsigned short us = 0x0000;

    unsigned short x[8];

    us = xupByteToUS(sn[0], sn[1]);

    x[0] = (unsigned short) (us ^ chksum);

    for (i = 0; i < 7; i++) {
        us = xupByteToUS(sn[(i * 2) + 2], sn[(i * 2) + 3]);
        x[i + 1] = (unsigned short) (us ^ x[i]);
    }

    /* Convert to the alphanumberic representation here */
    for (i = 0; i < 8; i++) {
        acode[i] = (byte_t) ((x[i] % 26) + 0x41);
    }
}

/*
 * Calculate the access code from the current serial number
 * and verify that it is what is contained in the access
 * code file.
 */
int xupVerifyAccess(int detChan, char* xup) {
    int status;

    char* acf = NULL;
    char* ext = "acf";

    unsigned long sizeNoChk = 0x00000000;

    unsigned short i;
    unsigned short nACs = 0x0000;
    unsigned short xCS = 0x0000;

    byte_t fCS;
    byte_t cs;

    byte_t hdr[8];
    byte_t ac[8];
    byte_t fileAC[8];
    byte_t sn[SERIAL_NUM_LEN];

    byte_t* contents = NULL;

    FILE* fp = NULL;

    /* The access file has the same name as the
     * XUP but with an .ACF extension instead of
     * a .XUP extension.
     */
    acf = (char*) MALLOC((strlen(xup) + 1) * sizeof(char));

    if (acf == NULL) {
        status = XIA_NOMEM;
        pslLogError("xupIsAccessRequired", "Out-of-memory trying to create string",
                    status);
        return status;
    }

    /* Swap file extensions, preserving the
     * original filename string.
     */
    strncpy(acf, xup, strlen(xup) - 3);
    strncpy(acf + strlen(xup) - 3, ext, strlen(ext) + 1);

    /* Verify the checksum first */
    fp = fopen(acf, "rb");

    if (fp == NULL) {
        FREE(acf);
        status = XIA_OPEN_FILE;
        pslLogError("xupIsAccessRequired", "Error opening access file", status);
        return status;
    }

    /* Read in the number of ACs so we now how
     * much memory to re-allocate.
     */
    contents = (byte_t*) MALLOC(2 * sizeof(byte_t));

    if (contents == NULL) {
        FREE(acf);
        fclose(fp);
        status = XIA_NOMEM;
        pslLogError("xupIsAccessRequired", "Out-of-memory trying to create array",
                    status);
        return status;
    }

    fread(contents, 2, 1, fp);
    fclose(fp);

    nACs = xupByteToUS(contents[0], contents[1]);

    FREE(contents);
    contents = NULL;

    sizeNoChk = (unsigned long) ((nACs * 8) + 2);

    contents = (byte_t*) MALLOC(sizeNoChk * sizeof(byte_t));

    if (contents == NULL) {
        FREE(acf);
        status = XIA_NOMEM;
        pslLogError("xupIsAccessRequired", "Out-of-memory trying to create array",
                    status);
        return status;
    }

    fp = fopen(acf, "rb");

    if (fp == NULL) {
        FREE(acf);
        FREE(contents);
        status = XIA_OPEN_FILE;
        pslLogError("xupIsAccessRequired", "Error opening access file", status);
        return status;
    }

    fread(contents, sizeNoChk, 1, fp);
    fread(&fCS, 1, 1, fp);
    fclose(fp);

    cs = xupXORChksum(sizeNoChk, contents);

    if (cs != fCS) {
        status = XIA_CHKSUM;
        sprintf(info_string, "chksum mismatch: cs = %#x, fCS = %#x", cs, fCS);
        pslLogError("xupIsAccessRequired", info_string, status);
        return status;
    }

    /* Calculate "true" access code for the current board. */
    status = xupReadSerialNumber(detChan, sn);

    if (status != XIA_SUCCESS) {
        FREE(acf);
        FREE(contents);
        pslLogError("xupIsAccessRequired", "Error reading data from hardware", status);
        return status;
    }

    fp = fopen(xup, "rb");

    if (fp == NULL) {
        FREE(acf);
        FREE(contents);
        status = XIA_OPEN_FILE;
        pslLogError("xupIsAccessRequired", "Error opening XUP file", status);
        return status;
    }

    fread(hdr, 8, 1, fp);
    fclose(fp);

    xCS = xupByteToUS(hdr[4], hdr[5]);

    xupCalculateAccessCode(sn, xCS, ac);

    for (i = 0; i < nACs; i++) {
        memcpy(fileAC, contents + ((i * 8) + 2), 8);

        if (memcmp(fileAC, ac, 8) == 0) {
            /* Matched the access code */
            FREE(acf);
            FREE(contents);
            return XIA_SUCCESS;
        }
    }

    FREE(acf);
    FREE(contents);

    return XIA_NO_ACCESS;
}

/*
 * Read in the access code and return TRUE_ if it
 * is anything other then 0x32.
 */
int xupIsAccessRequired(char* xup, boolean_t* isRequired) {
    int status;

    byte_t hdr[8];

    FILE* fp = NULL;

    fp = fopen(xup, "rb");

    if (fp == NULL) {
        status = XIA_OPEN_FILE;
        pslLogError("xupIsAccessRequired", "Unable to open access file", status);
        return status;
    }

    fread(hdr, 8, 1, fp);

    fclose(fp);

    if (hdr[3] == 0x32) {
        *isRequired = FALSE_;
    } else {
        *isRequired = TRUE_;
    }

    return XIA_SUCCESS;
}

/*
 * Converts 4 bytes into an unsigned long.
 */
static unsigned long xupByteToLong(byte_t lo0, byte_t lo1, byte_t hi0, byte_t hi1) {
    unsigned long dword = 0x00000000;

    dword = (unsigned long) (((unsigned long) hi1 << 24) | ((unsigned long) hi0 << 16) |
                             ((unsigned long) lo1 << 8) | ((unsigned long) lo0));

    return dword;
}

/*
 * Writes out an XUP containing GENSETs, PARSETs and the GLOBSET
 */
int xupCreateMasterParams(int detChan, char* target) {
    int status;

    unsigned short i;
    unsigned short h;
    unsigned short idx;

    unsigned short addrs[] = {0x0400, 0x0980, 0x5C00, 0xAE80};

    byte_t xupOffsets[] = {0x07, 0x0A, 0x0E, 0x12, 0x17};
    byte_t globset[SECTOR_LEN_BYTES];

    byte_t sets[4][FIVE_SECTOR_LEN_BYTES];

    /* 1) Assemble all of the data */
    for (i = 0; i < 4; i++) {
        status = xupReadFlash(detChan, addrs[i], (unsigned long) FIVE_SECTOR_LEN_BYTES,
                              sets[i]);

        if (status != XIA_SUCCESS) {
            sprintf(info_string, "Error assembling PARSET/GENSET data for detChan %d",
                    detChan);
            pslLogError("xupCreateMasterParams", info_string, status);
            return status;
        }
    }

    status = xupReadFlash(detChan, 0x680, (unsigned long) SECTOR_LEN_BYTES, globset);

    if (status != XIA_SUCCESS) {
        sprintf(info_string, "Error assembling GLOBSET data for detChan %d", detChan);
        pslLogError("xupCreateMasterParams", info_string, status);
        return status;
    }

    /* 2) Add sections to the XUP using xw.dll */
    status = OpenXUP(target, &h);

    if (status != 0) {
        pslLogError("xupCreateMasterParams", "Error opening target file", XIA_OPEN_XW);
        return XIA_OPEN_XW;
    }

    for (i = 0; i < 4; i++) {
        status = AddSection(xupOffsets[i], 0x0000,
                            (unsigned long) FIVE_SECTOR_LEN_BYTES, sets[i], &idx);

        if (status != 0) {
            sprintf(info_string, "Error building target file '%s'", target);
            pslLogError("xupCreateMasterParams", info_string, XIA_ADD_XW);
            return XIA_ADD_XW;
        }
    }

    status =
        AddSection(xupOffsets[4], 0x0, (unsigned long) SECTOR_LEN_BYTES, globset, &idx);

    if (status != 0) {
        sprintf(info_string, "Error building target file '%s'", target);
        pslLogError("xupCreateMasterParams", info_string, XIA_ADD_XW);
        return XIA_ADD_XW;
    }

    /* XXX Remove this? */
    DumpSections("sections.dump");

    /* GID = 0xFFFE is reserved for master param sets. */
    status = WriteXUP(h, 0xFFFE, 0);

    if (status != 0) {
        sprintf(info_string, "Error writing target file '%s'", target);
        pslLogError("xupCreateMasterParams", info_string, XIA_WRITE_XW);
        return XIA_WRITE_XW;
    }

    CloseXUP(h);

    return XIA_SUCCESS;
}

/*
 * Reads out Flash memory from an arbitrary address and of an
 * arbitrary size. "size" is assumed to be in BYTES! (not words)
 */
static int xupReadFlash(int detChan, unsigned short addr, unsigned long size,
                        byte_t* data) {
    int status;

    unsigned int lenS = 3;
    unsigned int lenR;
    unsigned int maxLenR = 65 + RECV_BASE;

    unsigned short maxWordsPerTransfer = 32;
    unsigned short i;
    unsigned short a = addr;
    unsigned short nWords = (unsigned short) (size / 2);
    unsigned short nFullReads =
        (unsigned short) floor((double) (nWords / maxWordsPerTransfer));

    byte_t cmd = CMD_READ_FLASH;

    byte_t send[3];
    byte_t maxR[65 + RECV_BASE];

    byte_t* finalR = NULL;

    for (i = 0; i < nFullReads; i++, a += 32) {
        send[0] = (byte_t) (a & 0xFF);
        send[1] = (byte_t) ((a >> 8) & 0xFF);
        send[2] = (byte_t) maxWordsPerTransfer;

        status = dxp_cmd(&detChan, &cmd, &lenS, send, &maxLenR, maxR);

        if (status != DXP_SUCCESS) {
            pslLogError("<XUP>", "Error reading data", status);
            return status;
        }

        memcpy(data + (i * maxWordsPerTransfer * 2), maxR + 5,
               maxWordsPerTransfer * sizeof(unsigned short));
    }

    /* We can skip all of this if there is nothing left to transfer.
     * Apparently, sending in a commmand with "0" words to read doesn't
     * work too well.
     */
    if ((nWords % 32) != 0) {
        send[0] = (byte_t) (a & 0xFF);
        send[1] = (byte_t) ((a >> 8) & 0xFF);
        send[2] = (byte_t) (nWords % 32);

        lenR = ((nWords % 32) * 2) + 1 + RECV_BASE;
        finalR = (byte_t*) MALLOC(lenR * sizeof(byte_t));

        if (finalR == NULL) {
            status = XIA_NOMEM;
            sprintf(info_string, "Out-of-memory allocating %zu bytes",
                    lenR * sizeof(byte_t));
            pslLogError("<XUP>", info_string, status);
            return status;
        }

        status = dxp_cmd(&detChan, &cmd, &lenS, send, &lenR, finalR);

        if (status != DXP_SUCCESS) {
            utils->funcs->dxp_md_free(finalR);
            pslLogError("<XUP>", "Error reading data", status);
            return status;
        }

        memcpy(data + (a - addr), finalR + 5, (nWords % 32) * sizeof(unsigned short));
        utils->funcs->dxp_md_free(finalR);
    }

    return XIA_SUCCESS;
}

static int xupWriteFlash(int detChan, unsigned short addr, unsigned long size,
                         byte_t* data) {
    int status;

    unsigned short a = addr;
    unsigned short j;

    unsigned int i;
    unsigned int nSectors = (unsigned int) (size / 256);

    DEFINE_CMD(CMD_WRITE_FLASH, 67, 1);

    if ((size % 256) != 0) {
        sprintf(info_string, "Size (%lu) mismatch: size is not %% 256", size);
        pslLogError("<XUP>", info_string, XIA_SIZE_MISMATCH);
        return XIA_SIZE_MISMATCH;
    }

    for (i = 0; i < nSectors; i++) {
        send[1] = (byte_t) ((a + (128 * i)) & 0xFF);
        send[2] = (byte_t) (((a + (128 * i)) >> 8) & 0xFF);

        for (j = 0; j < 4; j++) {
            send[0] = (byte_t) j;
            memcpy(send + 3, data + (i * 256) + (j * 64), 64);

            status = dxp_cmd(&detChan, &cmd, &lenS, send, &lenR, receive);

            if (status != XIA_SUCCESS) {
                pslLogError("<XUP>", "Error writing data", status);
                return status;
            }
        }
    }

    return XIA_SUCCESS;
}
