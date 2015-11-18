/*
 * Copyright (c) 2002-2004 X-ray Instrumentation Associates
 *               2005-2013 XIA LLC
 * All rights reserved
 *
 * NOT COVERED UNDER THE BSD LICENSE. NOT FOR RELEASE TO CUSTOMERS.
 *
 *
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>

#include "md_generic.h"
#include "xerxes_generic.h"
#include "xerxes_structures.h"
#include "xia_xerxes_structures.h"
#include "udxp.h"
#include "xerxes_errors.h"
#include "xia_udxp.h"
#include "udxp_common.h"
#include "xia_assert.h"


#define CMD_START_RUN       0x00
#define CMD_STOP_RUN        0x01
#define CMD_READ_DSP_PARAMS 0x42
#define CMD_RW_DSP_PARAM    0x43
#define CMD_READ_SERIAL_NUM 0x48
#define CMD_FIPPI_CONFIG    0x81

/* Define the length of the error reporting string info_string */
#define INFO_LEN 400

/*
 * Pointer to utility functions
 */
static DXP_MD_IO          udxp_md_io;
static DXP_MD_SET_MAXBLK  udxp_md_set_maxblk;
static DXP_MD_GET_MAXBLK  udxp_md_get_maxblk;
static DXP_MD_LOG         udxp_md_log;
static DXP_MD_ALLOC       udxp_md_alloc;
static DXP_MD_FREE        udxp_md_free;
static DXP_MD_PUTS        udxp_md_puts;
static DXP_MD_WAIT        udxp_md_wait;


static uDXP_Params currentParams;

static byte_t bytesPerBin = 3;

char info_string[INFO_LEN];

/* USB RS232 update
 * Need a modular variable to identify whether the board is USB or RS232
 * This value will determine ioFlags passed to dxp_command function
 */
static boolean_t IS_USB = FALSE_;

/******************************************************************************
 *
 * Routine to create pointers to all the internal routines
 *
 ******************************************************************************/
XERXES_EXPORT int dxp_init_udxp(Functions* funcs)
/* Functions *funcs;    */
{
  funcs->dxp_init_driver = dxp_init_driver;
  funcs->dxp_init_utils  = dxp_init_utils;
  funcs->dxp_get_dspinfo = dxp_get_dspinfo;
  funcs->dxp_get_fipinfo = dxp_get_fipinfo;
  funcs->dxp_get_defaultsinfo = dxp_get_defaultsinfo;
  funcs->dxp_get_dspconfig = dxp_get_dspconfig;
  funcs->dxp_get_fpgaconfig = dxp_get_fpgaconfig;
  funcs->dxp_get_dspdefaults = dxp_get_dspdefaults;
  funcs->dxp_download_fpgaconfig = dxp_download_fpgaconfig;
  funcs->dxp_download_fpga_done = dxp_download_fpga_done;
  funcs->dxp_download_dspconfig = dxp_download_dspconfig;
  funcs->dxp_download_dsp_done = dxp_download_dsp_done;
  funcs->dxp_calibrate_channel = dxp_calibrate_channel;
  funcs->dxp_calibrate_asc = dxp_calibrate_asc;

  funcs->dxp_look_at_me = dxp_look_at_me;
  funcs->dxp_ignore_me = dxp_ignore_me;
  funcs->dxp_clear_LAM = dxp_clear_LAM;
  funcs->dxp_loc = dxp_loc;
  funcs->dxp_symbolname = dxp_symbolname;

  funcs->dxp_read_spectrum = dxp_read_spectrum;
  funcs->dxp_test_spectrum_memory = dxp_test_spectrum_memory;
  funcs->dxp_get_spectrum_length = dxp_get_spectrum_length;
  funcs->dxp_read_baseline = dxp_read_baseline;
  funcs->dxp_test_baseline_memory = dxp_test_baseline_memory;
  funcs->dxp_get_baseline_length = dxp_get_baseline_length;
  funcs->dxp_test_event_memory = dxp_test_event_memory;
  funcs->dxp_get_event_length = dxp_get_event_length;
  funcs->dxp_write_dspparams = dxp_write_dspparams;
  funcs->dxp_write_dsp_param_addr = dxp_write_dsp_param_addr;
  funcs->dxp_read_dspparams = dxp_read_dspparams;
  funcs->dxp_read_dspsymbol = dxp_read_dspsymbol;
  funcs->dxp_modify_dspsymbol = dxp_modify_dspsymbol;
  funcs->dxp_dspparam_dump = dxp_dspparam_dump;

  funcs->dxp_prep_for_readout = dxp_prep_for_readout;
  funcs->dxp_done_with_readout = dxp_done_with_readout;
  funcs->dxp_begin_run = dxp_begin_run;
  funcs->dxp_end_run = dxp_end_run;
  funcs->dxp_run_active = dxp_run_active;
  funcs->dxp_begin_control_task = dxp_begin_control_task;
  funcs->dxp_end_control_task = dxp_end_control_task;
  funcs->dxp_control_task_params = dxp_control_task_params;
  funcs->dxp_control_task_data = dxp_control_task_data;

  funcs->dxp_decode_error = dxp_decode_error;
  funcs->dxp_clear_error = dxp_clear_error;
  funcs->dxp_get_runstats = dxp_get_runstats;
  funcs->dxp_change_gains = dxp_change_gains;
  funcs->dxp_setup_asc = dxp_setup_asc;

  funcs->dxp_setup_cmd = dxp_setup_cmd;

  funcs->dxp_read_mem = dxp_read_mem;
  funcs->dxp_write_mem = dxp_write_mem;

  funcs->dxp_write_reg = dxp_write_reg;
  funcs->dxp_read_reg  = dxp_read_reg;

  funcs->dxp_do_cmd = dxp_do_cmd;

  funcs->dxp_unhook = dxp_unhook;

  funcs->dxp_get_sca_length = dxp_get_sca_length;
  funcs->dxp_read_sca = dxp_read_sca;

  funcs->dxp_get_symbol_by_index = dxp_get_symbol_by_index;
  funcs->dxp_get_num_params      = dxp_get_num_params;

  /* Reset the current DSP parameter information */
  memset(currentParams.serialNumber, 0x00, sizeof(byte_t) * SERIAL_NUM_LEN);
  currentParams.num = 0;

  dxp_init_pic_version_cache();

  return DXP_SUCCESS;
}
/******************************************************************************
 *
 * Routine to initialize the IO Driver library, get the proper pointers
 *
 ******************************************************************************/
