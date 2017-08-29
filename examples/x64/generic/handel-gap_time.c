/*
 * Example code to demostrate setting of gap_time through minimum_gap_time
 *
 * Copyright (c) 2008-2016, XIA LLC
 * All rights reserved
 *
 * $Id$
 *
 */

#include <stdio.h>
#include <stdlib.h>

/* For Sleep() */
#ifdef _WIN32
#include <windows.h>
#endif /* _WIN32 */

#include "handel.h"
#include "handel_errors.h"
#include "md_generic.h"

static void print_usage(void);
static void CHECK_ERROR(int status);

int main(int argc, char *argv[])
{
  int status;
	int dummy = 0;
  
  /* Acquisition Values */
  double gap_time  = 0;
  double minimum_gap_time = 0.60;

  if (argc < 2) {
      print_usage();
      exit(1);
  }

  printf("-- Initializing Handel\n");
  status = xiaInit(argv[1]);
  CHECK_ERROR(status);

  xiaSetLogLevel(MD_ERROR);
  xiaSetLogOutput("errors.log");

  printf("-- Starting the system\n");
  status = xiaStartSystem();
  CHECK_ERROR(status);

  /* Read the original gap_time for comparision */
  status = xiaGetAcquisitionValues(0, "gap_time", (void *)&gap_time);
  CHECK_ERROR(status);
  
  printf("-- Read acquisition value gap_time: %0.2f\n", gap_time);

  /* To change gap_time, set the acquisition value minimum_gap_time */
  status = xiaSetAcquisitionValues(0, "minimum_gap_time", (void *)&minimum_gap_time);
  CHECK_ERROR(status);
  
	/* Apply new acquisition values */
	status = xiaBoardOperation(0, "apply", (void *)&dummy);
	CHECK_ERROR(status);
  
  printf("-- Set minimum_gap_time to %0.2f\n", minimum_gap_time);
  
  status = xiaGetAcquisitionValues(0, "gap_time", (void *)&gap_time);
  CHECK_ERROR(status);
  
  printf("-- Read acquisition value gap_time: %0.2f\n", gap_time);

  printf("-- Cleaning up Handel.\n");  
  xiaExit();
  xiaCloseLog();
  
  return 0;
}


/*
 * This is just an example of how to handle error values.
 * A program of any reasonable size should
 * implement a more robust error handling mechanism.
 */
static void CHECK_ERROR(int status)
{
  /* XIA_SUCCESS is defined in handel_errors.h */
  if (status != XIA_SUCCESS) {
    printf("-- Error encountered! Status = %d, please check errors.log.\n", status);
    exit(status);
  }
}

static void print_usage(void)
{
    fprintf(stdout, "Arguments: [.ini file]]\n");
    return;
}
