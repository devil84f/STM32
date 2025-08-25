#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/irq.h>
#include <linux/of_irq.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/input.h>
#include <linux/spi/spi.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include "w25q128_reg.h"

#define W25Q128_NAME    "w25q128"
#define W25Q128_CNT     1

struct w25q128_dev {
    void *private_data;
    int cs_gpio;
};

static struct w25q128_dev w25q128;

static int w25q128_read_jedec_id(struct spi_device *spi, u8 *id_buf)
{
    int ret;
    u8 tx_buf[1] = {W25Q64_JEDEC_ID};
    u8 rx_buf[3] = {0};

    struct spi_transfer t[] = {
        {
            .tx_buf = tx_buf,
            .len = sizeof(tx_buf),
        },
        {
            .rx_buf = rx_buf,
            .len = sizeof(rx_buf),
        },
    };

    struct spi_message m;
    
    spi_message_init(&m);
    spi_message_add_tail(&t[0], &m);
    spi_message_add_tail(&t[1], &m);

    dev_info(&spi->dev, "Sending JEDEC ID command: 0x%02x\n", W25Q64_JEDEC_ID);
    
    ret = spi_sync(spi, &m);
    if (ret < 0) {
        dev_err(&spi->dev, "spi_sync failed: %d\n", ret);
        return ret;
    }

    dev_info(&spi->dev, "Received ID: %02x %02x %02x\n", 
             rx_buf[0], rx_buf[1], rx_buf[2]);
    
    memcpy(id_buf, rx_buf, sizeof(rx_buf));
    return 0;
}

static int w25q128_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &w25q128;
    return 0;
}

static ssize_t w25q128_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
    return 0;
}

static int w25q128_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
    return 0;
}

static int w25q128_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static const struct file_operations w25q128_fops = {
    .owner  = THIS_MODULE,
    .open   = w25q128_open,
    .write  = w25q128_write,
    .read   = w25q128_read,
    .release= w25q128_release,
};

static struct miscdevice w25q128_misc = {
    .minor  = MISC_DYNAMIC_MINOR,
	.name   = W25Q128_NAME,
	.fops   = &w25q128_fops,
};

static void w25q128_init(struct spi_device *spi)
{
    u8 id_buf[3];
    w25q128_read_jedec_id(spi, id_buf);
    printk("JEDEC ID: %02x %02x %02x\n", id_buf[0], id_buf[1], id_buf[2]);
}
/*
static int w25q128_probe(struct spi_device *spi)
{
    int ret;
    printk("w25q128 driver and device matched!\r\n");

    ret = misc_register(&w25q128_misc);

    spi->mode = SPI_MODE_0;
    spi_setup(spi);
    w25q128.private_data = spi; // 设置私有数据为SPI设备

    w25q128_init(w25q128.private_data);
    return ret;
} */
static int w25q128_probe(struct spi_device *spi)
{
    int ret;
    u16 original_mode = spi->mode; // 保存原始模式
    
    printk("w25q128 driver and device matched!\r\n");
    printk("Current SPI mode: %u, max speed: %u Hz\n", spi->mode, spi->max_speed_hz);

    // 尝试强制设置为Mode 0
    spi->mode = SPI_MODE_0;
    ret = spi_setup(spi);
    if (ret) {
        dev_err(&spi->dev, "Failed to setup SPI mode 0\n");
        // 恢复原始模式
        spi->mode = original_mode;
        return ret;
    }

    ret = misc_register(&w25q128_misc);
    if (ret) {
        dev_err(&spi->dev, "Failed to register misc device\n");
        return ret;
    }

    w25q128.private_data = spi;
    w25q128_init(spi); // 直接传递spi设备
    return 0;
}

static int w25q128_remove(struct spi_device *spi)
{
    printk("w25q128 exit!\r\n");
    misc_deregister(&w25q128_misc);
    return 0;
} 

static const struct of_device_id w25q128_match_table[] = {
    { .compatible = "alientek,w25q128" },
	{ /* Sentinel */ },
};

static struct spi_driver w25q128_driver = {
    .driver     = {
        .owner  = THIS_MODULE,
        .name   = W25Q128_NAME,
        .of_match_table = w25q128_match_table,
    },
    .probe      = w25q128_probe,
    .remove     = w25q128_remove,
};

static int __init w25q128_driver_init(void)
{
    return spi_register_driver(&w25q128_driver);
}

static void __exit w25q128_driver_exit(void)
{
    spi_unregister_driver(&w25q128_driver);
}

module_init(w25q128_driver_init);
module_exit(w25q128_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LiNing");

