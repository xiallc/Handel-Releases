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

/** @file xia_manip.h
 * @brief XIA Utility functions related to manipulating arrays and objects.
 */

#ifndef XIA_UTIL_MANIP_H
#define XIA_UTIL_MANIP_H

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


/**
 * @brief Fills a char array with the provided character, ensuring null termination.
 * @param ary the array to fill
 * @param len the number of elements in that array, with the last element being
 *            null termination. I.e. Length of 5 -> "aaaa0"
 * @param val the character to write into the array.
 */
static void xia_fill_char_ary(char* ary, const unsigned int len, const char val) {
    if (ary != NULL) {
        for (unsigned int i = 0; i < len; i++) {
            if (i != len - 1) {
                ary[i] = val;
            } else {
                ary[i] = '\0';
            }
        }
    }
}

/**
 * @brief Fills a double array with the provided value
 * @param ary the array to fill
 * @param len the number of elements in that array.
 * @param val the value to write into the array.
 */
static void xia_fill_dbl_ary(double* ary, const unsigned int len, const double val) {
    if (ary != NULL) {
        for (unsigned int i = 0; i < len; i++) {
            ary[i] = val;
        }
    }
}

/**
 * @brief Fills an unsigned int array with the provided value
 * @param ary the array to fill
 * @param len the number of elements in that array.
 * @param val the value to write into the array.
 */
static void xia_fill_uint_ary(unsigned int* ary, const unsigned int len,
                          const unsigned int val) {
    if (ary != NULL) {
        for (unsigned int i = 0; i < len; i++) {
            ary[i] = val;
        }
    }
}

/**
 * @brief Fills an unsigned short array with the provided value
 * @param ary the array to fill
 * @param len the number of elements in that array.
 * @param val the value to write into the array.
 */
static void xia_fill_ushort_ary(unsigned short* ary, const unsigned int len,
                            const unsigned short val) {
    if (ary != NULL) {
        for (unsigned int i = 0; i < len; i++) {
            ary[i] = val;
        }
    }
}

/**
 * @brief Fills an unsigned long array with the provided value
 * @param ary the array to fill
 * @param len the number of elements in that array.
 * @param val the value to write into the array.
 */
static void xia_fill_ulong_ary(unsigned long* ary, const unsigned long len,
                           const unsigned long val) {
    if (ary != NULL) {
        for (unsigned long i = 0; i < len; i++) {
            ary[i] = val;
        }
    }
}

#endif  //XIA_UTIL_MANIP_H
