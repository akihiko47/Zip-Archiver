/* This file is part of hwzip from https://www.hanshq.net/zip.html
   It is put in the public domain; see the LICENSE file for details. */

#include "deflate.h"
#include "hamlet.h"
#include "test_utils.h"
#include "zlib_utils.h"
#include <stdlib.h>
#include <string.h>

void test_inflate_invalid_block_header(void)
{
        /* bfinal: 0, btype: 11 */
        static const uint8_t src = 0x6; /* 0000 0110 */
        size_t src_used, dst_used;
        uint8_t dst[10];

        CHECK(hwinflate(&src, 1, &src_used, dst, 10, &dst_used) == HWINF_ERR);
}

void test_inflate_uncompressed(void)
{
        uint8_t dst[10];
        size_t src_used, dst_used;

        static const uint8_t bad[] = {
                0x01,           /* 0000 0001  bfinal: 1, btype: 00 */
                0x05, 0x00,     /* len: 5 */
                0x12, 0x34      /* nlen: garbage */
        };

        static const uint8_t good[] = {
                0x01,           /* 0000 0001  bfinal: 1, btype: 00 */
                0x05, 0x00,     /* len: 5 */
                0xfa, 0xff,     /* nlen */
                'H', 'e', 'l', 'l', 'o'
        };


        /* Too short for block header. */
        CHECK(hwinflate(bad, 0, &src_used, dst, 10, &dst_used) == HWINF_ERR);
        /* Too short for len. */
        CHECK(hwinflate(bad, 1, &src_used, dst, 10, &dst_used) == HWINF_ERR);
        CHECK(hwinflate(bad, 2, &src_used, dst, 10, &dst_used) == HWINF_ERR);
        /* Too short for nlen. */
        CHECK(hwinflate(bad, 3, &src_used, dst, 10, &dst_used) == HWINF_ERR);
        CHECK(hwinflate(bad, 4, &src_used, dst, 10, &dst_used) == HWINF_ERR);
        /* nlen len mismatch. */
        CHECK(hwinflate(bad, 5, &src_used, dst, 10, &dst_used) == HWINF_ERR);

        /* Not enough input. */
        CHECK(hwinflate(good, 9, &src_used, dst, 4, &dst_used) == HWINF_ERR);

        /* Not enough room to output. */
        CHECK(hwinflate(good, 10, &src_used, dst, 4, &dst_used) == HWINF_FULL);

        /* Success. */
        CHECK(hwinflate(good, 10, &src_used, dst, 5, &dst_used) == HWINF_OK);
        CHECK(src_used == 10);
        CHECK(dst_used == 5);
        CHECK(memcmp(dst, "Hello", 5) == 0);
}

