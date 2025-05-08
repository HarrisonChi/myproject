#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/delay.h>

#define DEV_NAME "air_quality"
#define BUF_LEN 128

// Raspberry Pi GPIO pin numbers (BCM numbering)
#define PIN_MISO 580   // GPIO9
#define PIN_MOSI 581  // GPIO10
#define PIN_CLK  582  // GPIO11
#define PIN_CS   579   // GPIO8

static dev_t dev_num;
static struct cdev mq135_cdev;
static struct class *mq135_class;

static int gpio_pins[] = { PIN_MISO, PIN_MOSI, PIN_CLK, PIN_CS };

// --- Bitbang SPI ---
static void spi_delay(void)
{
    udelay(1);  // Short delay for timing (1 us)
}

static void spi_write_bit(int bit)
{
    gpio_set_value(PIN_MOSI, bit);
    spi_delay();
    gpio_set_value(PIN_CLK, 1);
    spi_delay();
    gpio_set_value(PIN_CLK, 0);
    spi_delay();
}

static int spi_read_bit(void)
{
    int bit;
    gpio_set_value(PIN_CLK, 1);
    spi_delay();
    bit = gpio_get_value(PIN_MISO);
    gpio_set_value(PIN_CLK, 0);
    spi_delay();
    return bit;
}

static int mcp3008_read_channel(uint8_t channel)
{
    int i, value = 0;

    // Start communication
    gpio_set_value(PIN_CS, 0);
    spi_delay();

    // Start bit (1), single-ended (1), channel (XXX)
    uint8_t command = 0b11000 | (channel << 0);

    for (i = 4; i >= 0; i--)
        spi_write_bit((command >> i) & 0x1);

    // Read 12 bits (null bit + 10-bit data + 1 extra bit)
    spi_read_bit();  // Null bit discard
    for (i = 9; i >= 0; i--)
        value |= spi_read_bit() << i;

    // End communication
    gpio_set_value(PIN_CS, 1);
    return value;
}

static ssize_t mq135_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    char data[BUF_LEN];
    int len;

    int co2 = mcp3008_read_channel(0);
    int nh3 = mcp3008_read_channel(1);
    int alcohol = mcp3008_read_channel(2);
    int co = mcp3008_read_channel(3);

    len = snprintf(data, sizeof(data), "CO2=%d&NH3=%d&Alcohol=%d&CO=%d\n", co2, nh3, alcohol, co);

    if (*f_pos >= len)
        return 0;

    if (count > len - *f_pos)
        count = len - *f_pos;

    if (copy_to_user(buf, data + *f_pos, count))
        return -EFAULT;

    *f_pos += count;
    return count;
}

static struct file_operations mq135_fops = {
    .owner = THIS_MODULE,
    .read = mq135_read,
};

static int __init mq135_init(void)
{
    int ret, i;

    ret = alloc_chrdev_region(&dev_num, 0, 1, DEV_NAME);
    if (ret < 0)
        return ret;

    cdev_init(&mq135_cdev, &mq135_fops);
    ret = cdev_add(&mq135_cdev, dev_num, 1);
    if (ret < 0)
        goto err_cdev;

    mq135_class = class_create(DEV_NAME);
    if (IS_ERR(mq135_class)) {
        ret = PTR_ERR(mq135_class);
        goto err_class;
    }

    device_create(mq135_class, NULL, dev_num, NULL, DEV_NAME);

    // Initialize GPIO pins
    for (i = 0; i < ARRAY_SIZE(gpio_pins); i++) {
        ret = gpio_request(gpio_pins[i], "mq135_gpio");
        if (ret)
            goto err_gpio;
    }

    gpio_direction_input(PIN_MISO);
    gpio_direction_output(PIN_MOSI, 0);
    gpio_direction_output(PIN_CLK, 0);
    gpio_direction_output(PIN_CS, 1);

    pr_info("mq135 driver loaded (MCP3008 SPI bitbang)\n");
    return 0;

err_gpio:
    while (--i >= 0)
        gpio_free(gpio_pins[i]);
    device_destroy(mq135_class, dev_num);
    class_destroy(mq135_class);
err_class:
    cdev_del(&mq135_cdev);
err_cdev:
    unregister_chrdev_region(dev_num, 1);
    return ret;
}

static void __exit mq135_exit(void)
{
    int i;
    device_destroy(mq135_class, dev_num);
    class_destroy(mq135_class);
    cdev_del(&mq135_cdev);
    unregister_chrdev_region(dev_num, 1);

    for (i = 0; i < ARRAY_SIZE(gpio_pins); i++)
        gpio_free(gpio_pins[i]);

    pr_info("mq135 driver unloaded\n");
}

module_init(mq135_init);
module_exit(mq135_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("MQ135 Air Quality Driver + MCP3008 Bitbang SPI (No spi.h)");

