/*
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <net/ethernet.h>

#include "libcom.h"
#include "lib_menu.h"
#include "error_define.h"

menu_command_t  reservedCommand_tbl[] = {
    { "ls",   LINUX_CMD_ATTR,     NULL, \
      "       ls", \
      "       list directory contents"},
    { "cd",   LINUX_CMD_ATTR,     NULL, \
      "       cd {dir, ..}", \
      "       change directory"},
    { "man",  LINUX_CMD_ATTR,     NULL, \
      "       man {cmd, dir}", \
      "       an interface to the reference manuals"},
    { "quit", LINUX_CMD_ATTR,     NULL, \
      "       quit", \
      "       exit program"},
    { 0,        LINUX_CMD_ATTR,     NULL, " ", " "}
};

void show_menuitem_in_details(menu_command_t *menu_tbl, int index)
{
    if(menu_tbl[index].name == NULL) return;

    if(menu_tbl[index].attr ==  DIRECTORY_ATTR) 
        printf(ANSI_COLOR_BLUE "[DIR] %-12s\n", menu_tbl[index].name);
    else    
        printf(ANSI_COLOR_RESET "[SUB-CMD] %-12s\n", menu_tbl[index].name);

    printf(ANSI_COLOR_BLUE  "SYNOPSIS\n");
    printf(ANSI_COLOR_RESET "%s\n", menu_tbl[index].usage);
    printf(ANSI_COLOR_BLUE  "DESCRIPTION\n");
    printf(ANSI_COLOR_RESET "%s\n", menu_tbl[index].desc);
}

void show_menu(menu_command_t *menu_tbl)
{
    int i;

    for(i = 0; menu_tbl[i].name != NULL; i++) 
        if(menu_tbl[i].attr ==  DIRECTORY_ATTR) 
            printf(ANSI_COLOR_BLUE "  %-12s\n", menu_tbl[i].name);
    
    for(i = 0; menu_tbl[i].name != NULL; i++) 
        if(menu_tbl[i].attr ==  EXECUTION_ATTR) 
            printf(ANSI_COLOR_RESET "  %-12s\n", menu_tbl[i].name);

    printf(ANSI_COLOR_RESET "\n");
}

void print_invalidParameterWarningMessage(const char *par, const char *msg)
{
    printf(ANSI_COLOR_RED   "Invalid parameter(%s)\n", par);
    printf(ANSI_COLOR_BLUE  "SYNOPSIS\n");
    printf(ANSI_COLOR_RESET "%s\n", msg);
}

void print_missingParameterWarningMessage(const char *msg)
{
    printf(ANSI_COLOR_RED   "Missing (a) Parameter(s)\n");
    printf(ANSI_COLOR_BLUE  "SYNOPSIS\n");
    printf(ANSI_COLOR_RESET "%s\n", msg);
}

int print_argumentWarningMessage(int argc, const char *argv[], menu_command_t *menu_tbl, char echo)
{
    int i;

    for (i = 0; menu_tbl[i].name; i++)
        if (!strcmp(argv[0], menu_tbl[i].name)) {
            printf(ANSI_COLOR_RED   "Invalid Parameter(s)\n");
            printf(ANSI_COLOR_BLUE  "SYNOPSIS\n");
            printf(ANSI_COLOR_RESET "%s\n", menu_tbl[i].usage);
            printf(ANSI_COLOR_BLUE  "DESCRIPTION\n");
            printf(ANSI_COLOR_RESET "%s\n", menu_tbl[i].desc);
            return i;
        }

    return ERR_INVALID_PARAMETER;
}

int lookup_menu_tbl(int argc, const char *argv[], menu_command_t *menu_tbl, char echo)
{
    int i;

    for (i = 0; menu_tbl[i].name; i++)
        if (!strcmp(argv[0], menu_tbl[i].name)) return i;

    if(echo) {
        printf(ANSI_COLOR_RED   "Unknown command '%s'", argv[0]);
        printf(ANSI_COLOR_RESET "\n");
    }
    return ERR_INVALID_PARAMETER;
}

int lookup_dir_tbl(int argc, const char *argv[], menu_command_t *menu_tbl, char echo)
{
    int i;

    if (argv[0] == NULL) {
        printf(ANSI_COLOR_GREEN "'cd' command needs a directory name !");
        printf(ANSI_COLOR_RESET "\n");
        return ERR_PARAMETER_MISSED;
    }

    for (i = 0; menu_tbl[i].name; i++)
        if(menu_tbl[i].attr ==  DIRECTORY_ATTR) 
            if (!strcmp(argv[0], menu_tbl[i].name)) return i;

    if (!strcmp(argv[0], "..")) return 1000;

    for (i = 0; menu_tbl[i].name; i++)
        if(menu_tbl[i].attr ==  EXECUTION_ATTR) 
            if (!strcmp(argv[0], menu_tbl[i].name)) {
                if(echo) {
                    printf(ANSI_COLOR_GREEN "'%s' is a command !", argv[0]);
                    printf(ANSI_COLOR_RESET "\n");
                }
                return ERR_INVALID_PARAMETER;
            }

    if(echo) {
        printf(ANSI_COLOR_GREEN "There is no '%s' directory !",argv[0]);
        printf(ANSI_COLOR_RESET "\n");
    }
    return ERR_INVALID_PARAMETER;
}

int lookup_cmd_tbl(int argc, const char *argv[], menu_command_t *menu_tbl, char echo)
{
    const char  **pav = NULL;
    int         ac, i;

    if (argv[0] == NULL) return ERR_PARAMETER_MISSED;

    pav = argv;
    ac  = argc;

    for (i = 0; menu_tbl[i].name; i++)
        if((menu_tbl[i].attr ==  EXECUTION_ATTR) && (!strcmp(argv[0], menu_tbl[i].name))) 
            if(menu_tbl[i].fP != NULL) return menu_tbl[i].fP(ac, pav, menu_tbl);
            

    for (i = 0; menu_tbl[i].name; i++)
        if((menu_tbl[i].attr ==  DIRECTORY_ATTR) && (!strcmp(argv[0], menu_tbl[i].name))) {
            if(echo) {
                printf(ANSI_COLOR_GREEN "'%s' is a directory !", argv[0]);
                printf(ANSI_COLOR_RESET "\n");
            }
            return ERR_INVALID_PARAMETER;
        }

    if(echo) {
        printf(ANSI_COLOR_RED   "'%s' is an unknown parameter !",argv[0]);
        printf(ANSI_COLOR_RESET "\n");
        printf("\nThere are some available parameters:\n");
        show_menu(menu_tbl);
        printf("If you want to know how to use each parameter in detail, type -h at the end.\n\n");
    }
    return ERR_INVALID_PARAMETER;
}

/********************* reserved command *********************/
void process_lsCmd(menu_command_t *menu_tbl, char *title)
{
    printf(ANSI_COLOR_BLUE  "\n%s", title);
    printf(ANSI_COLOR_RESET "\n");
    show_menu(menu_tbl);
}