void test_inflate_twocities_intro(void)
{
        static const uint8_t deflated[] = {
0x74,0xeb,0xcd,0x0d,0x80,0x20,0x0c,0x47,0x71,0xdc,0x9d,0xa2,0x03,0xb8,0x88,0x63,
0xf0,0xf1,0x47,0x9a,0x00,0x35,0xb4,0x86,0xf5,0x0d,0x27,0x63,0x82,0xe7,0xdf,0x7b,
0x87,0xd1,0x70,0x4a,0x96,0x41,0x1e,0x6a,0x24,0x89,0x8c,0x2b,0x74,0xdf,0xf8,0x95,
0x21,0xfd,0x8f,0xdc,0x89,0x09,0x83,0x35,0x4a,0x5d,0x49,0x12,0x29,0xac,0xb9,0x41,
0xbf,0x23,0x2e,0x09,0x79,0x06,0x1e,0x85,0x91,0xd6,0xc6,0x2d,0x74,0xc4,0xfb,0xa1,
0x7b,0x0f,0x52,0x20,0x84,0x61,0x28,0x0c,0x63,0xdf,0x53,0xf4,0x00,0x1e,0xc3,0xa5,
0x97,0x88,0xf4,0xd9,0x04,0xa5,0x2d,0x49,0x54,0xbc,0xfd,0x90,0xa5,0x0c,0xae,0xbf,
0x3f,0x84,0x77,0x88,0x3f,0xaf,0xc0,0x40,0xd6,0x5b,0x14,0x8b,0x54,0xf6,0x0f,0x9b,
0x49,0xf7,0xbf,0xbf,0x36,0x54,0x5a,0x0d,0xe6,0x3e,0xf0,0x9e,0x29,0xcd,0xa1,0x41,
0x05,0x36,0x48,0x74,0x4a,0xe9,0x46,0x66,0x2a,0x19,0x17,0xf4,0x71,0x8e,0xcb,0x15,
0x5b,0x57,0xe4,0xf3,0xc7,0xe7,0x1e,0x9d,0x50,0x08,0xc3,0x50,0x18,0xc6,0x2a,0x19,
0xa0,0xdd,0xc3,0x35,0x82,0x3d,0x6a,0xb0,0x34,0x92,0x16,0x8b,0xdb,0x1b,0xeb,0x7d,
0xbc,0xf8,0x16,0xf8,0xc2,0xe1,0xaf,0x81,0x7e,0x58,0xf4,0x9f,0x74,0xf8,0xcd,0x39,
0xd3,0xaa,0x0f,0x26,0x31,0xcc,0x8d,0x9a,0xd2,0x04,0x3e,0x51,0xbe,0x7e,0xbc,0xc5,
0x27,0x3d,0xa5,0xf3,0x15,0x63,0x94,0x42,0x75,0x53,0x6b,0x61,0xc8,0x01,0x13,0x4d,
0x23,0xba,0x2a,0x2d,0x6c,0x94,0x65,0xc7,0x4b,0x86,0x9b,0x25,0x3e,0xba,0x01,0x10,
0x84,0x81,0x28,0x80,0x55,0x1c,0xc0,0xa5,0xaa,0x36,0xa6,0x09,0xa8,0xa1,0x85,0xf9,
0x7d,0x45,0xbf,0x80,0xe4,0xd1,0xbb,0xde,0xb9,0x5e,0xf1,0x23,0x89,0x4b,0x00,0xd5,
0x59,0x84,0x85,0xe3,0xd4,0xdc,0xb2,0x66,0xe9,0xc1,0x44,0x0b,0x1e,0x84,0xec,0xe6,
0xa1,0xc7,0x42,0x6a,0x09,0x6d,0x9a,0x5e,0x70,0xa2,0x36,0x94,0x29,0x2c,0x85,0x3f,
0x24,0x39,0xf3,0xae,0xc3,0xca,0xca,0xaf,0x2f,0xce,0x8e,0x58,0x91,0x00,0x25,0xb5,
0xb3,0xe9,0xd4,0xda,0xef,0xfa,0x48,0x7b,0x3b,0xe2,0x63,0x12,0x00,0x00,0x20,0x04,
0x80,0x70,0x36,0x8c,0xbd,0x04,0x71,0xff,0xf6,0x0f,0x66,0x38,0xcf,0xa1,0x39,0x11,
0x0f };
        static const uint8_t expected[] =
"It was the best of times,\n"
"it was the worst of times,\n"
"it was the age of wisdom,\n"
"it was the age of foolishness,\n"
"it was the epoch of belief,\n"
"it was the epoch of incredulity,\n"
"it was the season of Light,\n"
"it was the season of Darkness,\n"
"it was the spring of hope,\n"
"it was the winter of despair,\n"
"\n"
"we had everything before us, we had nothing before us, "
"we were all going direct to Heaven, we were all going direct the other way"
"---in short, the period was so far like the present period, "
"that some of its noisiest authorities insisted on its being received, "
"for good or for evil, in the superlative degree of comparison only.\n";

        uint8_t dst[1000];
        size_t src_used, dst_used, i;

        CHECK(hwinflate(deflated, sizeof(deflated), &src_used,
                        dst, sizeof(dst), &dst_used) == HWINF_OK);
        CHECK(dst_used == sizeof(expected));
        CHECK(src_used == sizeof(deflated));
        CHECK(memcmp(dst, expected, sizeof(expected)) == 0);

        /* Truncated inputs should fail. */
        for (i = 0; i < sizeof(deflated); i++) {
                CHECK(hwinflate(deflated, i, &src_used, dst, sizeof(dst),
                                &dst_used) == HWINF_ERR);
        }
}

void test_inflate_hamlet(void)
{
        static uint8_t compressed[sizeof(hamlet) * 2];
        static uint8_t decompressed[sizeof(hamlet)];

        int level;
        size_t compressed_sz, src_used, dst_used;

        for (level = 0; level <= 9; level++) {
                compressed_sz = zlib_deflate(hamlet, sizeof(hamlet),
                                             level, compressed,
                                             sizeof(compressed));

                CHECK(hwinflate(compressed, compressed_sz, &src_used,
                                decompressed, sizeof(decompressed),
                                &dst_used) == HWINF_OK);
                CHECK(src_used == compressed_sz);
                CHECK(dst_used == sizeof(hamlet));
                CHECK(memcmp(decompressed, hamlet, sizeof(hamlet)) == 0);
        }
}
