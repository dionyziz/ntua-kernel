#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "libcurse.h"

int help(void) {
    printf(
       "Usage: curse <global action>\n"
       "Usage: curse <cursename> <action> [<option>]\n"
       "\n"
       " Global Actions                   Description\n"
       "----------------        ---------------------\n"
       "     help               show this help\n"
       "     list               list curses\n"
       "\n"
       " Curse Actions                    Description\n"
       "----------------        ---------------------\n"
       "       ?                show curse global status (enabled/disabled)\n"
       "       +                enable curse <cursename>\n"
       "       -                disable curse <cursename>\n"
       "\n"
       "     <pid>+             curse <pid> with <cursename>\n"
       "     <pid>-             lift curse <cursename> from pid\n"
       "     <pid>?             show <cursename> status for pid\n"
       "\n"
       "   Examples                        Description\n"
       "----------------        ---------------------\n"
       "curse list              show a list of available curses\n"
       "curse nocache ?         show if curse 'nocache' is enabled \n"
       "curse nocache +         enable curse 'nocache' \n"
       "curse nocache 7423?     show if process 7423 is cursed with 'nocache' \n"
       "curse nocache 7423+     curse process 7423 with 'nocache' \n"
       "\n"
    );
    exit(-1);
    return -1;
}

int main(int argc, char **argv) {
    pid_t pid;
    char action;
    int status;
    struct curse_list_t *node;
    char tmp[10];

    if (argc == 2) {
        if (strcmp(argv[1], "list") == 0) {
            printf("Available curses:\n");
            node = curse_get_list();
            curse_print_list(node, ", ");
            printf("\n");
            return 0;
        }
    }
    else if (argc == 3) {
        switch (argv[2][0]) {
            case '?':
                printf("Curse '%s' is globally ", argv[1]);
                if (curse_global_status(argv[1])) {
                    printf("enabled.\n");
                }
                else {
                    printf("disabled.\n");
                }
                return 0;
            case '+':
                printf("Globally enabling curse '%s'.\n", argv[1]);
                curse_enable(argv[1]);
                return 0;
            case '-':
                printf("Globally disabling curse '%s'.\n", argv[1]);
                curse_disable(argv[1]);
                return 0;
            default:
                sscanf(argv[2], "%i", &pid);
                if (pid > 0) {
                    sprintf(tmp, "%i", pid);
                    argv[2] += strlen(tmp);
                    sscanf(argv[2], "%c", &action);
                    switch (action) {
                        case '?':
                            printf("Curse '%s' for process %i is ", argv[1], pid);
                            status = curse_status(argv[1], pid);
                            if (status) {
                                printf("enabled.\n");
                            }
                            else {
                                printf("disabled.\n");
                            }
                            return 0;
                        case '+':
                            printf("Enabling curse '%s' for process %i.\n", argv[1], pid);
                            curse_cast(argv[1], pid);
                            return 0;
                        case '-':
                            printf("Disabling curse '%s' for process %i.\n", argv[1], pid);
                            curse_lift(argv[1], pid);
                            return 0;
                        default:
                            printf("Action was: '%c'\n", action);
                    }
                }
        }
    }
    return help();
}
