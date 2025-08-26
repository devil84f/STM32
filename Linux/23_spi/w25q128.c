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
    struct spi_device *spi;
    u8 id[3];
    struct mutex lock;
};

static struct w25q128_dev w25q128;

static int w25q128_read_id(struct w25q128_dev *dev, u8 *id_buf)
{
    int ret;
    u8 tx_buf[4] = {W25Q64_MANUFACTURER_DEVICE_ID, 0xff, 0xff, 0xff};
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

    mutex_lock(&dev->lock);
    ret = spi_sync(dev->spi, &m);
    mutex_unlock(&dev->lock);
    
    if (ret < 0) {
        pr_err("spi_sync failed: %d\n", ret);
        return ret;
    }

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
    struct w25q128_dev *dev = filp->private_data;
    u8 id_buf[3];
    int ret;
    
    if (cnt < 3) {
        return -EINVAL;
    }
    
    ret = w25q128_read_id(dev, id_buf);
    if (ret < 0) {
        return ret;
    }
    
    if (copy_to_user(buf, id_buf, 3)) {
        return -EFAULT;
    }
    
    return 3;
}

static ssize_t w25q128_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
    return -EINVAL; // 只读设备，不支持写操作
}

static long w25q128_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct w25q128_dev *dev = filp->private_data;
    int ret = 0;
    
    switch (cmd) {
    case W25Q128_GET_ID:
        if (copy_to_user((void __user *)arg, dev->id, 3)) {
            ret = -EFAULT;
        }
        break;
    default:
        ret = -ENOTTY;
        break;
    }
    
    return ret;
}

static int w25q128_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static const struct file_operations w25q128_fops = {
    .owner      = THIS_MODULE,
    .open       = w25q128_open,
    .read       = w25q128_read,
    .write      = w25q128_write,
    .unlocked_ioctl = w25q128_ioctl,
    .release    = w25q128_release,
};

static struct miscdevice w25q128_misc = {
    .minor      = MISC_DYNAMIC_MINOR,
    .name       = W25Q128_NAME,
    .fops       = &w25q128_fops,
};

static int w25q128_probe(struct spi_device *spi)
{
    int ret;
    dev_info(&spi->dev, "w25q128 probe started\n");
    dev_info(&spi->dev, "SPI device: %s\n", dev_name(&spi->dev));
    dev_info(&spi->dev, "SPI mode: %u, max speed: %u Hz\n", spi->mode, spi->max_speed_hz);
    
    printk("w25q128 driver and device matched!\n");
    printk("SPI mode: %u, max speed: %u Hz\n", spi->mode, spi->max_speed_hz);

    // 设置SPI模式为0
    spi->mode = SPI_MODE_0;
    ret = spi_setup(spi);
    if (ret) {
        dev_err(&spi->dev, "Failed to setup SPI\n");
        return ret;
    }

    // 初始化设备结构
    w25q128.spi = spi;
    mutex_init(&w25q128.lock);

    // 读取 ID
    ret = w25q128_read_id(&w25q128, w25q128.id);
    if (ret) {
        dev_err(&spi->dev, "Failed to read ID\n");
        return ret;
    }
    
    printk("ID: %02x %02x %02x\n", 
           w25q128.id[0], w25q128.id[1], w25q128.id[2]);

    // 注册misc设备
    ret = misc_register(&w25q128_misc);
    if (ret) {
        dev_err(&spi->dev, "Failed to register misc device\n");
        return ret;
    }

    return 0;
}

static int w25q128_remove(struct spi_device *spi)
{
    printk("w25q128 driver removed\n");
    misc_deregister(&w25q128_misc);
    mutex_destroy(&w25q128.lock);
    return 0;
} 

static const struct spi_device_id w25q128_id[] = {
	{"alientek,my_w25q128", 0},  
	{}
};

static const struct of_device_id w25q128_match_table[] = {
    { .compatible = "alientek,my_w25q128" },
    { /* Sentinel */ },
};

static struct spi_driver w25q128_driver = {
    .driver = {
        .name = W25Q128_NAME,
        .of_match_table = w25q128_match_table,
        .owner = THIS_MODULE,
    },
    .probe = w25q128_probe,
    .remove = w25q128_remove,
    .id_table = w25q128_id,
};

module_spi_driver(w25q128_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LiNing");
MODULE_DESCRIPTION("W25Q128 SPI Flash Driver");