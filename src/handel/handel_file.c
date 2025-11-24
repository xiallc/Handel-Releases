/* Routines used to restore and save various file formats in Handel. */

/*
 * Copyright (c) 2002-2004 X-ray Instrumentation Associates
 * Copyright (c) 2005-2017 XIA LLC
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

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "xia_assert.h"
#include "xia_common.h"
#include "xia_file.h"
#include "xia_handel.h"
#include "xia_module.h"

#include "handel_errors.h"
#include "handel_file.h"
#include "handel_generic.h"
#include "handel_log.h"

typedef int (*interfaceWrite_FP)(FILE*, Module*);

typedef struct _InterfaceWriter {
    unsigned int type;
    interfaceWrite_FP fn;
} InterfaceWriters_t;

static int writePLX(FILE* fp, Module* module);
static int writeEPP(FILE* fp, Module* module);
static int writeUSB(FILE* fp, Module* module);
static int writeUSB2(FILE* fp, Module* m);
static int writeSerial(FILE* fp, Module* m);

static int writeInterface(FILE* fp, Module* m);

static char line[XIA_LINE_LEN];

static int HANDEL_API xiaGetLine_N(FILE* fp, char* lline, int len);
static int HANDEL_API xiaGetLine(FILE* fp, char* line);

/* GLOBAL Variables */
static InterfaceWriters_t INTERFACE_WRITERS[] = {
    /* Sentinel */
    {0, NULL},
    {XIA_PLX, writePLX},
    {XIA_EPP, writeEPP},
    {XIA_GENERIC_EPP, writeEPP},
    {XIA_USB, writeUSB},
    {XIA_USB2, writeUSB2},
    {XIA_SERIAL, writeSerial},
};

SectionInfo sectionInfo[] = {{xiaLoadDetector, "detector definitions"},
                             {xiaLoadFirmware, "firmware definitions"},
                             {xiaLoadDefaults, "default definitions"},
                             {xiaLoadModule, "module definitions"}};

HANDEL_EXPORT int HANDEL_API xiaLoadSystem(char* type, char* filename) {
    int status;

    if (type == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_TYPE, "xiaLoadSystem", "type is NULL");
        return XIA_NULL_TYPE;
    }

    if (filename == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NO_FILENAME, "xiaLoadSystem", "filename is NULL");
        return XIA_NO_FILENAME;
    }

    /*
     * If we support different output types in the future, we need to change
     * this logic around.
     */
    if (!STREQ(type, "handel_ini")) {
        xiaLog(XIA_LOG_ERROR, XIA_FILE_TYPE, "xiaLoadSystem",
               "Unknown file type '%s' for target save file '%s'", type, filename);
        return XIA_FILE_TYPE;
    }

    status = xiaInit(filename);

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaLoadSystem",
               "Error reading in .INI file '%s'", filename);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Saves the configuration to the given filename and type. The only
 * supported type is "handel_ini".
 */
HANDEL_EXPORT int HANDEL_API xiaSaveSystem(char* type, char* filename) {
    int status;

    if (type == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_TYPE, "xiaSaveSystem", "type cannot be NULL");
        return XIA_NULL_TYPE;
    }

    if (filename == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NO_FILENAME, "xiaSaveSystem",
               "filename cannot be NULL");
        return XIA_NO_FILENAME;
    }

    if (STREQ(type, "handel_ini")) {
        status = xiaWriteIniFile(filename);
    } else {
        status = XIA_FILE_TYPE;
    }

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaSaveSystem", "Error writing %s", filename);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * This routine writes out a "handel_ini" file based on the current
 * information in the data structures.
 */
static int HANDEL_API xiaWriteIniFile(char* filename) {
    int status;
    int i;

    unsigned int j;

    FILE* iniFile = NULL;

    char typeStr[MAXITEM_LEN];

    Detector* detector = xiaGetDetectorHead();
    FirmwareSet* firmwareSet = xiaGetFirmwareSetHead();
    Firmware* firmware = NULL;
    XiaDefaults* defaults = xiaGetDefaultsHead();
    XiaDaqEntry* entry = NULL;
    Module* module = xiaGetModuleHead();

    if (filename == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NULL_PATH, "xiaWriteIniFile",
               "filename cannot be NULL");
        return XIA_NULL_PATH;
    }

    if (STREQ(filename, "")) {
        xiaLog(XIA_LOG_ERROR, XIA_NO_FILENAME, "xiaWriteIniFile",
               "filename cannot be empty string");
        return XIA_NO_FILENAME;
    }

    iniFile = xia_file_open(filename, "w");
    if (iniFile == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_OPEN_FILE, "xiaWriteIniFile", "Could not open %s",
               filename);
        return XIA_OPEN_FILE;
    }

    /* Write the sections in the same order that they are read in */
    fprintf(iniFile, "[detector definitions]\n\n");

    for (i = 0; detector != NULL; detector = detector->next, i++) {
        fprintf(iniFile, "START #%d\n", i);
        fprintf(iniFile, "alias = %s\n", detector->alias);
        fprintf(iniFile, "number_of_channels = %hu\n", detector->nchan);

        switch (detector->type) {
            case XIA_DET_RESET:
                strcpy(typeStr, "reset");
                break;
            case XIA_DET_RCFEED:
                strcpy(typeStr, "rc_feedback");
                break;
            default:
            case XIA_DET_UNKNOWN:
                xia_file_close(iniFile);
                xiaLog(XIA_LOG_ERROR, XIA_MISSING_TYPE, "xiaWriteIniFile",
                       "Unknown detector type for alias %s", detector->alias);
                return XIA_MISSING_TYPE;
                break;
        }

        fprintf(iniFile, "type = %s\n", typeStr);
        fprintf(iniFile, "type_value = %3.3f\n", detector->typeValue[0]);

        for (j = 0; j < detector->nchan; j++) {
            fprintf(iniFile, "channel%u_gain = %3.6f\n", j, detector->gain[j]);

            switch (detector->polarity[j]) {
                case 0: /* Negative */
                    fprintf(iniFile, "channel%u_polarity = %s\n", j, "-");
                    break;
                case 1: /* Positive */
                    fprintf(iniFile, "channel%u_polarity = %s\n", j, "+");
                    break;
                default:
                    xia_file_close(iniFile);
                    xiaLog(XIA_LOG_ERROR, XIA_POL_OOR, "xiaWriteIniFile",
                           "Unknown detector polarity %hu for alias %s",
                           detector->polarity[j], detector->alias);
                    return XIA_POL_OOR;
                    break;
            }
        }

        fprintf(iniFile, "END #%d\n\n", i);
    }

    fprintf(iniFile, "[firmware definitions]\n\n");

    for (i = 0; firmwareSet != NULL; firmwareSet = firmwareSet->next, i++) {
        fprintf(iniFile, "START #%d\n", i);
        fprintf(iniFile, "alias = %s\n", firmwareSet->alias);

        if (firmwareSet->mmu != NULL) {
            fprintf(iniFile, "mmu = %s\n", firmwareSet->mmu);
        }

        if (firmwareSet->filename != NULL) {
            fprintf(iniFile, "filename = %s\n", firmwareSet->filename);

            if (firmwareSet->tmpPath != NULL) {
                fprintf(iniFile, "fdd_tmp_path = %s\n", firmwareSet->tmpPath);
            }

            fprintf(iniFile, "num_keywords = %u\n", firmwareSet->numKeywords);

            for (j = 0; j < firmwareSet->numKeywords; j++) {
                fprintf(iniFile, "keyword%u = %s\n", j, firmwareSet->keywords[j]);
            }
        } else {
            firmware = firmwareSet->firmware;
            while (firmware != NULL) {
                fprintf(iniFile, "ptrr = %hu\n", firmware->ptrr);
                fprintf(iniFile, "min_peaking_time = %3.3f\n", firmware->min_ptime);
                fprintf(iniFile, "max_peaking_time = %3.3f\n", firmware->max_ptime);

                if (firmware->fippi != NULL) {
                    fprintf(iniFile, "fippi = %s\n", firmware->fippi);
                }

                if (firmware->user_fippi != NULL) {
                    fprintf(iniFile, "user_fippi = %s\n", firmware->user_fippi);
                }

                if (firmware->dsp != NULL) {
                    fprintf(iniFile, "dsp = %s\n", firmware->dsp);
                }

                fprintf(iniFile, "num_filter = %hu\n", firmware->numFilter);

                for (j = 0; j < firmware->numFilter; j++) {
                    fprintf(iniFile, "filter_info%u = %hu\n", j,
                            firmware->filterInfo[j]);
                }

                firmware = firmware->next;
            }
        }

        fprintf(iniFile, "END #%d\n\n", i);
    }

    fprintf(iniFile, "***** Generated by Handel -- DO NOT MODIFY *****\n");
    fprintf(iniFile, "[default definitions]\n\n");

    for (i = 0; defaults != NULL; defaults = defaults->next, i++) {
        fprintf(iniFile, "START #%d\n", i);
        fprintf(iniFile, "alias = %s\n", defaults->alias);

        entry = defaults->entry;
        while (entry != NULL) {
            fprintf(iniFile, "%s = %3.6f\n", entry->name, entry->data);
            entry = entry->next;
        }

        fprintf(iniFile, "END #%d\n\n", i);
    }

    fprintf(iniFile, "***** End of Generated Information *****\n\n");

    fprintf(iniFile, "[module definitions]\n\n");

    for (i = 0; module != NULL; module = module->next, i++) {
        fprintf(iniFile, "START #%d\n", i);
        fprintf(iniFile, "alias = %s\n", module->alias);
        fprintf(iniFile, "module_type = %s\n", module->type);

        status = writeInterface(iniFile, module);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaWriteIniFile",
                   "Error writing interface information for module '%s'",
                   module->alias);
            return status;
        }

        fprintf(iniFile, "number_of_channels = %u\n", module->number_of_channels);

        for (j = 0; j < module->number_of_channels; j++) {
            fprintf(iniFile, "channel%u_alias = %d\n", j, module->channels[j]);
            fprintf(iniFile, "channel%u_detector = %s:%u\n", j, module->detector[j],
                    (unsigned int) module->detector_chan[j]);
            fprintf(iniFile, "firmware_set_chan%u = %s\n", j, module->firmware[j]);
            fprintf(iniFile, "default_chan%u = %s\n", j, module->defaults[j]);
        }

        fprintf(iniFile, "END #%d\n\n", i);
    }

    xia_file_close(iniFile);

    return XIA_SUCCESS;
}

