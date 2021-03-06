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

#define CURSE_STINK    0
#define CURSE_NOCACHE  1
#define CURSE_RECKLESSNESS 2
#define CURSE_NO_FS_CACHE_WAVELENGTH 1024

/* forward declaration of curses operations */
static long curse_nocache_enable(pid_t);
static long curse_nocache_disable(pid_t);

struct name_list_t {
    int nr_names;
    char* names[];
};

static struct name_list_t curses_names = {
                        .nr_names = 3,
                        .names = { [CURSE_STINK]   = "stink",
                                   [CURSE_NOCACHE] = "nocache",
                                   [CURSE_RECKLESSNESS] = "recklessness" }
};

typedef long (*enable_fn_t)(pid_t pid);
typedef long (*disable_fn_t)(pid_t pid);


static enable_fn_t  curses_enable_list[]   =
                        { [CURSE_STINK]   = NULL,
                          [CURSE_NOCACHE] = &curse_nocache_enable,
                          [CURSE_RECKLESSNESS] = NULL };

static disable_fn_t curses_disable_list[]  =
                        { [CURSE_STINK]   = NULL,
                          [CURSE_NOCACHE] = &curse_nocache_disable,
                          [CURSE_RECKLESSNESS] = NULL };


/* ************************** */
/*  Global Curses Management  */
/* ************************** */

static int curses_status = 0xffffffff;

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
    const struct cred *own_creds = get_current_cred();

    if (own_creds->euid == 0) {
        curses_status |= 1 << curse_index;
        return 0;
    }
    printk(KERN_DEBUG "curse_global_enable permission denied.\n");
    return -EACCES;
}

int curse_global_disable(int curse_index) {
    const struct cred *own_creds = get_current_cred();

    if (own_creds->euid == 0) {
        curses_status &= ~(1 << curse_index);
        return 0;
    }
    printk(KERN_DEBUG "curse_global_disable permission denied.\n");
    return -EACCES;
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
        if (curse_global_status(curse_index) == 0) {
            err = -EINVAL;
        }
        else {
            target_task->curses |= (1 << curse_index);
            if (curses_enable_list[curse_index] != NULL) {
                (*(curses_enable_list[curse_index]))(pid);
            }
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

int curse_get_list(void* __user addr) {
    int SIZE = (MAX_NAME_LIST_NAME_LEN + 1) * MAX_NUM_CURSES + 1;
    char* buffer = (char*)kmalloc(SIZE);
    int i, last = 0;

    for (i = 0; i < curses_names.nr_names; ++i) {
        strcpy(buffer + last, curses_names.names[i]);
        last += strlen(curses_names.names[i]);
        buffer[last] = '\0';
        ++last;
    }
    buffer[last] = '\0';
    if (copy_to_user(addr, buffer, SIZE)) {
        return -EFAULT;
    }
    return 0;
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
         r = curse_get_list(addr);
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

int curse_nocache_vanish(pid_t pid) {
    int i, err;
    struct fdtable *files_table;

    files_table = files_fdtable(current->files);
    i = 0;
    while (files_table->fd[i] != NULL) {
        err = sys_fadvise64_64(i, 0, 0, POSIX_FADV_DONTNEED);
        if (err < 0) {
            return err;
        }
        ++i;
    }
    return 0;
}

static long curse_nocache_enable(pid_t pid) {
    write_lock_irq(&tasklist_lock);

    printk(KERN_INFO "curse_nocache_enable\n");

    current->curse_fs_no_cache_cnt = 0;

    curse_nocache_vanish(pid);

    write_unlock_irq(&tasklist_lock);

    return 0;
}

static long curse_nocache_disable(pid_t pid) {
    printk(KERN_INFO "curse_nocache_disable\n");
    return 0;
}

void curse_nocache_checkpoint(int amount) {
    if (curse_global_status(CURSE_NOCACHE) && curse_read_by_pid(CURSE_NOCACHE, current->pid)) {
        write_lock_irq(&tasklist_lock);
        current->curse_fs_no_cache_cnt += amount;

        if (amount == 0 || current->curse_fs_no_cache_cnt > CURSE_NO_FS_CACHE_WAVELENGTH) {
            current->curse_fs_no_cache_cnt = 0;
            write_unlock_irq(&tasklist_lock);
            if (curse_nocache_vanish() < 0) {
                // invalidating data in RAM failed
            }
            // printk(KERN_INFO "curse_nocache_checkpoint invalidating data from RAM\n");
        }
        else {
            write_unlock_irq(&tasklist_lock);
        }
    }
}
