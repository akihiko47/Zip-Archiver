/* This file is part of hwzip from https://www.hanshq.net/zip.html
   It is put in the public domain; see the LICENSE file for details. */

#include "zip.h"

#include "crc32.h"
#include "deflate.h"
#include "test_utils.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Created by:
   $ echo -n foo > foo
   $ echo -n nanananana > bar
   $ mkdir dir
   $ echo -n baz > dir/baz
   $ touch --date="2019-09-21 12:34:56" foo bar dir dir/baz
   $ zip test.zip --entry-comments --archive-comment -r foo bar dir
     adding: foo (stored 0%)
     adding: bar (deflated 40%)
     adding: dir/ (stored 0%)
     adding: dir/baz (stored 0%)
   Enter comment for foo:
   foo
   Enter comment for bar:
   bar
   Enter comment for dir/:
   dir
   Enter comment for dir/baz:
   dirbaz
   enter new zip file comment (end with .):
   testzip
   .
   $ xxd -i < test.zip
*/
static const uint8_t basic_zip[] = {
  0x50, 0x4b, 0x03, 0x04, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5c, 0x64,
  0x35, 0x4f, 0x21, 0x65, 0x73, 0x8c, 0x03, 0x00, 0x00, 0x00, 0x03, 0x00,
  0x00, 0x00, 0x03, 0x00, 0x1c, 0x00, 0x66, 0x6f, 0x6f, 0x55, 0x54, 0x09,
  0x00, 0x03, 0xd0, 0xfc, 0x85, 0x5d, 0x5b, 0xca, 0x8b, 0x5d, 0x75, 0x78,
  0x0b, 0x00, 0x01, 0x04, 0xe8, 0x03, 0x00, 0x00, 0x04, 0xe8, 0x03, 0x00,
  0x00, 0x66, 0x6f, 0x6f, 0x50, 0x4b, 0x03, 0x04, 0x14, 0x00, 0x00, 0x00,
  0x08, 0x00, 0x5c, 0x64, 0x35, 0x4f, 0x9d, 0x3a, 0x97, 0x4a, 0x06, 0x00,
  0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x03, 0x00, 0x1c, 0x00, 0x62, 0x61,
  0x72, 0x55, 0x54, 0x09, 0x00, 0x03, 0xd0, 0xfc, 0x85, 0x5d, 0x5b, 0xca,
  0x8b, 0x5d, 0x75, 0x78, 0x0b, 0x00, 0x01, 0x04, 0xe8, 0x03, 0x00, 0x00,
  0x04, 0xe8, 0x03, 0x00, 0x00, 0xcb, 0x4b, 0xcc, 0x83, 0x42, 0x00, 0x50,
  0x4b, 0x03, 0x04, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5c, 0x64, 0x35,
  0x4f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x04, 0x00, 0x1c, 0x00, 0x64, 0x69, 0x72, 0x2f, 0x55, 0x54, 0x09,
  0x00, 0x03, 0xd0, 0xfc, 0x85, 0x5d, 0x6e, 0xca, 0x8b, 0x5d, 0x75, 0x78,
  0x0b, 0x00, 0x01, 0x04, 0xe8, 0x03, 0x00, 0x00, 0x04, 0xe8, 0x03, 0x00,
  0x00, 0x50, 0x4b, 0x03, 0x04, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5c,
  0x64, 0x35, 0x4f, 0x98, 0x04, 0x24, 0x78, 0x03, 0x00, 0x00, 0x00, 0x03,
  0x00, 0x00, 0x00, 0x07, 0x00, 0x1c, 0x00, 0x64, 0x69, 0x72, 0x2f, 0x62,
  0x61, 0x7a, 0x55, 0x54, 0x09, 0x00, 0x03, 0xd0, 0xfc, 0x85, 0x5d, 0xd0,
  0xfc, 0x85, 0x5d, 0x75, 0x78, 0x0b, 0x00, 0x01, 0x04, 0xe8, 0x03, 0x00,
  0x00, 0x04, 0xe8, 0x03, 0x00, 0x00, 0x62, 0x61, 0x7a, 0x50, 0x4b, 0x01,
  0x02, 0x1e, 0x03, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5c, 0x64, 0x35,
  0x4f, 0x21, 0x65, 0x73, 0x8c, 0x03, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00,
  0x00, 0x03, 0x00, 0x18, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
  0x00, 0xa4, 0x81, 0x00, 0x00, 0x00, 0x00, 0x66, 0x6f, 0x6f, 0x55, 0x54,
  0x05, 0x00, 0x03, 0xd0, 0xfc, 0x85, 0x5d, 0x75, 0x78, 0x0b, 0x00, 0x01,
  0x04, 0xe8, 0x03, 0x00, 0x00, 0x04, 0xe8, 0x03, 0x00, 0x00, 0x66, 0x6f,
  0x6f, 0x50, 0x4b, 0x01, 0x02, 0x1e, 0x03, 0x14, 0x00, 0x00, 0x00, 0x08,
  0x00, 0x5c, 0x64, 0x35, 0x4f, 0x9d, 0x3a, 0x97, 0x4a, 0x06, 0x00, 0x00,
  0x00, 0x0a, 0x00, 0x00, 0x00, 0x03, 0x00, 0x18, 0x00, 0x03, 0x00, 0x00,
  0x00, 0x01, 0x00, 0x00, 0x00, 0xa4, 0x81, 0x40, 0x00, 0x00, 0x00, 0x62,
  0x61, 0x72, 0x55, 0x54, 0x05, 0x00, 0x03, 0xd0, 0xfc, 0x85, 0x5d, 0x75,
  0x78, 0x0b, 0x00, 0x01, 0x04, 0xe8, 0x03, 0x00, 0x00, 0x04, 0xe8, 0x03,
  0x00, 0x00, 0x62, 0x61, 0x72, 0x50, 0x4b, 0x01, 0x02, 0x1e, 0x03, 0x0a,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x5c, 0x64, 0x35, 0x4f, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x18,
  0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0xed, 0x41, 0x83,
  0x00, 0x00, 0x00, 0x64, 0x69, 0x72, 0x2f, 0x55, 0x54, 0x05, 0x00, 0x03,
  0xd0, 0xfc, 0x85, 0x5d, 0x75, 0x78, 0x0b, 0x00, 0x01, 0x04, 0xe8, 0x03,
  0x00, 0x00, 0x04, 0xe8, 0x03, 0x00, 0x00, 0x64, 0x69, 0x72, 0x50, 0x4b,
  0x01, 0x02, 0x1e, 0x03, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5c, 0x64,
  0x35, 0x4f, 0x98, 0x04, 0x24, 0x78, 0x03, 0x00, 0x00, 0x00, 0x03, 0x00,
  0x00, 0x00, 0x07, 0x00, 0x18, 0x00, 0x06, 0x00, 0x00, 0x00, 0x01, 0x00,
  0x00, 0x00, 0xa4, 0x81, 0xc1, 0x00, 0x00, 0x00, 0x64, 0x69, 0x72, 0x2f,
  0x62, 0x61, 0x7a, 0x55, 0x54, 0x05, 0x00, 0x03, 0xd0, 0xfc, 0x85, 0x5d,
  0x75, 0x78, 0x0b, 0x00, 0x01, 0x04, 0xe8, 0x03, 0x00, 0x00, 0x04, 0xe8,
  0x03, 0x00, 0x00, 0x64, 0x69, 0x72, 0x62, 0x61, 0x7a, 0x50, 0x4b, 0x05,
  0x06, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x04, 0x00, 0x38, 0x01, 0x00,
  0x00, 0x05, 0x01, 0x00, 0x00, 0x07, 0x00, 0x74, 0x65, 0x73, 0x74, 0x7a,
  0x69, 0x70
};