static int dxp_init_driver(Interface* iface)
/* Interface *iface;    Input: Pointer to the IO Interface  */
{
  /* Assign all the static vars here to point at the proper library routines */
  udxp_md_io         = iface->funcs->dxp_md_io;
  udxp_md_set_maxblk = iface->funcs->dxp_md_set_maxblk;
  udxp_md_get_maxblk = iface->funcs->dxp_md_get_maxblk;

  /* Try to determine whether the iface is usb2 */
  IS_USB = (boolean_t) (strcmp(iface->dllname, "usb2") == 0);

  return DXP_SUCCESS;
}
/******************************************************************************
 *
 * Routine to initialize the Utility routines, get the proper pointers
 *
 ******************************************************************************/
static int dxp_init_utils(Utils* utils)
/* Utils *utils;     Input: Pointer to the utility functions */
{

  /* Assign all the static vars here to point at the proper library routines */
  udxp_md_log = utils->funcs->dxp_md_log;

  udxp_md_alloc = utils->funcs->dxp_md_alloc;
  udxp_md_free  = utils->funcs->dxp_md_free;

  udxp_md_wait = utils->funcs->dxp_md_wait;
  udxp_md_puts = utils->funcs->dxp_md_puts;

  return DXP_SUCCESS;
}


/******************************************************************************
 * Routine to enable the LAMs(Look-At-Me) on the specified DXP
 *
 * Enable the LAM for a single DXP module.
 *
 ******************************************************************************/
static int dxp_look_at_me(int* ioChan, int* modChan)
{
  UNUSED(ioChan);
  UNUSED(modChan);

  return DXP_SUCCESS;
}


/******************************************************************************
 * Routine to enable the LAMs(Look-At-Me) on the specified DXP
 *
 * Disable the LAM for a single DXP module.
 *
 ******************************************************************************/
static int dxp_ignore_me(int* ioChan, int* modChan)
{
  UNUSED(ioChan);
  UNUSED(modChan);

  return DXP_SUCCESS;
}


/******************************************************************************
 * Routine to clear the LAM(Look-At-Me) on the specified DXP
 *
 * Clear the LAM for a single DXP module.
 *
 ******************************************************************************/
static int dxp_clear_LAM(int* ioChan, int* modChan)
{
  UNUSED(ioChan);
  UNUSED(modChan);

  return DXP_SUCCESS;
}


/********------******------*****------******-------******-------******------*****
 * Now begins the section with higher level routines.  Mostly to handle reading
 * in the DSP and FiPPi programs.  And handling starting up runs, ending runs,
 * runing calibration tasks.
 *
 ********------******------*****------******-------******-------******------*****/

/******************************************************************************
 * Routine to download the FiPPi configuration
 *
 * Specify as either "fippi0", "fippi1" or "fippi2".
 *
 ******************************************************************************/
static int dxp_download_fpgaconfig(int* ioChan, int* modChan, char *name, Board* board)
{
  int status;

  byte_t io_normal = IO_NORMAL;

  byte_t send[2];
  byte_t receive[8];

  unsigned int lenR      = 8;
  unsigned int lenS      = 2;

  unsigned short fippiNum = 0;

  UNUSED(modChan);
  UNUSED(board);

  if (STREQ(name, "all")) {
    return DXP_SUCCESS;
  }

  sscanf(name, "fippi%hu", &fippiNum);

  if (fippiNum > 2) {
    status = DXP_UDXP;
    dxp_log_error("dxp_download_fpgaconfig",
                  "Specified FiPPI configuration is out-of-range",
                  status);
    return status;
  }

  send[0] = (byte_t)0x00;
  send[1] = (byte_t)fippiNum;

  if (IS_USB) {
    io_normal |= IO_USB;
  }

  status = dxp_command(*ioChan, udxp_md_io, CMD_FIPPI_CONFIG, lenS,
                       send, lenR, receive, io_normal);

  if (status != DXP_SUCCESS) {
    status = DXP_UDXP;
    dxp_log_error("dxp_download_fpgaconfig",
                  "Error executing command",
                  status);
    return status;
  }

  if (receive[4] != 0) {
    status = DXP_UDXP;
    dxp_log_error("dxp_download_fpgaconfig",
                  "Board reported an error condition",
                  status);
    return status;
  }

  return DXP_SUCCESS;
}


/******************************************************************************
 * Routine to read the FiPPi configuration file into memory
 *
 * A moot point for the uDXP.
 *
 ******************************************************************************/
static int dxp_get_fpgaconfig(void* fippi)
{
    ((Fippi_Info *)fippi)->maxproglen = MAXFIP_LEN;
    ((Fippi_Info *)fippi)->proglen    = 0;

    return DXP_SUCCESS;
}