/*
 * Read in "handel_ini" type ini files.
 *
 * Returns #XIA_OPEN_FILE if inifile cannot be found.
 */
int HANDEL_API xiaReadIniFile(char* inifile) {
    int status = XIA_SUCCESS;
    int numSections;
    int i;

    /*
     * Pointers to keep track of the xia.ini file
     */
    FILE* fp = NULL;

    char tmpLine[XIA_LINE_LEN];

    fpos_t start;
    fpos_t end;
    fpos_t local;
    fpos_t local_end;

    char xiaini[8] = "xia.ini";

    /* Check if an .INI file was specified */
    if (inifile == NULL) {
        inifile = xiaini;
    }

    xiaLog(XIA_LOG_INFO, "xiaReadIniFile", "Reading in .INI file '%s'", inifile);

    /* Open the .ini file */
    fp = xia_find_file(inifile, "rb");

    if (fp == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_OPEN_FILE, "xiaReadIniFile", "Could not open %s",
               inifile);
        return XIA_OPEN_FILE;
    }

    /* Loop over all the sections as defined in sectionInfo
     * XXX BUG: Should be sectionInfo / sectionInfo[0]?
     */
    numSections = (int) (sizeof(sectionInfo) / sizeof(SectionInfo));

    for (i = 0; i < numSections; i++) {
        status = xiaFindEntryLimits(fp, sectionInfo[i].section, &start, &end);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_WARNING, "xiaReadIniFile",
                   "Section missing from ini file: %s", sectionInfo[i].section);
            continue;
        }

        /*
         * Here is the pseudocode for parsing in a section w/ multiple headings
         *
         * 1) Set local to line with START on it (this is a one shot thing)
         * 2) Cache line pointed to by "end" (this is because we can't do direct
         *    arithmetic comparisons with fpos_t structs). Also, actually do a
         *    comparison between local's "line" and the end's "line". If they
         *    match then we've reached the end of the section and are finished.
         *    N.b.: I pray to god that a situation never comes up that would
         *    break this comparison. In principle, it shouldn't happen since
         *    end's "line" is either EOF or a section heading.
         * 3) Increment local until we run into END
         * 4) Set local_end
         * 5) xiaLoadxxxxx(local, local_end);
         * 6) Set current to local_end and jump to step (2)
         */

        status = fsetpos(fp, &end);

        if (status != 0) {
            xiaLog(XIA_LOG_ERROR, XIA_SET_POS, "xiaReadIniFile",
                   "Error setting file position to the end of "
                   "the current section. errno = %d, '%s'.",
                   (int) errno, strerror(errno));
            return XIA_SET_POS;
        }

        status = xiaGetLine(fp, tmpLine);

        if (status != XIA_SUCCESS && status != XIA_EOF) {
            xiaLog(
                XIA_LOG_ERROR, status, "xiaReadIniFile",
                "Error getting end of section line after setting the file position.");
            return status;
        }

        ASSERT(tmpLine[0] == '[');

        xiaLog(XIA_LOG_DEBUG, "xiaReadIniFile", "Cached end string = %s", tmpLine);

        status = fsetpos(fp, &start);

        if (status != 0) {
            xiaLog(XIA_LOG_ERROR, XIA_SET_POS, "xiaReadIniFile",
                   "Error setting file position to the end of "
                   "the current section. errno = %d, '%s'.",
                   (int) errno, strerror(errno));
            return XIA_SET_POS;
        }

        status = xiaGetLine(fp, line);

        if (status != XIA_SUCCESS && status != XIA_EOF) {
            xiaLog(
                XIA_LOG_ERROR, status, "xiaReadIniFile",
                "Error getting start of section line after setting the file position.");
            return status;
        }

        while (!STREQ(line, tmpLine)) {
            status = fgetpos(fp, &local);

            if (strncmp(line, "START", 5) == 0) {
                do {
                    status = fgetpos(fp, &local_end);
                    status = xiaGetLine(fp, line);
                    xiaLog(XIA_LOG_DEBUG, "xiaReadIniFile",
                           "Inside START/END bracket: %s", line);
                } while (strncmp(line, "END", 3) != 0);

                status = sectionInfo[i].function_ptr(fp, &local, &local_end);

                if (status != XIA_SUCCESS) {
                    xia_file_close(fp);
                    xiaLog(XIA_LOG_ERROR, status, "xiaReadIniFile",
                           "Error loading information from ini file");
                    return status;
                }
            }

            status = xiaGetLine(fp, line);

            if (status == XIA_EOF) {
                break;
            }

            xiaLog(XIA_LOG_DEBUG, "xiaReadIniFile", "Looking for START: %s", line);
        }
    }

    xia_file_close(fp);
    xiaLog(XIA_LOG_INFO, "xiaReadIniFile", "Successfully read ini file.");

    return XIA_SUCCESS;
}

