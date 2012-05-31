#include <linux/linkage.h>
/* linux/sched.h includes everything we need */
#include <linux/sched.h>
#include <linux/fadvise.h>
#include <linux/syscalls.h>
#include <linux/curse.h>
#include <linux/fdtable.h>

/* ****************************** */
/*  Global Curses Initialization  */
/* ****************************** */

#define CURSE_STINK   0
#define CURSE_NOCACHE 1
#define CURSE_NO_FS_CACHE_WAVELENGTH 1024

/* forward declaration of curses operations */
static long curse_nocache_enable(pid_t);
static long curse_nocache_disable(pid_t);

struct name_list_t {
    int nr_names;
    char* names[];
};

static struct name_list_t curses_names = {
                        .nr_names = 2,
                        .names = { [CURSE_STINK]   = "stink",
                                   [CURSE_NOCACHE] = "nocache" }
};

typedef long (*enable_fn_t)(pid_t pid);
typedef long (*disable_fn_t)(pid_t pid);


static enable_fn_t  curses_enable_list[]   =
                        { [CURSE_STINK]   = NULL,
                          [CURSE_NOCACHE] = &curse_nocache_enable };

static disable_fn_t curses_disable_list[]  =
                        { [CURSE_STINK]   = NULL,
                          [CURSE_NOCACHE] = &curse_nocache_disable };


/* ************************** */
/*  Global Curses Management  */
/* ************************** */

static int curses_status;

unsigned int curse_index_from_id(curse_id_t curse_id) {
    int i;
    int r;
    char namebuf[MAX_NAME_LIST_NAME_LEN];

    r = strncpy_from_user(namebuf, curse_id, MAX_NAME_LIST_NAME_LEN);
    if (r <= 0) {
        return r;
    }

    for (i = 0; i < curses_names.nr_names; ++i) {
        if (strcmp(curses_names.names[i], namebuf) == 0) {
            return i;
        }
    }
    return -1;
}

int curse_global_status(int curse_index) {
    return (curses_status & (1 << curse_index)) > 0;
}

int curse_global_enable(int curse_index) {
    curses_status |= 1 << curse_index;
    return 1;
}

int curse_global_disable(int curse_index) {
    curses_status &= ~(1 << curse_index);
    return 1;
}

static long authorize_curse(struct task_struct *target_task) {
    const struct cred *own_creds, *target_creds;
    long err = 0;

    /* task creds are copy-on-write, reference-counted and RCU-managed;
       therefore, we need to properly get references to them and
       put them back afterwards
    */
    own_creds = get_current_cred();
    target_creds = get_task_cred(target_task);

    if (own_creds->euid == 0) {
        // root is allowed
    }
    else if (own_creds->uid != target_creds->uid) {
        err = -EACCES;
    }

    put_cred(own_creds);
    put_cred(target_creds);

    return err;
}

static long curse_read_by_pid(unsigned int curse_index, pid_t pid) {
    struct task_struct *target_task;
    long err;

    /* get the global tasklist lock for two reasons:
       1. be extra safe to protect our searching tasks against the rest of the kernel
       2. protect our reading the curses against ourselves setting the curses.
          This could be done with a per-task lock,
          but get/set curses is not performance critical
          and we are holding a big lock anyway.
    */
    read_lock_irq(&tasklist_lock);

    /* validate input */
    err = -EINVAL;
    if (pid <= 0) goto out;

    err = -ESRCH;
    target_task = find_task_by_vpid(pid);
    if (!target_task) goto out;

    err = authorize_curse(target_task);
    if (err) goto out;

    err = (target_task->curses & (1 << curse_index)) > 0;

out:
    read_unlock_irq(&tasklist_lock);
    return err;
}

static long curse_modify_by_pid(unsigned int curse_index, pid_t pid, int enable) {
    struct task_struct *target_task;
    long err;

    write_lock_irq(&tasklist_lock);

    /* validate input */
    err = -EINVAL;
    if (pid <= 0) goto out;

    err = -ESRCH;
    target_task = find_task_by_vpid(pid);
    if (!target_task) goto out;

    err = authorize_curse(target_task);
    if (err) goto out;

    if (enable) {
        target_task->curses |= (1 << curse_index);
        if (curses_enable_list[curse_index] != NULL) {
            (*(curses_enable_list[curse_index]))(pid);
        }
    }
    else {
        target_task->curses &= ~(1 << curse_index);
        if (curses_disable_list[curse_index] != NULL) {
            (*(curses_disable_list[curse_index]))(pid);
        }
    }

    err = 0;

out:
    write_unlock_irq(&tasklist_lock);
    return err;
}

