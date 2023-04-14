/**
 * File:    lkm_pa2_in.c
 * Adapted for Linux 5.15 by: John Aedo
 * Class:   COP4600-SP23
 * Group members: Sam Durkee, Grayson Crozier, and Chance Rupnow
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

#define DEVICE_NAME "lkm_pa2_in"
#define CLASS_NAME "in_char"
#define BUFFER_LENGTH 1024

MODULE_INFO(name, "lkm_pa2_in");
MODULE_INFO(description, "Input Kernel Module");
MODULE_INFO(version, "0.1");
MODULE_INFO(author, "The homies");
MODULE_LICENSE("GPL");

static int major_number;
static int numberOpens = 0;
static struct class *lkm_inputClass = NULL;
static struct device *lkm_inputDevice = NULL;

// Define shared variables
char shared_buffer[BUFFER_LENGTH];
short sizeOfMessage;
struct mutex shared_buffer_mutex;

EXPORT_SYMBOL(shared_buffer);
EXPORT_SYMBOL(sizeOfMessage);
EXPORT_SYMBOL(shared_buffer_mutex);

static int open(struct inode *, struct file *);
static int close(struct inode *, struct file *);
static ssize_t write(struct file *, const char *, size_t, loff_t *);

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = open,
    .release = close,
    .write = write,
};

static int __init lkm_pa2_in_init(void)
{
    printk(KERN_INFO "%s: installing input module.\n", DEVICE_NAME);

    // Init mutex
    mutex_init(&shared_buffer_mutex);

    // Allocate a major number for the device.
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0)
    {
        printk(KERN_ALERT "%s could not register number.\n", DEVICE_NAME);
        return major_number;
    }
    printk(KERN_INFO "%s: registered correctly with major number %d\n", DEVICE_NAME, major_number);

    // Register the device class
    lkm_inputClass = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(lkm_inputClass))
    { // Check for error and clean up if there is
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "Failed to register device class\n");
        return PTR_ERR(lkm_inputClass); // Correct way to return an error on a pointer
    }
    printk(KERN_INFO "%s: device class registered correctly\n", DEVICE_NAME);

    // Register the device driver
    lkm_inputDevice = device_create(lkm_inputClass, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(lkm_inputDevice))
    {                               // Clean up if there is an error
        class_destroy(lkm_inputClass); // Repeated code but the alternative is goto statements
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create the device\n");
        return PTR_ERR(lkm_inputDevice);
    }
    printk(KERN_INFO "%s: device class created correctly\n", DEVICE_NAME); // Made it! device was initialized

    return 0;
}

static void __exit lkm_pa2_in_exit(void)
{
    printk(KERN_INFO "%s: removing input module.\n", DEVICE_NAME);
    mutex_destroy(&shared_buffer_mutex);                    // remove mutex
    device_destroy(lkm_inputClass, MKDEV(major_number, 0)); // remove the device
    class_unregister(lkm_inputClass);                       // unregister the device class
    class_destroy(lkm_inputClass);                          // remove the device class
    unregister_chrdev(major_number, DEVICE_NAME);         // unregister the major number
    printk(KERN_INFO "%s: Goodbye from the LKM!\n", DEVICE_NAME);
    unregister_chrdev(major_number, DEVICE_NAME);
    return;
}

static int open(struct inode *inodep, struct file *filep)
{
    numberOpens++;
    printk(KERN_INFO "%s: device opened.\n", DEVICE_NAME);
    return 0;
}

static int close(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "%s: device closed.\n", DEVICE_NAME);
    return 0;
}

static ssize_t write(struct file *filep, const char *buffer, size_t len, loff_t *offset)
{
    int bytes_to_write;
    printk(KERN_INFO "%s: waiting on lock.\n", DEVICE_NAME);
    mutex_lock(&shared_buffer_mutex);

    printk(KERN_INFO "%s: acquiring lock.\n", DEVICE_NAME);
    bytes_to_write = min(len, (size_t)(BUFFER_LENGTH - sizeOfMessage));

    if (bytes_to_write < len)
    {
        printk(KERN_INFO "%s: not enough space left in the buffer, dropping the rest.\n", DEVICE_NAME);
    }

    printk(KERN_INFO "%s: in critical section.\n", DEVICE_NAME);
    memcpy(shared_buffer + sizeOfMessage, buffer, bytes_to_write);
    sizeOfMessage += bytes_to_write;

    printk(KERN_INFO "%s: releasing lock.\n", DEVICE_NAME);
    mutex_unlock(&shared_buffer_mutex);

    printk(KERN_INFO "%s: wrote %d bytes.\n", DEVICE_NAME, bytes_to_write);
    return bytes_to_write;
}

module_init(lkm_pa2_in_init);
module_exit(lkm_pa2_in_exit);