/******************************************************************************
 *
 * Routine to check that all the FiPPis downloaded successfully to
 * a single module.  If the routine returns DXP_SUCCESS, then the
 * FiPPis are OK
 *
 ******************************************************************************/
static int dxp_download_fpga_done(int* modChan, char *name, Board *board)
{
  UNUSED(modChan);
  UNUSED(name);
  UNUSED(board);

  /* Return to this if it turns out that
   * checking "something" on the board is 
   * required for the uDXP.
   */

  return DXP_SUCCESS;
}


/******************************************************************************
 * Routine to download the DSP Program
 *
 * DSP code doesn't need to be downloaded to the board for the uDXP.
 *
 ******************************************************************************/
static int dxp_download_dspconfig(int* ioChan, int* modChan, Board *board)
{
  UNUSED(ioChan);
  UNUSED(modChan);
  UNUSED(board);

  return DXP_SUCCESS;
}


/******************************************************************************
 *
 * Routine to check that the DSP is done with its initialization.
 * If the routine returns DXP_SUCCESS, then the DSP is ready to run.
 *
 ******************************************************************************/
static int dxp_download_dsp_done(int* ioChan, int* modChan, int* mod,
                                 Board *board, unsigned short* value,
                                 float* timeout)
{
  UNUSED(ioChan);
  UNUSED(modChan);
  UNUSED(mod);
  UNUSED(board);
  UNUSED(value);
  UNUSED(timeout);

  return DXP_SUCCESS;
}


/*****************************************************************************
 *
 * Routine to retrieve the FIPPI program maximum sizes so that memory
 * can be allocated.
 *
 ******************************************************************************/
static int dxp_get_fipinfo(void* fippi)
{
    ((Fippi_Info *)fippi)->maxproglen = MAXFIP_LEN;

    return DXP_SUCCESS;
}


/******************************************************************************
 *
 * Routine to retrieve the DSP defaults parameters maximum sizes so that memory
 * can be allocated.
 *
 ******************************************************************************/
static int dxp_get_defaultsinfo(Dsp_Defaults* defaults)
{

  defaults->params->maxsym     = MAXSYM;
  defaults->params->maxsymlen  = MAX_DSP_PARAM_NAME_LEN;
  defaults->params->nsymbol    = 0;

  return DXP_SUCCESS;
}


/******************************************************************************
 *
 * Routine to retrieve the DSP program maximum sizes so that memory
 * can be allocated.
 *
 ******************************************************************************/
static int dxp_get_dspinfo(Dsp_Info* dsp)
{
  dsp->params->maxsym    = MAXSYM;
  dsp->params->maxsymlen = MAX_DSP_PARAM_NAME_LEN;
  dsp->params->nsymbol   = 0;
  dsp->maxproglen        = MAXDSP_LEN;

  return DXP_SUCCESS;

}


/******************************************************************************
 * Routine to retrieve the DSP program and symbol table
 *
 * Read the DSP configuration file  -- logical name (or symbolic link)
 * DSP_CONFIG must be defined prior to execution of the program. At present,
 * it is NOT possible to use different configurations for different DXP
 * channels.
 *
 ******************************************************************************/
static int dxp_get_dspconfig(Dsp_Info* dsp)
{
  dsp->params->maxsym    = MAXSYM;
  dsp->params->maxsymlen = MAX_DSP_PARAM_NAME_LEN;
  dsp->params->nsymbol   = 0;
  dsp->maxproglen        = MAXDSP_LEN;
  dsp->proglen           = 0;

  return DXP_SUCCESS;
}


/******************************************************************************
 * Routine to retrieve the default DSP parameter values
 *
 * Read the file pointed to by filename and read in the
 * symbol value pairs into the array.  No writing of data
 * here.
 *
 ******************************************************************************/
static int dxp_get_dspdefaults(Dsp_Defaults* defaults)
{
  defaults->params->nsymbol = 0;

  return DXP_SUCCESS;
}


/******************************************************************************
 * Routine to return the symbol name at location index in the symbol table
 * of the DSP
 *
 ******************************************************************************/
static int dxp_symbolname(unsigned short* lindex, Dsp_Info* dsp, char string[])
{
  UNUSED(lindex);
  UNUSED(dsp);
  UNUSED(string);

  return DXP_SUCCESS;
}


/******************************************************************************
 * Routine to locate a symbol in the DSP symbol table.
 *
 * This routine returns the address of the symbol called name in the DSP
 * symbol table.
 *
 ******************************************************************************/
static int dxp_loc(char name[], Dsp_Info* dsp, unsigned short* address)
{
  int status;

  unsigned short i = 0;

  UNUSED(name);
  UNUSED(dsp);
  UNUSED(address);


  for (i = 0; i < currentParams.num; i++) {
    if (STREQ(currentParams.names[i], name)) {
      *address = i;
      return DXP_SUCCESS;
    }
  }

  status = DXP_UDXP;
  dxp_log_error("dxp_loc", "Unknown DSP parameter", status);
  return status;
}


/******************************************************************************
 * Routine to dump the memory contents of the DSP.
 *
 * This routine prints the memory contents of the DSP in channel modChan
 * of the DXP of ioChan.
 *
 ******************************************************************************/
static int dxp_dspparam_dump(int* ioChan, int* modChan, Dsp_Info* dsp)
{
  UNUSED(ioChan);
  UNUSED(modChan);
  UNUSED(dsp);

  return DXP_SUCCESS;
}


/******************************************************************************
 * Routine to test the DSP spectrum memory
 *
 * Write the specified pattern into the spectrum memory and then
 * read it out. If it doesn't match then we know that we have a problem some
 * where. For the record, this is a pretty paranoid check since this
 * usually indicates a pathological communication flaw that would be
 * seen in any communication prior to the calling of this routine.
 *
 ******************************************************************************/
