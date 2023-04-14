#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

#define DEVICE_NAME "lkm_pa2-out"
#define CLASS_NAME "char"
#define BUFFER_LENGTH 1024

MODULE_LICENSE("GPL");
MODULE_AUTHOR("The homies");
MODULE_DESCRIPTION("Output Kernel Module");
MODULE_VERSION("0.1");

static int major_number;
static int numberOpens = 0;
static struct class *lkm_outputClass = NULL;
static struct device *lkm_outputDevice = NULL;

extern char shared_buffer[BUFFER_LENGTH];
extern short sizeOfMessage;
extern struct mutex shared_buffer_mutex;

static int open(struct inode *, struct file *);
static int close(struct inode *, struct file *);
static ssize_t read(struct file *, char *, size_t, loff_t *);

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = open,
    .release = close,
    .read = read,
};

// Include functions for open, close, read and other necessary functions as in the original code
// Make sure to use mutex_lock and mutex_unlock for critical sections

int init_module(void)
{
    printk(KERN_INFO "%s: installing output module.\n", DEVICE_NAME);

    // Allocate a major number for the device.
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0)
    {
        printk(KERN_ALERT "%s could not register number.\n", DEVICE_NAME);
        return major_number;
    }
    printk(KERN_INFO "%s: registered correctly with major number %d\n", DEVICE_NAME, major_number);

    // Register the device class
    deviceClass = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(deviceClass))
    { // Check for error and clean up if there is
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "Failed to register device class\n");
        return PTR_ERR(deviceClass); // Correct way to return an error on a pointer
    }
    printk(KERN_INFO "%s: device class registered correctly\n", DEVICE_NAME);

    // Register the device driver
    deviceDriver = device_create(deviceClass, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(deviceDriver))
    {                                // Clean up if there is an error
        class_destroy(deviceClass); // Repeated code but the alternative is goto statements
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create the device\n");
        return PTR_ERR(deviceDriver);
    }
    printk(KERN_INFO "%s: device class created correctly\n", DEVICE_NAME); // Made it! device was initialized

    return 0;
}

void cleanup_module(void)
{
    printk(KERN_INFO "%s: removing output module.\n", DEVICE_NAME);
    device_destroy(lkmasg1Class, MKDEV(major_number, 0)); // remove the device
    class_unregister(lkmasg1Class);                       // unregister the device class
    class_destroy(lkmasg1Class);                          // remove the device class
    unregister_chrdev(major_number, DEVICE_NAME);         // unregister the major number
    printk(KERN_INFO "%s: Goodbye from the LKM!\n", DEVICE_NAME);
    unregister_chrdev(major_number, DEVICE_NAME);
    return;
}

static int open(struct inode *inodep, struct file *filep) {
    numberOpens++;
    printk(KERN_INFO "%s: device opened.\n", DEVICE_NAME);
    return 0;
}

static int close(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "%s: device closed.\n", DEVICE_NAME);
    return 0;
}

static ssize_t read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
    printk(KERN_INFO "%s: waiting on lock.\n", DEVICE_NAME);
    mutex_lock(&shared_buffer_mutex);

    printk(KERN_INFO "%s: acquiring lock.\n", DEVICE_NAME);
    int bytes_to_read = min(len, (size_t)sizeOfMessage);

    if (bytes_to_read == 0) {
        printk(KERN_INFO "%s: buffer is empty.\n", DEVICE_NAME);
    }

    printk(KERN_INFO "%s: in critical section.\n", DEVICE_NAME);
    int error = copy_to_user(buffer, shared_buffer, bytes_to_read);

    if (error == 0) {
        memmove(shared_buffer, shared_buffer + bytes_to_read, sizeOfMessage - bytes_to_read);
        sizeOfMessage -= bytes_to_read;

        printk(KERN_INFO "%s: releasing lock.\n", DEVICE_NAME);
        mutex_unlock(&shared_buffer_mutex);

        printk(KERN_INFO "%s: read %d bytes.\n", DEVICE_NAME, bytes_to_read);
        return bytes_to_read;
    } else {
        printk(KERN_INFO "%s: failed to read.\n", DEVICE_NAME);
        printk(KERN_INFO "%s: releasing lock.\n", DEVICE_NAME);
        mutex_unlock(&shared_buffer_mutex);
        return -EFAULT;
    }
}
