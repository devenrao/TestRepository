/* Copyright 2016 IBM Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 	http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>  // struct stat, stat()
#include <sys/types.h> // Needed for struct stat
#include <unistd.h>    // access(), F_OK, stat()

#include "sbefifo_private.h"

void sbefifo_ffdc_clear(struct sbefifo_context* sctx)
{
    sctx->status = 0;

    if (sctx->ffdc)
    {
        free(sctx->ffdc);
        sctx->ffdc = NULL;
        sctx->ffdc_len = 0;
    }
}
/*
static const uint8_t test_ffdc_data_bytes1[] = {
0xFF, 0xDC, 0x00, 0x08, //magic len in words
0x00, 0x00, 0xA2, 0x01, //seqid cmd class
0x00, 0xA5, 0xBA, 0x9B, //ret code
0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x0C,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0xC0, 0xDE, 0xA8, 0x01,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03};
*/
static const uint8_t test_pozffdc_data_bytes1[] = {
    0xFB, 0XAD, 0x00, 0x09, // magic len inwords
    0x00, 0x00, 0xA2, 0x01, // seqid cmd class cmd
    0x00, 0x01, 0x40, 0x00, // slid sev chipid
    0x00, 0xA5, 0xBA, 0x9B, // return code
    0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x0C,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xC0, 0xDE, 0xA8, 0x01, // resp end
    0x00, 0x00, 0x00, 0x00,                         // resp status
    0x00, 0x00, 0x00, 0x03};                        // distance to0xcode
/*
static const uint8_t test_ffdc_data_bytes2[] = {
0xFB, 0xAD, 0x00, 0x38,  //magic len words
0x00, 0x00, 0xA8, 0x01,  //seqid cmd class
0x00, 0x01, 0x40, 0x00,  //slid sev chipid
0x00, 0x5b, 0xCB, 0x95,  //fapirc //5bcb95
0x00, 0x00, 0x00, 0x00,
0x7A, 0x95, 0x10, 0x49, 0xFF, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x04, 0x00, 0x04, 0x10, 0x38,
0x00, 0x02, 0x00, 0x00, 0x53, 0x42, 0x45, 0x5F,
0x54, 0x52, 0x41, 0x43, 0x45, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x0F,
0x38, 0x13, 0x10, 0x00, 0xFE, 0x23, 0x29, 0xAF,
0x23, 0xC3, 0x46, 0x00, 0x00, 0x00, 0x00, 0x00,
0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0x73, 0xFA, 0x8F,
0x00, 0x00, 0x00, 0x18, 0x00, 0x01, 0x5F, 0xD8,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x2C, 0x8D, 0x01, 0x01, 0x9D, 0x5C, 0x3E, 0x06,
0x5F, 0xD6, 0x00, 0x00, 0x9D, 0x5C, 0x43, 0x11,
0xA6, 0x39, 0x00, 0x06, 0x9F, 0x4D, 0x53, 0xA9,
0x00, 0x00, 0xDA, 0x7A, 0x00, 0x00, 0x00, 0x00,
0xD0, 0xA4, 0x01, 0x02, 0x9F, 0x4D, 0x56, 0x1E,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
0xC4, 0x9C, 0x01, 0x02, 0x9F, 0x4D, 0x58, 0xDA,
0x91, 0xA0, 0x00, 0x06, 0x9F, 0x4D, 0x5A, 0xF1,
0xC9, 0x20, 0x00, 0x00, 0x9F, 0x4D, 0x60, 0x21,
0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
0xEB, 0x80, 0x01, 0x02, 0x9F, 0x4D, 0x61, 0xC6,
0x00, 0x00, 0x00, 0xA2, 0x00, 0x00, 0x00, 0x01,
0x1F, 0x77, 0x01, 0x02, 0x9F, 0x62, 0xC1, 0x36,
0x00, 0x00, 0x00, 0xA2, 0x00, 0x00, 0x00, 0x01,
0xF0, 0xF1, 0x01, 0x02, 0x9F, 0x62, 0xC3, 0x92,
0x00, 0x00, 0x00, 0x03, 0xC0, 0xDE, 0xA8, 0x01,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03};
*/
void sbefifo_ffdc_set(struct sbefifo_context* sctx, uint32_t status,
                      uint8_t* ffdc, uint32_t ffdc_len)
{
    sctx->status = status;

    sctx->ffdc = malloc(ffdc_len);
    if (!sctx->ffdc)
    {
        fprintf(stderr, "Memory allocation error\n");
        return;
    }

    memcpy(sctx->ffdc, ffdc, ffdc_len);
    sctx->ffdc_len = ffdc_len;
}