static int dxp_test_spectrum_memory(int* ioChan, int* modChan, int* pattern,
                                    Board *board)
{
  UNUSED(ioChan);
  UNUSED(modChan);
  UNUSED(pattern);
  UNUSED(board);

  /* Defer implementation of this routine
   * until later.
   */

  return DXP_SUCCESS;
}


/******************************************************************************
 * Routine to test the DSP baseline memory
 *
 * Test the DSP memory for a single channel of a single DXP module by writing
 * a string of words to reserved baseline memory space, reading them back and
 * performing a compare.  The input pattern is defined using
 *
 * see dxp_test_mem for the meaning of *pattern..
 *
 ******************************************************************************/
static int dxp_test_baseline_memory(int* ioChan, int* modChan, int* pattern,
                                    Board *board)
{
  UNUSED(ioChan);
  UNUSED(modChan);
  UNUSED(pattern);
  UNUSED(board);

  return DXP_SUCCESS;
}


/******************************************************************************
 * Routine to test the DSP event buffermemory
 *
 * Test the DSP memory for a single channel of a single DXP module by writing
 * a string of words to reserved event buffer memory space, reading them back and
 * performing a compare.  The input pattern is defined using
 *
 * see dxp_test_mem for the meaning of *pattern..
 *
 ******************************************************************************/
static int dxp_test_event_memory(int* ioChan, int* modChan, int* pattern,
                                 Board *board)
{
  UNUSED(ioChan);
  UNUSED(modChan);
  UNUSED(pattern);
  UNUSED(board);

  return DXP_SUCCESS;
}


/******************************************************************************
 *
 * Set a parameter of the DSP.  Pass the symbol name, value to set and module
 * pointer and channel number.
 *
 ******************************************************************************/
static int dxp_modify_dspsymbol(int* ioChan, int* modChan, char* name,
                                unsigned short* value, Board *board)
{
  int status;

  unsigned int lenS = 4;
  unsigned int lenR = 8;

  unsigned short addr = 0x0000;

  byte_t io_normal = IO_NORMAL;

  byte_t send[4];
  byte_t receive[8];

  Dsp_Info *dsp = NULL;

  UNUSED(ioChan);
  UNUSED(name);
  UNUSED(value);
  UNUSED(board);


  dsp = board->dsp[*modChan];
  ASSERT(dsp != NULL);

  /* Get index of symbol */
  status = dxp_loc(name, dsp, &addr);

  if (status != DXP_SUCCESS) {
    dxp_log_error("dxp_modify_dspsymbol",
                  "Error finding DSP parameter",
                  status);

    return status;
  }

  send[0] = 0x01;
  send[1] = (byte_t)addr;
  send[2] = (byte_t)(*value & 0xFF);
  send[3] = (byte_t)(((*value) >> 8) & 0xFF);

  /* Write new value to the board */
  if (IS_USB) {
    io_normal |= IO_USB;
  }
  
  status = dxp_command(*ioChan, udxp_md_io, CMD_RW_DSP_PARAM,
                       lenS, send, lenR, receive, io_normal);

  if (status != DXP_SUCCESS) {
    dxp_log_error("dxp_modify_dspsymbol",
                  "Error executing command",
                  status);

    return status;

  }

  if (receive[4] != 0) {
    dxp_log_error("dxp_modify_dspsymbol",
                  "Board reported an error condition",
                  status);

    return status;

  }

  return DXP_SUCCESS;
}


/******************************************************************************
 *
 * Write a single parameter to the DSP.  Pass the symbol address, value, module
 * pointer and channel number.
 *
 ******************************************************************************/
static int dxp_write_dsp_param_addr(int* ioChan, int* modChan,
                                    unsigned int* addr, unsigned short* value)
{
  UNUSED(ioChan);
  UNUSED(modChan);
  UNUSED(addr);
  UNUSED(value);

  return DXP_SUCCESS;
}


/******************************************************************************
 *
 * Read a single parameter of the DSP.  Pass the symbol name, module
 * pointer and channel number.  Returns the value read using the variable value.
 * If the symbol name has the 0/1 dropped from the end, then the 32-bit
 * value is created from the 0/1 contents.  e.g. zigzag0 and zigzag1 exist
 * as a 32 bit number called zigzag.  if this routine is called with just
 * zigzag, then the 32 bit number is returned, else the 16 bit value
 * for zigzag0 or zigzag1 is returned depending on what was passed as *name.
 *
 ******************************************************************************/
static int dxp_read_dspsymbol(int* ioChan, int* modChan, char* name,
                              Board *board, double* value)
{
  int status;

  unsigned int lenS = 2;
  unsigned int lenR = 8;

  byte_t io_normal = IO_NORMAL;

  byte_t send[2];
  byte_t receive[8];

  unsigned short addr = 0x0000;


  Dsp_Info *dsp = NULL;

  UNUSED(ioChan);
  UNUSED(value);


  dsp = board->dsp[*modChan];
  ASSERT(dsp != NULL);

  if (strlen(name) > dsp->params->maxsymlen) {
    status = DXP_NOSYMBOL;
    sprintf(info_string,
            "Symbol name must be < %i characters",
            dsp->params->maxsymlen - 1);
    dxp_log_error("dxp_read_dspsymbol", info_string, status);
    return status;
  }

  status = dxp_loc(name, dsp, &addr);

  sprintf(info_string, "%s @ %u", name, addr);
  dxp_log_debug("dxp_read_dspsymbol", info_string);

  if (status != DXP_SUCCESS) {
    dxp_log_error("dxp_read_dspsymbol",
                  "Error finding DSP parameter",
                  status);
    return status;
  }

  send[0] = 0x00;
  send[1] = (byte_t)(addr & 0xFF);

  if (IS_USB) {
    io_normal |= IO_USB;
  }
  
  status = dxp_command(*ioChan, udxp_md_io, CMD_RW_DSP_PARAM,
                       lenS, send, lenR, receive, io_normal);

  if (status != DXP_SUCCESS) {
    dxp_log_error("dxp_read_dspsymbol",
                  "Error executing command",
                  status);
    return status;
  }

  if (receive[4] != 0) {
    status = DXP_UDXP;
    dxp_log_error("dxp_read_dspsymbol",
                  "Board reported an error condition",
                  status);
    return status;
  }

  *value = (double)((double) receive[5] +
                    ((double) receive[6] * 256.));

  return DXP_SUCCESS;
}


