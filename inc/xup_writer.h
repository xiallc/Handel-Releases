/**
 * xup_writer.h
 *
 * Created 04/19/03 -- Patrick J. Franz
 *
 * Copyright (c) 2003, X-ray Instrumentation Associates
 * All rights reserved
 *
 * The header file for IMPORT/EXPORT of the main DLL funcs
 *
 */

#ifndef _XUP_WRITER_H_
#define _XUP_WRITER_H_

#ifdef __EXPORT
#define DLL_SPEC __declspec(dllexport)
#else
#define DLL_SPEC __declspec(dllimport)
#endif

#ifdef __VB
#define DLL_API _stdcall
#else
#define DLL_API
#endif

typedef unsigned char byte_t;

enum { NO_ACCESS = 0, ACCESS_REQUIRED };

#define CURRENT_VERSION 1

#define UNUSED(x) ((x) = (x))

DLL_SPEC int DLL_API OpenXUP(char* name, unsigned short* h);
DLL_SPEC int DLL_API AddSection(byte_t offset, unsigned short opt_mem_offset,
                                unsigned long size, byte_t* data, unsigned short* idx);
DLL_SPEC int DLL_API RemoveSection(unsigned short idx);
DLL_SPEC int DLL_API WriteXUP(unsigned short h, unsigned short gid, byte_t req);
DLL_SPEC int DLL_API CreateACF(char* file, char* sn);
DLL_SPEC void DLL_API CloseXUP(unsigned short h);
DLL_SPEC void DLL_API DumpSections(char* name);

#endif /* _XUP_WRITER_H_ */
