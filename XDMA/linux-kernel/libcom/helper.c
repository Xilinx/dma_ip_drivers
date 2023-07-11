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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>  /* For TIOCNOTTY */

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <signal.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <ctype.h>

#if HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef HAVE_PATHS_H
# include <paths.h>
#else
# define _PATH_RUN "/run/"
#endif

#include <helper.h>
#include <log.h>

const struct valstr completion_code_vals[] = {
    { 0x00, "Command completed normally" },
    { 0xc1, "Invalid command" },
    { 0xc3, "Timeout" },
    { 0xc4, "Out of space" },
    { 0xc5, "Reservation cancelled or invalid" },
    { 0xc7, "Request data length invalid" },
    { 0xc9, "Parameter out of range" },
    { 0xca, "Cannot return number of requested data bytes" },
    { 0xcc, "Invalid data field in request" },
    { 0xff, "Unspecified error" },
    { 0x00, NULL }
};

extern int verbose;

uint32_t buf2long(uint8_t * buf) {
    return (uint32_t)(buf[3] << 24 | buf[2] << 16 | buf[1] << 8 | buf[0]);
}

uint16_t buf2short(uint8_t * buf) {
    return (uint16_t)(buf[1] << 8 | buf[0]);
}

/* buf2str_extended - convert sequence of bytes to hexadecimal string with
 * optional separator
 *
 * @param buf - data to convert
 * @param len - size of data
 * @param sep - optional separator (can be NULL)
 *
 * @returns     buf representation in hex, possibly truncated to fit
 *              allocated static memory
 */
const char *
buf2str_extended(const uint8_t *buf, int len, const char *sep) {
    static char str[BUF2STR_MAXIMUM_OUTPUT_SIZE];
    char *cur;
    int i;
    int sz;
    int left;
    int sep_len;

    if (!buf) {
        snprintf(str, sizeof(str), "<NULL>");
        return (const char *)str;
    }
    cur = str;
    left = sizeof(str);
    if (sep) {
        sep_len = strlen(sep);
    } else {
        sep_len = 0;
    }
    for (i = 0; i < len; i++) {
        /* may return more than 2, depending on locale */
        sz = snprintf(cur, left, "%2.2x", buf[i]);
        if (sz >= left) {
            /* buffer overflow, truncate */
            break;
        }
        cur += sz;
        left -= sz;
        /* do not write separator after last byte */
        if (sep && i != (len - 1)) {
            if (sep_len >= left) {
                break;
            }
            strncpy(cur, sep, left - sz);
            cur += sep_len;
            left -= sep_len;
        }
    }
    *cur = '\0';

    return (const char *)str;
}

const char *
buf2str(const uint8_t *buf, int len) {
    return buf2str_extended(buf, len, NULL);
}

/* dla_parse_hex - convert hexadecimal numbers to ascii string
 *                  Input string must be composed of two-characer
 *                  hexadecimal numbers.
 *                  There is no separator between the numbers. Each number
 *                  results in one byte of the converted string.
 *
 *                  Example: dla_parse_hex("50415353574F5244")
 *                  returns 'PASSWORD'
 *
 * @param str:  input string. It must contain only even number
 *              of '0'-'9','a'-'f' and 'A-F' characters.
 * @param out: pointer to output data
 * @param size: size of the output buffer
 * @returns 0 for empty input string
 *         -1 for string with odd length
 *         -2 if out is NULL
 *         -3 if there is non-hexadecimal char in string
 *         >0 length of resulting binary data even if it is > size
 */