/*
 * Gets the first line with text after the current file position.
 */
static int HANDEL_API xiaGetLineData(char* lline, char* name, char* value) {
    int status = XIA_SUCCESS;

    size_t loc;
    size_t begin;
    size_t end;
    size_t len;

    /*
     * If this line is a comment then skip it. See BUG ID #64.
     */
    if (lline[0] == '*') {
        strcpy(name, "COMMENT");
        strcpy(value, lline);
        return XIA_SUCCESS;
    }

    /* Start by finding the '=' within the line */
    loc = strcspn(lline, "=");
    if (loc == 0) {
        xiaLog(XIA_LOG_ERROR, XIA_FORMAT_ERROR, "xiaGetLineData",
               "No = present in xia.ini line: \n %s", lline);
        return XIA_FORMAT_ERROR;
    }

    /* Strip all the leading blanks */
    begin = 0;
    while (isspace(CTYPE_CHAR(lline[begin]))) {
        begin++;
    }

    /* Strip all the trailing blanks */
    end = loc - 1;
    while (isspace(CTYPE_CHAR(lline[end]))) {
        end--;
    }

    /* Bug #76, prevents a bad core dump */
    if (begin > end) {
        xiaLog(XIA_LOG_ERROR, XIA_FORMAT_ERROR, "xiaGetLineData",
               "Invalid name found in line:  %s", lline);
        return XIA_FORMAT_ERROR;
    }

    /* copy the name */
    len = end - begin + 1;
    strncpy(name, lline + begin, len);
    name[len] = '\0';

    /* Strip all the leading blanks */
    begin = loc + 1;

    while (isspace(CTYPE_CHAR(lline[begin]))) {
        begin++;
    }

    /* Strip all the trailing blanks */
    end = strlen(lline) - 1;

    while (isspace(CTYPE_CHAR(lline[end]))) {
        end--;
    }

    /* Bug #76, prevents a bad core dump */
    if (begin > end) {
        xiaLog(XIA_LOG_ERROR, XIA_FORMAT_ERROR, "xiaGetLineData",
               "Invalid value found in line:  %s", lline);
        return XIA_FORMAT_ERROR;
    }

    /* copy the value */
    len = end - begin + 1;
    strncpy(value, lline + begin, len);
    value[len] = '\0';

    return status;
}

static int HANDEL_API xiaGetLine(FILE* fp, char* lline) {
    return xiaGetLine_N(fp, lline, XIA_LINE_LEN);
}

/*
 * Gets the first line with text after the current file position.
 *
 * If the line is longer than llen, the file position is scanned to
 * the start of the next line.
 */
static int HANDEL_API xiaGetLine_N(FILE* fp, char* lline, int llen) {
    unsigned int j;

    char* cstatus;

    do {
        size_t len;
        char extra[8193];

        cstatus = handel_md_fgets(lline, llen, fp);

        /* lline won't be overwritten in this case */
        if (cstatus == NULL) {
            return XIA_EOF;
        }

        /*
         * If a partial line was read, flush the rest of that line so the next read
         * gets a new line.
         */
        while ((len = strlen(cstatus)) > 0 && cstatus[len - 1] != '\r' &&
               cstatus[len - 1] != '\n') {
            cstatus = handel_md_fgets(extra, sizeof(extra) - 1, fp);

            if (!cstatus) {
                break;
            }
        }

        len = strlen(lline);

        /*
         * Remove the new line character to keep the log file output from
         * containing the extra white space.
         */
        if (len > 1 && lline[len - 2] == '\n')
            lline[len - 2] = '\0';
        if (len > 0 && lline[len - 1] == '\n')
            lline[len - 1] = '\0';

        /* Check for any alphanumeric character in the line */
        for (j = 0; j < (unsigned int) len; j++) {
            if (isgraph(CTYPE_CHAR(lline[j]))) {
                return XIA_SUCCESS;
            }
        }
    } while (cstatus != NULL);

    return XIA_EOF;
}

/*
 * Searches through the .ini file and finds the start and end of a
 * specific section of the .ini file starting at [section] and ending at the
 * next [].
 */
static int HANDEL_API xiaFindEntryLimits(FILE* fp, const char* section, fpos_t* start,
                                         fpos_t* end) {
    int status = XIA_SUCCESS;
    unsigned int j;

    boolean_t isStartFound = FALSE_;

    char* cstatus;

    /* First rewind the file to the beginning */
    rewind(fp);

    /* Now fine the match to the section entry */
    do {
        do {
            cstatus = handel_md_fgets(line, XIA_LINE_LEN, fp);
        } while ((line[0] != '[') && (cstatus != NULL));

        if (cstatus == NULL) {
            /* This isn't an error since the user has the option of specifying the missing
             * information using the dynamic configuration routines.
             */
            xiaLog(XIA_LOG_WARNING, "xiaFindEntryLimits", "Unable to find section %s",
                   section);
            return XIA_NOSECTION;
        }

        /* Find the terminating ] to this section */
        for (j = 1; j < (unsigned int) strlen(line) + 1; j++) {
            if (line[j] == ']') {
                break;
            }
        }
        /* Did we not find the terminating ]? */
        if (j == (unsigned int) strlen(line)) {
            xiaLog(XIA_LOG_ERROR, XIA_FORMAT_ERROR, "xiaFindEntryLimits",
                   "Syntax error in Init file, no terminating ] found");
            return XIA_FORMAT_ERROR;
        }

        if (strncmp(line + 1, section, j - 1) == 0) {
            /* Record the starting position) */
            fgetpos(fp, start);

            isStartFound = TRUE_;
        }
        /* Else look for the next section entry */
    } while (!isStartFound);

    do {
        /*
         * Get the current file position before the next read.  If the file
         * ends, or we find a '[' then we are done and want to send the ending
         * position to the location of the previous read
         */
        status = fgetpos(fp, end);
        cstatus = handel_md_fgets(line, XIA_LINE_LEN, fp);
    } while ((line[0] != '[') && (cstatus != NULL));

    if (!cstatus) {
        ASSERT(feof(fp));
    }

    return XIA_SUCCESS;
}

