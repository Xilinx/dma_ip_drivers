/*****************************************************************************
******************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

#define POPT_ARGV_ARRAY_GROW_DELTA 5
#define POPT_ERROR_NOARG    -10
#define POPT_ERROR_MALLOC   -21
#define POPT_ERROR_BADQUOTE -15

#include <readline/readline.h>
#include <readline/history.h>

#include <libcom.h>

char *rl_gets(char *prompt, char *out_char) {
    char *line_read = NULL;

    line_read = readline(prompt);

    if (line_read && *line_read) {
        add_history(line_read);
    }

    if (line_read != NULL) {
        strcpy(out_char, line_read);
    } else {
        out_char[0] = EOF;
        out_char[1] = '\0';
    }

    if (line_read) {
        free(line_read);
    }

    return out_char;
}

int poptDupArgv(int argc, const char **argv,
                int * argcPtr, const char *** argvPtr) {
    size_t nb = (argc + 1) * sizeof(*argv);
    const char ** argv2;
    char * dst;
    int i;

    if (argc <= 0 || argv == NULL) {      /* XXX can't happen */
        return POPT_ERROR_NOARG;
    }
    for (i = 0; i < argc; i++) {
        if (argv[i] == NULL) {
            return POPT_ERROR_NOARG;
        }
        nb += strlen(argv[i]) + 1;
    }

    dst = malloc(nb);
    if (dst == NULL) {                    /* XXX can't happen */
        return POPT_ERROR_MALLOC;
    }
    argv2 = (void *) dst;
    dst += (argc + 1) * sizeof(*argv);

    /*@-branchstate@*/
    for (i = 0; i < argc; i++) {
        argv2[i] = dst;
        dst += strlen(strcpy(dst, argv[i])) + 1;
    }
    /*@=branchstate@*/
    argv2[argc] = NULL;

    if (argvPtr) {
        *argvPtr = argv2;
    } else {
        free(argv2);
        argv2 = NULL;
    }
    if (argcPtr) {
        *argcPtr = argc;
    }
    return 0;
}

int poptParseArgvString(const char * s, int * argcPtr, const char *** argvPtr) {
    const char * src;
    char quote = '\0';
    int argvAlloced = POPT_ARGV_ARRAY_GROW_DELTA;
    const char ** argv = malloc(sizeof(*argv) * argvAlloced);
    int argc = 0;
    int buflen = strlen(s) + 1;
    char * buf = memset(alloca(buflen), 0, buflen);
    int rc = POPT_ERROR_MALLOC;

    if (argv == NULL) {
        return rc;
    }
    argv[argc] = buf;

    for (src = s; *src != '\0'; src++) {
        if (quote == *src) {
            quote = '\0';
        } else if (quote != '\0') {
            if (*src == '\\') {
                src++;
                if (!*src) {
                    rc = POPT_ERROR_BADQUOTE;
                    goto exit;
                }
                if (*src != quote) {
                    *buf++ = '\\';
                }
            }
            *buf++ = *src;
        } else if (isspace(*src)) {
            if (*argv[argc] != '\0') {
                buf++, argc++;
                if (argc == argvAlloced) {
                    argvAlloced += POPT_ARGV_ARRAY_GROW_DELTA;
                    argv = realloc(argv, sizeof(*argv) * argvAlloced);
                    if (argv == NULL) {
                        goto exit;
                    }
                }
                argv[argc] = buf;
            }
        } else {
            switch (*src) {
            case '"':
            case '\'':
                quote = *src;
                /*@switchbreak@*/
                break;
            case '\\':
                src++;
                if (!*src) {
                    rc = POPT_ERROR_BADQUOTE;
                    goto exit;
                }
                /*@fallthrough@*/
            default:
                *buf++ = *src;
                /*@switchbreak@*/
                break;
            }
        }
    }

    if (strlen(argv[argc])) {
        argc++, buf++;
    }

    rc = poptDupArgv(argc, argv, argcPtr, argvPtr);

exit:
    if (argv) {
        free(argv);
    }
    return rc;
}

int IsDecimal(char *param) {
    int    i;
    int len;

    len = strlen(param);

    for (i = 0; i < len; i++) {
        if (!isdigit(param[i])) {
            return -1;
        }
    }

    return 0;
}

int IsHexDecimal(char *param) {
    int    i;
    int len;

    len = strlen(param);

    for (i = 0; i < len; i++) {
        if (!isxdigit(param[i])) {
            return -1;
        }
    }

    return 0;
}

int CharToHex(char c) {
    if ((c >= '0') && (c <= '9')) {
        return (c - 48);
    } else if ((c >= 'A') && (c <= 'F')) {
        return (c - 55);
    } else if ((c >= 'a') && (c <= 'f')) {
        return (c - 87);
    } else {
        return -1;
    }
}