int dla_parse_hex(const char *str, uint8_t *out, int size) {
    const char *p;
    uint8_t *q;
    uint8_t d = 0;
    uint8_t b = 0;
    int shift = 4;
    int len;

    len = strlen(str);
    if (len == 0) {
        return 0;
    }

    if (len % 2 != 0) {
        return -1;
    }

    len /= 2; /* out bytes */
    if (!out) {
        return -2;
    }

    for (p = str, q = out; *p; p++) {
        if (!isxdigit(*p)) {
            return -3;
        }

        if (*p < 'A') {
            /* it must be 0-9 */
            d = *p - '0';
        } else {
            /* it's A-F or a-f */
            /* convert to lowercase and to 10-15 */
            d = (*p | 0x20) - 'a' + 10;
        }

        if (q < (out + size)) {
            /* there is space, store */
            b += d << shift;
            if (shift) {
                shift = 0;
            } else {
                shift = 4;
                *q = b;
                b = 0;
                q++;
            }
        }
    }

    return len;
}

void printbuf(const uint8_t * buf, int len, const char * desc) {
    int i;

    if (len <= 0) {
        return;
    }

    if (verbose < 1) {
        return;
    }

    fprintf(stderr, "%s (%d bytes)\n", desc, len);
    for (i=0; i<len; i++) {
        if (((i%16) == 0) && (i != 0)) {
            fprintf(stderr, "\n");
        }
        fprintf(stderr, " %2.2x", buf[i]);
    }
    fprintf(stderr, "\n");
}

/*
 * Unconditionally reverse the order of arbitrarily long strings of bytes
 */
uint8_t *array_byteswap(uint8_t *buffer, size_t length) {
    size_t i;
    uint8_t temp;
    size_t max = length - 1;

    for (i = 0; i < length / 2; ++i) {
        temp = buffer[i];
        buffer[i] = buffer[max - i];
        buffer[max - i] = temp;
    }

    return buffer;
}

/* Convert data array from network (big-endian) to host byte order */
uint8_t *array_ntoh(uint8_t *buffer, size_t length) {
#if WORDS_BIGENDIAN
    /* Big-endian host doesn't need conversion from big-endian network */
    (void)length; /* Silence the compiler */
    return buffer;
#else
    /* Little-endian host needs conversion from big-endian network */
    return array_byteswap(buffer, length);
#endif
}

/* Convert data array from little-endian to host byte order */
uint8_t *array_letoh(uint8_t *buffer, size_t length) {
#if WORDS_BIGENDIAN
    return array_byteswap(buffer, length);
#else
    (void)length; /* Silence the compiler */
    return buffer;
#endif
}

/* str2mac - parse-out MAC address from given string and store it
 * into buffer.
 *
 * @arg: string to be parsed.
 * @buf: buffer of 6 to hold parsed MAC address.
 *
 * returns zero on success, (-1) on error and error message is printed-out.
 */
int str2mac(const char *arg, uint8_t *buf) {
    unsigned int m1 = 0;
    unsigned int m2 = 0;
    unsigned int m3 = 0;
    unsigned int m4 = 0;
    unsigned int m5 = 0;
    unsigned int m6 = 0;
    if (sscanf(arg, "%02x:%02x:%02x:%02x:%02x:%02x",
           &m1, &m2, &m3, &m4, &m5, &m6) != 6) {
        lprintf(LOG_ERR, "Invalid MAC address: %s", arg);
        return -1;
    }
    if (m1 > UINT8_MAX || m2 > UINT8_MAX
            || m3 > UINT8_MAX || m4 > UINT8_MAX
            || m5 > UINT8_MAX || m6 > UINT8_MAX) {
        lprintf(LOG_ERR, "Invalid MAC address: %s", arg);
        return -1;
    }
    buf[0] = (uint8_t)m1;
    buf[1] = (uint8_t)m2;
    buf[2] = (uint8_t)m3;
    buf[3] = (uint8_t)m4;
    buf[4] = (uint8_t)m5;
    buf[5] = (uint8_t)m6;
    return 0;
}

/* mac2str   -- return MAC address as a string
 *
 * @buf: buffer of 6 to hold parsed MAC address.
 */
const char * mac2str(const uint8_t *buf) {
    return buf2str_extended(buf, 6, ":");
}

/**
 * Find the index of value in a valstr array
 *
 * @param[in] val The value to search for
 * @param[in] vs  The valstr array to search in
 * @return >=0    The index into \p vs
 * @return -1     Error: value \p val was not found in \p vs
 */