/******************************************************************************
 * Routine to readout the parameter memory from a single DSP.
 *
 * This routine reads the parameter list from the DSP pointed to by ioChan and
 * modChan.  It returns the array to the caller.
 *
 ******************************************************************************/
static int dxp_read_dspparams(int* ioChan, int* modChan, Board *b,
                              unsigned short* params)
{
  UNUSED(ioChan);
  UNUSED(modChan);
  UNUSED(b);
  UNUSED(params);

  /* Since this normally just modifies the
   * Dsp_Info struct, we won't do anything
   * for now [since we have our own
   * private uDXP strucut].
   */

  return DXP_SUCCESS;
}


/******************************************************************************
 * Routine to write parameter memory to a single DSP.
 *
 * This routine writes the parameter list to the DSP pointed to by ioChan and
 * modChan.
 *
 ******************************************************************************/
static int dxp_write_dspparams(int* ioChan, int* modChan, Dsp_Info* dsp,
                               unsigned short* params)
{
  UNUSED(ioChan);
  UNUSED(modChan);
  UNUSED(dsp);
  UNUSED(params);

  return DXP_SUCCESS;
}


/******************************************************************************
 * Routine to return the length of the spectrum memory.
 *
 * For 4C-2X boards, this value is stored in the DSP and dynamic.
 * For 4C boards, it is fixed.
 *
 ******************************************************************************/
static int dxp_get_spectrum_length(int *ioChan, int *modChan,
                                   Board *board, unsigned int *len)
{
  UNUSED(ioChan);
  UNUSED(modChan);
  UNUSED(board);
  UNUSED(len);

  return DXP_SUCCESS;
}


/******************************************************************************
 * Routine to return the length of the baseline memory.
 *
 * For 4C-2X boards, this value is stored in the DSP and dynamic.
 * For 4C boards, it is fixed.
 *
 ******************************************************************************/
static int dxp_get_baseline_length(int *modChan, Board *b, unsigned int *len)
{
  UNUSED(modChan);
  UNUSED(b);
  UNUSED(len);


  return DXP_SUCCESS;
}


/******************************************************************************
 * Routine to return the length of the event memory.
 *
 * For 4C-2X boards, this value is stored in the DSP and dynamic.
 * For 4C boards, it is fixed.
 *
 ******************************************************************************/
static unsigned int dxp_get_event_length(Dsp_Info* dsp, unsigned short* params)
{
  UNUSED(dsp);
  UNUSED(params);

  return 0;
}


/******************************************************************************
 * Routine to readout the spectrum memory from a single DSP.
 *
 * This routine reads the spectrum histogramfrom the DSP pointed to by ioChan and
 * modChan.  It returns the array to the caller.
 *
 ******************************************************************************/
static int dxp_read_spectrum(int* ioChan, int* modChan, Board* board,
                             unsigned long* spectrum)
{
  UNUSED(ioChan);
  UNUSED(modChan);
  UNUSED(board);
  UNUSED(spectrum);




  return DXP_SUCCESS;
}


/******************************************************************************
 * Routine to readout the baseline histogram from a single DSP.
 *
 * This routine reads the baselin histogram from the DSP pointed to by ioChan and
 * modChan.  It returns the array to the caller.
 *
 ******************************************************************************/
static int dxp_read_baseline(int* ioChan, int* modChan, Board* board,
                             unsigned long* baseline)
{
  UNUSED(ioChan);
  UNUSED(modChan);
  UNUSED(board);
  UNUSED(baseline);

  return DXP_SUCCESS;
}


/******************************************************************************
 * Routine to prepare the device for data readout.
 *
 *
 ******************************************************************************/
int dxp_prep_for_readout(int* ioChan, int *modChan)
{
  UNUSED(ioChan);
  UNUSED(modChan);

  /* Nothing needs to be done */

  return DXP_SUCCESS;
}


/******************************************************************************
 * Routine to prepare the device for data readout.
 *
 *
 ******************************************************************************/
int dxp_done_with_readout(int* ioChan, int *modChan, Board *board)
{
  UNUSED(ioChan);
  UNUSED(modChan);
  UNUSED(board);

  /* Nothing needs to be done */

  return DXP_SUCCESS;
}


/******************************************************************************
 * Routine to begin a data taking run.
 *
 * This routine starts a run on the specified channel.  It tells the DXP
 * whether to ignore the gate signal and whether to clear the MCA.
 *
 ******************************************************************************/
