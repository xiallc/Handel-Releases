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

/** @file test_crc.c
 * @brief Tests all the utility headers.
 */
#include <stdint.h>

#include <util/xia_ary_manip.h>
#include <util/xia_compare.h>
#include <util/xia_crc.h>
#include <util/xia_str_manip.h>

#include <acutest.h>

void approx(void) {
    TEST_CASE("Approx percentage");
    {
        TEST_CHECK(xia_pct_diff(100., 98., 10));
        TEST_MSG("98 is within 10%% of 100");

        TEST_CHECK(xia_pct_diff(98., 100., 10));
        TEST_MSG("100 is within 10%% of 98");

        TEST_CHECK(xia_pct_diff(100., 90., 11));
        TEST_MSG("90 is within 10%% of 100");

        TEST_CHECK(xia_pct_diff(90., 100., 11));
        TEST_MSG("100 is within 10%% of 90");

        TEST_CHECK(!xia_pct_diff(100., 80., 10));
        TEST_MSG("80 is NOT within 10%% of 100");

        TEST_CHECK(!xia_pct_diff(80., 100., 10));
        TEST_MSG("100 is NOT within 10%% of 80");
    }

    TEST_CASE("Approx double");
    {
        TEST_CHECK(xia_approx_dbl(0.5598, 0.1, 0.5));
        TEST_MSG("0.5598 is within 0.5 of 0.1");

        TEST_CHECK(!xia_approx_dbl(0.5598, 0.1, 0.1));
        TEST_MSG("0.5598 is NOT within 0.1 of 0.1");

        TEST_CHECK(xia_approx_dbl(0.1, 0.1, 0.1));
        TEST_MSG("0.1 is within 0.1 of 0.1");
    }

    TEST_CASE("Approx int");
    {
        TEST_CHECK(xia_approx_int(5, 1, 4));
        TEST_MSG("5 is within 4 of 1");

        TEST_CHECK(!xia_approx_int(5, 1, 1));
        TEST_MSG("5 is NOT within 1 of 1");
    }
}

void compare_arrays(void) {
    TEST_CASE("Double array");
    {
        double aryA[] = {0.1, 0.2, 0.3, 0.4};
        double aryB[] = {0.2, 0.3, 0.4, 0.5};

        TEST_CHECK(!xia_compare_dbl_ary(NULL, aryB, 4, 0.1));
        TEST_CHECK(!xia_compare_dbl_ary(aryA, NULL, 4, 0.1));
        TEST_CHECK(xia_compare_dbl_ary(aryA, aryB, 4, 0.11));
        TEST_CHECK(!xia_compare_dbl_ary(aryA, aryB, 4, 0.1));
    }

    TEST_CASE("Unsigned int array");
    {
        unsigned int aryA[] = {1, 2, 3, 4};
        unsigned int aryB[] = {2, 3, 4, 5};

        TEST_CHECK(!xia_compare_uint_ary(NULL, aryB, 4));
        TEST_CHECK(!xia_compare_uint_ary(aryA, NULL, 4));
        TEST_CHECK(xia_compare_uint_ary(aryA, aryA, 4));
        TEST_CHECK(!xia_compare_uint_ary(aryA, aryB, 4));
    }

    TEST_CASE("Unsigned long array");
    {
        unsigned long aryA[] = {1, 2, 3, 4};
        unsigned long aryB[] = {2, 3, 4, 5};

        TEST_CHECK(!xia_compare_ulong_ary(NULL, aryB, 4));
        TEST_CHECK(!xia_compare_ulong_ary(aryA, NULL, 4));
        TEST_CHECK(xia_compare_ulong_ary(aryA, aryA, 4));
        TEST_CHECK(!xia_compare_ulong_ary(aryA, aryB, 4));
    }

    TEST_CASE("Unsigned short array");
    {
        unsigned short aryA[] = {1, 2, 3, 4};
        unsigned short aryB[] = {2, 3, 4, 5};

        TEST_CHECK(!xia_compare_ushort_ary(NULL, aryB, 4));
        TEST_CHECK(!xia_compare_ushort_ary(aryA, NULL, 4));
        TEST_CHECK(xia_compare_ushort_ary(aryA, aryA, 4));
        TEST_CHECK(!xia_compare_ushort_ary(aryA, aryB, 4));
    }
}

