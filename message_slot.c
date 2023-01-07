// Declare what kind of code we want
// from the header files. Defining __KERNEL__
// and MODULE allows us to access kernel-level
// code not usually available to userspace programs.
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE


#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include <linux/slab.h>
#include <linux/ioctl.h>

MODULE_LICENSE("GPL");

//Our custom definitions of IOCTL operations
#include "message_slot.h"

static struct head *minor_numbers_array[257];

// The message_from_file the device will give when asked
static char the_message[BUF_LEN];

//================== DEVICE FUNCTIONS ===========================
static int look_for_channel(int minor_number, unsigned long num_of_channel) {
    struct head *minor_num_head;
    struct channel *tmp;

    minor_num_head = minor_numbers_array[minor_number];

    if (minor_num_head != NULL) {
        tmp = minor_num_head->first_channel;
        while (tmp->next != NULL) {
            if (tmp->channel_id==num_of_channel) {
                memcpy(the_message, tmp->message, tmp->message_length);
                return tmp->message_length;
            }
            tmp = tmp->next;
        }

        if (tmp->channel_id==num_of_channel) {
            memcpy(the_message, tmp->message, tmp->message_length);
            return tmp->message_length;
        }
    }

    return -1;
}

//---------------------------------------------------------------
static int device_open(struct inode* inode, struct file*  file) {
    int minor;
    struct file_message *message;

    minor = iminor(inode);
    message = kmalloc(sizeof(struct file_message), GFP_KERNEL);
    if (message == NULL) {
        return -ENOMEM;
    }

    message->minor_number = minor;
    message->last_channel_num = 0;
    file->private_data = (void *)message;

    return SUCCESS;
}

//---------------------------------------------------------------
// a process which has already opened
// the device file attempts to read from it
static ssize_t device_read(struct file* file, char __user* buffer, size_t length, loff_t* offset) {
    int i, minor, message_length;
    unsigned long num_of_channel;
    struct file_message *message;

    message = (struct file_message*)(file->private_data);


    if (message->last_channel_num==0) {
        return -EINVAL;
    }

    minor = message->minor_number;
    num_of_channel = message->last_channel_num;

    message_length = look_for_channel(minor, num_of_channel);

    /* if no message exists on the channel */
    if (message_length==-1) {
        return -EWOULDBLOCK;
    }

    /* if the provided buffer length is too small to hold the last message written on the channel */
    if (length < message_length) {
        return -ENOSPC;
    }

    /* reads the last message written on the channel into the user’s buffer */
    for (i=0; i<message_length; i++) {
        if (put_user(the_message[i], buffer+i) != 0) {
            return -EIO;
        }
    }

    return i;
}

//---------------------------------------------------------------
// a processs which has already opened
// the device file attempts to write to it
static ssize_t device_write(struct file* file, const char __user* buffer, size_t length, loff_t* offset) {
    int i, minor, message_length, found_channel;
    unsigned long num_of_channel;
    struct file_message *message;
    struct head *minor_num_head, *new_head;
    struct channel *tmp, *new_channel;
    char message_content[BUF_LEN];

    message = (struct file_message*)(file->private_data);
    minor = message->minor_number;
    num_of_channel = message->last_channel_num;

    /* if no channel has been set on the file descriptor */
    if (message->last_channel_num == 0) {
        return -EINVAL;
    }

    if (buffer == NULL) {
        return -EINVAL;
    }

    /* If the passed message length is 0 or more than 128 */
    if (length == 0 || BUF_LEN < length) {
        return -EMSGSIZE;
    }

    message_length = look_for_channel(minor, num_of_channel);

    for (i=0; i < length && i < BUF_LEN; ++i) {
        if (get_user(message_content[i], &buffer[i]) != 0) {
            return -EIO;
        }
    }

    // the channel doesn't exist
    if (message_length==-1) {
        message_length = length;
        new_channel = (struct channel*) kmalloc(sizeof(struct channel), GFP_KERNEL);
        new_channel->message_length = message_length;
        memcpy(new_channel->message, message_content, message_length);
        new_channel->channel_id = num_of_channel;
        new_channel->next = NULL;

        minor_num_head = minor_numbers_array[minor];

        // the minor's linked list doesn't exist
        if (minor_num_head==NULL) {
            new_head = (struct head*) kmalloc(sizeof(struct head), GFP_KERNEL);
            new_head->first_channel = new_channel;
            minor_numbers_array[minor] = new_head;
        }
        else {
            tmp = minor_num_head->first_channel;
            new_channel->next = tmp;
            minor_num_head->first_channel = new_channel;
        }
    }

    // channel exists
    else {
        message_length = length;
        minor_num_head = minor_numbers_array[minor];
        tmp = minor_num_head->first_channel;

        if (tmp->channel_id==num_of_channel) {
            found_channel = 1;
            memcpy(tmp->message, message_content, message_length);
            tmp->message_length = message_length;
        }

        while (tmp != NULL && found_channel==0) {
            tmp = tmp->next;
            if (tmp->channel_id==num_of_channel) {
                found_channel = 1;
                memcpy(tmp->message, message_content, message_length);
                tmp->message_length = message_length;
            }
        }
    }
    // return the number of input characters used
    return i;
}

//----------------------------------------------------------------
static long device_ioctl(struct file* file, unsigned int ioctl_command, unsigned long ioctl_param) {
    struct file_message* message;

    /* If the passed command is not MSG_SLOT_CHANNEL */
    if (ioctl_command != MSG_SLOT_CHANNEL) {
        return -EINVAL;
    }

    /* If the passed channel id is 0 */
    if (ioctl_param==0) {
        return -EINVAL;
    }

    message = (struct file_message*)(file->private_data);
    message->last_channel_num = ioctl_param;

    return SUCCESS;
}

//==================== DEVICE SETUP =============================

// This structure will hold the functions to be called
// when a process does something to the device we created
struct file_operations Fops = {
        .owner	  = THIS_MODULE,
        .read           = device_read,
        .write          = device_write,
        .open           = device_open,
        .unlocked_ioctl = device_ioctl,
};

//---------------------------------------------------------------
// Initialize the module - Register the character device
static int __init simple_init(void)
{
    int rc = -1;

    // Register driver capabilities. Obtain major num
    rc = register_chrdev( MAJOR_NUM, DEVICE_RANGE_NAME, &Fops );

    // Negative values signify an error
    if (rc < 0) {
        return rc;
    }

    printk( "Registeration is successful.\n");
    printk( "The major device number is %d.\n", MAJOR_NUM );
    printk( "If you want to talk to the device driver,\n" );
    printk( "you have to create a device file:\n" );
    printk( "mknod /dev/%s c %d 0\n", DEVICE_RANGE_NAME, MAJOR_NUM );
    printk( "You can echo/cat to/from the device file.\n" );
    printk( "Dont forget to rm the device file and "
            "rmmod when you're done\n" );

    return 0;
}

//---------------------------------------------------------------
static void __exit simple_cleanup(void) {
    int i;
    struct head *curr_head;
    struct channel *tmp, *curr_channel;

    for (i=0; i<257; i++) {
        curr_head = minor_numbers_array[i];
        if (curr_head != NULL) {
            tmp = curr_head->first_channel;
            while (tmp->next != NULL) {
                curr_channel = tmp->next;
                kfree(tmp);
                tmp = curr_channel;
            }
            kfree(curr_channel);
            kfree(curr_head);
        }
    }

    // Unregister the device
    // Should always succeed
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);

//========================= END OF FILE =========================