/*
 * Parses data in from fp (and bounded by start & end) as
 * detector information. If it fails, then it fails hard and the user needs
 * to fix their inifile.
 */
static int xiaLoadDetector(FILE* fp, fpos_t* start, fpos_t* end) {
    int status;

    unsigned short i;
    unsigned short numChans;

    double gain;
    double typeValue;

    char value[MAXITEM_LEN];
    char alias[MAXALIAS_LEN];
    char name[MAXITEM_LEN];

    /*
     * We need to load things in a certain order since some information must be
     * specified to HanDeL before others. The following order should work well:
     * 1) alias
     * 2) number of channels
     * 3) rest of the detector information
     */

    status = xiaFileRA(fp, start, end, "alias", value);

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaLoadDetector",
               "Unable to load alias information");
        return status;
    }

    xiaLog(XIA_LOG_DEBUG, "xiaLoadDetector", "alias = %s", value);

    strcpy(alias, value);

    status = xiaNewDetector(alias);

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaLoadDetector", "Error creating new detector");
        return status;
    }

    status = xiaFileRA(fp, start, end, "number_of_channels", value);

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaLoadDetector",
               "Unable to find number_of_channels");
        return status;
    }

    sscanf(value, "%hu", &numChans);

    xiaLog(XIA_LOG_DEBUG, "xiaLoadDetector", "number_of_channels = %hu", numChans);

    status = xiaAddDetectorItem(alias, "number_of_channels", (void*) &numChans);

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaLoadDetector",
               "Error adding number_of_channels to detector %s", alias);
        return status;
    }

    status = xiaFileRA(fp, start, end, "type", value);

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaLoadDetector",
               "Unable to find type for detector %s", alias);
        return status;
    }

    status = xiaAddDetectorItem(alias, "type", (void*) value);

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaLoadDetector",
               "Error adding type to detector %s", alias);
        return status;
    }

    status = xiaFileRA(fp, start, end, "type_value", value);

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaLoadDetector",
               "Unable to find type_value for detector %s", alias);
        return status;
    }

    sscanf(value, "%lf", &typeValue);

    status = xiaAddDetectorItem(alias, "type_value", (void*) &typeValue);

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaLoadDetector",
               "Error adding type_value to detector %s", alias);
        return status;
    }

    for (i = 0; i < numChans; i++) {
        sprintf(name, "channel%hu_gain", i);
        status = xiaFileRA(fp, start, end, name, value);

        if (status == XIA_FILE_RA) {
            xiaLog(XIA_LOG_WARNING, "xiaLoadDetector",
                   "Current configuration file missing %s", name);
        } else {
            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaLoadDetector",
                       "Unable to load channel gain");
                return status;
            }

            sscanf(value, "%6lf", &gain);

            xiaLog(XIA_LOG_DEBUG, "xiaLoadDetector", "%s = %f", name, gain);

            status = xiaAddDetectorItem(alias, name, (void*) &gain);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaLoadDetector",
                       "Error adding %s to detector %s", name, alias);
                return status;
            }
        }

        sprintf(name, "channel%hu_polarity", i);
        status = xiaFileRA(fp, start, end, name, value);

        if (status == XIA_FILE_RA) {
            xiaLog(XIA_LOG_WARNING, "xiaLoadDetector",
                   "Current configuration file missing %s", name);
        } else {
            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaLoadDetector",
                       "Unable to load channel polarity");
                return status;
            }

            xiaLog(XIA_LOG_DEBUG, "xiaLoadDetector", "%s = %s", name, value);

            status = xiaAddDetectorItem(alias, name, (void*) value);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaLoadDetector",
                       "Error adding %s to detector %s", name, alias);
                return status;
            }
        }
    }

    return XIA_SUCCESS;
}

/*
 * Parses data in from fp (and bounded by start & end) as
 * module information. If it fails, then it fails hard and the user needs
 * to fix their inifile.
 */
