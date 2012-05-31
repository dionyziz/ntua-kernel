#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

#define CURSE_CMD_GET_CURSES_LIST            1
#define CURSE_CMD_CURSE_GLOBAL_STATUS        2
#define CURSE_CMD_CURSE_GLOBAL_ENABLE        3
#define CURSE_CMD_CURSE_GLOBAL_DISABLE       4
#define CURSE_CMD_CURSE_STATUS               5
#define CURSE_CMD_CURSE_CAST                 6
#define CURSE_CMD_CURSE_LIFT                 7

#include "libcurse.h"

long curse(long call, curse_id_t curse_id, pid_t pid, void* addr) {
    return syscall(__NR_curse, call, curse_id, pid, addr);
}

long curse_global_status(curse_id_t curse_id) {
    return curse(CURSE_CMD_CURSE_GLOBAL_STATUS, curse_id, 0, NULL);
}

long curse_enable(curse_id_t curse_id) {
    return curse(CURSE_CMD_CURSE_GLOBAL_ENABLE, curse_id, 0, NULL);
}

long curse_disable(curse_id_t curse_id) {
    return curse(CURSE_CMD_CURSE_GLOBAL_DISABLE, curse_id, 0, NULL);
}

long curse_status(curse_id_t curse_id, pid_t pid) {
    return curse(CURSE_CMD_CURSE_STATUS, curse_id, pid, NULL);
}

long curse_cast(curse_id_t curse_id, pid_t pid) {
    return curse(CURSE_CMD_CURSE_CAST, curse_id, pid, NULL);
}

long curse_lift(curse_id_t curse_id, pid_t pid) {
    return curse(CURSE_CMD_CURSE_LIFT, curse_id, pid, NULL);
}

struct curse_list_t *curse_get_list(void) {
    struct curse_list_t *curse_list = (struct curse_list_t*)malloc(sizeof(struct curse_list_t));
    struct curse_list_t *next = curse_list, *tmp;
    int BUFFER_LEN = MAX_NAME_LIST_NAME_LEN * MAX_NUM_CURSES;
    char buffer[BUFFER_LEN];
    int i, previ = 0;

    curse(CURSE_CMD_GET_CURSES_LIST, "", 0, buffer);
    curse_list->next = NULL;

    for (i = 0; i < BUFFER_LEN; ++i) {
        if (buffer[i] == '\0') {
            if (i == previ) {
                // end of curse list
                break;
            }
            next->name = malloc(MAX_NAME_LIST_NAME_LEN * sizeof(char));
            strcpy(next->name, buffer + previ);
            previ = i + 1;
            tmp = (struct curse_list_t*)malloc(sizeof(struct curse_list_t));
            next->next = tmp;
            tmp->name = "";
            tmp->next = NULL;
            next = tmp;
        }
    }

    return curse_list;
}

int curse_print_list(struct curse_list_t *curse_list, char *separator) {
    int cnt = 0, i;
    struct curse_list_t *node;

    node = curse_list;
    while (node->next != NULL) {
        node = node->next;
        ++cnt;
    }

    node = curse_list;
    for (i = 0; i < cnt; ++i) {
        printf("%s", node->name);
        if (i != cnt - 1) {
            printf("%s", separator);
        }
        node = node->next;
    }
    return 0;
}

