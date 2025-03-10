/*
 * Copyright (c) 2007-2016 XIA LLC
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

#ifndef _PSL_UDXP_ALPHA_H_
#define _PSL_UDXP_ALPHA_H_

/* DXP Commands, USB command type 1 (UART 1) */
#define CMD_ALPHA_FREE_EVENTS 0xC0
#define CMD_ALPHA_READ_STATISTICS 0xC1
#define CMD_ULTRA_SLAVE_MASTER 0xC2
#define CMD_ALPHA_PARAMS 0xC4
#define CMD_SET_ALPHA_EXT_TRIGGER 0xC5

/*
 * I2C access, USB command type 3. The I2C command packet must be
 * wrapped in an rs-232 command. The command ID is ignored by the
 * motherboard, and it always replies 0x40. Send 0x40 to allow simple
 * command/response validation.
 */
#define CMD_ACCESS_I2C 0x40

/* Pulser commands: USB command type 2 (UART 2) */
#define CMD_ALPHA_PULSER_ENABLE_DISABLE 0xD5
#define CMD_ALPHA_PULSER_CONFIG_1 0xD1
#define CMD_ALPHA_PULSER_CONFIG_2 0xD2
#define CMD_ALPHA_PULSER_SET_MODE 0xD3
#define CMD_ALPHA_PULSER_CONFIG_VETO 0xD7
#define CMD_ALPHA_PULSER_ENABLE_DISABLE_VETO 0xD6
#define CMD_ALPHA_PULSER_CONTROL 0xD0

/* Master/slave module clock configuration */
#define ULTRA_CLOCK_MASTER 1

/* Bit-masks for the pulser mode command */
#define ALPHA_PULSER_P1_MODE 0x1
#define ALPHA_PULSER_P2_ENABLE 0x2
#define ALPHA_PULSER_P2_MODE 0x4

/* Bit-masks for device status (DSR) */
#define ULTRA_DSR_INIT 0
#define ULTRA_DSR_MM 1
#define ULTRA_DSR_HV 2
#define ULTRA_DSR_DXP1 3
#define ULTRA_DSR_DXP2 4
#define ULTRA_DSR_ELECTRODE 5
#define ULTRA_DSR_MBV42_BIT 7

/* I2C API Constants */
#define ALPHA_I2C_READ 0
#define ALPHA_I2C_WRITE 1

/* Parameter indexes */
#define ALPHA_EVENT_LEN 0
#define ALPHA_PRE_BUF_LEN 1
#define ALPHA_DAC_TARGET 2
#define ALPHA_DAC_TOL 3

/* PSL acquisition value entry member for Alpha parameters.
 * See also AV_MEM_* in psl_udxp.h.
 */
#define AV_MEM_ALPHA 0x80 /* AV_MEM_CUST */
#define AV_MEM_R_ALPHA (0x02 | AV_MEM_ALPHA) /* (AV_MEM_REQ | AV_MEM_ALPHA) */

/* Parameter validation
 */
#define ALPHA_EVENT_LEN_MIN 2 /* pre-buffer min + 1 */
#define ALPHA_EVENT_LEN_MAX 4086 /* 4096 - 10 (header) */

#define ALPHA_PRE_BUF_LEN_MIN 1
#define ALPHA_PRE_BUF_LEN_MAX 4085 /* event length max - 1 */

#define ALPHA_DAC_TARGET_MIN 1
#define ALPHA_DAC_TARGET_MAX 999

#define ALPHA_DAC_TOL_MIN 1
#define ALPHA_DAC_TOL_MAX 999

#define ALPHA_HV_MIN 0
#define ALPHA_HV_MAX 1250
#define ALPHA_HV_SCALE (4096.0 / ALPHA_HV_MAX)

#define ALPHA_PULSER_DAC_RANGE 192623.0
#define ALPHA_PULSER_DAC_MAX 16383

#define ALPHA_REALTIME_CLOCK_TICK 125.0e-9

/* I2C device addresses */
#define I2C_WRITE_ADDR(addr) ((addr << 1) | 1)
#define ULTRA_TILT_I2C_ADDR 0x3A
#define ULTRA_MB_EEPROM_I2C_ADDR I2C_WRITE_ADDR(0x58)  // DS28CN01
#define ULTRA_MB_V42_EEPROM_I2C_ADDR I2C_WRITE_ADDR(0x57)  // 24AA256UIDT
#define ULTRA_MB_DSR_I2C_ADDR 0xE1

