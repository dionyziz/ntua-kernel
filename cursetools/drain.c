#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#define BUFLEN 4096
#define THRESH 8*1024*1024

int main(int argc, char **argv) {
    char *buf;
    int err;
    unsigned long counter = 0;

    if (argc > 1) {
        printf("Usage: ./drain  < infile  > outfile\n"
               "Simple pipe from standard input to standard output.\n"
               "Uses fadvise to suppress caching (needs files, won't work with pipes).\n");
        return -1;
    }

    buf = malloc(BUFLEN);
    if (!buf) {
        perror("malloc");
        return -1;
    }

    for (;;) {
         int b;

         err = read(0, buf, BUFLEN);
         if (err == 0) break;
         if (err < 0) {
             perror("read");
             return err;
         }

         b = err;
         counter += b;

         while (b > 0) {
             err = write(1, buf, b);
             if (err < 0) {
                 perror("write");
                 return err;
             }
             b -= err;
         }

         if (counter > THRESH) {
             counter = 0;
             posix_fadvise(0, 0, 0, POSIX_FADV_DONTNEED);
             posix_fadvise(1, 0, 0, POSIX_FADV_DONTNEED);
             /* Ignore errors (from pipes, etc.) */
         } 
    }

    return 0;
}
