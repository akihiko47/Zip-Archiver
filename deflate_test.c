/* This file is part of hwzip from https://www.hanshq.net/zip.html
   It is put in the public domain; see the LICENSE file for details. */

#include "deflate.h"

#include "bitstream.h"
#include "hamlet.h"
#include "test_utils.h"
#include "zlib_utils.h"
#include <stdlib.h>

/* Compress src, then decompress it and check that it matches.
   Returns the size of the compressed data. */
static size_t deflate_roundtrip(const uint8_t *src, size_t len)
{
        uint8_t *compressed, *decompressed;
        size_t compressed_sz, decompressed_sz, compressed_used;
        size_t i, tmp;

        compressed = malloc(len * 2 + 100);
        CHECK(hwdeflate(src, len, compressed, 2 * len + 100, &compressed_sz));

        decompressed = malloc(len);
        CHECK(hwinflate(compressed, compressed_sz, &compressed_used,
                        decompressed, len, &decompressed_sz) == HWINF_OK);
        CHECK(compressed_used == compressed_sz);
        CHECK(decompressed_sz == len);
        CHECK(src == NULL || memcmp(src, decompressed, len) == 0);

        if (len < 1000) {
                /* For small inputs, check that any too small buffer fails. */
                for (i = 0; i < compressed_used; i++) {
                        CHECK(!hwdeflate(src, len, compressed, i, &tmp));
                }
        } else if (compressed_sz > 500) {
                /* For larger inputs, try cutting off the first block. */
                CHECK(!hwdeflate(src, len, compressed, 500, &tmp));
        }

        free(compressed);
        free(decompressed);

        return compressed_sz;
}

typedef enum {
        UNCOMP = 0x0,
        STATIC = 0x1,
        DYNAMIC = 0x2
} block_t;

static void check_deflate_string(const char *str, block_t expected_type)
{
        uint8_t comp[1000];
        size_t comp_sz;

        CHECK(hwdeflate((const uint8_t*)str, strlen(str), comp,
                        sizeof(comp), &comp_sz));
        CHECK(((comp[0] & 7) >> 1) == expected_type);

        deflate_roundtrip((const uint8_t*)str, strlen(str));
}

void test_deflate_basic(void)
{
        char buf[256];
        size_t i;

        /* Empty input; a static block is shortest. */
        deflate_roundtrip(NULL, 0);
        check_deflate_string("", STATIC);

        /* One byte; a static block is shortest. */
        check_deflate_string("a", STATIC);

        /* Repeated substring. */
        check_deflate_string("hellohello", STATIC);

        /* Non-repeated long string with small alphabet. Dynamic. */
        check_deflate_string("abcdefghijklmnopqrstuvwxyz"
                             "zyxwvutsrqponmlkjihgfedcba", DYNAMIC);

        /* No repetition, uniform distribution. Uncompressed. */
        for (i = 0; i < 255; i++) {
                buf[i] = (char)(i + 1);
        }
        buf[255] = 0;
        check_deflate_string(buf, UNCOMP);
}

void test_deflate_hamlet(void)
{
        size_t len;

        len = deflate_roundtrip(hamlet, sizeof(hamlet));

        /* Update if we make compression better. */
        CHECK(len == 80134);
}

void test_deflate_mixed_blocks(void)
{
        uint8_t *src, *p;
        uint32_t r;
        size_t i, j;
        const size_t src_size = 2 * 1024 * 1024;

        src = malloc(src_size);

        memset(src, 0, src_size);

        p = src;
        r = 0;
        for (i = 0; i < 5; i++) {
                /* Data suitable for compressed blocks. */
                memcpy(src, hamlet, sizeof(hamlet));
                p += sizeof(hamlet);

                /* Random data, likely to go in an uncompressed block. */
                for (j = 0; j < 128000; j++) {
                        r = next_test_rand(r);
                        *p++ = (uint8_t)(r >> 24);
                }
        }

        deflate_roundtrip(src, src_size);

        free(src);
}

void test_deflate_random(void)
{
        uint8_t *src;
        const size_t src_size = 3 * 1024 * 1024;
        uint32_t r;
        size_t i;

        src = malloc(src_size);

        r = 0;
        for (i = 0; i < src_size; i++) {
                r = next_test_rand(r);
                src[i] = (uint8_t)(r >> 24);
        }

        deflate_roundtrip(src, src_size);

        free(src);
}
