#ifndef LIBCURSE_H
#define LIBCURSE_H

#include <unistd.h>
#include <linux/unistd.h>
#include <linux/curse.h>

typedef char* curse_id_t;

struct curse_list_t;

struct curse_list_t {
    curse_id_t name;
    struct curse_list_t* next;
};

long curse(long call, curse_id_t curse_id, pid_t pid, void* addr);

long curse_global_status(curse_id_t curse);
long curse_enable(curse_id_t curse);
long curse_disable(curse_id_t curse);
long curse_status(curse_id_t curse, pid_t pid);
long curse_cast(curse_id_t curse, pid_t pid);
long curse_lift(curse_id_t curse, pid_t pid);
struct curse_list_t *curse_get_list(void);
int curse_print_list(struct curse_list_t *curse_list, char *separator);

#endif
