/*
 * Copyright (c) 2004 X-ray Instrumentation Associates
 *               2005-2015 XIA LLC
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

#ifndef __XIA_XMAP_H__
#define __XIA_XMAP_H__

#include <stdio.h>

#include "md_generic.h"

#include "xerxesdef.h"

#include "xia_xerxes_structures.h"

XERXES_EXPORT int XERXES_API dxp_init_xmap(Functions* funcs);

static int XERXES_API dxp_init_driver(Interface*);
static int XERXES_API dxp_init_utils(Utils*);

static int XERXES_API dxp_begin_run(int* ioChan, int* modChan, unsigned short* gate,
                                    unsigned short* resume, Board* board, int* id);
static int dxp_end_run(int* ioChan, int* modChan, Board* board);
static int XERXES_API dxp_run_active(int*, int*, int*);
static int XERXES_API dxp_begin_control_task(int* ioChan, int* modChan, short* type,
                                             unsigned int* length, int* info,
                                             Board* board);
static int XERXES_API dxp_end_control_task(int* ioChan, int* modChan, Board* board);
static int XERXES_API dxp_control_task_params(int* ioChan, int* modChan, short* type,
                                              Board* board, int* info);
static int XERXES_API dxp_control_task_data(int* ioChan, int* modChan, short* type,
                                            Board* board, void* data);
static int XERXES_API dxp_loc(char*, Dsp_Info*, unsigned short*);

static int XERXES_API dxp_get_dspinfo(Dsp_Info*);
static int XERXES_API dxp_get_fipinfo(void* fip);
static int XERXES_API dxp_get_fpgaconfig(void* fip);
static int XERXES_API dxp_download_fpga_done(int* modChan, char* name, Board* board);
static int XERXES_API dxp_download_fpgaconfig(int* ioChan, int* modChan, char* name,
                                              Board* board);

static int XERXES_API dxp_download_dspconfig(int* ioChan, int* modChan, Board* board);
static int XERXES_API dxp_get_dspconfig(Dsp_Info*);
static int XERXES_API dxp_decode_error(int*, int*, Dsp_Info*, unsigned short*,
                                       unsigned short*);
static int XERXES_API dxp_clear_error(int*, int*, Board*);
static int dxp_get_runstats(int* ioChan, int* modChan, Board* b, unsigned long* evts,
                            unsigned long* under, unsigned long* over,
                            unsigned long* fast, unsigned long* base, double* live,
                            double* icr, double* ocr);

static int XERXES_API dxp_modify_dspsymbol(int*, int*, char*, unsigned short*, Board*);
static int dxp_read_dspsymbol(int*, int*, char*, Board*, double*);
static int dxp_read_dspparams(int* ioChan, int* modChan, Board* b,
                              unsigned short* params);
static int XERXES_API dxp_write_dspparams(int*, int*, Dsp_Info*, unsigned short*);

static int dxp_get_spectrum_length(int* ioChan, int* modChan, Board* board,
                                   unsigned int* len);
static int dxp_get_baseline_length(int* modChan, Board* b, unsigned int* len);
static int XERXES_API dxp_read_spectrum(int*, int*, Board*, unsigned long*);
static int dxp_read_baseline(int* ioChan, int* modchan, Board* board,
                             unsigned long* baseline);

static int XERXES_API dxp_read_mem(int* ioChan, int* modChan, Board* board, char* name,
                                   unsigned long* base, unsigned long* offset,
                                   unsigned long* data);
static int XERXES_API dxp_write_mem(int* ioChan, int* modChan, Board* board, char* name,
                                    unsigned long* base, unsigned long* offset,
                                    unsigned long* data);

static int dxp_write_reg(int* ioChan, int* modChan, char* name, unsigned long* data);
static int dxp_read_reg(int* ioChan, int* modChan, char* name, unsigned long* data);

static FILE* XERXES_API dxp_find_file(const char*, const char*);

static int XERXES_API dxp_do_cmd(int modChan, Board* board, byte_t cmd,
                                 unsigned int lenS, byte_t* send, unsigned int lenR,
                                 byte_t* receive);

static int XERXES_API dxp_unhook(Board* board);

static int dxp_download_fpga(int ioChan, unsigned long target, Fippi_Info* fpga);

static int dxp_write_global_register(int ioChan, unsigned long reg, unsigned long val);
static int dxp_read_global_register(int ioChan, unsigned long reg, unsigned long* val);

static int dxp_load_symbols_from_file(char* file, Dsp_Params* params);
static int dxp_load_dsp_code_from_file(char* file, Dsp_Info* dsp);
static int dxp_reset_dsp(int ioChan);
static int dxp_boot_dsp(int ioChan, int modChan, Board* b);

static int dxp_get_symbol_by_index(int modChan, unsigned short index, Board* board,
                                   char* name);
static int dxp_get_num_params(int modChan, Board* b, unsigned short* n_params);

/* Logging macro wrappers */
#define dxp_log_error(x, y, z) xmap_md_log(MD_ERROR, (x), (y), (z), __FILE__, __LINE__)
#define dxp_log_warning(x, y) xmap_md_log(MD_WARNING, (x), (y), 0, __FILE__, __LINE__)
#define dxp_log_info(x, y) xmap_md_log(MD_INFO, (x), (y), 0, __FILE__, __LINE__)
#define dxp_log_debug(x, y) xmap_md_log(MD_DEBUG, (x), (y), 0, __FILE__, __LINE__)

/*
 * These constants define the values that dxp_md_plx_io() accepts for the
 * 'function' argument.
 */
