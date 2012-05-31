#ifndef _CURSE_H
#define _CURSE_H

/* define the sys_curse functions */

#define CURSE_CMD_GET_CURSES_LIST            1
#define CURSE_CMD_CURSE_GLOBAL_STATUS        2
#define CURSE_CMD_CURSE_GLOBAL_ENABLE        3
#define CURSE_CMD_CURSE_GLOBAL_DISABLE       4
#define CURSE_CMD_CURSE_STATUS               5
#define CURSE_CMD_CURSE_CAST                 6
#define CURSE_CMD_CURSE_LIFT                 7

#define MAX_NAME_LIST_NAME_LEN              32
#define MAX_NUM_CURSES                      32

#ifdef __KERNEL__
/* this section is needed only when including from kernel source */

#include <linux/types.h>

int curse_global_status(int curse_id);
int curse_global_enable(int curse_id);
int curse_global_disable(int curse_id);

void curse_nocache_checkpoint(void);

/* this checkpoint is to be inserted into system calls */
// void curse_nocache_checkpoint(/* arguments */);
#endif

#endif

