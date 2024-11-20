# Zip Archiver

Heavily based on [this article.](https://habr.com/ru/companies/vk/articles/490790/)

## How to build

**Linux**
1) `clang generate_tables.c && ./a.out > tables.c`
2) `clang -O3 -DNDEBUG -march=native -o hwzip crc32.c deflate.c huffman.c hwzip.c lz77.c tables.c zip.c`
3) `./hwzip`

**Windows** (Visual Studio)
1) `cl /TC generate_tables.c && generate_tables > tables.c`
2) `cl /O2 /DNDEBUG /MT /Fehwzip.exe /TC crc32.c deflate.c huffman.c hwzip.c lz77.c tables.c zip.c /link setargv.obj`


This archiver uses *deflated* compression method (combination of LZ77 and Huffman coding)