char *CutStringByDelim(char **str, const char delim) {
    char *ptr;

    if (*str == NULL) {
        return NULL;
    }

    //ignore space and delim that exist at string's ahead
    //while(*(*str) != 0 && (isspace(*(*str)) || (*(*str) == (delim)))) {
    while (*(*str) != 0 && (*(*str) == (delim))) {
        (*str)++;
    }

    if (*(*str) == '\0') {
        return NULL;
    }

    ptr = *str;

    // exchange delim with '\0'
    while (1) {
        if (*(*str) == delim) {
            *(*str) = '\0';
            (*str)++;
            break;
        } else if (*(*str) == '\0') {
            break;
        } else {
            (*str)++;
        }
    }

    //remove space that exist at string's back
#if 1
    if (delim != ' ') {
        int i;
        i = strlen(ptr) - 2;
        //while(ptr[i] != '\0' && ptr[i] == ' ') {
        while (ptr[i] != '\0' && isspace(ptr[i])) {
            ptr[i] = '\0';
            i--;
        }
    }
#endif

    return ptr;
}

char *SkipWhite(char *ptr) {
    int i;

    if (ptr == NULL) {
        return (NULL);
    }

    while (*ptr != 0 && isspace(*ptr)) {
        ptr++;
    }

    if (*ptr == 0 || *ptr == '#') {
        return (NULL);
    }

    i = strlen(ptr) - 2;
    while (ptr[i] != 0 && isspace(ptr[i])) {
        ptr[i] = '\0';
        i--;
    }

    return (ptr);
}

char *SkipSpace(const char *p)
{
    while (isspace((unsigned char)*p)) {
        p++;
    }
    return (char *)p;
}

int  checkIsPrint(const u_char c, int flags)
{
    if(flags) {
        if ((c >= 32 && c <= 126) || c == 10 || c == 11 || c == 13) {
            return 1;
        } else {
            return 0;
        }
    } else {
        if ((c >= 32 && c <= 126)) {
            return 1;
        } else {
            return 0;
        }
    }
}

int StrToDecimal(char *str, int *n) {
    int    i;
    int len;
    int v = 0;

    len = strlen(str);

    for (i = 0; i < len; i++) {
        if (!isdigit(str[i])) {
            return -1;
        }
        v = v * 10 + CharToHex(str[i]);
    }

    *n = v;

    return 0;
}

int StrToHex(char *str, int *n) {
    int    i;
    int len;
    int v = 0;

    len = strlen(str);

    for (i = 0; i < len; i++) {
        if (!isxdigit(str[i])) {
            return -1;
        }
        v = v * 16 + CharToHex(str[i]);
    }

    *n = v;

    return 0;
}

int HexStrToCharArray(u_char *data, char *str, int *size) {
    int     i = 0, len;

    if((IsHexDecimal(str) < 0) || (data == NULL)) {
        return -1;
    }

    len = strlen(str);

    if(len % 2) {
        i = 1, *size = (len / 2) + 1;
        data[0] = CharToHex(str[0]);
    } else {
        *size = len / 2;
    }

    data[*size] = 0;
    
    for( ; i<len; i += 2) {
        data[(i+1)/2] = (CharToHex(str[i]) << 4) + CharToHex(str[i+1]);
    }

    return 0;
}

int ArgumentsToHex(int argc, const char *argv[], int *v) {       
    const char *value;

    if(argc < 2) return -1;

    if((strcmp(argv[1], "=") == 0) || (strcmp(argv[1], ":") == 0)) {
        if(argc < 3) return -1;
        value = argv[2];
    } else value = argv[1];

    if(IsHexDecimal((char *)value) < 0) return -1;

    sscanf(value, "%x", v);

    return 0;
}

int ArgumentsToDecimal(int argc, const char *argv[], int *v)
{       
    const char *value;

    if(argc < 2) return -1;

    if((strcmp(argv[1], "=") == 0) || (strcmp(argv[1], ":") == 0)) {
        if(argc < 3) return -1;
        value = argv[2];
    } else value = argv[1];

    if(IsDecimal((char *)value) < 0) return -1;

    *v = atoi(value);

    return 0;
}

void FileClose(FILE **fp)
{
    if(*fp != NULL) fclose(*fp);
    *fp = NULL;
}

int CheckFileExists(const char * filename)
{
    FILE *fp;

    fp = fopen(filename, "r");
    if (fp != NULL) {
        FileClose(&fp);
        return 1;
    }
    return 0;
}

int get_args(char* cmdl, char** args)
{
    int i;
    for (i = 0; (args[i] = strtok(NULL, "\n\t, ")); i++)
        ;
    return i;
}

void DataDump(unsigned char *data, int len)
{
    
    int i;
    for(i=0; i<len; i++)
    {
        if((i % 16) == 0 )
            printf("\n[%04x] ", i);
        
        printf("%02x ", data[i] & 0xff);
    }
    printf("\n");
}

#define IHL_WORD_SIZE       4    // Internet Header Length
#define FULL_SIZE_OF_LINE   16
#define HALF_SIZE_OF_LINE   8
#define BUFFER_SIZE_OF_LINE 76

