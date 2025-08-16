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
#include <linux/timer.h>
#include <linux/semaphore.h>
#include <linux/jiffies.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define TIMER_CNT 		1
#define TIMER_NAME 		"timer"
#define LEDOFF 			0
#define LEDON 			1

#define CLOSE_CMD		_IO(0XEF, 1)
#define OPEN_CMD		_IO(0XEF, 2)
#define SETPERIOD_CMD	_IOW(0XEF, 3, int)

struct timer_dev
{
	dev_t devid;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	struct device_node *nd;
	struct timer_list timer;
	int timeperiod;
	int major;
	int minor;
	int led_gpio;
	spinlock_t lock;
};

struct timer_dev timer;

static int timer_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &timer;
	return 0;
}

static long timer_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct timer_dev *dev = filp->private_data;
	int timerperiod;
	unsigned long flags;
	switch (cmd) {
		case CLOSE_CMD:
			del_timer_sync(&dev->timer);
			break;
		case OPEN_CMD:
			spin_lock_irqsave(&dev->lock, flags);
			timerperiod = dev->timeperiod;
			spin_unlock_irqrestore(&dev->lock, flags);
			mod_timer(&dev->timer, jiffies + msecs_to_jiffies(dev->timeperiod));
			break;
		case SETPERIOD_CMD:
			spin_lock_irqsave(&dev->lock, flags);
			dev->timeperiod = arg;
			spin_unlock_irqrestore(&dev->lock, flags);
			mod_timer(&dev->timer, jiffies + msecs_to_jiffies(arg));
			break;
		default:
			break;
	}
	return 0;
}


static int timer_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static struct file_operations timer_fops = {
	.owner = THIS_MODULE,
	.open = timer_open,
	.release = timer_release,
	.unlocked_ioctl = timer_ioctl,
};

void timer_func(unsigned long data)
{
	static int sta = 1;
	int timerperiod;
	unsigned long flags;
	struct timer_dev *dev = (struct timer_dev *)data;

	sta = !sta;
	gpio_set_value(dev->led_gpio, sta);
	spin_lock_irqsave(&dev->lock, flags);
	timerperiod = dev->timeperiod;
	spin_unlock_irqrestore(&dev->lock, flags);
	mod_timer(&dev->timer, jiffies + msecs_to_jiffies(dev->timeperiod));
}

static int __init timer_init(void)
{
	int ret;

	spin_lock_init(&timer.lock);

	/* 获取设备节点 */
	timer.nd = of_find_node_by_path("/gpioled");
	if(timer.nd == NULL) {
		printk("timer node can not found!\r\n");
		return -EINVAL;
	} else {
		printk("timer node has been found!\r\n");
	}

	/* 获取gpio标号 */
	timer.led_gpio = of_get_named_gpio(timer.nd, "led-gpio", 0);
	if(timer.led_gpio < 0) {
		printk("can't get led-gpio");
		return -EINVAL;
	}
	printk("led-gpio num = %d\r\n", timer.led_gpio);

	/* 申请gpio管脚 */
	ret = gpio_request(timer.led_gpio, "led_gpio");
	if (ret != 0) {
		printk("led_gpio can not request!\r\n");
	} else {
		printk("led_gpio request succesful!\r\n");
	}
	/* 设置gpio输出 */
	ret = gpio_direction_output(timer.led_gpio, 1);
	if (ret != 0) {
		printk("gpio_direction_output can not set!\r\n");
	} else {
		printk("gpio_direction_output set succesful!\r\n");
	}

	/* 申请设备号 */
	if (timer.major) {
		timer.devid = MKDEV(timer.major, 0);
		register_chrdev_region(timer.devid, TIMER_CNT, TIMER_NAME);
	} else {
		alloc_chrdev_region(&timer.devid, 0, TIMER_CNT, TIMER_NAME);
		timer.major = MAJOR(timer.devid);
		timer.minor = MINOR(timer.devid);
	}
	printk("timer major = %d, minor = %d\r\n", timer.major, timer.minor);

	/* 初始化驱动cdev */
	timer.cdev.owner = THIS_MODULE;
	cdev_init(&timer.cdev, &timer_fops);
	cdev_add(&timer.cdev, timer.devid, TIMER_CNT);

	/* 创建类 */
	timer.class = class_create(THIS_MODULE, TIMER_NAME);
	if(IS_ERR(timer.class)){
		return PTR_ERR(timer.class);
	}

	/* 创建设备 */
	timer.device = device_create(timer.class, NULL, timer.devid, NULL, TIMER_NAME);
	if(IS_ERR(timer.device)){
		return PTR_ERR(timer.device);
	}

	/* 初始化定时器 */
	init_timer(&timer.timer);
	/* 初始化定时器 */
	timer.timeperiod = 1000;
	timer.timer.data = (unsigned long)&timer;
	timer.timer.function = timer_func;

	return 0;
}

static void __exit timer_exit(void)
{
	gpio_set_value(timer.led_gpio, 1);
	del_timer_sync(&timer.timer);

	cdev_del(&timer.cdev);
	unregister_chrdev_region(timer.devid, TIMER_CNT);
	gpio_free(timer.led_gpio);
	device_destroy(timer.class, timer.devid);
    class_destroy(timer.class);
}

module_init(timer_init);
module_exit(timer_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("LiNing");