#define XMAP_IO_SINGLE_WRITE 0
#define XMAP_IO_SINGLE_READ 1
#define XMAP_IO_BURST_READ 2

/* These are the addresses for the various registers. */
#define XMAP_REG_CFG_CONTROL 0x4
#define XMAP_REG_CFG_DATA 0x8
#define XMAP_REG_CFG_STATUS 0xC
#define XMAP_REG_CSR 0x48
#define XMAP_REG_TAR 0x50
#define XMAP_REG_TDR 0x54
#define XMAP_REG_TCR 0x58
#define XMAP_REG_ARB 0x70

/* Special arbitration bits */
#define XMAP_CLEAR_ARB 0x0

/* These are the constants for the CFG Control register. */
#define XMAP_CONTROL_SYS_FPGA 0x1
#define XMAP_CONTROL_FIP_A 0x2
#define XMAP_CONTROL_FIP_B 0x4

/*
 * A map of the status values for the XDONE and INIT* lines indexed by target.
 * To access this, you would use syntax like:
 *
 * system_fpga_xdone = XMAP_CFG_STATUS[0][XMAP_XDONE];
 *
 * etc.
 */
#define XMAP_NUM_TARGETS 3

#define XMAP_INIT 0
#define XMAP_XDONE 1

/* Used for controlling the DSP interface */
#define XMAP_DSP_RESET_BIT 2
#define XMAP_DSP_BOOT_BIT 3
#define XMAP_CSR_RUN_ENA 0
#define XMAP_CSR_RESET_MCA 1
#define XMAP_CSR_RUN_ACT_BIT 16
#define XMAP_CSR_DSP_ACT_BIT 17

static unsigned long XMAP_CFG_STATUS[XMAP_NUM_TARGETS][2] = {
    {0x1, 0x2},
    {0x4, 0x8},
    {0x10, 0x20},
};

static char* XMAP_FPGA_NAMES[XMAP_NUM_TARGETS] = {
    "system FPGA",
    "FiPPI A",
    "FiPPI B",
};

/* Transfer Address Register (TAR) constants */
#define XMAP_PROGRAM_MEMORY 0x0000000UL
#define XMAP_DATA_MEMORY 0x1000000UL
#define XMAP_32_EXT_MEMORY 0x3000000UL
#define XMAP_BUF_A_MEMORY 0x4000000UL

/* RUNTYPE constants */
#define XMAP_RUNTYPE_NORMAL 0
#define XMAP_RUNTYPE_SPECIAL 1

/* TRACETYPE constants */
#define XMAP_TRACETYPE_ADC 0x0
#define XMAP_TRACETYPE_FAST_BASE_SUB 0x4
#define XMAP_TRACETYPE_BASE_INST 0x6
#define XMAP_TRACETYPE_BASE_HIST 0x7
#define XMAP_TRACETYPE_BASE_SUB 0x8
#define XMAP_TRACETYPE_SLOW_BASE_SUB 0xA
#define XMAP_TRACETYPE_EVENTS 0xB

/* SPECIALRUN constants */
#define XMAP_SPECIALRUN_APPLY 0
#define XMAP_SPECIALRUN_TRACE 1
#define XMAP_SPECIALRUN_TEST_1 2 /* Writes test pattern #1 to external memory */
#define XMAP_SPECIALRUN_TEST_2 3 /* Writes test pattern #2 to external memory */
#define XMAP_SPECIALRUN_DSP_SLEEP 7

/* Memory constants */
#define XMAP_MEMORY_BLOCK_SIZE 256U
#define XMAP_MEMORY_32_MAX_ADDR 0x100000UL

/*
 * These are relative offsets for each channel in the external memory
 * statistics block.
 */
static unsigned long XMAP_STATS_CHAN_OFFSET[] = {0x000000, 0x000040, 0x000080,
                                                 0x0000C0};

#define XMAP_STATS_REALTIME_OFFSET 0x0
#define XMAP_STATS_TLIVETIME_OFFSET 0x2
#define XMAP_STATS_TRIGGERS_OFFSET 0x6
#define XMAP_STATS_MCAEVENTS_OFFSET 0x8
#define XMAP_STATS_UNDERFLOWS_OFFSET 0xA
#define XMAP_STATS_OVERFLOWS_OFFSET 0xC

/* Run control constants */
#define RESUME_RUN 1

/* Debugging constants */
#define MAX_NUM_REWRITES 10
#define MAX_NUM_DSP_RETRY 10
#define MAX_NUM_FPGA_ATTEMPTS 5

/* Function pointers */
typedef int (*do_control_task_FP)(int, int, Board*);
typedef int (*do_control_task_data_FP)(int ioChan, int modChan, unsigned long* data,
                                       Board* b);
typedef int (*do_control_task_info_FP)(int ioChan, int modChan, unsigned int length,
                                       int* info, Board* b);
typedef int (*memory_func_FP)(int ioChan, unsigned long base, unsigned long offset,
                              unsigned long* data);

/* Structures */
typedef struct _control_task {
    int type;
    do_control_task_info_FP fn_info;
    do_control_task_FP fn;
} control_task_t;

typedef struct _control_task_data {
    int type;
    do_control_task_data_FP fn;
} control_task_data_t;

typedef struct _memory_accessor {
    char* name;
    memory_func_FP fn;
} memory_accessor_t;

typedef struct _register_table {
    char* name;
    unsigned long addr;
} register_table_t;

/* Useful structure access macros */
#define PARAMS(x) (x)->system_dsp->params

#endif /* __XIA_XMAP_H__ */