uint32_t sbefifo_ffdc_get(struct sbefifo_context* sctx, const uint8_t** ffdc,
                          uint32_t* ffdc_len)
{
    // testcode++
    extern const uint8_t
        test_pozffdc_data_bytes1[]; // Or define locally if needed
    sbefifo_ffdc_set(sctx, 0, (uint8_t*)test_pozffdc_data_bytes1,
                     sizeof(test_pozffdc_data_bytes1));
    //++testcode

    if (sctx->ffdc_len > 0)
    {
        *ffdc = sctx->ffdc;
    }
    *ffdc_len = sctx->ffdc_len;

    return sctx->status;
}

static int sbefifo_ffdc_get_uint32(struct sbefifo_context* sctx,
                                   uint32_t offset, uint32_t* value)
{
    uint32_t v;

    if (offset + 4 > sctx->ffdc_len)
        return -1;

    memcpy(&v, sctx->ffdc + offset, 4);
    *value = be32toh(v);

    return 0;
}

static int sbefifo_ffdc_dump_pkg(struct sbefifo_context* sctx, uint32_t offset)
{
    uint32_t offset2 = offset;
    uint32_t header, value;
    uint16_t magic, len_words;
    int i, rc;

    rc = sbefifo_ffdc_get_uint32(sctx, offset2, &header);
    if (rc < 0)
        return -1;
    offset2 += 4;

    /*
     * FFDC package structure
     *
     *             +----------+----------+----------+----------+
     *             |  Byte 0  |  Byte 1  |  Byte 2  |  Byte  3 |
     *  +----------+----------+----------+----------+----------+
     *  |  word 0  |        magic        | length in words (N) |
     *  +----------+---------------------+---------------------+
     *  |  word 1  |      sequence id    |       command       |
     *  +----------+---------------------+---------------------+
     *  |  word 2  |              return code                  |
     *  +----------+-------------------------------------------+
     *  |  word 3  |            FFDC Data - word 0             |
     *  +----------+-------------------------------------------+
     *  |  word 4  |            FFDC Data - word 1             |
     *  +----------+-------------------------------------------+
     *  |    ...   |                    ...                    |
     *  +----------+-------------------------------------------+
     *  |  word N  |            FFDC Data - word N-3           |
     *  +----------+----------+----------+----------+----------+
     */

    magic = header >> 16;
    if (magic != 0xffdc)
    {
        fprintf(stderr, "sbefifo: ffdc expected 0xffdc, got 0x%04x\n", magic);
        return -1;
    }

    len_words = header & 0xffff;

    rc = sbefifo_ffdc_get_uint32(sctx, offset2, &value);
    if (rc < 0)
        return -1;
    offset2 += 4;

    printf("FFDC: Sequence = %u\n", value >> 16);
    printf("FFDC: Command = 0x%08x\n", value & 0xffff);

    rc = sbefifo_ffdc_get_uint32(sctx, offset2, &value);
    if (rc < 0)
        return -1;
    offset2 += 4;

    printf("FFDC: RC = 0x%08x\n", value);

    for (i = 0; i < len_words - 3; i++)
    {
        rc = sbefifo_ffdc_get_uint32(sctx, offset2, &value);
        if (rc < 0)
            return -1;
        offset2 += 4;

        printf("FFDC: Data: 0x%08x\n", value);
    }

    return offset2 - offset;
}

void sbefifo_ffdc_dump(struct sbefifo_context* sctx)
{
    uint32_t offset = 0;

    while (offset < sctx->ffdc_len)
    {
        int n;

        n = sbefifo_ffdc_dump_pkg(sctx, offset);
        if (n <= 0)
            break;

        offset += n;
    }
}
