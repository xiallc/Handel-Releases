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

/** @file xia_compare.h
 * @brief XIA utility functions related to approximations and comparisons.
 */

#ifndef XIA_UTIL_APPROX_H
#define XIA_UTIL_APPROX_H

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Determines if the lhs and rhs are the same within a percentage.
 * @param lhs first value for the comparison
 * @param rhs second value for the comparison
 * @param epsilon The percentage used to compare the values.
 * @return True if they are the same within the epsilon.
 */
static bool xia_pct_diff(const double lhs, const double rhs, const double epsilon) {
    return (100 * (fabs(lhs - rhs) / (0.5 * (lhs + rhs)))) <= epsilon;
}

/**
 * @brief Determines if the lhs and rhs are the same within an absolute value.
 * @param lhs first value for the comparison
 * @param rhs second value for the comparison
 * @param epsilon The absolute value to compare the difference.
 * @return True if they are the same within the epsilon.
 */
static bool xia_approx_dbl(const double lhs, const double rhs, const double epsilon) {
    return fabs(lhs - rhs) <= epsilon;
}

/**
 * @brief Determines if the lhs and rhs are the same within an absolute value.
 * @param lhs first value for the comparison
 * @param rhs second value for the comparison
 * @param epsilon The absolute value to compare the difference.
 * @return True if they are the same within the epsilon.
 */
static bool xia_approx_int(const int lhs, const int rhs, const int epsilon) {
    return abs(lhs - rhs) <= epsilon;
}

/**
 * @brief Compares two double arrays to see if the contents are the same within an error
 * @param lhs first array for the comparison
 * @param rhs second array for the comparison
 * @param len the length of the arrays
 * @param epsilon the absolute difference the values are allowed to vary within
 * @return True if each element is the same as its corresponding element.
 */
static bool xia_compare_dbl_ary(const double* lhs, const double* rhs,
                                const unsigned int len, const double epsilon) {
    if (lhs == NULL || rhs == NULL) {
        return false;
    }
    for (unsigned int i = 0; i < len; i++) {
        if (!xia_approx_dbl(lhs[i], rhs[i], epsilon)) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Compares the two arrays and checks that corresponding elements are the same.
 * @param lhs first array for the comparison
 * @param rhs second array for the comparison
 * @param len the length of the arrays
 * @return True if each element is the same as its corresponding element.
 */
static bool xia_compare_uint_ary(const unsigned int* lhs, const unsigned int* rhs,
                                 const unsigned int len) {
    if (lhs == NULL || rhs == NULL) {
        return false;
    }
    for (unsigned int i = 0; i < len; i++) {
        if (lhs[i] != rhs[i]) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Compares the two arrays and checks that corresponding elemnts are the same.
 * @param lhs first array for the comparison
 * @param rhs second array for the comparison
 * @param len the length of the arrays
 * @return True if each element is the same as its corresponding element.
 */
static bool xia_compare_ulong_ary(const unsigned long* lhs, const unsigned long* rhs,
                                  const unsigned long len) {
    if (lhs == NULL || rhs == NULL) {
        return false;
    }
    for (unsigned long i = 0; i < len; i++) {
        if (lhs[i] != rhs[i]) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Compares the two arrays and checks that corresponding elemnts are the same.
 * @param lhs first array for the comparison
 * @param rhs second array for the comparison
 * @param len the length of the arrays
 * @return True if each element is the same as its corresponding element.
 */
static bool xia_compare_ushort_ary(const unsigned short* lhs, const unsigned short* rhs,
                                   const unsigned int len) {
    if (lhs == NULL || rhs == NULL) {
        return false;
    }
    for (unsigned int i = 0; i < len; i++) {
        if (lhs[i] != rhs[i]) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Implements a round half away from zero strategy
 * @param x the value we will round.
 * @return the rounded value to the nearest whole number.
 */
static double xia_round(double x) {
    return (x < 0.) ? ceil(x - 0.5) : floor(x + 0.5);
}

#endif  //XIA_UTIL_APPROX_H
