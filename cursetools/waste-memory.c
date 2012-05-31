#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int help(void) {
    printf("Usage: ./waste-memory <MB>\n");
    return -1;
}

int main(int argc, char **argv) {
    unsigned long bytes, r;
    unsigned char *buf;

    if (argc < 2) return help();

    bytes = 1024*1024*strtoul(argv[1], NULL, 10);

    buf = malloc(bytes);
    if (!buf) {
        perror("malloc");
        return -1;
    }

    for (r = 0; r < bytes; r++) {
         buf[r] = r;
    }

    for (;;) usleep(1000000);

    return 0;
}