static inline off_t find_val_idx(uint32_t val, const struct valstr *vs) {
    if (vs) {
        for (off_t i = 0; vs[i].str; ++i) {
            if (vs[i].val == val) {
                return i;
            }
        }
    }

    return -1;
}

/**
 * Generate a statically allocated 'Unknown' string for the provided value.
 * The function is not thread-safe (as most of dlatool).
 *
 * @param[in] val The value to put into the string
 * @returns       A pointer to a statically allocated string
 */
static inline const char *unknown_val_str(uint32_t val) {
    static char un_str[32];
    memset(un_str, 0, 32);
    snprintf(un_str, 32, "Unknown (0x%02X)", val);

    return un_str;
}

const char * specific_val2str(uint32_t val,
                 const struct valstr *specific,
                 const struct valstr *generic) {
    int i;

    if (0 <= (i = find_val_idx(val, specific))) {
        return specific[i].str;
    }

    if (0 <= (i = find_val_idx(val, generic))) {
        return generic[i].str;
    }

    return unknown_val_str(val);
}

const char *val2str(uint32_t val, const struct valstr *vs) {
    return specific_val2str(val, NULL, vs);
}


/* str2double - safely convert string to double
 *
 * @str: source string to convert from
 * @double_ptr: pointer where to store result
 *
 * returns zero on success
 * returns (-1) if one of args is NULL, (-2) invalid input, (-3) for *flow
 */
int str2double(const char * str, double * double_ptr) {
    char * end_ptr = 0;
    if (!str || !double_ptr) {
        return (-1);
    }

    *double_ptr = 0;
    errno = 0;
    *double_ptr = strtod(str, &end_ptr);

    if (*end_ptr != '\0') {
        return (-2);
    }

    if (errno != 0) {
        return (-3);
    }

    return 0;
} /* str2double(...) */

/* str2long - safely convert string to int64_t
 *
 * @str: source string to convert from
 * @lng_ptr: pointer where to store result
 *
 * returns zero on success
 * returns (-1) if one of args is NULL, (-2) invalid input, (-3) for *flow
 */
int str2long(const char * str, int64_t * lng_ptr) {
    char * end_ptr = 0;
    if (!str || !lng_ptr) {
        return (-1);
    }

    *lng_ptr = 0;
    errno = 0;
    *lng_ptr = strtol(str, &end_ptr, 0);

    if (*end_ptr != '\0') {
        return (-2);
    }

    if (errno != 0) {
        return (-3);
    }

    return 0;
} /* str2long(...) */

/* str2ulong - safely convert string to uint64_t
 *
 * @str: source string to convert from
 * @ulng_ptr: pointer where to store result
 *
 * returns zero on success
 * returns (-1) if one of args is NULL, (-2) invalid input, (-3) for *flow
 */
int str2ulong(const char * str, uint64_t * ulng_ptr) {
    char * end_ptr = 0;
    if (!str || !ulng_ptr) {
        return (-1);
    }

    *ulng_ptr = 0;
    errno = 0;
    *ulng_ptr = strtoul(str, &end_ptr, 0);

    if (*end_ptr != '\0') {
        return (-2);
    }

    if (errno != 0) {
        return (-3);
    }

    return 0;
} /* str2ulong(...) */

/* str2int - safely convert string to int32_t
 *
 * @str: source string to convert from
 * @int_ptr: pointer where to store result
 *
 * returns zero on success
 * returns (-1) if one of args is NULL, (-2) invalid input, (-3) for *flow
 */
int str2int(const char * str, int32_t * int_ptr) {
    int rc = 0;
    int64_t arg_long = 0;
    if (!str || !int_ptr) {
        return (-1);
    }

    if ( (rc = str2long(str, &arg_long)) != 0 ) {
        *int_ptr = 0;
        return rc;
    }

    if (arg_long < INT32_MIN || arg_long > INT32_MAX) {
        return (-3);
    }

    *int_ptr = (int32_t)arg_long;
    return 0;
} /* str2int(...) */