void concat(void) {
    char s1[] = "begin at ";
    char s2[] = "the beginning";
    char expect[] = "begin at the beginning";

    TEST_CHECK(xia_concat(NULL, NULL) == NULL);
    TEST_CHECK(xia_concat(s1, NULL) == NULL);
    TEST_CHECK(strncmp(xia_concat(s1, s2), expect, strlen(expect)) == 0);
}

void crc32(void) {
    char vals[] = "123456789";
    const uint32_t expected = 0xcbf43926;

    TEST_CASE("Char array");
    {
        uint32_t crc = xia_crc32(0, (unsigned char*) vals, 9);
        TEST_CHECK(crc == expected);
        TEST_MSG("%i != %u", crc, expected);
    }
}

void fill_array(void) {
    TEST_CASE("Character arrays");
    {
        char a[6];
        char expected[] = "aaaaa";

        xia_fill_char_ary(a, 6, 'a');
        TEST_CHECK(strncmp(a, expected, 5) == 0);
        TEST_MSG("%s != %s", a, expected);
    }

    TEST_CASE("Double arrays");
    {
        double a[5];
        xia_fill_dbl_ary(a, 5, 3.);
        for (unsigned int i = 0; i < 5; i++) {
            TEST_CHECK(fabs(a[i] - 3.) <= 0.1);
        }
    }

    TEST_CASE("Unsigned int arrays");
    {
        unsigned int a[5];
        xia_fill_uint_ary(a, 5, 3);
        for (unsigned int i = 0; i < 5; i++) {
            TEST_CHECK(a[i] == 3);
        }
    }

    TEST_CASE("Unsigned short arrays");
    {
        unsigned short a[5];
        xia_fill_ushort_ary(a, 5, 3);
        for (unsigned int i = 0; i < 5; i++) {
            TEST_CHECK(a[i] == 3);
        }
    }

    TEST_CASE("Unsigned long arrays");
    {
        unsigned long a[5];
        xia_fill_ulong_ary(a, 5, 3);
        for (unsigned int i = 0; i < 5; i++) {
            TEST_CHECK(a[i] == 3);
        }
    }
}

void lower(void) {
    TEST_CASE("Null");
    {
        TEST_CHECK(xia_lower(NULL) == NULL);
    }

    TEST_CASE("Happy path");
    {
        char str1[] = "AlIcE";
        char expect[] = "alice";
        char* result = xia_lower(str1);
        TEST_CHECK(strncmp(result, expect, strlen(result)) == 0);
        TEST_MSG("%s != %s", result, expect);
    }
}

void rounding(void) {
    TEST_CASE("Positive");
    {
        TEST_CHECK(xia_round(4.0) == 4.0);
        TEST_CHECK(xia_round(4.1) == 4.0);
        TEST_CHECK(xia_round(4.4999) == 4.0);
        TEST_CHECK(xia_round(4.5) == 5.0);
        TEST_CHECK(xia_round(4.7) == 5.0);
    }

    TEST_CASE("Negative");
    {
        TEST_CHECK(xia_round(-4.0) == -4.0);
        TEST_CHECK(xia_round(-4.1) == -4.0);
        TEST_CHECK(xia_round(-4.5) == -5.0);
        TEST_CHECK(xia_round(-4.7) == -5.0);
    }
}

TEST_LIST = {
    {"Approx", approx},
    {"Compare Arrays", compare_arrays},
    {"Concat", concat},
    {"CRC32", crc32},
    {"Fill Arrays", fill_array},
    {"Lower", lower},
    {"Rounding", rounding},
    {NULL, NULL} /* zeroed record marking the end of the list */
};
