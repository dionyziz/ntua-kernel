#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define FILENAME "./.test--_fio_torture.tmp"
#define BUFSIZE 64

double gettime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

int help(void) {
    printf("Usage: ./fio-torture <loops> [<pattern> (default: owwwsrrrc)]\n");
    printf("  Symbol        Description\n"
           "     o           open file\n"
           "     c           close file\n"
           "     r           read from file\n"
           "     w           write to file\n"
           "     s           seek to start of file\n");
    return -1;
}

static char buf[BUFSIZE];

int main(int argc, char **argv) {
    unsigned long loops, i, j;
    char *pattern = "owwwsrrrc", c;
    int f, r, patlen;
    double t;

    if (argc < 2) return help();

    loops = strtoul(argv[1], NULL, 10);

    if (argc > 2) pattern = argv[2];
    patlen = strlen(pattern);

    t = gettime();

    f = open(FILENAME, O_RDWR | O_CREAT, 0644);

    for (i = 0; i < loops; i++) {
         for (j = 0; j < patlen; j++) {
              c = pattern[j];
              switch (c) {
              case 'o':
                  f = open(FILENAME, O_RDWR, 0644); break;
              case 'r':
                  r = read(f, buf, BUFSIZE); break;
              case 'w':
                  r = write(f, buf, BUFSIZE); break;
              case 's':
                  lseek(f, 0, SEEK_SET); break;
              case 'c':
                  close(f); break;
              }
              if (r < 0) perror("error");
              r = 0;
         }
    }

    t = gettime() - t;

    printf("%lu loops in %.3f sec, %.3f loops/sec\n", loops, t, loops/t);
    return 0;
}
