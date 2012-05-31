#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char **argv) {
    int i;

    if (argc < 2) {
        printf("Usage: ./uncache files...\n"
               "Removes files from memory with fadvise.\n");
        return 1;
    }

    for (i = 1; i < argc; i++) {
         int fd = open(argv[i], O_RDONLY);
         if (fd < 0) continue;
         posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED);
    }

    return 0;
}
