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
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define GPIOLED_CNT 	1
#define GPIOLED_NAME 	"gpioled"
#define LEDOFF 			0
#define LEDON 			1

struct gpioled_dev
{
	dev_t devid;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	struct device_node *nd;
	int major;
	int minor;
	int led_gpio;
};

struct gpioled_dev gpioled;

static int led_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &gpioled;
	return 0;
}

static ssize_t led_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
	int retvalue;
	unsigned char databuf[1];
	unsigned char ledstat;

	struct gpioled_dev *dev = filp->private_data;

	retvalue = copy_from_user(databuf, buf, cnt);
	if(retvalue < 0) {
		printk("kernel write failed!\r\n");
		return -EFAULT;
	}

	ledstat = databuf[0];		/* 获取状态值 */

	if(ledstat == LEDON) {	
		gpio_set_value(dev->led_gpio, 0);		/* 打开LED灯 */
	} else if(ledstat == LEDOFF) {
		gpio_set_value(dev->led_gpio, 1);		/* 关闭LED灯 */
	}

	return 0;
}

static ssize_t led_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	return 0;
}

static int led_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static struct file_operations gpioled_fops = {
	.owner = THIS_MODULE,
	.open = led_open,
	.write = led_write,
	.read = led_read,
	.release = led_release,
};

static int __init gpio_led_init(void)
{
	int ret;
	/* 获取设备节点 */
	gpioled.nd = of_find_node_by_path("/gpioled");
	if(gpioled.nd == NULL) {
		printk("gpioled node can not found!\r\n");
		return -EINVAL;
	} else {
		printk("gpioled node has been found!\r\n");
	}

	/* 获取gpio标号 */
	gpioled.led_gpio = of_get_named_gpio(gpioled.nd, "led-gpio", 0);
	if(gpioled.led_gpio < 0) {
		printk("can't get led-gpio");
		return -EINVAL;
	}
	printk("led-gpio num = %d\r\n", gpioled.led_gpio);

	/* 申请gpio管脚 */
	ret = gpio_request(gpioled.led_gpio, "led_gpio");
	if (ret != 0) {
		printk("led_gpio can not request!\r\n");
	} else {
		printk("led_gpio request succesful!\r\n");
	}
	/* 设置gpio输出 */
	ret = gpio_direction_output(gpioled.led_gpio, 1);
	if (ret != 0) {
		printk("gpio_direction_output can not set!\r\n");
	} else {
		printk("gpio_direction_output set succesful!\r\n");
	}

	/* 申请设备号 */
	if (gpioled.major) {
		gpioled.devid = MKDEV(gpioled.major, 0);
		register_chrdev_region(gpioled.devid, GPIOLED_CNT, GPIOLED_NAME);
	} else {
		alloc_chrdev_region(&gpioled.devid, 0, GPIOLED_CNT, GPIOLED_NAME);
		gpioled.major = MAJOR(gpioled.devid);
		gpioled.minor = MINOR(gpioled.devid);
	}
	printk("gpioled major = %d, minor = %d\r\n", gpioled.major, gpioled.minor);

	/* 初始化驱动cdev */
	gpioled.cdev.owner = THIS_MODULE;
	cdev_init(&gpioled.cdev, &gpioled_fops);
	cdev_add(&gpioled.cdev, gpioled.devid, GPIOLED_CNT);

	/* 创建类 */
	gpioled.class = class_create(THIS_MODULE, GPIOLED_NAME);
	if(IS_ERR(gpioled.class)){
		return PTR_ERR(gpioled.class);
	}

	/* 创建设备 */
	gpioled.device = device_create(gpioled.class, NULL, gpioled.devid, NULL, GPIOLED_NAME);
	if(IS_ERR(gpioled.device)){
		return PTR_ERR(gpioled.device);
	}

	return 0;
}

static void __exit gpio_led_exit(void)
{
	cdev_del(&gpioled.cdev);
	unregister_chrdev_region(gpioled.devid, GPIOLED_CNT);
	gpio_free(gpioled.led_gpio);
	device_destroy(gpioled.class, gpioled.devid);
    class_destroy(gpioled.class);
}

module_init(gpio_led_init);
module_exit(gpio_led_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("LiNing");