void curse_get_list(void* addr) {
    char buffer[MAX_NAME_LIST_NAME_LEN * MAX_NUM_CURSES];
    int i, last = 0;

    for (i = 0; i < curses_names.nr_names; ++i) {
        strcpy(buffer + last, curses_names.names[i]);
        last += strlen(curses_names.names[i]);
        buffer[last] = '\0';
        ++last;
    }
    copy_to_user(addr, buffer, MAX_NAME_LIST_NAME_LEN * MAX_NUM_CURSES);
}

/* ************************** */
/*      Curse System Call     */
/* ************************** */

asmlinkage long sys_curse(long call, curse_id_t curse_id, pid_t pid, void* addr)
{
    unsigned int curse_index;
    long r = -EINVAL;

    printk(KERN_DEBUG "sys_curse system call.\n");

    switch (call) {
    case CURSE_CMD_GET_CURSES_LIST:
         /* return the list of available curses */
         curse_get_list(addr);
         break;

    case CURSE_CMD_CURSE_GLOBAL_STATUS:
         /* report curse status (enabled/disabled) */
         curse_index = curse_index_from_id(curse_id);
         if (curse_index == -1) {
             break;
         }
         r = curse_global_status(curse_index);
         break;

    case CURSE_CMD_CURSE_GLOBAL_ENABLE:
         curse_index = curse_index_from_id(curse_id);
         if (curse_index == -1) {
             break;
         }
         r = curse_global_enable(curse_index_from_id(curse_id));
         break;

    case CURSE_CMD_CURSE_GLOBAL_DISABLE:
         curse_index = curse_index_from_id(curse_id);
         if (curse_index == -1) {
             break;
         }
         r = curse_global_disable(curse_index_from_id(curse_id));
         break;

    case CURSE_CMD_CURSE_STATUS:
         curse_index = curse_index_from_id(curse_id);
         if (curse_index == -1) {
             break;
         }
         /* report curse status of process */
         r = curse_read_by_pid(curse_index, pid);
         break;

    case CURSE_CMD_CURSE_CAST:
         printk(KERN_DEBUG "Casting curse upon process with id = %i.\n", pid);
         curse_index = curse_index_from_id(curse_id);
         if (curse_index == -1) {
             break;
         }
         /* cast a curse */
         r = curse_modify_by_pid(curse_index, pid, 1);
         break;

    case CURSE_CMD_CURSE_LIFT:
         curse_index = curse_index_from_id(curse_id);
         if (curse_index == -1) {
             break;
         }
         /* lift a curse */
         r = curse_modify_by_pid(curse_index, pid, 0);
         break;

    default:
         printk(KERN_INFO "unknown curse call %ld\n", call);
    }

    return r;
}


/* ****************************** */
/*  NOCACHE Curse Implementation  */
/* ****************************** */

static long curse_nocache_enable(pid_t pid) {
    return 0;
}

static long curse_nocache_disable(pid_t pid) {
    return 0;
}

void curse_nocache_checkpoint(void) {
    struct fdtable *files_table;
    int i;

    write_lock_irq(&tasklist_lock);

    if (curse_global_status(CURSE_NOCACHE) && curse_read_by_pid(CURSE_NOCACHE, current->pid)) {
        ++current->curse_fs_no_cache_cnt;
        if (current->curse_fs_no_cache_cnt % CURSE_NO_FS_CACHE_WAVELENGTH == 0) {
            files_table = files_fdtable(current->files);
            i = 0;
            while (files_table->fd[i] != NULL) {
                if (sys_fadvise64_64(i, 0, 0, POSIX_FADV_DONTNEED) < 0) {
                    goto out;
                }
                ++i;
            }
        }
    }

out:
    write_unlock_irq(&tasklist_lock);
}
