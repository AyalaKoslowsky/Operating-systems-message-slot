#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/ioctl.h>
#include "message_slot.h"

int main(int argc, char **argv) {
    if (argc != 4) {
        perror("ERROR: doesn't mach expected amount of parameters.\n");
        exit(1);
    }

    int file_path, write_res, ioctl_res;
    unsigned long target_chanel_id;
    int message_length = strlen(argv[3]);

    file_path = open(argv[1], O_WRONLY);
    if (file_path == -1) {
        perror("ERROR: failed to open the file.\n");
        exit(1);
    }

    target_chanel_id = atoi(argv[2]);

    ioctl_res = ioctl(file_path, MSG_SLOT_CHANNEL, target_chanel_id);
    if (ioctl_res == -1) {
        perror("ERROR: failed to do ioctl.\n");
        exit(1);
    }

    write_res = write(file_path, argv[3], message_length);
    if (write_res != message_length) {
        perror("ERROR: failed to write.\n");
        exit(1);
    }

    close(file_path);
    return 0;
}