/* str2uint - safely convert string to uint32_t
 *
 * @str: source string to convert from
 * @uint_ptr: pointer where to store result
 *
 * returns zero on success
 * returns (-1) if one of args is NULL, (-2) invalid input, (-3) for *flow
 */
int str2uint(const char * str, uint32_t * uint_ptr) {
    int rc = 0;
    uint64_t arg_ulong = 0;
    if (!str || !uint_ptr) {
        return (-1);
    }

    if ( (rc = str2ulong(str, &arg_ulong)) != 0) {
        *uint_ptr = 0;
        return rc;
    }

    if (arg_ulong > UINT32_MAX) {
        return (-3);
    }

    *uint_ptr = (uint32_t)arg_ulong;
    return 0;
} /* str2uint(...) */

/* str2short - safely convert string to int16_t
 *
 * @str: source string to convert from
 * @shrt_ptr: pointer where to store result
 *
 * returns zero on success
 * returns (-1) if one of args is NULL, (-2) invalid input, (-3) for *flow
 */
int str2short(const char * str, int16_t * shrt_ptr) {
    int rc = (-3);
    int64_t arg_long = 0;
    if (!str || !shrt_ptr) {
        return (-1);
    }

    if ( (rc = str2long(str, &arg_long)) != 0 ) {
        *shrt_ptr = 0;
        return rc;
    }

    if (arg_long < INT16_MIN || arg_long > INT16_MAX) {
        return (-3);
    }

    *shrt_ptr = (int16_t)arg_long;
    return 0;
} /* str2short(...) */

/* str2ushort - safely convert string to uint16_t
 *
 * @str: source string to convert from
 * @ushrt_ptr: pointer where to store result
 *
 * returns zero on success
 * returns (-1) if one of args is NULL, (-2) invalid input, (-3) for *flow
 */
int str2ushort(const char * str, uint16_t * ushrt_ptr) {
    int rc = (-3);
    uint64_t arg_ulong = 0;

    if (!str || !ushrt_ptr) {
        return (-1);
    }

    if ( (rc = str2ulong(str, &arg_ulong)) != 0 ) {
        *ushrt_ptr = 0;
        return rc;
    }

    if (arg_ulong > UINT16_MAX) {
        return (-3);
    }

    *ushrt_ptr = (uint16_t)arg_ulong;
    return 0;
} /* str2ushort(...) */

/* str2char - safely convert string to int8
 *
 * @str: source string to convert from
 * @chr_ptr: pointer where to store result
 *
 * returns zero on success
 * returns (-1) if one of args is NULL, (-2) or (-3) if conversion fails
 */
int str2char(const char *str, int8_t * chr_ptr) {
    int rc = (-3);
    int64_t arg_long = 0;
    if (!str || !chr_ptr) {
        return (-1);
    }
    if ((rc = str2long(str, &arg_long)) != 0) {
        *chr_ptr = 0;
        return rc;
    }
    if (arg_long < INT8_MIN || arg_long > INT8_MAX) {
        return (-3);
    }
    *chr_ptr = (uint8_t)arg_long;
    return 0;
} /* str2char(...) */

/* str2uchar - safely convert string to uint8
 *
 * @str: source string to convert from
 * @uchr_ptr: pointer where to store result
 *
 * returns zero on success
 * returns (-1) if one of args is NULL, (-2) or (-3) if conversion fails
 */
int str2uchar(const char * str, uint8_t * uchr_ptr) {
    int rc = (-3);
    uint64_t arg_ulong = 0;
    if (!str || !uchr_ptr) {
        return (-1);
    }

    if ( (rc = str2ulong(str, &arg_ulong)) != 0 ) {
        *uchr_ptr = 0;
        return rc;
    }

    if (arg_ulong > UINT8_MAX) {
        return (-3);
    }

    *uchr_ptr = (uint8_t)arg_ulong;
    return 0;
} /* str2uchar(...) */