static int xiaLoadModule(FILE* fp, fpos_t* start, fpos_t* end) {
    int status;
    int chanAlias;

    unsigned int pciSlot;
    unsigned int pciBus;
    unsigned int numChans;
    unsigned int scsiBus;
    unsigned int crate;
    unsigned int slot;
    unsigned int eppAddr;
    unsigned int daisyID;
    unsigned int i;
    unsigned int comPort;
    unsigned int baudRate;
    unsigned int deviceNumber;

    byte_t bPciSlot;
    byte_t bPciBus;

    char value[MAXITEM_LEN];
    char alias[MAXALIAS_LEN];
    char interface[MAXITEM_LEN];
    char moduleType[MAXITEM_LEN];
    char name[MAXITEM_LEN];
    char detAlias[MAXALIAS_LEN];
    char firmAlias[MAXALIAS_LEN];
    char defAlias[MAXALIAS_LEN];

    status = xiaFileRA(fp, start, end, "alias", value);

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule",
               "Unable to load alias information");
        return status;
    }

    xiaLog(XIA_LOG_DEBUG, "xiaLoadModule", "alias = %s", value);

    strcpy(alias, value);

    status = xiaNewModule(alias);

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule", "Error creating new module");
        return status;
    }

    status = xiaFileRA(fp, start, end, "module_type", value);

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule", "Unable to load module type");
        return status;
    }

    sscanf(value, "%s", moduleType);

    xiaLog(XIA_LOG_DEBUG, "xiaLoadModule", "moduleType = %s", moduleType);

    status = xiaAddModuleItem(alias, "module_type", (void*) moduleType);

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule",
               "Error adding module type to module %s", alias);
        return status;
    }

    status = xiaFileRA(fp, start, end, "number_of_channels", value);

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule",
               "Unable to load number of channels");
        return status;
    }

    sscanf(value, "%u", &numChans);

    xiaLog(XIA_LOG_DEBUG, "xiaLoadModule", "number_of_channels = %u", numChans);

    status = xiaAddModuleItem(alias, "number_of_channels", (void*) &numChans);

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule",
               "Error adding number_of_channels to module %s", alias);
        return status;
    }

    /* Deal with interface here */
    status = xiaFileRA(fp, start, end, "interface", value);

    sscanf(value, "%s", interface);

    xiaLog(XIA_LOG_DEBUG, "xiaLoadModule", "interface = %s", interface);

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule", "Unable to load interface");
        return status;
    }

    if ((STREQ(interface, "j73a")) || (STREQ(interface, "genericSCSI"))) {
        status = xiaAddModuleItem(alias, "interface", (void*) interface);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule",
                   "Error adding interface to module %s", alias);
            return status;
        }

        status = xiaFileRA(fp, start, end, "scsibus_number", value);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule",
                   "Unable to load SCSI bus number");
            return status;
        }

        sscanf(value, "%u", &scsiBus);

        xiaLog(XIA_LOG_DEBUG, "xiaLoadModule", "scsiBus = %u", scsiBus);

        status = xiaAddModuleItem(alias, "scsibus_number", (void*) &scsiBus);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule",
                   "Error adding scsi bus number to module %s", alias);
            return status;
        }

        status = xiaFileRA(fp, start, end, "crate_number", value);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule",
                   "Unable to load crate number");
            return status;
        }

        sscanf(value, "%u", &crate);

        xiaLog(XIA_LOG_DEBUG, "xiaLoadModule", "crate = %u", crate);

        status = xiaAddModuleItem(alias, "crate_number", (void*) &crate);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule",
                   "Error adding crate number to module %s", alias);
            return status;
        }

        status = xiaFileRA(fp, start, end, "slot", value);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule",
                   "Unable to load slot number");
            return status;
        }

        sscanf(value, "%u", &slot);

        xiaLog(XIA_LOG_DEBUG, "xiaLoadModule", "slot = %u", slot);

        status = xiaAddModuleItem(alias, "slot", (void*) &slot);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule",
                   "Error adding slot number to module %s", alias);
            return status;
        }
    } else if ((STREQ(interface, "epp")) || (STREQ(interface, "genericEPP"))) {
        status = xiaFileRA(fp, start, end, "epp_address", value);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule",
                   "Unable to load EPP address");
            return status;
        }

        sscanf(value, "%x", &eppAddr);

        xiaLog(XIA_LOG_DEBUG, "xiaLoadModule", "EPP Address = %#x", eppAddr);

        status = xiaAddModuleItem(alias, "epp_address", (void*) &eppAddr);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule",
                   "Error adding EPP address to module %s", alias);
            return status;
        }

        status = xiaFileRA(fp, start, end, "daisy_chain_id", value);

        /*
         * This is an extremely optional setting, so we really only want to do
         * anything if the value actually shows up in the section.
         */
        if (status == XIA_SUCCESS) {
            sscanf(value, "%u", &daisyID);

            xiaLog(XIA_LOG_DEBUG, "xiaLoadModule", "Daisy Chain ID = %#x", daisyID);

            status = xiaAddModuleItem(alias, "daisy_chain_id", (void*) &daisyID);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule",
                       "Error adding daisy chain id to module %s", alias);
                return status;
            }
        }
    } else if (STREQ(interface, "usb")) {
        status = xiaAddModuleItem(alias, "interface", interface);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule",
                   "Error setting interface to '%s' for module with alias '%s'",
                   interface, alias);
            return status;
        }

        status = xiaFileRA(fp, start, end, "device_number", value);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule",
                   "Unable to load device number");
            return status;
        }

        sscanf(value, "%u", &deviceNumber);

        xiaLog(XIA_LOG_DEBUG, "xiaLoadModule", "Device Number = %u", deviceNumber);

        status = xiaAddModuleItem(alias, "device_number", (void*) &deviceNumber);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule",
                   "Error adding Device Number to module %s", alias);
            return status;
        }
    } else if (STREQ(interface, "usb2")) {
        status = xiaAddModuleItem(alias, "interface", interface);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule",
                   "Error setting interface to '%s' for module with alias '%s'",
                   interface, alias);
            return status;
        }

        status = xiaFileRA(fp, start, end, "device_number", value);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule",
                   "Unable to load device number");
            return status;
        }

        sscanf(value, "%u", &deviceNumber);

        xiaLog(XIA_LOG_DEBUG, "xiaLoadModule", "Device Number = %u", deviceNumber);

        status = xiaAddModuleItem(alias, "device_number", (void*) &deviceNumber);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule",
                   "Error adding Device Number to module %s", alias);
            return status;
        }
    } else if (STREQ(interface, "pxi")) {
        status = xiaFileRA(fp, start, end, "pci_slot", value);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule", "Unable to load 'pci_slot'");
            return status;
        }

        sscanf(value, "%u", &pciSlot);

        xiaLog(XIA_LOG_DEBUG, "xiaLoadModule", "PCI Slot = %u", pciSlot);

        bPciSlot = (byte_t) pciSlot;

        status = xiaAddModuleItem(alias, "pci_slot", (void*) &bPciSlot);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule",
                   "Error adding PCI slot to module %s", alias);
            return status;
        }

        status = xiaFileRA(fp, start, end, "pci_bus", value);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule", "Unable to load 'pci_bus'");
            return status;
        }

        sscanf(value, "%u", &pciBus);

        xiaLog(XIA_LOG_DEBUG, "xiaLoadModule", "PCI Bus = %u", pciBus);

        bPciBus = (byte_t) pciBus;

        status = xiaAddModuleItem(alias, "pci_bus", (void*) &bPciBus);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule",
                   "Error adding PCI bus to module %s", alias);
            return status;
        }
    } else if (STREQ(interface, "serial")) {
        status = xiaFileRA(fp, start, end, "com_port", value);

        if (status == XIA_SUCCESS) {
            sscanf(value, "%u", &comPort);

            xiaLog(XIA_LOG_DEBUG, "xiaLoadModule", "COM Port = %u", comPort);

            status = xiaAddModuleItem(alias, "com_port", (void*) &comPort);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule",
                       "Error adding COM port to module %s", alias);
                return status;
            }
        } else {
            status = xiaFileRA(fp, start, end, "device_file", value);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule",
                       "Unable to load COM port or device_file");
                return status;
            }

            xiaLog(XIA_LOG_DEBUG, "xiaLoadModule", "Device file = %s", value);

            status = xiaAddModuleItem(alias, "device_file", value);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule",
                       "Error adding device file to module %s", alias);
                return status;
            }
        }

        status = xiaFileRA(fp, start, end, "baud_rate", value);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule", "Unable to load baud rate");
            return status;
        }

        sscanf(value, "%u", &baudRate);

        xiaLog(XIA_LOG_DEBUG, "xiaLoadModule", "Baud Rate = %u", baudRate);

        status = xiaAddModuleItem(alias, "baud_rate", (void*) &baudRate);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule",
                   "Error adding baud rate to module %s", alias);
            return status;
        }
    } else {
        xiaLog(XIA_LOG_ERROR, XIA_BAD_INTERFACE, "xiaLoadModule",
               "The interface defined for %s does not exist", alias);
        return XIA_BAD_INTERFACE;
    }

    for (i = 0; i < numChans; i++) {
        sprintf(name, "channel%u_alias", i);
        status = xiaFileRA(fp, start, end, name, value);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule", "Unable to load %s from %s",
                   name, alias);
            return status;
        }

        sscanf(value, "%d", &chanAlias);

        xiaLog(XIA_LOG_DEBUG, "xiaLoadDetector", "%s = %d", name, chanAlias);

        status = xiaAddModuleItem(alias, name, (void*) &chanAlias);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule",
                   "Error adding %s to module %s", name, alias);
            return status;
        }

        sprintf(name, "channel%u_detector", i);
        status = xiaFileRA(fp, start, end, name, value);

        if (status == XIA_FILE_RA) {
            xiaLog(XIA_LOG_WARNING, "xiaLoadModule",
                   "Current configuration file missing %s", name);
        } else {
            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule",
                       "Unable to load channel detector alias");
                return status;
            }

            sscanf(value, "%s", detAlias);

            xiaLog(XIA_LOG_DEBUG, "xiaLoadModule", "%s = %s", name, detAlias);

            status = xiaAddModuleItem(alias, name, (void*) detAlias);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule",
                       "Error adding %s to module %s", name, alias);
                return status;
            }
        }
    }

    /*
     * Need a little extra logic to determine how to load the firmware
     * and defaults. Check for *_all first and if that isn't found then
     * try and find ones for individual channels.
     */
    status = xiaFileRA(fp, start, end, "firmware_set_all", value);

    if (status != XIA_SUCCESS) {
        for (i = 0; i < numChans; i++) {
            sprintf(name, "firmware_set_chan%u", i);
            status = xiaFileRA(fp, start, end, name, value);

            if (status == XIA_FILE_RA) {
                xiaLog(XIA_LOG_WARNING, "xiaLoadModule",
                       "Current configuration file missing %s", name);
            } else {
                if (status != XIA_SUCCESS) {
                    xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule",
                           "Unable to load channel firmware information");
                    return status;
                }

                strcpy(firmAlias, value);

                xiaLog(XIA_LOG_DEBUG, "xiaLoadModule", "%s = %s", name, firmAlias);

                status = xiaAddModuleItem(alias, name, (void*) firmAlias);

                if (status != XIA_SUCCESS) {
                    xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule",
                           "Error adding %s to module %s", name, alias);
                    return status;
                }
            }
        }
    } else {
        strcpy(firmAlias, value);

        status = xiaAddModuleItem(alias, "firmware_set_all", (void*) firmAlias);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule",
                   "Error adding firmware_set_all to module %s", alias);
            return status;
        }
    }

    status = xiaFileRA(fp, start, end, "default_all", value);

    if (status != XIA_SUCCESS) {
        for (i = 0; i < numChans; i++) {
            sprintf(name, "default_chan%u", i);
            status = xiaFileRA(fp, start, end, name, value);

            if (status == XIA_FILE_RA) {
                xiaLog(XIA_LOG_INFO, "xiaLoadModule",
                       "Current configuration file missing %s", name);
            } else {
                if (status != XIA_SUCCESS) {
                    xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule",
                           "Unable to load channel default information");
                    return status;
                }

                strcpy(defAlias, value);

                xiaLog(XIA_LOG_DEBUG, "xiaLoadModule", "%s = %s", name, defAlias);

                status = xiaAddModuleItem(alias, name, (void*) defAlias);

                if (status != XIA_SUCCESS) {
                    xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule",
                           "Error adding %s to module %s", name, alias);
                    return status;
                }
            }
        }
    } else {
        strcpy(defAlias, value);

        status = xiaAddModuleItem(alias, "default_all", (void*) defAlias);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaLoadModule",
                   "Error adding firmware_set_all to module %s", alias);
            return status;
        }
    }
    return XIA_SUCCESS;
}

