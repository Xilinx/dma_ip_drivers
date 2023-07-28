/*
 *
 */

#ifndef __LIB_MENU_H__
#define __LIB_MENU_H__

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define DIRECTORY_ATTR 0
#define EXECUTION_ATTR 1
#define LINUX_CMD_ATTR 2

#define COMMAND_LINE_LENGTH  65536
#define NO_ECHO              0
#define ECHO                 1

#define CMD_ARGS_MAXNUM  100
#define ARGV_TRACEABLE   1

#define QUIT_CMD_PROCESS 100

struct menu_command *command_tbl;
typedef int (*cbFn)(int argc, const char *argv[], struct menu_command *menu_tbl);
typedef int (*argFn)(int32_t argc, const char *argv[]);

typedef struct argument_list {
    char *name;
    argFn fP;
} argument_list_t;

typedef struct menu_command {
    char *name;
    char attr;    // DIRECTORY_ATTR  0 EXECUTION_ATTR  1
                  // LINUX_CMD_ATTR  2
    cbFn fP;
    char *usage;
    char *desc;
} menu_command_t;

extern menu_command_t  reservedCommand_tbl[];

void show_menuitem_in_details(menu_command_t *menu_tbl, int index);
void show_menu(menu_command_t *menu_tbl);

void print_invalidParameterWarningMessage(const char *par, const char *msg);
void print_missingParameterWarningMessage(const char *msg);
int  print_argumentWarningMessage(int argc, const char *argv[], 
                                  menu_command_t *menu_tbl, char echo);
int  lookup_menu_tbl(int argc, const char *argv[], menu_command_t *menu_tbl, char echo);
int  lookup_dir_tbl(int argc, const char *argv[], menu_command_t *menu_tbl, char echo);
int  lookup_cmd_tbl(int argc, const char *argv[], menu_command_t *menu_tbl, char echo);

void process_lsCmd(menu_command_t *menu_tbl, char *title);
int  process_cdCmd(int argc, const char *argv[], menu_command_t *menu_tbl, char echo);
int  process_manCmd(int argc, const char *argv[], menu_command_t *menu_tbl, char echo);
int  process_reservedCmd(int cmd_idx, int argc, const char *argv[], 
                         menu_command_t *menu_tbl, char echo, char *title);

#endif // __LIB_MENU_H__