/* Tilt sensor registers */
#define ULTRA_TILT_WHO_AM_I 0x0F
#define ULTRA_TILT_CTRL_REG1 0x20
#define ULTRA_TILT_CTRL_REG2 0x21
#define ULTRA_TILT_CTRL_REG3 0x22
#define ULTRA_TILT_OUTX_L 0x28
#define ULTRA_TILT_OUTX_H 0x29
#define ULTRA_TILT_OUTY_L 0x2A
#define ULTRA_TILT_OUTY_H 0x2B
#define ULTRA_TILT_OUTZ_L 0x2C
#define ULTRA_TILT_OUTZ_H 0x2D
#define ULTRA_TILT_DD_CFG 0x38
#define ULTRA_TILT_DD_THSI_L 0x3C
#define ULTRA_TILT_DD_THSI_H 0x3D
#define ULTRA_TILT_DD_THSE_L 0x3E
#define ULTRA_TILT_DD_THSE_H 0x3F

#define ULTRA_TILT_G_MAX 2.0
#define ULTRA_TILT_G_MIN -2.0

/* Predefined tilt sensor register settings */

/* PD1=0,PD0=1       (Normal mode)
 * DF1=1,DF0=0       (Decimate by 32)
 * ST=0              (Normal mode)
 * Zen=1,Yen=1,Xen=1 (All axes enabled)
 */
#define ULTRA_TILT_CTRL_REG1_NORMAL_MODE 0x67

/* FS=0              (+/-2g)
 * BDU=1             (Output registers not updated until MSB+LSB are read)
 * BLE=0             (Little endian)
 * BOOT=0            (Normal mode)
 * IEN=1             (Interrupt signal on RDY)
 * DRDY=0            (Disable data-ready generation)
 * SIM=0             (4-wire interface)
 * DAS=1             (16-bit left justified alignment)
 */
#define ULTRA_TILT_CTRL_REG2_NORMAL_MODE 0x49

/* IEND=1            (Interrupt signal enabled)
 * LIR=1             (Interrupt request latched)
 * ZHIE=0            (Z high event)
 * ZLIE=0            (Z low event)
 * YHIE=0            (Y high event)
 * YLIE=0            (Y low event)
 * XHIE=0            (X high event)
 * XLIE=1            (X low event)
 */
#define ULTRA_TILT_DD_CFG_X_LOW 0xC1

#define ULTRA_TILT_STATUS_TRIGGERED 1

/* USB command templates */
/* todo: rename ADDR macros from ULTRA to UDXP */
#define ULTRA_CMD_ADDR_TGT(target) (((unsigned long) target) << 16)
#define ULTRA_CMD_ADDR(type, target, low_word)                                         \
    ((type << 24) | ULTRA_CMD_ADDR_TGT(target) | low_word)
#define ULTRA_CMD_UART2 0x02000000
#define ULTRA_CMD_USB_CONFIGURATION 0x04000000

/* Direct USB commands */
#define ULTRA_USB_VERSION ULTRA_CMD_ADDR(4, 0, 0x0000)
#define ULTRA_USB_RENUMERATE ULTRA_CMD_ADDR(4, 0, 0x0001)
#define ULTRA_USB_TILT_STATUS ULTRA_CMD_ADDR(4, 0, 0x0010)
#define ULTRA_USB_GET_DSR ULTRA_CMD_ADDR(4, 1, 0x0000)

/* Moisture meter */
#define ULTRA_MM_REQUEST ULTRA_CMD_UART2 | 0x010000
#define ULTRA_MM_REQUEST_LEN 3

#define ULTRA_MM_READ ULTRA_CMD_UART2 | 0x010000
#define ULTRA_MM_READ_LEN 6

/* EEPROM DS28CN01 registration number register */
#define ULTRA_MB_EEPROM_ID 0xA0 /* register */
#define ULTRA_MB_EEPROM_FAM 0x70 /* expected family number */

/* EEPROM 24AA256UIDT */
#define ULTRA_MB_V42_EEPROM_MFG 0xFA7F /* mfg code address */
#define ULTRA_MB_V42_EEPROM_ID 0xFC7F /* 32-bit serial number address */

#endif /* _PSL_UDXP_ALPHA_H_ */