static int dxp_begin_run(int* ioChan, int* modChan, unsigned short* gate,
                         unsigned short* resume, Board *board, int *id)
{
  int status;

  unsigned int lenS = 1;
  unsigned int lenR = 8;

  byte_t io_normal = IO_NORMAL;

  byte_t send[1];
  byte_t receive[8];

  UNUSED(modChan);
  UNUSED(gate);
  UNUSED(board);
  UNUSED(id);


  /* Xerxes and the uDXP have different ideas
   * of what resume should be set to. For instance,
   * Xerxes thinks that resume = 1 means that the run
   * should be resumed w/o clearing the MCA. The uDXP
   * prefers that resume = 0.
   */
  send[0] = (byte_t)(0x01 ^ *resume);


  if (IS_USB) {
    io_normal |= IO_USB;
  }

  status = dxp_command(*ioChan, udxp_md_io, CMD_START_RUN,
                       lenS, send, lenR, receive, io_normal);

  if (status != DXP_SUCCESS) {
    dxp_log_error("dxp_begin_run",
                  "Error executing command",
                  status);
    return status;
  }

  if (receive[4] != 0) {
    dxp_log_error("dxp_begin_run",
                  "Board reported an error condition",
                  status);
    return status;
  }

  /* Probably need to return the run number
   * (receive[5] and receive[6]) back to
   * the user but there is no mechanism in 
   * Xerxes for that kind of information.
   */

  return DXP_SUCCESS;
}


/******************************************************************************
 * Routine to end a data taking run.
 *
 * This routine ends the run on the specified channel.
 *
 ******************************************************************************/
static int dxp_end_run(int* ioChan, int* modChan, Board *board)
{
  int status;

  unsigned int lenS = 0;
  unsigned int lenR = 6;

  byte_t io_normal = IO_NORMAL;

  byte_t receive[6];

  UNUSED(modChan);
  UNUSED(board);


  if (IS_USB) {
    io_normal |= IO_USB;
  }

  status = dxp_command(*ioChan, udxp_md_io, CMD_STOP_RUN,
                       lenS, NULL, lenR, receive, io_normal);

  if (status != DXP_SUCCESS) {
    dxp_log_error("dxp_end_run",
                  "Error executing command",
                  status);
    return status;
  }

  if (receive[4] != 0) {
    dxp_log_error("dxp_end_run",
                  "Board reported an error condition",
                  status);
    return status;
  }

  return DXP_SUCCESS;
}


/******************************************************************************
 * Routine to determine if the module thinks a run is active.
 *
 ******************************************************************************/
static int dxp_run_active(int* ioChan, int* modChan, int* active)
{
  UNUSED(ioChan);
  UNUSED(modChan);
  UNUSED(active);

  return DXP_SUCCESS;
}


/******************************************************************************
 *
 * Routine to start a control task routine.  Definitions for types are contained
 * in the xerxes_generic.h file.
 *
 ******************************************************************************/
static int dxp_begin_control_task(int* ioChan, int* modChan, short *type,
                                  unsigned int *length, int *info, Board *board)
{
  UNUSED(ioChan);
  UNUSED(modChan);
  UNUSED(type);
  UNUSED(length);
  UNUSED(info);
  UNUSED(board);

  return DXP_SUCCESS;
}


/******************************************************************************
 *
 * Routine to end a control task routine.
 *
 ******************************************************************************/
static int dxp_end_control_task(int* ioChan, int* modChan, Board *board)
{
  UNUSED(ioChan);
  UNUSED(modChan);
  UNUSED(board);

  return DXP_SUCCESS;
}


/******************************************************************************
 *
 * Routine to get control task parameters.
 *
 ******************************************************************************/
static int dxp_control_task_params(int* ioChan, int* modChan, short *type,
                                   Board *board, int *info)
{
  UNUSED(ioChan);
  UNUSED(modChan);
  UNUSED(type);
  UNUSED(board);
  UNUSED(info);

  return DXP_SUCCESS;
}


/******************************************************************************
 *
 * Routine to return control task data.
 *
 ******************************************************************************/
static int dxp_control_task_data(int* ioChan, int* modChan, short *type,
                                 Board *board, void *data)
{
  UNUSED(ioChan);
  UNUSED(modChan);
  UNUSED(type);
  UNUSED(board);
  UNUSED(data);

  return DXP_SUCCESS;
}


/******************************************************************************
 * Routine to decode the error message from the DSP after a run if performed
 *
 * Returns the RUNERROR and ERRINFO words from the DSP parameter block
 *
 ******************************************************************************/
static int dxp_decode_error(int* ioChan, int* modChan, Dsp_Info* dsp,
                            unsigned short* runerror, unsigned short* errinfo)
{
  UNUSED(ioChan);
  UNUSED(modChan);
  UNUSED(dsp);

  *runerror = 0;
  *errinfo  = 0;

  return DXP_SUCCESS;
}


/******************************************************************************
 * Routine to clear an error in the DSP
 *
 * Clears non-fatal DSP error in one or all channels of a single DXP module.
 * If modChan is -1 then all channels are cleared on the DXP.
 *
 ******************************************************************************/
static int dxp_clear_error(int* ioChan, int* modChan, Board *board)
{
  UNUSED(ioChan);
  UNUSED(modChan);
  UNUSED(board);

  return DXP_SUCCESS;
}


/******************************************************************************
 * Routine to get run statistics from the DXP.
 *
 * Returns some run statistics from the parameter block array[]
 *
 ******************************************************************************/
static int dxp_get_runstats(int *ioChan, int *modChan, Board *b,
                            unsigned long* evts, unsigned long* under,
                            unsigned long* over, unsigned long* fast,
                            unsigned long* base, double* live,
                            double* icr, double* ocr)
{
  UNUSED(ioChan);
  UNUSED(modChan);
  UNUSED(b);
  UNUSED(evts);
  UNUSED(under);
  UNUSED(over);
  UNUSED(fast);
  UNUSED(base);
  UNUSED(live);
  UNUSED(icr);
  UNUSED(ocr);

  return DXP_SUCCESS;
}