void fillPrintBufferPartially(char *Line, const u_char *pPkt, int pktLen) 
{
    int i;

    for(i=0; i<pktLen; i++) {
        sprintf(Line, "%s%02x ", Line, pPkt[i] & 0xFF);
        if(i == (HALF_SIZE_OF_LINE-1)) sprintf(Line, "%s ", Line);
    }

    if (pktLen < HALF_SIZE_OF_LINE) sprintf(Line, "%s ", Line);

    if (pktLen < FULL_SIZE_OF_LINE) {
        for (i = 0; i < (FULL_SIZE_OF_LINE - pktLen); i++)
            sprintf(Line, "%s   ", Line);
    }
    sprintf(Line, "%s   ", Line);
}

void dump_packetWithWiresharkForm(int pktLen, const u_char* packet)
{
    const u_char*   pPkt;
    int             idx = 0;
    char            Line[BUFFER_SIZE_OF_LINE];

    pPkt    = packet;

    while(pktLen > 0) {
        memset(Line, 0, BUFFER_SIZE_OF_LINE);
        sprintf(Line, "%04x  ", idx);
        if(pktLen >= FULL_SIZE_OF_LINE) {
            fillPrintBufferPartially(Line, pPkt, FULL_SIZE_OF_LINE);
            pPkt += FULL_SIZE_OF_LINE;
        }
        else
            fillPrintBufferPartially(Line, pPkt, pktLen);

        printf("%s\n", Line);
        pktLen  -= FULL_SIZE_OF_LINE;
        idx++;
    }
    printf("\n");
}

void show_arguments_list(int argc, const char *argv[])
{
    int i;
    for (i = 0; i<argc; i++) printf("%s ", argv[i]);
}

void show_function_arguments(const char *fn, int argc, const char *argv[])
{
    printf("    %s( ", fn);
    show_arguments_list(argc, argv);
    printf(")\n");
}

u_int cksumUpdate(void *pBuf, int32_t size, u_int cksum)
{
    u_int nWords;
    u_short *pWd = (u_short *)pBuf;

    for (nWords = (size >> 5); nWords > 0; nWords--) {
        cksum += *pWd++; cksum += *pWd++; cksum += *pWd++;
        cksum += *pWd++;
        cksum += *pWd++; cksum += *pWd++; cksum += *pWd++;
        cksum += *pWd++;
        cksum += *pWd++; cksum += *pWd++; cksum += *pWd++;
        cksum += *pWd++;
        cksum += *pWd++; cksum += *pWd++; cksum += *pWd++;
        cksum += *pWd++;
    }

    /* handle the odd number size */
    for (nWords = (size & 0x1f) >> 1; nWords > 0; nWords--)
        cksum   += *pWd++;

    /* Handle the odd byte length */
    if (size & 1)
        cksum   += *pWd & htons(0xFF00);

    return cksum;
}

u_short cksumDone(u_int cksum)
{
    /* Fold at most twice */
    cksum = (cksum & 0xFFFF) + (cksum >> 16);
    cksum = (cksum & 0xFFFF) + (cksum >> 16);

    return ~((u_short)cksum);
}

u_short cksum(void *pBuf, int32_t size, u_int cksum)
{           
    return cksumDone(cksumUpdate(pBuf, size, cksum) );
}

int    get_ltime_string(char *str)
{
    time_t         t;
    struct tm tm;

    if(str == NULL) return -1;

    t = time(NULL);
    tm = *localtime(&t);
    sprintf(str, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, 
                tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    return 0;
}

int    get_ltime_string_with_time(time_t t, char *str)
{
    struct tm tm;

    if(str == NULL) return -1;

    tm = *localtime(&t);
    sprintf(str, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, 
                tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    return 0;
}

int exist_dir (const char *d)
{
    DIR *dirptr;

    if (access ( d, F_OK ) != -1 ) {
        // file exists
        if ((dirptr = opendir (d)) != NULL) closedir (dirptr);
        else return IS_NOT_FOLDER; /* d exists, but not dir */
    } 
    else return NOT_EXIST_FOLDER;     /* d does not exist */

    return EXIST_FOLDER;
}

int exist_file (const char *d)
{
    DIR *dirptr;

    if (access ( d, F_OK ) != -1 ) {
        // file exists
        if ((dirptr = opendir (d)) != NULL) {
            closedir (dirptr);
            return IS_NOT_FILE;
        }
    } 
    else return NOT_EXIST_FILE;     /* d does not exist */

    return EXIST_FILE;
}

void make_directory(const char *folder)
{ 
    int ret;
    char buf[1024];

    memset(buf, 0, 1024);
    ret = exist_dir(folder); 
    if(ret == IS_NOT_FOLDER) {
        sprintf(buf, "rm %s", folder);
        ret = system(buf);
        sprintf(buf, "mkdir %s", folder);
        ret = system(buf);
    }
    else if(ret == NOT_EXIST_FOLDER) {
        sprintf(buf, "mkdir %s", folder);
        ret = system(buf);
    }
}