/*
 * Parses data in from fp (and bounded by start & end) as
 * firmware information. If it fails, then it fails hard and the user needs
 * to fix their inifile.
 */
static int xiaLoadFirmware(FILE* fp, fpos_t* start, fpos_t* end) {
    int status;

    unsigned short i;
    unsigned short numKeywords;

    char value[MAXITEM_LEN];
    char alias[MAXALIAS_LEN];
    char file[MAXFILENAME_LEN];
    char mmu[MAXFILENAME_LEN];
    char path[MAXITEM_LEN];
    /*
     * k-e-y-w-o-r-d + (possibly) 2 numeric digits + \0
     * N.b. Better not be more than 100 keywords
     */
    char keyword[10];

    status = xiaFileRA(fp, start, end, "alias", value);

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaLoadFirmware",
               "Unable to load alias information");
        return status;
    }

    xiaLog(XIA_LOG_DEBUG, "xiaLoadFirmware", "alias = %s", value);

    strcpy(alias, value);

    status = xiaNewFirmware(alias);

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaLoadFirmware", "Error creating new firmware");
        return status;
    }

    /* Check for an MMU first since we'll be exiting if we find a filename */
    status = xiaFileRA(fp, start, end, "mmu", value);

    if (status == XIA_SUCCESS) {
        xiaLog(XIA_LOG_DEBUG, "xiaLoadFirmware", "mmu = %s", value);

        strcpy(mmu, value);
        status = xiaAddFirmwareItem(alias, "mmu", (void*) mmu);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaLoadFirmware",
                   "Error adding MMU to alias %s", alias);
            return status;
        }
    }

    /* If we find a filename, then we are done and can return */
    status = xiaFileRA(fp, start, end, "filename", value);

    if (status == XIA_SUCCESS) {
        xiaLog(XIA_LOG_DEBUG, "xiaLoadFirmware", "filename = %s", value);

        strcpy(file, value);
        status = xiaAddFirmwareItem(alias, "filename", (void*) file);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaLoadFirmware",
                   "Error adding filename to alias %s", alias);
            return status;
        }

        status = xiaFileRA(fp, start, end, "fdd_tmp_path", value);

        if (status == XIA_SUCCESS) {
            strcpy(path, value);

            status = xiaAddFirmwareItem(alias, "fdd_tmp_path", (void*) path);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaLoadFirmware",
                       "Error adding FDD temporary path to '%s'", alias);
                return status;
            }
        }

        /*
         * Check for keywords, if any...no need to really warn since the most
         * important "keywords" are generated by Handel.
         */
        status = xiaFileRA(fp, start, end, "num_keywords", value);

        if (status == XIA_SUCCESS) {
            xiaLog(XIA_LOG_DEBUG, "xiaLoadFirmware", "num_keywords = %s", value);

            sscanf(value, "%hu", &numKeywords);
            for (i = 0; i < numKeywords; i++) {
                sprintf(keyword, "keyword%hu", i);
                status = xiaFileRA(fp, start, end, keyword, value);

                if (status != XIA_SUCCESS) {
                    xiaLog(XIA_LOG_ERROR, status, "xiaLoadFirmware",
                           "Unable to load keyword");
                    return status;
                }

                xiaLog(XIA_LOG_DEBUG, "xiaLoadFirmware", "%s = %s", keyword, value);

                status = xiaAddFirmwareItem(alias, "keyword", (void*) keyword);

                if (status != XIA_SUCCESS) {
                    xiaLog(XIA_LOG_ERROR, status, "xiaLoadFirmware",
                           "Error adding keyword, %s, to alias %s", keyword, alias);
                    return status;
                }
            }
        }
        /* Don't even bother trying to parse in more information */
        return XIA_SUCCESS;
    }

    /*
     * Need to be a little careful here about how we parse in the PTRR chunks.
     * Start slowly by getting the number of PTRRs first.
     */
    status = xiaReadPTRRs(fp, start, end, alias);

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaLoadFirmware",
               "Error loading PTRR information for alias %s", alias);
        return status;
    }

    return XIA_SUCCESS;
}

/*
 * Parses in the information specified in the defaults definitions.
 */