uint32_t str2val32(const char *str, const struct valstr *vs) {
    int i;

    for (i = 0; vs[i].str; i++) {
        if (strcasecmp(vs[i].str, str) == 0) {
            return vs[i].val;
        }
    }

    return vs[i].val;
}

/* print_valstr  -  print value string list to log or stdout
 *
 * @vs:        value string list to print
 * @title:    name of this value string list
 * @loglevel:    what log level to print, -1 for stdout
 */
void print_valstr(const struct valstr * vs, const char * title, int loglevel) {
    int i;

    if (!vs) {
        return;
    }

    if (title) {
        if (loglevel < 0) {
            printf("\n%s:\n\n", title);
        } else {
            lprintf(loglevel, "\n%s:\n", title);
        }
    }

    if (loglevel < 0) {
        printf("  VALUE\tHEX\tSTRING\n");
        printf("==============================================\n");
    } else {
        lprintf(loglevel, "  VAL\tHEX\tSTRING");
        lprintf(loglevel, "==============================================");
    }

    for (i = 0; vs[i].str; i++) {
        if (loglevel < 0) {
            if (vs[i].val < 256) {
                printf("  %d\t0x%02x\t%s\n", vs[i].val, vs[i].val, vs[i].str);
            } else {
                printf("  %d\t0x%04x\t%s\n", vs[i].val, vs[i].val, vs[i].str);
            }
        } else {
            if (vs[i].val < 256) {
                lprintf(loglevel, "  %d\t0x%02x\t%s", vs[i].val, vs[i].val, vs[i].str);
            } else {
                lprintf(loglevel, "  %d\t0x%04x\t%s", vs[i].val, vs[i].val, vs[i].str);
            }
        }
    }

    if (loglevel < 0) {
        printf("\n");
    } else {
        lprintf(loglevel, "");
    }
}

/* print_valstr_2col  -  print value string list in two columns to log or stdout
 *
 * @vs:        value string list to print
 * @title:    name of this value string list
 * @loglevel:    what log level to print, -1 for stdout
 */
void print_valstr_2col(const struct valstr * vs, 
                       const char * title, int loglevel) {
    int i;

    if (!vs) {
        return;
    }

    if (title) {
        if (loglevel < 0) {
            printf("\n%s:\n\n", title);
        } else {
            lprintf(loglevel, "\n%s:\n", title);
        }
    }

    for (i = 0; vs[i].str; i++) {
        if (!vs[i+1].str) {
            /* last one */
            if (loglevel < 0) {
                printf("  %4d  %-32s\n", vs[i].val, vs[i].str);
            } else {
                lprintf(loglevel, "  %4d  %-32s\n", vs[i].val, vs[i].str);
            }
        } else {
            if (loglevel < 0) {
                printf("  %4d  %-32s    %4d  %-32s\n",
                       vs[i].val, vs[i].str, vs[i+1].val, vs[i+1].str);
            } else {
                lprintf(loglevel, "  %4d  %-32s    %4d  %-32s\n",
                    vs[i].val, vs[i].str, vs[i+1].val, vs[i+1].str);
            }
            i++;
        }
    }

    if (loglevel < 0) {
        printf("\n");
    } else {
        lprintf(loglevel, "");
    }
}

/* dla_csum  -  calculate an dla checksum
 *
 * @d:        buffer to check
 * @s:        position in buffer to start checksum from
 */
uint8_t dla_csum(uint8_t * d, int s) {
    uint8_t c = 0;
    for (; s > 0; s--, d++) {
        c += *d;
    }
    return -c;
}

/* dla_open_file  -  safely open a file for reading or writing
 *
 * @file:    filename
 * @rw:        read-write flag, 1=write
 *
 * returns pointer to file handler on success
 * returns NULL on error
 */