/******************************************************************************
 *
 * This routine changes all DXP channel gains:
 *
 *            initial         final
 *            gain_i -> gain_i*gainchange
 *
 * THRESHOLD is also modified to keep the energy threshold fixed.
 * Note: DACPERADC must be modified -- this can be done via calibration
 *       run or by scaling DACPERADC --> DACPERADC/gainchange.  This routine
 *       does not do either!
 *
 ******************************************************************************/
static int dxp_change_gains(int* ioChan, int* modChan, int* module,
                            float* gainchange, Board *board)
{
  UNUSED(ioChan);
  UNUSED(modChan);
  UNUSED(module);
  UNUSED(gainchange);
  UNUSED(board);

  return DXP_SUCCESS;
}


/******************************************************************************
 *
 * Adjust the DXP channel's gain parameters to achieve the desired ADC rule.
 * The ADC rule is the fraction of the ADC full scale that a single x-ray step
 * of energy *energy contributes.
 *
 * set following, detector specific parameters for all DXP channels:
 *       COARSEGAIN
 *       FINEGAIN
 *       VRYFINGAIN (default:128)
 *       POLARITY
 *       OFFDACVAL
 *
 ******************************************************************************/
static int dxp_setup_asc(int* ioChan, int* modChan, int* module, float* adcRule,
                         float* gainmod, unsigned short* polarity, float* vmin,
                         float* vmax, float* vstep, Board *board)
{
  UNUSED(ioChan);
  UNUSED(modChan);
  UNUSED(module);
  UNUSED(adcRule);
  UNUSED(gainmod);
  UNUSED(polarity);
  UNUSED(vmin);
  UNUSED(vmax);
  UNUSED(vstep);
  UNUSED(board);

  return DXP_SUCCESS;
}


/******************************************************************************
 *
 * Perform the neccessary calibration runs to get the ASC ready to take data
 *
 ******************************************************************************/
static int dxp_calibrate_asc(int* mod, int* camChan, unsigned short* used,
                             Board *board)
{
  UNUSED(mod);
  UNUSED(camChan);
  UNUSED(used);
  UNUSED(board);

  return DXP_SUCCESS;
}


/******************************************************************************
 *
 * Preform internal gain calibration or internal TRACKDAC reset point
 * for all DXP channels:
 *    o save the current value of RUNTASKS for each channel
 *    o start run
 *    o wait
 *    o stop run
 *    o readout parameter memory
 *    o check for errors, clear errors if set
 *    o check calibration results..
 *    o restore RUNTASKS for each channel
 *
 ******************************************************************************/
static int dxp_calibrate_channel(int* mod, int* camChan, unsigned short* used,
                                 int* calibtask, Board *board)
{
  UNUSED(mod);
  UNUSED(camChan);
  UNUSED(used);
  UNUSED(calibtask);
  UNUSED(board);

  return DXP_SUCCESS;
}


/**********
 * This routine sets special values specific
 * to the uDXP which can't be accomodated by
 * Xerxes. This is a bit of a hack but it really
 * isn't that much of a stretch.
 **********/
static int dxp_setup_cmd(Board *board, char *name, unsigned int *lenS, byte_t *send,
                         unsigned int *lenR, byte_t *receive, byte_t ioFlags)
{
  UNUSED(board);
  UNUSED(name);
  UNUSED(lenS);
  UNUSED(send);
  UNUSED(lenR);
  UNUSED(receive);
  UNUSED(ioFlags);

  return DXP_SUCCESS;
}


/**
 * Directly reads @a offset count entries from the memory starting at @a base.
 *
 * The @a data array is unsigned long, but the result of the
 * underlying read might not be. The underlying values will be copied
 * into @a data but not packed into it, which means you might have
 * some 0 padding in @a data. It is the job of the calling routine to transfer
 * from @a data to the correctly typed array.
 */
XERXES_STATIC int dxp_read_mem(int *ioChan, int *modChan, Board *board,
                               char *name, unsigned long *base,
                               unsigned long *offset,
                               unsigned long *data)
{
  int status;

  unsigned long i;

  unsigned short *us_data = NULL;


  ASSERT(name != NULL);
  ASSERT(offset != NULL);
  ASSERT(board != NULL);
  ASSERT(ioChan != NULL);
  ASSERT(modChan != NULL);
  ASSERT(base != NULL);
  ASSERT(data != NULL);
  ASSERT((*modChan) == 0);


  if (!STREQ(name, "direct")) {
      sprintf(info_string, "A memory read of type '%s' is not currently "
              "supported for ioChan = %d, modChan = %d", name, *ioChan,
              *modChan);
      dxp_log_error("dxp_read_mem", info_string, DXP_UNIMPLEMENTED);
      return DXP_UNIMPLEMENTED;
  }

  if (!IS_USB) {
    sprintf(info_string, "Memory access only supported in USB mode");
    dxp_log_error("dxp_read_mem", info_string, DXP_UNIMPLEMENTED);
    return DXP_UNIMPLEMENTED;
  }

  us_data = udxp_md_alloc((*offset) * sizeof(unsigned short));
  
  if (us_data == NULL) {
      sprintf(info_string, "Unable to allocate %d bytes for 'us_data' for "
              "ioChan = %d, modChan = %d",
              (int)((*offset) * sizeof(unsigned short)), *ioChan, *modChan);
      dxp_log_error("dxp_read_mem", info_string, DXP_NOMEM);
      return DXP_NOMEM;
  }

  status = dxp_usb_read_block(board->detChan[*modChan], udxp_md_io, *base,
                              *offset, us_data);


  if (status != DXP_SUCCESS) {
      udxp_md_free(us_data);
      sprintf(info_string, "Error reading data memory block (addr = %#lx, "
              "n = %lu) for ioChan = %d", *base, *offset, *ioChan);
      dxp_log_error("dxp_read_mem", info_string, status);
      return status;
  }

  for (i = 0; i < *offset; i++) {
      data[i] = (unsigned long)us_data[i];
  }

  udxp_md_free(us_data);

  return DXP_SUCCESS;
}


