#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/ioctl.h>
#include "message_slot.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        perror("ERROR: doesn't mach expected amount of parameters.\n");
        exit(1);
    }

    int file_path, read_res;
    unsigned long target_chanel_id;
    char buffer[BUF_LEN];

    file_path = open(argv[1], O_RDONLY);
    if (file_path == -1) {
        perror("ERROR: failed to open the file.\n");
        exit(1);
    }

    target_chanel_id = atoi(argv[2]);
    if (ioctl(file_path, MSG_SLOT_CHANNEL, target_chanel_id) == -1) {
        perror("ERROR: failed to do ioctl.\n");
        exit(1);
    }

    read_res = read(file_path, buffer, BUF_LEN);
    if (read_res <= 0) {
        perror("ERROR: failed to read.\n");
        exit(1);
    }

    if (write(1, buffer, read_res) != read_res) {
        perror("ERROR: failed to write to console.\n");
        exit(1);
    }

    close(file_path);
    return 0;
}