FILE * dla_open_file(const char * file, int rw) {
    struct stat st1, st2;
    FILE * fp;

    /* verify existence */
    if (lstat(file, &st1) < 0) {
        if (rw) {
            /* does not exist, ok to create */
            fp = fopen(file, "w");
            if (!fp) {
                lperror(LOG_ERR, "Unable to open file %s "
                    "for write", file);
                return NULL;
            }
            /* created ok, now return the descriptor */
            return fp;
        } else {
            lprintf(LOG_ERR, "File %s does not exist", file);
            return NULL;
        }
    }

#ifndef ENABLE_FILE_SECURITY
    if (!rw) {
        /* on read skip the extra checks */
        fp = fopen(file, "r");
        if (!fp) {
            lperror(LOG_ERR, "Unable to open file %s", file);
            return NULL;
        }
        return fp;
    }
#endif

    /* it exists - only regular files, not links */
    if (S_ISREG(st1.st_mode) == 0) {
        lprintf(LOG_ERR, "File %s has invalid mode: %d",
            file, st1.st_mode);
        return NULL;
    }

    /* allow only files with 1 link (itself) */
    if (st1.st_nlink != 1) {
        lprintf(LOG_ERR, "File %s has invalid link count: %d != 1",
               file, (int)st1.st_nlink);
        return NULL;
    }

    fp = fopen(file, rw ? "w+" : "r");
    if (!fp) {
        lperror(LOG_ERR, "Unable to open file %s", file);
        return NULL;
    }

    /* stat again */
    if (fstat(fileno(fp), &st2) < 0) {
        lperror(LOG_ERR, "Unable to stat file %s", file);
        fclose(fp);
        return NULL;
    }

    /* verify inode */
    if (st1.st_ino != st2.st_ino) {
        lprintf(LOG_ERR, "File %s has invalid inode: %d != %d",
            file, st1.st_ino, st2.st_ino);
        fclose(fp);
        return NULL;
    }

    /* verify owner */
    if (st1.st_uid != st2.st_uid) {
        lprintf(LOG_ERR, "File %s has invalid user id: %d != %d",
            file, st1.st_uid, st2.st_uid);
        fclose(fp);
        return NULL;
    }

    /* verify inode */
    if (st2.st_nlink != 1) {
        lprintf(LOG_ERR, "File %s has invalid link count: %d != 1",
            file, st2.st_nlink);
        fclose(fp);
        return NULL;
    }

    return fp;
}

/* eval_ccode - evaluate return value of _dla_* functions and print error error
 * message, if conditions are met.
 *
 * @ccode - return value of _dla_* function.
 *
 * returns - 0 if ccode is 0, otherwise (-1) and error might get printed-out.
 */
int eval_ccode(const int ccode) {
    if (!ccode) {
        return 0;
    } else if (ccode < 0) {
        switch (ccode) {
        case (-1):
            lprintf(LOG_ERR, "Response is NULL.");
            break;
        case (-2):
            lprintf(LOG_ERR, "Unexpected data length received.");
            break;
        case (-3):
            lprintf(LOG_ERR, "Invalid function parameter.");
            break;
        case (-4):
            lprintf(LOG_ERR, "dlatool: malloc failure.");
            break;
        default:
            break;
        }
        return (-1);
    } else {
        lprintf(LOG_ERR, "Command failed: %s",
                val2str(ccode, completion_code_vals));
        return (-1);
    }
}

/** Parse command line arguments as numeric byte values (dec or hex)
 *  and store them in a \p len sized buffer \p out.
 *
 * @param[in] argc Number of arguments
 * @param[in] argv Array of arguments
 * @param[out] out The output buffer
 * @param[in] len Length of the output buffer in bytes (no null-termination
 *                is assumed, the input data is treated as raw byte values,
 *                not as a string.
 *
 * @returns A success status indicator
 * @return false Error
 * @return true Success
 */
bool args2buf(int argc, char *argv[], uint8_t *out, size_t len) {
    size_t i;

    for (i = 0; i < len && i < (size_t)argc; ++i) {
        uint8_t byte;

        if (str2uchar(argv[i], &byte)) {
            lprintf(LOG_ERR, "Bad byte value: %s", argv[i]);
            return false;
        }

        out[i] = byte;
    }
    return true;
}
