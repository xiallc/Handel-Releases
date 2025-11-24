/* SPDX-License-Identifier: Apache-2.0 */

/*
 * Copyright 2025 XIA LLC, All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/** @file xia_str_manip.h
 * @brief Functions for manipulating strings
 */

#ifndef XIA_UTIL_STRING_MANIP_H
#define XIA_UTIL_STRING_MANIP_H

#include <ctype.h>
#include <string.h>

/**
 * @brief Concatenates two char arrays into a single one that's null terminated.
 * @param s1 The first part of the new array
 * @param s2 The second part of the new array.
 * @return NULL if either array is null, the concatenation otherwise.
 */
static char* xia_concat(const char* s1, const char* s2) {
    if (s1 == NULL || s2 == NULL){
        return NULL;
    }
    char* result = calloc(sizeof(char), strlen(s1) + strlen(s2) + 1);
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

static char* xia_lower(const char* s1) {
    if (s1 == NULL) {
        return NULL;
    }
    char* result = calloc(sizeof(char), strlen(s1)+1);
    for (unsigned int i = 0; i < strlen(s1); i++) {
        result[i] = (char)tolower(s1[i]);
    }
    return result;
}

#endif  //XIA_UTIL_STRING_MANIP_H