/* Created by:
   (After running the steps for basic_zip above)
   $ wget https://www.hanshq.net/files/pkz204g.exe
   $ unzip pkz204g.exe PKZIP.EXE
   $ dosbox -c "mount c ." -c "c:" -c "pkzip pk.zip -P foo bar dir/baz" -c exit
   $ xxd -i < PK.ZIP
*/
static const uint8_t pk_zip[] = {
  0x50, 0x4b, 0x03, 0x04, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5c, 0x64,
  0x35, 0x4f, 0x21, 0x65, 0x73, 0x8c, 0x03, 0x00, 0x00, 0x00, 0x03, 0x00,
  0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x46, 0x4f, 0x4f, 0x66, 0x6f, 0x6f,
  0x50, 0x4b, 0x03, 0x04, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5c, 0x64,
  0x35, 0x4f, 0x9d, 0x3a, 0x97, 0x4a, 0x0a, 0x00, 0x00, 0x00, 0x0a, 0x00,
  0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x42, 0x41, 0x52, 0x6e, 0x61, 0x6e,
  0x61, 0x6e, 0x61, 0x6e, 0x61, 0x6e, 0x61, 0x50, 0x4b, 0x03, 0x04, 0x0a,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x5c, 0x64, 0x35, 0x4f, 0x98, 0x04, 0x24,
  0x78, 0x03, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00,
  0x00, 0x44, 0x49, 0x52, 0x2f, 0x42, 0x41, 0x5a, 0x62, 0x61, 0x7a, 0x50,
  0x4b, 0x01, 0x02, 0x14, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5c,
  0x64, 0x35, 0x4f, 0x21, 0x65, 0x73, 0x8c, 0x03, 0x00, 0x00, 0x00, 0x03,
  0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
  0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x4f, 0x4f,
  0x50, 0x4b, 0x01, 0x02, 0x14, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x5c, 0x64, 0x35, 0x4f, 0x9d, 0x3a, 0x97, 0x4a, 0x0a, 0x00, 0x00, 0x00,
  0x0a, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x20, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x42, 0x41,
  0x52, 0x50, 0x4b, 0x01, 0x02, 0x14, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x5c, 0x64, 0x35, 0x4f, 0x98, 0x04, 0x24, 0x78, 0x03, 0x00, 0x00,
  0x00, 0x03, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x01, 0x00, 0x20, 0x00, 0x00, 0x00, 0x4f, 0x00, 0x00, 0x00, 0x44,
  0x49, 0x52, 0x2f, 0x42, 0x41, 0x5a, 0x50, 0x4b, 0x05, 0x06, 0x00, 0x00,
  0x00, 0x00, 0x03, 0x00, 0x03, 0x00, 0x97, 0x00, 0x00, 0x00, 0x77, 0x00,
  0x00, 0x00, 0x00, 0x00
};

