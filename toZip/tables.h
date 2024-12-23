/* This file is part of hwzip from https://www.hanshq.net/zip.html
   It is put in the public domain; see the LICENSE file for details. */

#ifndef TABLES_H
#define TABLES_H

#include <stdint.h>

/* Element x contains the value of x with the bits in reverse order. */
extern const uint8_t reverse8_tbl[UINT8_MAX + 1];

/* Code lengths for "fixed" Huffman coding of litlen and dist symbols. */
extern const uint8_t fixed_litlen_lengths[288];
extern const uint8_t fixed_dist_lengths[32];

/* Table of litlen symbol values minus 257 with corresponding base length
   and number of extra bits. */
struct litlen_tbl_t {
        uint16_t base_len : 9;
        uint16_t ebits : 7;
};
extern const struct litlen_tbl_t litlen_tbl[29];

/* Mapping from length (3--258) to litlen symbol (257--285). */
extern const uint16_t len2litlen[259];

/* Table of dist symbol values with corresponding base distance and number of
   extra bits. */
struct dist_tbl_t {
        uint32_t base_dist : 16;
        uint32_t ebits : 16;
};
extern const struct dist_tbl_t dist_tbl[30];

/* Mapping from distance (1--32768) to dist symbol (0--29). */
extern const uint8_t distance2dist[32769];

/* Table for computing CRC-32. */
extern const uint32_t crc32_tbl[256];

#endif