static int xiaLoadDefaults(FILE* fp, fpos_t* start, fpos_t* end) {
    int status;

    char value[MAXITEM_LEN];
    char alias[MAXALIAS_LEN];
    char tmpName[MAXITEM_LEN];
    char tmpValue[MAXITEM_LEN];
    char endLine[XIA_LINE_LEN];
    char currentLine[XIA_LINE_LEN];

    double defValue;

    fpos_t dataStart;

    status = xiaFileRA(fp, start, end, "alias", value);

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaLoadDefaults",
               "Unable to load alias information");
        return status;
    }

    xiaLog(XIA_LOG_DEBUG, "xiaLoadDefaults", "alias = %s", value);

    strcpy(alias, value);

    status = xiaNewDefault(alias);

    if (status != XIA_SUCCESS) {
        xiaLog(XIA_LOG_ERROR, status, "xiaLoadDefaults", "Error creating new default");
        return status;
    }

    /*
     * Want a position after the alias line so that we can just read in line-
     * by-line until we reach endLine
     */
    xiaSetPosOnNext(fp, start, end, "alias", &dataStart, TRUE_);

    fsetpos(fp, end);
    status = xiaGetLine(fp, endLine);

    fsetpos(fp, &dataStart);
    status = xiaGetLine(fp, currentLine);

    while (!STREQ(currentLine, endLine)) {
        status = xiaGetLineData(currentLine, tmpName, tmpValue);
        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaLoadDefaults",
                   "Error getting data for entry %s", tmpName);
            return status;
        }

        if (!STREQ(tmpName, "COMMENT")) {
            sscanf(tmpValue, "%lf", &defValue);

            status = xiaAddDefaultItem(alias, tmpName, (void*) &defValue);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaLoadDefaults",
                       "Error adding %s (value = %.3f) to alias %s", tmpName, defValue,
                       alias);
                return status;
            }

            xiaLog(XIA_LOG_DEBUG, "xiaLoadDefaults",
                   "Added %s (value = %.3f) to alias %s", tmpName, defValue, alias);
        }

        status = xiaGetLine(fp, currentLine);
    }

    return XIA_SUCCESS;
}

/*
 * Read in several PTRRs(*) and add them to the Firmware indicated by alias.
 *
 * (*) -- Actually, it will read in the number specified by number_of_ptrrs.
 */
static int HANDEL_API xiaReadPTRRs(FILE* fp, fpos_t* start, fpos_t* end, char* alias) {
    int status;

    unsigned short i;
    unsigned short ptrr;
    unsigned short numFilter;
    unsigned short filterInfo;

    double min_peaking_time;
    double max_peaking_time;

    /* f+i+l+t+e+r+_+i+n+f+o+ 2 digits + \0 */
    char filterName[14];
    char value[MAXITEM_LEN];

    fpos_t newStart;
    fpos_t newEnd;
    fpos_t lookAheadStart;

    boolean_t isLast = FALSE_;

    xiaLog(XIA_LOG_DEBUG, "xiaReadPTRRs", "Starting parse of PTRRs");

    /* This assumes that there is at least one PTRR for a specified alias */
    newEnd = *start;
    while (!isLast) {
        xiaSetPosOnNext(fp, &newEnd, end, "ptrr", &lookAheadStart, TRUE_);
        xiaSetPosOnNext(fp, &newEnd, end, "ptrr", &newStart, FALSE_);

        /* Find the end here: either the END or another ptrr */
        status = xiaSetPosOnNext(fp, &lookAheadStart, end, "ptrr", &newEnd, FALSE_);

        if (status == XIA_END) {
            isLast = TRUE_;
        }

        /* Do the actual actions here */
        status = xiaFileRA(fp, &newStart, &newEnd, "ptrr", value);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaReadPTRRs",
                   "Unable to read ptrr from file");
            return status;
        }

        sscanf(value, "%hu", &ptrr);

        xiaLog(XIA_LOG_DEBUG, "xiaReadPTRRs", "ptrr = %hu", ptrr);

        status = xiaAddFirmwareItem(alias, "ptrr", (void*) &ptrr);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaReadPTRRs",
                   "Error adding ptrr to alias %s", alias);
            return status;
        }

        status = xiaFileRA(fp, &newStart, &newEnd, "min_peaking_time", value);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaReadPTRRs",
                   "Unable to read min_peaking_time from ptrr = %hu", ptrr);
            return status;
        }

        sscanf(value, "%lf", &min_peaking_time);

        status =
            xiaAddFirmwareItem(alias, "min_peaking_time", (void*) &min_peaking_time);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaReadPTRRs",
                   "Error adding min_peaking_time to alias %s", alias);
            return status;
        }

        status = xiaFileRA(fp, &newStart, &newEnd, "max_peaking_time", value);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaReadPTRRs",
                   "Unable to read max_peaking_time from ptrr = %hu", ptrr);
            return status;
        }

        sscanf(value, "%lf", &max_peaking_time);

        status =
            xiaAddFirmwareItem(alias, "max_peaking_time", (void*) &max_peaking_time);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaReadPTRRs",
                   "Error adding max_peaking_time to alias %s", alias);
            return status;
        }

        status = xiaFileRA(fp, &newStart, &newEnd, "fippi", value);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaReadPTRRs",
                   "Unable to read fippi from ptrr = %hu", ptrr);
            return status;
        }

        status = xiaAddFirmwareItem(alias, "fippi", (void*) value);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaReadPTRRs",
                   "Error adding fippi to alias %s", alias);
            return status;
        }

        status = xiaFileRA(fp, &newStart, &newEnd, "dsp", value);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaReadPTRRs",
                   "Unable to read dsp from ptrr = %hu", ptrr);
            return status;
        }

        status = xiaAddFirmwareItem(alias, "dsp", (void*) value);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaReadPTRRs",
                   "Error adding dsp to alias %s", alias);
            return status;
        }

        /* Check for the quite optional "user_fippi"... */
        status = xiaFileRA(fp, &newStart, &newEnd, "user_fippi", value);

        if (status == XIA_SUCCESS) {
            status = xiaAddFirmwareItem(alias, "user_fippi", (void*) value);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaReadPTRRs",
                       "Error adding user_fippi to alias %s", alias);
                return status;
            }
        } else if (status == XIA_FILE_RA) {
            xiaLog(XIA_LOG_INFO, "xiaReadPTRRs", "No user_fippi present in .ini file");
        } else {
            xiaLog(XIA_LOG_ERROR, status, "xiaReadPTRRs",
                   "Unable to read user_fippi from ptrr = %hu", ptrr);
            return status;
        }

        status = xiaFileRA(fp, &newStart, &newEnd, "num_filter", value);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaReadPTRRs",
                   "Unable to read num_filter from ptrr = %hu", ptrr);
            return status;
        }

        sscanf(value, "%hu", &numFilter);

        xiaLog(XIA_LOG_DEBUG, "xiaReadPTRRs", "numFilter = %hu", numFilter);

        for (i = 0; i < numFilter; i++) {
            sprintf(filterName, "filter_info%hu", i);
            status = xiaFileRA(fp, &newStart, &newEnd, filterName, value);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaReadPTRRs",
                       "Unable to read %s from ptrr = %hu", filterName, ptrr);
                return status;
            }

            sscanf(value, "%hu", &filterInfo);

            xiaLog(XIA_LOG_DEBUG, "xiaReadPTRRs", "filterInfo = %hu", filterInfo);

            status = xiaAddFirmwareItem(alias, "filter_info", (void*) &filterInfo);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "xiaReadPTRRs",
                       "Error adding filter_info to alias %s", alias);
                return status;
            }
        }
    }

    return XIA_SUCCESS;
}

