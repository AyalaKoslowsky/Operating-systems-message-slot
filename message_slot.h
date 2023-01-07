#ifndef OS3_OLD_MESSAGE_SLOT_H
#define OS3_MESSAGE_SLOT_H

#define BUF_LEN 128
#define MAJOR_NUM 235
#define DEVICE_RANGE_NAME "message_slot"
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned int)
#define SUCCESS 0

struct head {
    struct channel *first_channel;
};

/* A regular chanel */
struct channel {
    unsigned long channel_id;
    char message[BUF_LEN];
    int message_length;
    struct channel *next;
};

struct file_message {
    int minor_number;
    unsigned long last_channel_num;
};

#endif //OS3_OLD_MESSAGE_SLOT_H
