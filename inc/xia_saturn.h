/*
 * Copyright (c) 2004 X-ray Instrumentation Associates
 *               2005-2012 XIA LLC
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

#ifndef __XIA_SATURN_H__
#define __XIA_SATURN_H__

#include "xerxesdef.h"
#include "xia_xerxes_structures.h"

/* If this is compiled by a C++ compiler, make it clear that these are C routines */
#ifdef __cplusplus
extern "C" {
#endif

#ifdef _XERXES_PROTO_ /* Begin ANSI C prototypes */
XERXES_EXPORT int XERXES_API dxp_init_saturn(Functions* funcs);
static int XERXES_API dxp_init_driver(Interface*);
static int XERXES_API dxp_init_utils(Utils*);
static int XERXES_API dxp_write_tsar(int*, unsigned short*);
static int XERXES_API dxp_write_csr(int*, unsigned short*);
static int XERXES_API dxp_read_csr(int*, unsigned short*);
static int XERXES_API dxp_write_data(int*, unsigned short*, unsigned int);
static int XERXES_API dxp_read_data(int*, unsigned short*, unsigned int);
static int XERXES_API dxp_write_fippi(int*, unsigned short*, unsigned int);
static int XERXES_API dxp_read_word(int*, int*, unsigned short*, unsigned short*);
static int XERXES_API dxp_write_word(int*, int*, unsigned short*, unsigned short*);
static int XERXES_API dxp_read_block(int*, int*, unsigned short*, unsigned int*,
                                     unsigned short*);
static int XERXES_API dxp_write_block(int*, int*, unsigned short*, unsigned int*,
                                      unsigned short*);
static int dxp_begin_run(int* ioChan, int* modChan, unsigned short* gate,
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
static int XERXES_API dxp_get_fpgaconfig(void* fpga);
static int XERXES_API dxp_download_fpga_done(int* modChan, char* name, Board* board);
static int XERXES_API dxp_download_fpgaconfig(int* ioChan, int* modChan, char* name,
                                              Board* board);

static int XERXES_API dxp_download_dspconfig(int*, int*, Board*);
static int XERXES_API dxp_download_dsp_done(int*, int*, int*, Board*, unsigned short*,
                                            float*);
static int XERXES_API dxp_get_dspconfig(Dsp_Info*);
static int XERXES_API dxp_load_dspfile(FILE*, Dsp_Info*);
static int XERXES_API dxp_load_dspsymbol_table(FILE*, Dsp_Info*);
static int XERXES_API dxp_load_dspconfig(FILE*, Dsp_Info*);

static int XERXES_API dxp_decode_error(int*, int*, Dsp_Info*, unsigned short*,
                                       unsigned short*);
static int XERXES_API dxp_clear_error(int*, int*, Board*);

static int dxp_get_runstats(int* ioChan, int* modChan, Board* b, unsigned long* evts,
                            unsigned long* under, unsigned long* over,
                            unsigned long* fast, unsigned long* base, double* live,
                            double* icr, double* ocr);

static int XERXES_API dxp_modify_dspsymbol(int*, int*, char*, unsigned short*, Board*);
static int XERXES_API dxp_write_dsp_param_addr(int*, int*, unsigned int*,
                                               unsigned short*);
static int XERXES_API dxp_read_dspsymbol(int*, int*, char*, Board*, double*);
static int dxp_read_dspparams(int* ioChan, int* modchan, Board* b,
                              unsigned short* params);
static int XERXES_API dxp_write_dspparams(int*, int*, Dsp_Info*, unsigned short*);

static int dxp_get_spectrum_length(int* ioChan, int* modChan, Board* board,
                                   unsigned int* len);
static int dxp_get_baseline_length(int* modChan, Board* b, unsigned int* len);
static unsigned int XERXES_API dxp_get_history_length(Dsp_Info*, unsigned short*);
static int XERXES_API dxp_read_spectrum(int*, int*, Board*, unsigned long*);
static int dxp_read_baseline(int* ioChan, int* modChan, Board* board,
                             unsigned long* baseline);
static int XERXES_API dxp_read_history(int*, int*, Board*, unsigned short*);
static int XERXES_API dxp_write_history(int*, int*, Board*, unsigned int* length,
                                        unsigned short* buffer);
static int XERXES_API dxp_read_mem(int* ioChan, int* modChan, Board* board, char* name,
                                   unsigned long* base, unsigned long* offset,
                                   unsigned long* data);
static int XERXES_API dxp_write_mem(int* ioChan, int* modChan, Board* board, char* name,
                                    unsigned long* base, unsigned long* offset,
                                    unsigned long* data);

static int dxp_write_reg(int* ioChan, int* modChan, char* name, unsigned long* data);
static int dxp_read_reg(int* ioChan, int* modChan, char* name, unsigned long* data);

static int XERXES_API dxp_unhook(Board* board);

static int dxp_get_symbol_by_index(int modChan, unsigned short index, Board* board,
                                   char* name);
static int dxp_get_num_params(int modChan, Board* b, unsigned short* n_params);

#else /* Begin old style C prototypes */
XERXES_EXPORT int XERXES_API dxp_init_saturn();
static int XERXES_API dxp_init_driver();
static int XERXES_API dxp_init_utils();
static int XERXES_API dxp_write_tsar();
static int XERXES_API dxp_write_csr();
static int XERXES_API dxp_read_csr();
static int XERXES_API dxp_write_data();
static int XERXES_API dxp_read_data();
static int XERXES_API dxp_write_fippi();
static int XERXES_API dxp_read_word();
static int XERXES_API dxp_write_word();
static int XERXES_API dxp_read_block();
static int XERXES_API dxp_write_block();
static int XERXES_API dxp_download_fpgaconfig();
static int XERXES_API dxp_download_dspconfig();
static int XERXES_API dxp_download_dsp_done();
static int XERXES_API dxp_get_spectrum_length();
static int XERXES_API dxp_get_baseline_length();
static int XERXES_API dxp_get_history_length();
static int XERXES_API dxp_begin_run();
static int XERXES_API dxp_end_run();
static int XERXES_API dxp_run_active();
static int XERXES_API dxp_begin_control_task();
static int XERXES_API dxp_end_control_task();
static int XERXES_API dxp_control_task_params();
static int XERXES_API dxp_control_task_data();
static int XERXES_API dxp_loc();

static int XERXES_API dxp_get_fpgaconfig();
static int XERXES_API dxp_download_fpga_done();

static int XERXES_API dxp_get_dspconfig();
static int XERXES_API dxp_get_dspdefaults();
static int XERXES_API dxp_load_dspfile();
static int XERXES_API dxp_load_dspsymbol_table();
static int XERXES_API dxp_load_dspconfig();

static int XERXES_API dxp_decode_error();
static int XERXES_API dxp_clear_error();
static int XERXES_API dxp_get_runstats();
static int XERXES_API dxp_modify_dspsymbol();
static int XERXES_API dxp_write_dsp_param_addr();
static int XERXES_API dxp_read_dspsymbol();
static int XERXES_API dxp_read_dspparams();
static int XERXES_API dxp_write_dspparams();
static int XERXES_API dxp_read_spectrum();
static int XERXES_API dxp_read_baseline();
static int XERXES_API dxp_read_history();
static int XERXES_API dxp_write_history static int XERXES_API dxp_read_mem();
static int XERXES_API dxp_write_mem();

static int XERXES_API dxp_write_reg();
static int XERXES_API dxp_read_reg();

static int XERXES_API dxp_unhook();

#endif /*   end if _XERXES_PROTO_ */

#ifdef __cplusplus
}
#endif

/* Logging macro wrappers */
#define dxp_log_error(x, y, z)                                                         \
    saturn_md_log(MD_ERROR, (x), (y), (z), __FILE__, __LINE__)
#define dxp_log_warning(x, y) saturn_md_log(MD_WARNING, (x), (y), 0, __FILE__, __LINE__)
#define dxp_log_info(x, y) saturn_md_log(MD_INFO, (x), (y), 0, __FILE__, __LINE__)
#define dxp_log_debug(x, y) saturn_md_log(MD_DEBUG, (x), (y), 0, __FILE__, __LINE__)

#endif /* Endif for __XIA_SATURN_H__ */