/*
 * This routine searches between start and end for name. If it finds a name,
 * it sets newPos to that location. If not, it returns a value of XIA_END
 * and sets newPos to end. There are a couple of important caveats here:
 * You can't use fpos_t pointers in direct arithmetic comparisons. They just
 * don't work that way. Instead, I set the file to the position and then read
 * a line from the file. This line serves as the "unique" identifier from
 * which all future comparisons are done. There is a finite probability that
 * the same string may appear elsewhere in the file. Hopefully the same string
 * won't appear twice between start and end.
 */
static int HANDEL_API xiaSetPosOnNext(FILE* fp, fpos_t* start, fpos_t* end, char* name,
                                      fpos_t* newPos, boolean_t after) {
    int status;

    char endLine[XIA_LINE_LEN];
    char currentLine[XIA_LINE_LEN];
    char tmpValue[XIA_LINE_LEN];
    char tmpName[XIA_LINE_LEN];

    fsetpos(fp, end);
    status = xiaGetLine(fp, endLine);

    fsetpos(fp, start);
    fgetpos(fp, newPos);
    status = xiaGetLine(fp, currentLine);

    xiaLog(XIA_LOG_DEBUG, "xiaSetPosOnNext", "endLine: %s", endLine);
    xiaLog(XIA_LOG_DEBUG, "xiaSetPosOnNext", "startLine: %s", currentLine);

    while (!STREQ(currentLine, endLine)) {
        status = xiaGetLineData(currentLine, tmpName, tmpValue);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaSetPosOnNext", "Error trying to find %s",
                   name);
            return status;
        }

        if (STREQ(name, tmpName)) {
            if (after) {
                fgetpos(fp, newPos);
            }

            fsetpos(fp, newPos);
            status = xiaGetLine(fp, currentLine);
            xiaLog(XIA_LOG_DEBUG, "xiaSetPosOnNext", "newPos set to line: %s",
                   currentLine);

            return XIA_SUCCESS;
        }

        fgetpos(fp, newPos);

        status = xiaGetLine(fp, currentLine);
    }

    /* Okay, we must have made it to the end of the file */
    return XIA_END;
}

/*
 * This routine will attempt to find the value from the specified name-value
 * pair. Returns XIA_FILE_RA if it couldn't find anything. It calls
 * xiaGetLine() until the name is matched or the end condition is satisfied.
 */
static int HANDEL_API xiaFileRA(FILE* fp, fpos_t* start, fpos_t* end, char* name,
                                char* value) {
    int status;

    /*
     * Use this to hold the "value" of value until we're sure that this is the
     * right name and we can return.
     */
    char tmpValue[XIA_LINE_LEN];
    char tmpName[XIA_LINE_LEN];
    char endLine[XIA_LINE_LEN];

    fsetpos(fp, end);
    status = xiaGetLine(fp, endLine);

    fsetpos(fp, start);
    status = xiaGetLine(fp, line);

    while (!STREQ(line, endLine)) {
        status = xiaGetLineData(line, tmpName, tmpValue);

        if (status != XIA_SUCCESS) {
            xiaLog(XIA_LOG_ERROR, status, "xiaFileRA",
                   "Error trying to find value for %s", name);
            return status;
        }

        if (STREQ(name, tmpName)) {
            strcpy(value, tmpValue);

            return XIA_SUCCESS;
        }

        status = xiaGetLine(fp, line);
    }

    return XIA_FILE_RA;
}

/*
 * Writes the interface portion of the module configuration to the .ini file.
 */
static int writeInterface(FILE* fp, Module* module) {
    int i;
    int status;

    if (fp == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NO_FILENAME, "writeInterface", "fp cannot be NULL");
        return XIA_NO_FILENAME;
    }

    if (module == NULL) {
        xiaLog(XIA_LOG_ERROR, XIA_NO_MODULE, "writeInterface", "module cannot be NULL");
        return XIA_NO_MODULE;
    }

    for (i = 0; i < (int) N_ELEMS(INTERFACE_WRITERS); i++) {
        if (module->interface_info->type == INTERFACE_WRITERS[i].type) {
            status = INTERFACE_WRITERS[i].fn(fp, module);

            if (status != XIA_SUCCESS) {
                xiaLog(XIA_LOG_ERROR, status, "writeInterface",
                       "Error writing interface data for type '%u'",
                       module->interface_info->type);
                return status;
            }

            return XIA_SUCCESS;
        }
    }

    xiaLog(XIA_LOG_ERROR, XIA_BAD_INTERFACE, "writeInterface",
           "Unknown interface type: '%u'", module->interface_info->type);
    return XIA_BAD_INTERFACE;
}

/*
 * Writes the PLX interface info to the passed in file pointer.
 *
 * Assumes that the file has been advanced to the proper location. Also assumes
 * that the module is using the PLX communication interface.
 */
static int writePLX(FILE* fp, Module* module) {
    ASSERT(fp != NULL);
    ASSERT(module != NULL);

    fprintf(fp, "interface = pxi\n");
    fprintf(fp, "pci_bus = %hu\n",
            (unsigned short) module->interface_info->info.plx->bus);
    fprintf(fp, "pci_slot = %hu\n",
            (unsigned short) module->interface_info->info.plx->slot);

    return XIA_SUCCESS;
}

/*
 * Writes the EPP interface info to the passed in file pointer.
 */
static int writeEPP(FILE* fp, Module* m) {
    ASSERT(fp != NULL);
    ASSERT(m != NULL);

    fprintf(fp, "interface = epp\n");
    fprintf(fp, "epp_address = %#x\n", m->interface_info->info.epp->epp_address);
    fprintf(fp, "daisy_chain_id = %u\n", m->interface_info->info.epp->daisy_chain_id);

    return XIA_SUCCESS;
}

/*
 * Writes the USB interface info to the passed in file pointer.
 */
static int writeUSB(FILE* fp, Module* m) {
    ASSERT(fp != NULL);
    ASSERT(m != NULL);

    fprintf(fp, "interface = usb\n");
    fprintf(fp, "device_number = %u\n", m->interface_info->info.usb->device_number);

    return XIA_SUCCESS;
}

static int writeUSB2(FILE* fp, Module* m) {
    ASSERT(fp != NULL);
    ASSERT(m != NULL);
    ASSERT(m->interface_info->type == XIA_USB2);

    fprintf(fp, "interface = usb2\n");
    fprintf(fp, "device_number = %u\n", m->interface_info->info.usb2->device_number);

    return XIA_SUCCESS;
}

static int writeSerial(FILE* fp, Module* m) {
    ASSERT(fp != NULL);
    ASSERT(m != NULL);
    ASSERT(m->interface_info->type == XIA_SERIAL);

    fprintf(fp, "interface = serial\n");
    if (m->interface_info->info.serial->device_file) {
        fprintf(fp, "device_file = %s\n", m->interface_info->info.serial->device_file);
    } else {
        fprintf(fp, "com_port = %u\n", m->interface_info->info.serial->com_port);
    }
    fprintf(fp, "baud_rate = %u\n", m->interface_info->info.serial->baud_rate);

    return XIA_SUCCESS;
}
