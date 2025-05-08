#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>

int get_co2_value(void);
int get_nh3_value(void);
int get_alcohol_value(void);
int get_co_value(void);

int get_co2_value(void) { return 400 + (get_random_u32() % 10000); }
int get_nh3_value(void) { return 10 + (get_random_u32() % 200); }
int get_alcohol_value(void) { return 50 + (get_random_u32() % 2000); }
int get_co_value(void) { return 0 + (get_random_u32() % 200); }

static int major_number;
static struct class *air_quality_class = NULL;
static struct device *air_quality_device = NULL;

static int air_quality_open(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "Air quality sender: Device opened\n");
    return 0;
}

static int air_quality_release(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "Air quality sender: Device closed\n");
    return 0;
}

static ssize_t air_quality_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset) {
    if (*offset > 0) return 0;

    int co2 = get_co2_value();
    int nh3 = get_nh3_value();
    int alcohol = get_alcohol_value();
    int co = get_co_value();
    char data[128];

    int data_len = snprintf(data, sizeof(data), "CO2=%d&NH3=%d&Alcohol=%d&CO=%d\n", co2, nh3, alcohol, co);

    if (data_len > len) return -ENOMEM;
    if (copy_to_user(buffer, data, data_len)) return -EFAULT;

    *offset += data_len;
    return data_len;
}

static ssize_t air_quality_write(struct file *filep, const char *buffer, size_t len, loff_t *offset) {
    printk(KERN_INFO "Air quality sender: Write operation\n");
    return len;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = air_quality_open,
    .read = air_quality_read,
    .write = air_quality_write,
    .release = air_quality_release,
};

static int __init air_quality_sender_init(void) {
    int result;
    result = register_chrdev(0, "air_quality", &fops);
    if (result < 0) {
        printk(KERN_ALERT "Failed to register major number\n");
        return result;
    }
    major_number = result;

    air_quality_class = class_create("air_quality_class");
    if (IS_ERR(air_quality_class)) {
        unregister_chrdev(major_number, "air_quality");
        return PTR_ERR(air_quality_class);
    }

    air_quality_device = device_create(air_quality_class, NULL, MKDEV(major_number, 0), NULL, "air_quality");
    if (IS_ERR(air_quality_device)) {
        class_destroy(air_quality_class);
        unregister_chrdev(major_number, "air_quality");
        return PTR_ERR(air_quality_device);
    }

    printk(KERN_INFO "Device created correctly\n");
    return 0;
}

static void __exit air_quality_sender_exit(void) {
    device_destroy(air_quality_class, MKDEV(major_number, 0));
    class_destroy(air_quality_class);
    unregister_chrdev(major_number, "air_quality");
    printk(KERN_INFO "Goodbye from the LKM!\n");
}

module_init(air_quality_sender_init);
module_exit(air_quality_sender_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Harrison");
MODULE_DESCRIPTION("A simple Linux char driver for air quality sender.");
MODULE_VERSION("0.1");

