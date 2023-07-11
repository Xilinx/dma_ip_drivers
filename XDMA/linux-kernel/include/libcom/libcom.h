#ifndef __LIBCOM_H__
#define __LIBCOM_H__

#include<stdio.h>
#include <sys/types.h>

#define IS_NOT_FOLDER    (-2)
#define NOT_EXIST_FOLDER (-1)
#define EXIST_FOLDER     (0)

#define IS_NOT_FILE    (-2)
#define NOT_EXIST_FILE (-1)
#define EXIST_FILE     (0)    

char *rl_gets(char *prompt, char *out_char);
int  poptDupArgv(int argc, const char **argv, int * argcPtr, const char *** argvPtr);
int  poptParseArgvString(const char * s, int * argcPtr, const char *** argvPtr);
int  IsDecimal(char *param);
int  IsHexDecimal(char *param);
int  CharToHex(char c);
char *CutStringByDelim(char **str, const char delim);
char *SkipWhite(char *ptr);
char *SkipSpace(const char *p);
int  checkIsPrint(const u_char c, int flags);
int  StrToDecimal(char *str, int *n);
int  StrToHex(char *str, int *n);
int  HexStrToCharArray(u_char *data, char *str, int *size);
int  ArgumentsToHex(int argc, const char *argv[], int *v);
int  ArgumentsToDecimal(int argc, const char *argv[], int *v);
void FileClose(FILE **fp);
int  CheckFileExists(const char * filename);
int  get_args(char* cmdl, char** args);
void DataDump(unsigned char *data, int len);
void fillPrintBufferPartially(char *Line, const u_char *pPkt, int pktLen);
void dump_packetWithWiresharkForm(int pktLen, const u_char* packet);
void show_arguments_list(int argc, const char *argv[]);
void show_function_arguments(const char *fn, int argc, const char *argv[]);

unsigned int   cksumUpdate(void *pBuf, int32_t size, u_int cksum);
unsigned short cksumDone(u_int cksum);
unsigned short cksum(void *pBuf, int32_t size, u_int cksum);

int get_ltime_string(char *str);
int get_ltime_string_with_time(time_t t, char *str);

int exist_dir (const char *d);
void make_directory(const char *folder);

#endif // __LIBCOM_H__
