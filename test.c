#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <err.h>
#include <string.h>
#include <assert.h>

#include "version.h"

#define BUF_LEN 4096

void read_version(char *buffer)
{
    FILE *file = fopen("/dev/version", "r");
    if (!file)
        err(1, "Could not open /dev/version");

    size_t len = fread(buffer, 1, BUF_LEN, file);
    buffer[len] = '\0'; // Stringify the buffer

    fclose(file);
}

void write_version(char *string, size_t len)
{
    FILE *file = fopen("/dev/version", "w");
    if (!file)
        err(1, "Could not open /dev/version");

    fwrite(string, len, sizeof(char), file);

    fclose(file);
}

int is_modified_version(void)
{
    FILE *file = fopen("/dev/version", "r");
    if (!file)
        err(1, "Could not open /dev/version");

    int fd = fileno(file);

    int is_modified;
    ioctl(fd, VERSION_MODIFIED, &is_modified);

    return is_modified;
}

void reset_version(void)
{
    FILE *file = fopen("/dev/version", "r");
    if (!file)
        err(1, "Could not open /dev/version");

    int fd = fileno(file);

    ioctl(fd, VERSION_RESET);

    fclose(file);
}

int main(void)
{
    char buffer[BUF_LEN];

    read_version(buffer);
    printf("Original version: %s", buffer);
    assert(is_modified_version() == 0);

    write_version("THIS IS A FAKE MODIFIED VERSION", 32);
    assert(is_modified_version() == 1);

    read_version(buffer);
    printf("Modified version: %s", buffer);

    reset_version();
    assert(is_modified_version() == 0);

    read_version(buffer);
    printf("Restored version: %s", buffer);

    return 0;
}
