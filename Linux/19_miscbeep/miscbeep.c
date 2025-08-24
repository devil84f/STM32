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
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define MISCBEEP_NAME   "miscbeep"
#define BEEPON          1
#define BEEPOFF         0

struct miscbeep_device {
    struct device_node *nd;
    int beepgpio;
};

static struct miscbeep_device miscbeep_dev;

static int miscbeep_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &miscbeep_dev;
    return 0;
}

static ssize_t miscbeep_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
    int retvalue;
    unsigned char databuf[1];
    unsigned char beepstat;
    struct miscbeep_device *dev = filp->private_data;

    retvalue = copy_from_user(databuf, buf, cnt);
    if(retvalue < 0) {
        printk("kernel write failed!\r\n");
        return -EFAULT;
    }

    beepstat = databuf[0];        /* 获取状态值 */

    if(beepstat == BEEPON) {    
        gpio_set_value(dev->beepgpio, 0);        
    } else if(beepstat == BEEPOFF) {
        gpio_set_value(dev->beepgpio, 1);
    }
    return 0;
}

static struct file_operations miscbeep_fops = {
    .owner  = THIS_MODULE,
    .open   = miscbeep_open,
    .write  = miscbeep_write,
};

static struct miscdevice miscbeep = {
    .minor  = MISC_DYNAMIC_MINOR,
    .name   = MISCBEEP_NAME,
    .fops   = &miscbeep_fops,
};

static int miscbeep_probe(struct platform_device *pdev) 
{
    int ret;
    
    printk("beep driver and device has matched!\r\n");

    // 正确的方法：直接从 platform_device 获取设备树节点
    miscbeep_dev.nd = pdev->dev.of_node;
    if(miscbeep_dev.nd == NULL) {
        printk("device node not find!\r\n");
        return -EINVAL;
    }

    miscbeep_dev.beepgpio = of_get_named_gpio(miscbeep_dev.nd, "beep-gpio", 0);
    if(miscbeep_dev.beepgpio < 0) {
        printk("beep-gpio not find! error: %d\r\n", miscbeep_dev.beepgpio);
        return miscbeep_dev.beepgpio;
    }

    ret = gpio_request(miscbeep_dev.beepgpio, MISCBEEP_NAME);
    if(ret) {
        printk("Failed to request GPIO, error: %d\r\n", ret);
        return ret;
    }

    ret = gpio_direction_output(miscbeep_dev.beepgpio, 1);
    if(ret) {
        printk("Failed to set GPIO direction, error: %d\r\n", ret);
        gpio_free(miscbeep_dev.beepgpio);
        return ret;
    }

    // 最后才注册 misc 设备
    ret = misc_register(&miscbeep);
    if(ret) {
        printk("Failed to register misc device, error: %d\r\n", ret);
        gpio_free(miscbeep_dev.beepgpio);
        return ret;
    }

    printk("miscbeep probe success! GPIO: %d\r\n", miscbeep_dev.beepgpio);
    return 0;
}

static int miscbeep_remove(struct platform_device *pdev) 
{
    // 先注销 misc 设备
    misc_deregister(&miscbeep);
    
    // 然后释放 GPIO
    gpio_set_value(miscbeep_dev.beepgpio, 1);
    gpio_free(miscbeep_dev.beepgpio);
    
    printk("miscbeep removed\r\n");
    return 0;
}

static const struct of_device_id miscbeep_of_match[] = {
    {.compatible = "atkmini-beep"},
    { /* sentinel */ },
};

static struct platform_driver beepdrivers = {
    .driver             = {
        .name           = MISCBEEP_NAME,
        .of_match_table = miscbeep_of_match,
    },
    .probe              = miscbeep_probe,
    .remove             = miscbeep_remove,
};

static int __init miscbeep_init(void)
{
    return platform_driver_register(&beepdrivers);
}

static void __exit miscbeep_exit(void)
{
    platform_driver_unregister(&beepdrivers);
}

module_init(miscbeep_init);
module_exit(miscbeep_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("liNing");