int process_cdCmd(int argc, const char *argv[], menu_command_t *menu_tbl, char echo)
{
    int     cmd_idx;

    if ((cmd_idx = lookup_dir_tbl(argc, argv, menu_tbl, echo)) >= 0 ) {
        if(cmd_idx == 1000) return 1;
        if(menu_tbl[cmd_idx].fP != NULL) {
            menu_tbl[cmd_idx].fP(argc, argv, menu_tbl);
        }
        else {
            printf(ANSI_COLOR_RED   "%dth function is NULL !", cmd_idx);
            printf(ANSI_COLOR_RESET "\n");
        }
    }
    return 0;
}

int process_manCmd(int argc, const char *argv[], menu_command_t *menu_tbl, char echo)
{
    int i;

    if (argv[0] == NULL) {
        printf(ANSI_COLOR_GREEN "'man' command needs an argument !");
        printf(ANSI_COLOR_RESET "\n");
        return ERR_PARAMETER_MISSED;
    }

    for (i = 0; menu_tbl[i].name; i++)
        if (!strcmp(argv[0], menu_tbl[i].name)) { 
            show_menuitem_in_details(menu_tbl, i);
            return 0;
        }

    if(echo) {
        printf(ANSI_COLOR_GREEN "No manual entry for %s",argv[0]);
        printf(ANSI_COLOR_RESET "\n");
    }
    return ERR_INVALID_PARAMETER;
}

int process_reservedCmd(int cmd_idx, int argc, const char *argv[], menu_command_t *menu_tbl, char echo, char *title)
{
    switch(cmd_idx) {
        case 0 : process_lsCmd(menu_tbl, title); break;
        case 1 : return process_cdCmd(argc, argv, menu_tbl, ECHO); 
        case 2 : process_manCmd(argc, argv, menu_tbl, ECHO); break;
        case 3 :    // quit
            return QUIT_CMD_PROCESS;
        default:
            printf(ANSI_COLOR_GREEN "Unknown reserved command index: %d", cmd_idx);
            printf(ANSI_COLOR_RESET "\n");
            break;
    }
    return 0;
}