/* From https://en.wikipedia.org/wiki/Zip_(file_format)#Limits */
static const uint8_t empty_zip[] = {
  0x50, 0x4b, 0x05, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* Created by:
   $ echo -n 1234567 > a
   $ wget https://www.hanshq.net/files/pkz204g.exe
   $ unzip pkz204g.exe PKZIP.EXE
   $ dosbox -c "mount c ." -c "c:" -c "pkzip a.zip a" -c exit
   $ xxd -i < A.ZIP | sed 's/0x07/0x4d/g'

   Why 0x4d? Because there is room for a 0x4c payload (if we allow it to
   overlap with the cfh and eocdr):
   "1234567" (7 bytes) + cfh (46 bytes) + filename (1 byte) + eocdr (22 bytes)
 */
static const uint8_t out_of_bounds_member_zip[] = {
  0x50, 0x4b, 0x03, 0x04, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x70,
  0x88, 0x4f, 0x9f, 0x69, 0x03, 0x50, 0x4d, 0x00, 0x00, 0x00, 0x4d, 0x00,
  0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x41, 0x31, 0x32, 0x33, 0x34, 0x35,
  0x36, 0x37, 0x50, 0x4b, 0x01, 0x02, 0x14, 0x00, 0x0a, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x70, 0x70, 0x88, 0x4f, 0x9f, 0x69, 0x03, 0x50, 0x4d, 0x00,
  0x00, 0x00, 0x4d, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x01, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x41, 0x50, 0x4b, 0x05, 0x06, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01,
  0x00, 0x2f, 0x00, 0x00, 0x00, 0x26, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* Created by:
   $ for x in foo bar baz ; do echo $x > $x && zip $x.zip $x ; done
   $ zip test.zip foo.zip bar.zip baz.zip
   $ xxd -i < test.zip
 */
static const uint8_t zip_in_zip[] = {
  0x50, 0x4b, 0x03, 0x04, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd2, 0x4b,
  0x8f, 0x4f, 0xfc, 0xe0, 0x94, 0x8d, 0xa0, 0x00, 0x00, 0x00, 0xa0, 0x00,
  0x00, 0x00, 0x07, 0x00, 0x1c, 0x00, 0x66, 0x6f, 0x6f, 0x2e, 0x7a, 0x69,
  0x70, 0x55, 0x54, 0x09, 0x00, 0x03, 0x2c, 0xef, 0xf5, 0x5d, 0x2c, 0xef,
  0xf5, 0x5d, 0x75, 0x78, 0x0b, 0x00, 0x01, 0x04, 0xe8, 0x03, 0x00, 0x00,
  0x04, 0xe8, 0x03, 0x00, 0x00, 0x50, 0x4b, 0x03, 0x04, 0x0a, 0x00, 0x00,
  0x00, 0x00, 0x00, 0xd2, 0x4b, 0x8f, 0x4f, 0xa8, 0x65, 0x32, 0x7e, 0x04,
  0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x03, 0x00, 0x1c, 0x00, 0x66,
  0x6f, 0x6f, 0x55, 0x54, 0x09, 0x00, 0x03, 0x2c, 0xef, 0xf5, 0x5d, 0x0b,
  0xef, 0xf5, 0x5d, 0x75, 0x78, 0x0b, 0x00, 0x01, 0x04, 0xe8, 0x03, 0x00,
  0x00, 0x04, 0xe8, 0x03, 0x00, 0x00, 0x66, 0x6f, 0x6f, 0x0a, 0x50, 0x4b,
  0x01, 0x02, 0x1e, 0x03, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd2, 0x4b,
  0x8f, 0x4f, 0xa8, 0x65, 0x32, 0x7e, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00,
  0x00, 0x00, 0x03, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
  0x00, 0x00, 0xa4, 0x81, 0x00, 0x00, 0x00, 0x00, 0x66, 0x6f, 0x6f, 0x55,
  0x54, 0x05, 0x00, 0x03, 0x2c, 0xef, 0xf5, 0x5d, 0x75, 0x78, 0x0b, 0x00,
  0x01, 0x04, 0xe8, 0x03, 0x00, 0x00, 0x04, 0xe8, 0x03, 0x00, 0x00, 0x50,
  0x4b, 0x05, 0x06, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x49,
  0x00, 0x00, 0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x4b, 0x03,
  0x04, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd2, 0x4b, 0x8f, 0x4f, 0x09,
  0x2e, 0x40, 0x1a, 0xa0, 0x00, 0x00, 0x00, 0xa0, 0x00, 0x00, 0x00, 0x07,
  0x00, 0x1c, 0x00, 0x62, 0x61, 0x72, 0x2e, 0x7a, 0x69, 0x70, 0x55, 0x54,
  0x09, 0x00, 0x03, 0x2c, 0xef, 0xf5, 0x5d, 0x2c, 0xef, 0xf5, 0x5d, 0x75,
  0x78, 0x0b, 0x00, 0x01, 0x04, 0xe8, 0x03, 0x00, 0x00, 0x04, 0xe8, 0x03,
  0x00, 0x00, 0x50, 0x4b, 0x03, 0x04, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xd2, 0x4b, 0x8f, 0x4f, 0xe9, 0xb3, 0xa2, 0x04, 0x04, 0x00, 0x00, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x03, 0x00, 0x1c, 0x00, 0x62, 0x61, 0x72, 0x55,
  0x54, 0x09, 0x00, 0x03, 0x2c, 0xef, 0xf5, 0x5d, 0x0b, 0xef, 0xf5, 0x5d,
  0x75, 0x78, 0x0b, 0x00, 0x01, 0x04, 0xe8, 0x03, 0x00, 0x00, 0x04, 0xe8,
  0x03, 0x00, 0x00, 0x62, 0x61, 0x72, 0x0a, 0x50, 0x4b, 0x01, 0x02, 0x1e,
  0x03, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd2, 0x4b, 0x8f, 0x4f, 0xe9,
  0xb3, 0xa2, 0x04, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x03,
  0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xa4,
  0x81, 0x00, 0x00, 0x00, 0x00, 0x62, 0x61, 0x72, 0x55, 0x54, 0x05, 0x00,
  0x03, 0x2c, 0xef, 0xf5, 0x5d, 0x75, 0x78, 0x0b, 0x00, 0x01, 0x04, 0xe8,
  0x03, 0x00, 0x00, 0x04, 0xe8, 0x03, 0x00, 0x00, 0x50, 0x4b, 0x05, 0x06,
  0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x49, 0x00, 0x00, 0x00,
  0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x4b, 0x03, 0x04, 0x0a, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xd2, 0x4b, 0x8f, 0x4f, 0x38, 0xcd, 0x36, 0x40,
  0xa0, 0x00, 0x00, 0x00, 0xa0, 0x00, 0x00, 0x00, 0x07, 0x00, 0x1c, 0x00,
  0x62, 0x61, 0x7a, 0x2e, 0x7a, 0x69, 0x70, 0x55, 0x54, 0x09, 0x00, 0x03,
  0x2c, 0xef, 0xf5, 0x5d, 0x2c, 0xef, 0xf5, 0x5d, 0x75, 0x78, 0x0b, 0x00,
  0x01, 0x04, 0xe8, 0x03, 0x00, 0x00, 0x04, 0xe8, 0x03, 0x00, 0x00, 0x50,
  0x4b, 0x03, 0x04, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd2, 0x4b, 0x8f,
  0x4f, 0xe1, 0x39, 0x7b, 0xcc, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00,
  0x00, 0x03, 0x00, 0x1c, 0x00, 0x62, 0x61, 0x7a, 0x55, 0x54, 0x09, 0x00,
  0x03, 0x2c, 0xef, 0xf5, 0x5d, 0x0b, 0xef, 0xf5, 0x5d, 0x75, 0x78, 0x0b,
  0x00, 0x01, 0x04, 0xe8, 0x03, 0x00, 0x00, 0x04, 0xe8, 0x03, 0x00, 0x00,
  0x62, 0x61, 0x7a, 0x0a, 0x50, 0x4b, 0x01, 0x02, 0x1e, 0x03, 0x0a, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xd2, 0x4b, 0x8f, 0x4f, 0xe1, 0x39, 0x7b, 0xcc,
  0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x03, 0x00, 0x18, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xa4, 0x81, 0x00, 0x00,
  0x00, 0x00, 0x62, 0x61, 0x7a, 0x55, 0x54, 0x05, 0x00, 0x03, 0x2c, 0xef,
  0xf5, 0x5d, 0x75, 0x78, 0x0b, 0x00, 0x01, 0x04, 0xe8, 0x03, 0x00, 0x00,
  0x04, 0xe8, 0x03, 0x00, 0x00, 0x50, 0x4b, 0x05, 0x06, 0x00, 0x00, 0x00,
  0x00, 0x01, 0x00, 0x01, 0x00, 0x49, 0x00, 0x00, 0x00, 0x41, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x50, 0x4b, 0x01, 0x02, 0x1e, 0x03, 0x0a, 0x00, 0x00,
  0x00, 0x00, 0x00, 0xd2, 0x4b, 0x8f, 0x4f, 0xfc, 0xe0, 0x94, 0x8d, 0xa0,
  0x00, 0x00, 0x00, 0xa0, 0x00, 0x00, 0x00, 0x07, 0x00, 0x18, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa4, 0x81, 0x00, 0x00, 0x00,
  0x00, 0x66, 0x6f, 0x6f, 0x2e, 0x7a, 0x69, 0x70, 0x55, 0x54, 0x05, 0x00,
  0x03, 0x2c, 0xef, 0xf5, 0x5d, 0x75, 0x78, 0x0b, 0x00, 0x01, 0x04, 0xe8,
  0x03, 0x00, 0x00, 0x04, 0xe8, 0x03, 0x00, 0x00, 0x50, 0x4b, 0x01, 0x02,
  0x1e, 0x03, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd2, 0x4b, 0x8f, 0x4f,
  0x09, 0x2e, 0x40, 0x1a, 0xa0, 0x00, 0x00, 0x00, 0xa0, 0x00, 0x00, 0x00,
  0x07, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xa4, 0x81, 0xe1, 0x00, 0x00, 0x00, 0x62, 0x61, 0x72, 0x2e, 0x7a, 0x69,
  0x70, 0x55, 0x54, 0x05, 0x00, 0x03, 0x2c, 0xef, 0xf5, 0x5d, 0x75, 0x78,
  0x0b, 0x00, 0x01, 0x04, 0xe8, 0x03, 0x00, 0x00, 0x04, 0xe8, 0x03, 0x00,
  0x00, 0x50, 0x4b, 0x01, 0x02, 0x1e, 0x03, 0x0a, 0x00, 0x00, 0x00, 0x00,
  0x00, 0xd2, 0x4b, 0x8f, 0x4f, 0x38, 0xcd, 0x36, 0x40, 0xa0, 0x00, 0x00,
  0x00, 0xa0, 0x00, 0x00, 0x00, 0x07, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0xa4, 0x81, 0xc2, 0x01, 0x00, 0x00, 0x62,
  0x61, 0x7a, 0x2e, 0x7a, 0x69, 0x70, 0x55, 0x54, 0x05, 0x00, 0x03, 0x2c,
  0xef, 0xf5, 0x5d, 0x75, 0x78, 0x0b, 0x00, 0x01, 0x04, 0xe8, 0x03, 0x00,
  0x00, 0x04, 0xe8, 0x03, 0x00, 0x00, 0x50, 0x4b, 0x05, 0x06, 0x00, 0x00,
  0x00, 0x00, 0x03, 0x00, 0x03, 0x00, 0xe7, 0x00, 0x00, 0x00, 0xa3, 0x02,
  0x00, 0x00, 0x00, 0x00
};

/* Created by:
   $ echo -n 1234567 > a
   $ wget https://www.hanshq.net/files/pkz204g.exe
   $ unzip pkz204g.exe PKZIP.EXE
   $ dosbox -c "mount c ." -c "c:" -c "pkzip a.zip a" -c exit
   $ xxd -i < A.ZIP

   Then hand modify the lowest byte in uncomp_size from 0x07 to 0x08.
   This makes the uncompressed size not match the compressed size as it should.
 */
static const uint8_t bad_stored_uncomp_size_zip[] = {
  0x50, 0x4b, 0x03, 0x04, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x70,
  0x88, 0x4f, 0x9f, 0x69, 0x03, 0x50, 0x07, 0x00, 0x00, 0x00, 0x08, 0x00,
  0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x41, 0x31, 0x32, 0x33, 0x34, 0x35,
  0x36, 0x37, 0x50, 0x4b, 0x01, 0x02, 0x14, 0x00, 0x0a, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x70, 0x70, 0x88, 0x4f, 0x9f, 0x69, 0x03, 0x50, 0x07, 0x00,
  0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x01, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x41, 0x50, 0x4b, 0x05, 0x06, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01,
  0x00, 0x2f, 0x00, 0x00, 0x00, 0x26, 0x00, 0x00, 0x00, 0x00, 0x00
};

static time_t magic_time(void)
{
        struct tm tm = {0};

        /* 2019-09-21 12:34:56 */
        tm.tm_year = 2019 - 1900;
        tm.tm_mon = 9 - 1;
        tm.tm_mday = 21;
        tm.tm_hour = 12;
        tm.tm_min = 34;
        tm.tm_sec = 56;

        tm.tm_isdst = -1;

        return mktime(&tm);
}

static void check_extract(const zipmemb_t *m, const char *expected)
{
        uint8_t *uncomp;
        size_t comp_used, uncomp_sz;
        size_t n;

        n = strlen(expected);

        CHECK(m->uncomp_size == n);
        CHECK(m->crc32 == crc32((const uint8_t*)expected, n));

        if (m->method == ZIP_STORED) {
                CHECK(m->comp_size == n);
                CHECK(memcmp(m->comp_data, expected, n) == 0);
                return;
        }

        uncomp = malloc(n);
        assert(uncomp);

        CHECK(hwinflate(m->comp_data, m->comp_size, &comp_used, uncomp, n,
                        &uncomp_sz) == HWINF_OK);
        CHECK(uncomp_sz == n);
        CHECK(memcmp(uncomp, expected, n) == 0);

        free(uncomp);
}

void test_zip_basic(void)
{
        zip_t z;
        zipiter_t i;
        zipmemb_t m;

        CHECK(zip_read(&z, basic_zip, sizeof(basic_zip)));
        CHECK(z.num_members == 4);
        CHECK(z.comment_len == 7);
        CHECK(memcmp(z.comment, "testzip", 7) == 0);

        i = z.members_begin;
        m = zip_member(&z, i);
        CHECK(m.name_len == 3);
        CHECK(memcmp(m.name, "foo", 3) == 0);
        CHECK(m.mtime == magic_time());
        CHECK(m.comment_len == 3);
        CHECK(memcmp(m.comment, "foo", 3) == 0);
        CHECK(m.is_dir == false);
        check_extract(&m, "foo");

        i = m.next;
        m = zip_member(&z, i);
        CHECK(m.name_len == 3);
        CHECK(memcmp(m.name, "bar", 3) == 0);
        CHECK(m.mtime == magic_time());
        CHECK(m.comment_len == 3);
        CHECK(memcmp(m.comment, "bar", 3) == 0);
        CHECK(m.is_dir == false);
        check_extract(&m, "nanananana");

        i = m.next;
        m = zip_member(&z, i);
        CHECK(m.name_len == 4);
        CHECK(memcmp(m.name, "dir/", 4) == 0);
        CHECK(m.mtime == magic_time());
        CHECK(m.comment_len == 3);
        CHECK(memcmp(m.comment, "dir", 3) == 0);
        CHECK(m.is_dir == true);

        i = m.next;
        m = zip_member(&z, i);
        CHECK(m.name_len == 7);
        CHECK(memcmp(m.name, "dir/baz", 7) == 0);
        CHECK(m.mtime == magic_time());
        CHECK(m.comment_len == 6);
        CHECK(memcmp(m.comment, "dirbaz", 3) == 0);
        CHECK(m.is_dir == false);
        check_extract(&m, "baz");

        i = m.next;
        CHECK(i == z.members_end);
}

void test_zip_pk(void)
{
        zip_t z;
        zipiter_t i;
        zipmemb_t m;

        CHECK(zip_read(&z, pk_zip, sizeof(pk_zip)));
        CHECK(z.num_members == 3);
        CHECK(z.comment_len == 0);

        i = z.members_begin;
        m = zip_member(&z, i);
        CHECK(m.name_len == 3);
        CHECK(memcmp(m.name, "FOO", 3) == 0);
        CHECK(m.mtime == magic_time());
        CHECK(m.comment_len == 0);
        CHECK(m.is_dir == false);
        check_extract(&m, "foo");

        i = m.next;
        m = zip_member(&z, i);
        CHECK(m.name_len == 3);
        CHECK(memcmp(m.name, "BAR", 3) == 0);
        CHECK(m.mtime == magic_time());
        CHECK(m.comment_len == 0);
        CHECK(m.is_dir == false);
        check_extract(&m, "nanananana");

        i = m.next;
        m = zip_member(&z, i);
        CHECK(m.name_len == 7);
        CHECK(memcmp(m.name, "DIR/BAZ", 7) == 0);
        CHECK(m.mtime == magic_time());
        CHECK(m.comment_len == 0);
        CHECK(m.is_dir == false);
        check_extract(&m, "baz");

        i = m.next;
        CHECK(i == z.members_end);
}

void test_zip_out_of_bounds_member(void)
{
        zip_t z;

        CHECK(!zip_read(&z, out_of_bounds_member_zip,
                        sizeof(out_of_bounds_member_zip)));
}

void test_zip_empty(void)
{
        zip_t z;

        /* Not enough bytes. */
        CHECK(!zip_read(&z, empty_zip, sizeof(empty_zip) - 1));

        CHECK(zip_read(&z, empty_zip, sizeof(empty_zip)));
        CHECK(z.comment_len == 0);
        CHECK(z.num_members == 0);
        CHECK(z.members_begin == z.members_end);
}

static void write_basic_callback(const char *filename, uint32_t size,
                                 uint32_t comp_size)
{
        static int n = 0;

        switch (n) {
        case 0:
                CHECK(memcmp(filename, "one", 3) == 0);
                CHECK(size == 3);
                CHECK(comp_size <= 3);
                break;
        case 1:
                CHECK(memcmp(filename, "two", 3) == 0);
                CHECK(size == 9);
                CHECK(comp_size <= 9);
                break;
        default:
                CHECK(false);
        }

        n++;
}

void test_zip_write_basic(void)
{
        static const char *const names[] = { "one", "two" };
        static const uint8_t data1[] = "foo";
        static const uint8_t data2[] = "barbarbar";
        static const uint8_t *const data[] = { data1, data2 };
        static const uint32_t sizes[] = { 3, 9 };
        time_t mtimes[2];
        static const char comment[] = "comment";
        size_t max_size, size;
        uint8_t *out, *in;
        zip_t z;
        zipiter_t i;
        zipmemb_t m;

        mtimes[0] = mtimes[1] = magic_time();

        max_size = zip_max_size(2, names, sizes, comment);
        out = malloc(max_size);

        size = zip_write(out, 2, names, data, sizes, mtimes, comment,
                         write_basic_callback);

        in = malloc(size);
        memcpy(in, out, size);
        free(out);

        CHECK(zip_read(&z, in, size));
        CHECK(z.num_members == 2);
        CHECK(z.comment_len == strlen(comment));
        CHECK(memcmp(z.comment, comment, z.comment_len) == 0);

        i = z.members_begin;
        m = zip_member(&z, i);
        CHECK(m.name_len == strlen(names[0]));
        CHECK(memcmp(m.name, names[0], m.name_len) == 0);
        CHECK(m.mtime == mtimes[0]);
        check_extract(&m, (const char*)data[0]);

        i = m.next;
        m = zip_member(&z, i);
        CHECK(m.name_len == strlen(names[1]));
        CHECK(memcmp(m.name, names[1], m.name_len) == 0);
        CHECK(m.mtime == mtimes[1]);
        check_extract(&m, (const char*)data[1]);

        i = m.next;
        CHECK(i == z.members_end);

        free(in);
}

void test_zip_write_empty(void)
{
        size_t max_size, size;
        uint8_t *out;

        max_size = zip_max_size(0, NULL, NULL, NULL);
        out = malloc(max_size);

        size = zip_write(out, 0, NULL, NULL, NULL, NULL, NULL, NULL);
        CHECK(size == sizeof(empty_zip));
        CHECK(memcmp(out, empty_zip, size) == 0);

        free(out);
}

void test_zip_max_comment(void)
{
        char comment[UINT16_MAX + 1];
        size_t max_size, size;
        uint8_t *out;
        zip_t z;

        memset(comment, 'a', UINT16_MAX);
        comment[UINT16_MAX] = '\0';

        max_size = zip_max_size(0, NULL, NULL, comment);
        out = malloc(max_size + 1);
        size = zip_write(out, 0, NULL, NULL, NULL, NULL, comment, NULL);
        CHECK(size <= max_size);
        CHECK(size == sizeof(empty_zip) + UINT16_MAX);

        CHECK(zip_read(&z, out, size));
        CHECK(z.comment_len == UINT16_MAX);
        CHECK(memcmp(z.comment, comment, UINT16_MAX) == 0);
        CHECK(z.num_members == 0);
        CHECK(z.members_begin == z.members_end);

        /* The EOCDR + comment are supposed to be at the end of the file. */
        CHECK(!zip_read(&z, out, size + 1));

        /* Again, check that the EOCDR + comment hit the end of the file. */
        size = zip_write(out, 0, NULL, NULL, NULL, NULL, "12345", NULL);
        CHECK(size == sizeof(empty_zip) + 5);
        CHECK(zip_read(&z, out, size));
        CHECK(!zip_read(&z, out, size + 1));

        free(out);
}

void test_zip_in_zip(void)
{
        zip_t z;
        zipiter_t i;
        zipmemb_t m;

        CHECK(zip_read(&z, zip_in_zip, sizeof(zip_in_zip)));
        CHECK(z.num_members == 3);

        i = z.members_begin;
        m = zip_member(&z, i);
        CHECK(m.name_len == 7);
        CHECK(memcmp(m.name, "foo.zip", 7) == 0);

        i = m.next;
        m = zip_member(&z, i);
        CHECK(m.name_len == 7);
        CHECK(memcmp(m.name, "bar.zip", 7) == 0);

        i = m.next;
        m = zip_member(&z, i);
        CHECK(m.name_len == 7);
        CHECK(memcmp(m.name, "baz.zip", 7) == 0);

        CHECK(m.next == z.members_end);
}

void test_zip_bad_stored_uncomp_size(void)
{
        zip_t z;

        CHECK(!zip_read(&z, bad_stored_uncomp_size_zip,
                        sizeof(bad_stored_uncomp_size_zip)));
}