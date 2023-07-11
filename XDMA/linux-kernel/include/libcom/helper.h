/*
 * Copyright (c) 2003 Sun Microsystems, Inc.  All Rights Reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * Redistribution of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 
 * Redistribution in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of Sun Microsystems, Inc. or the names of
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * 
 * This software is provided "AS IS," without a warranty of any kind.
 * ALL EXPRESS OR IMPLIED CONDITIONS, REPRESENTATIONS AND WARRANTIES,
 * INCLUDING ANY IMPLIED WARRANTY OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE OR NON-INFRINGEMENT, ARE HEREBY EXCLUDED.
 * SUN MICROSYSTEMS, INC. ("SUN") AND ITS LICENSORS SHALL NOT BE LIABLE
 * FOR ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING
 * OR DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.  IN NO EVENT WILL
 * SUN OR ITS LICENSORS BE LIABLE FOR ANY LOST REVENUE, PROFIT OR DATA,
 * OR FOR DIRECT, INDIRECT, SPECIAL, CONSEQUENTIAL, INCIDENTAL OR
 * PUNITIVE DAMAGES, HOWEVER CAUSED AND REGARDLESS OF THE THEORY OF
 * LIABILITY, ARISING OUT OF THE USE OF OR INABILITY TO USE THIS SOFTWARE,
 * EVEN IF SUN HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 */

#pragma once

#include <sys/types.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h> /* For free() */
#include <stdbool.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

#ifndef tboolean
#define tboolean   int
#endif

#ifdef __GNUC__
    #define __UNUSED__(x) x __attribute__((unused))
#else
    #define __UNUSED__(x) x
#endif

struct dla_intf;

struct valstr {
    uint32_t val;
    const char * str;
};
struct oemvalstr {
    uint32_t oem;
    uint16_t val;
    const char * str;
};

const char *
specific_val2str(uint32_t val,
                 const struct valstr *specific,
                 const struct valstr *generic);
const char *val2str(uint32_t val, const struct valstr * vs);
const char *oemval2str(uint32_t oem, uint32_t val, const struct oemvalstr * vs);

int str2double(const char * str, double * double_ptr);
int str2long(const char * str, int64_t * lng_ptr);
int str2ulong(const char * str, uint64_t * ulng_ptr);
int str2int(const char * str, int32_t * int_ptr);
int str2uint(const char * str, uint32_t * uint_ptr);
int str2short(const char * str, int16_t * shrt_ptr);
int str2ushort(const char * str, uint16_t * ushrt_ptr);
int str2char(const char * str, int8_t * chr_ptr);
int str2uchar(const char * str, uint8_t * uchr_ptr);

bool args2buf(int argc, char *argv[], uint8_t *out, size_t len);

int eval_ccode(const int ccode);

int is_fru_id(const char *argv_ptr, uint8_t *fru_id_ptr);
int is_dla_channel_num(const char *argv_ptr, uint8_t *channel_ptr);
int is_dla_user_id(const char *argv_ptr, uint8_t *dla_uid_ptr);
int is_dla_user_priv_limit(const char *argv_ptr, uint8_t *dla_priv_limit_ptr);

uint32_t str2val32(const char *str, const struct valstr *vs);
static inline uint16_t str2val(const char *str, const struct valstr *vs)
{
    return (uint16_t)str2val32(str, vs);
}
void print_valstr(const struct valstr * vs, const char * title, int loglevel);
void print_valstr_2col(const struct valstr * vs, const char * title, int loglevel);


uint16_t buf2short(uint8_t * buf);
uint32_t buf2long(uint8_t * buf);
#define BUF2STR_MAXIMUM_OUTPUT_SIZE    (3*1024 + 1)
const char * buf2str_extended(const uint8_t *buf, int len, const char *sep);
const char * buf2str(const uint8_t *buf, int len);
int str2mac(const char *arg, uint8_t *buf);
const char * mac2str(const uint8_t *buf);
int dla_parse_hex(const char *str, uint8_t *out, int size);
void printbuf(const uint8_t * buf, int len, const char * desc);
uint8_t dla_csum(uint8_t * d, int s);
FILE * dla_open_file(const char * file, int rw);
void dla_start_daemon(struct dla_intf *intf);
uint16_t dla_get_oem_id(struct dla_intf *intf);

#define IS_SET(v, b) ((v) & (1 << (b)))

/**
 * Free the memory and clear the pointer.
 * @param[in] ptr - a pointer to your pointer to free.
 */
static inline void free_n(void *ptr) {
    void **pptr = (void **)ptr;

    if (pptr && *pptr) {
        free(*pptr);
        *pptr = NULL;
    }
}

/* le16toh(), hto16le(), et. al. don't exist for Windows or Apple */
/* For portability, let's simply define our own versions here */

/* little-endian */
static inline uint16_t dla16toh(void *dla16)
{
    uint8_t *dla = (uint8_t *)dla16;
    uint16_t h;

    h = dla[1] << 8; /* MSB */
    h |= dla[0]; /* LSB */

    return h;
}

static inline void htodla16(uint16_t h, uint8_t *dla)
{
    dla[0] = h & 0xFF; /* LSB */
    dla[1] = h >> 8; /* MSB */
}

static inline uint32_t dla24toh(void *dla24)
{
    uint8_t *dla = (uint8_t *)dla24;
    uint32_t h = 0;

    h = dla[2] << 16; /* MSB */
    h |= dla[1] << 8;
    h |= dla[0]; /* LSB */

    return h;
}

static inline void htodla24(uint32_t h, uint8_t *dla)
{
    dla[0] = h & 0xFF; /* LSB */
    dla[1] = (h >> 8) & 0xFF;
    dla[2] = (h >> 16) & 0xFF; /* MSB */
}

static inline uint32_t dla32toh(void *dla32)
{
    uint8_t *dla = dla32;
    uint32_t h;

    h = dla[3] << 24; /* MSB */
    h |= dla[2] << 16;
    h |= dla[1] << 8;
    h |= dla[0]; /* LSB */

    return h;
}

static inline void htodla32(uint32_t h, uint8_t *dla)
{
    dla[0] = h & 0xFF; /* LSB */
    dla[1] = (h >> 8) & 0xFF;
    dla[2] = (h >> 16) & 0xFF;
    dla[3] = (h >> 24) & 0xFF; /* MSB */
}

uint8_t *array_byteswap(uint8_t *buffer, size_t length);
uint8_t *array_ntoh(uint8_t *buffer, size_t length);
uint8_t *array_letoh(uint8_t *buffer, size_t length);

#define dla_open_file_read(file)    dla_open_file(file, 0)
#define dla_open_file_write(file)    dla_open_file(file, 1)

#ifndef __min
# define __min(a, b)  ((a) < (b) ? (a) : (b))
#endif

#ifndef __max
# define __max(a, b)  ((a) > (b) ? (a) : (b))
#endif

#ifndef __minlen
# define __minlen(a, b) ({ int x=strlen(a); int y=strlen(b); (x < y) ? x : y;})
#endif

#ifndef __maxlen
# define __maxlen(a, b) ({ int x=strlen(a); int y=strlen(b); (x > y) ? x : y;})
#endif