XERXES_STATIC int dxp_write_mem(int *ioChan, int *modChan, Board *board,
                                char *name, unsigned long *base,
                                unsigned long *offset, unsigned long *data)
{
    int status;

    unsigned short *us_data = NULL;

    unsigned long i;

    UNUSED(board);


    ASSERT(name);
    ASSERT(ioChan);
    ASSERT(modChan);
    ASSERT(offset);
    ASSERT((*offset) > 0);
    ASSERT(base);
    ASSERT(data);


    if (!STREQ(name, "direct")) {
        sprintf(info_string, "A memory write of type '%s' is not currently "
                "supported for ioChan = %d, modChan = %d.", name, *ioChan,
                *modChan);
        dxp_log_error("dxp_write_mem", info_string, DXP_UNIMPLEMENTED);
        return DXP_UNIMPLEMENTED;
    }

    if (!IS_USB) {
        dxp_log_error("dxp_write_mem", "Direct memory writes are only "
                      "supported in USB mode.", DXP_UNIMPLEMENTED);
        return DXP_UNIMPLEMENTED;
    }

    us_data = udxp_md_alloc((*offset) * sizeof(unsigned short));

    if (!us_data) {
        sprintf(info_string, "Unable to allocate %d bytes for 'us_data' for "
                "ioChan = %d, modChan = %d.",
                (*offset) * sizeof(unsigned short), *ioChan, *modChan);
        dxp_log_error("dxp_write_mem", info_string, DXP_NOMEM);
        return DXP_NOMEM;
    }

    for (i = 0; i < *offset; i++) {
        /* Yes this clips the top 16-bits off your data. */
        us_data[i] = (unsigned short)(data[i] & 0xFFFF);
    }

    status = dxp_usb_write_block(*ioChan, udxp_md_io, *base, *offset, us_data);

    udxp_md_free(us_data);

    if (status != DXP_SUCCESS) {
        sprintf(info_string, "Error writing %lu words to %#lx for ioChan = %d, "
                "modChan = %d", *offset, *base, *ioChan, *modChan);
        dxp_log_error("dxp_write_mem", info_string, status);
        return status;
    }
    
    return DXP_SUCCESS;
}


/**********
 * This routine does nothing currently.
 **********/
static int dxp_write_reg(int *ioChan, int *modChan, char *name,
                         unsigned long *data)
{
  UNUSED(ioChan);
  UNUSED(modChan);
  UNUSED(name);
  UNUSED(data);

  return DXP_SUCCESS;
}


/**********
 * This routine currently does nothing.
 **********/
XERXES_STATIC int dxp_read_reg(int *ioChan, int *modChan, char *name,
                               unsigned long *data)
{
  UNUSED(ioChan);
  UNUSED(modChan);
  UNUSED(name);
  UNUSED(data);

  return DXP_SUCCESS;
}


/**********
 * This routine simply calls the dxp_command() routine.
 **********/
XERXES_STATIC int XERXES_API dxp_do_cmd(int *ioChan, byte_t cmd, unsigned int lenS,
                                        byte_t *send, unsigned int lenR, byte_t *receive)
{
  int status;
  byte_t ioFlags = IO_NORMAL;


  if (IS_USB) {
    ioFlags |= IO_USB;
  }

  status = dxp_command(*ioChan, udxp_md_io, cmd, lenS, send,
                       lenR, receive, ioFlags);

  if (status != DXP_SUCCESS) {
    dxp_log_error("dxp_do_cmd", "Command error", status);
    return status;
  }

  return DXP_SUCCESS;
}


/**********
 * Calls the interface close routine.
 **********/
XERXES_STATIC int XERXES_API dxp_unhook(Board *board)
{
  int status;

  sprintf(info_string, "Attempting to unhook ioChan = %d", board->ioChan);
  dxp_log_debug("dxp_unhook", info_string);

  status = board->iface->funcs->dxp_md_close(&(board->ioChan));

  /* Ignore the status due to some issues involving
   * repetitive function calls.
   */

  return DXP_SUCCESS;
}


/** @brief Get the length of the SCA data buffer
 *
 */
XERXES_STATIC unsigned int dxp_get_sca_length(Dsp_Info *dsp,
                                              unsigned short *params)
{
    UNUSED(dsp);
    UNUSED(params);

    return DXP_SUCCESS;
}


/** @brief Read out the SCA data buffer from the board
 *
 */
XERXES_STATIC int XERXES_API dxp_read_sca(int *ioChan, int *modChan,
                                          Board* board, unsigned long *sca)
{
    UNUSED(ioChan);
    UNUSED(modChan);
    UNUSED(board);
    UNUSED(sca);

    return DXP_SUCCESS;
}


XERXES_STATIC int dxp_get_symbol_by_index(int modChan, unsigned short index,
                                          Board *board, char *name)
{
  UNUSED(modChan);
  UNUSED(index);
  UNUSED(board);
  UNUSED(name);

  return DXP_SUCCESS;
}


XERXES_STATIC int dxp_get_num_params(int modChan, Board *b,
                                     unsigned short *n_params)
{
  UNUSED(modChan);
  UNUSED(b);
  UNUSED(n_params);

  return DXP_SUCCESS;
}
