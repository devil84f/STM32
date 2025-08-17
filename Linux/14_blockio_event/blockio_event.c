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
#include <linux/of_irq.h>
#include <linux/irq.h>
#include <linux/semaphore.h>
#include <linux/jiffies.h>
#include <linux/interrupt.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define IMX6UIRQ_CNT 		1
#define IMX6UIRQ_NAME 		"blockio_event"
#define KEY0VALUE 			0X01
#define INVAKEY 			0XFF
#define KEY_NUM 			1

struct irq_keydesc {
	int gpio;
	int irqnum;
	unsigned char value;
	char name[10];
	irqreturn_t (*handler)(int, void *);
};

struct imx6uirq_dev
{
	dev_t devid;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	struct device_node *nd;
	struct timer_list timer;
	struct irq_keydesc irqkeydesc[KEY_NUM];
	wait_queue_head_t r_wait;
	int timeperiod;
	int major;
	int minor;
	unsigned char curkeynum;
	atomic_t keyvalue;
	atomic_t releasekey;
};

struct imx6uirq_dev imx6uirq;

static int imx6uirq_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &imx6uirq;
	return 0;
}

static ssize_t imx6uirq_read(struct file* filp, char __user *buf, size_t cnt, loff_t *offt)
{
	int ret = 0;
	unsigned char keyvalue = 0;
	unsigned char releasekey = 0;
	struct imx6uirq_dev *dev = (struct imx6uirq_dev *)filp->private_data;
	/* 等待事件触发 */
	wait_event_interruptible(dev->r_wait, atomic_read(&dev->releasekey));

	keyvalue = atomic_read(&dev->keyvalue);
	releasekey = atomic_read(&dev->releasekey);	

	if (releasekey) { /* 有按键按下 */
		if (keyvalue & 0x80) {
			keyvalue &= ~0x80;
			ret = copy_to_user(buf, &keyvalue, sizeof(keyvalue));
		} else {
			goto data_error;
		}
		atomic_set(&dev->releasekey, 0); /* 按下标志清零 */
	} else {
		goto data_error;
	}
	return 0;

data_error:
	return -EINVAL;
}

static struct file_operations imx6uirq_fops = {
	.owner 	= THIS_MODULE,
	.open 	= imx6uirq_open,
	.read 	= imx6uirq_read,
};

static irqreturn_t key0_handler(int irq, void *dev_id)
{
	struct imx6uirq_dev *dev = (struct imx6uirq_dev *)dev_id;
	dev->curkeynum = 0;
	dev->timer.data = (volatile long)dev_id;
	mod_timer(&dev->timer, jiffies + msecs_to_jiffies(10));
	return IRQ_RETVAL(IRQ_HANDLED);
}

void timer_function(unsigned long arg)
{
	unsigned char value;
	unsigned char num;
	struct irq_keydesc *keydesc;
	struct imx6uirq_dev *dev = (struct imx6uirq_dev *)arg;
	num = dev->curkeynum;
	keydesc = &dev->irqkeydesc[num];
	value = gpio_get_value(keydesc->gpio);
	if(value == 0){
		atomic_set(&dev->keyvalue, keydesc->value);
	}
	else{ 
		atomic_set(&dev->keyvalue, 0x80 | keydesc->value);
		atomic_set(&dev->releasekey, 1);
	}

	/* 唤醒进程 */
	if(atomic_read(&dev->releasekey))
	{
		wake_up(&dev->r_wait);
	}
}

static int key_io_init(void)
{
	int ret;
	int i;

	/* 获取设备节点 */
	imx6uirq.nd = of_find_node_by_path("/key");
	if(imx6uirq.nd == NULL) {
		printk("imx6uirq node can not found!\r\n");
		return -EINVAL;
	} else {
		printk("imx6uirq node has been found!\r\n");
	}

	/* 获取gpio标号 */
	for (i = 0; i < KEY_NUM; i++){
		imx6uirq.irqkeydesc[i].gpio = of_get_named_gpio(imx6uirq.nd, "key-gpio", i);
		if(imx6uirq.irqkeydesc[i].gpio < 0) {
			printk("can't get irqkeydesc[%d].gpio\r\n", i);
			return -EINVAL;
		}

		/* 申请gpio管脚 */
		memset(imx6uirq.irqkeydesc[i].name, 0, sizeof(imx6uirq.irqkeydesc[i].name));
		sprintf(imx6uirq.irqkeydesc[i].name, "KEY%d", i);
		ret = gpio_request(imx6uirq.irqkeydesc[i].gpio, imx6uirq.irqkeydesc[i].name);
		imx6uirq.irqkeydesc[i].irqnum = irq_of_parse_and_map(imx6uirq.nd, 0);
		printk("key%d:gpio=%d, irqnum=%d\r\n", i, imx6uirq.irqkeydesc[i].gpio, imx6uirq.irqkeydesc[i].irqnum);
	}
	imx6uirq.irqkeydesc[0].handler = key0_handler;
	imx6uirq.irqkeydesc[0].value = KEY0VALUE;
	ret = request_irq(imx6uirq.irqkeydesc[0].irqnum,
						imx6uirq.irqkeydesc[0].handler,
						IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING,
						imx6uirq.irqkeydesc[0].name, &imx6uirq);
	init_timer(&imx6uirq.timer);
	imx6uirq.timer.function = timer_function;
	return 0;
}

static int __init imx6uirq_init(void)
{
	/* 申请设备号 */
	if (imx6uirq.major) {
		imx6uirq.devid = MKDEV(imx6uirq.major, 0);
		register_chrdev_region(imx6uirq.devid, IMX6UIRQ_CNT, IMX6UIRQ_NAME);
	} else {
		alloc_chrdev_region(&imx6uirq.devid, 0, IMX6UIRQ_CNT, IMX6UIRQ_NAME);
		imx6uirq.major = MAJOR(imx6uirq.devid);
		imx6uirq.minor = MINOR(imx6uirq.devid);
	}
	printk("imx6uirq major = %d, minor = %d\r\n", imx6uirq.major, imx6uirq.minor);

	/* 初始化驱动cdev */
	imx6uirq.cdev.owner = THIS_MODULE;
	cdev_init(&imx6uirq.cdev, &imx6uirq_fops);
	cdev_add(&imx6uirq.cdev, imx6uirq.devid, IMX6UIRQ_CNT);

	/* 创建类 */
	imx6uirq.class = class_create(THIS_MODULE, IMX6UIRQ_NAME);
	if(IS_ERR(imx6uirq.class)){
		return PTR_ERR(imx6uirq.class);
	}

	/* 创建设备 */
	imx6uirq.device = device_create(imx6uirq.class, NULL, imx6uirq.devid, NULL, IMX6UIRQ_NAME);
	if(IS_ERR(imx6uirq.device)){
		return PTR_ERR(imx6uirq.device);
	}

	/* 初始化按键 */
	key_io_init();
	atomic_set(&imx6uirq.keyvalue, INVAKEY);
	atomic_set(&imx6uirq.releasekey, 0);

	/* 初始化等待队列头 */
	init_waitqueue_head(&imx6uirq.r_wait);

	return 0;
}

static void __exit imx6uirq_exit(void)
{
	int i;
	del_timer_sync(&imx6uirq.timer);
	for (i = 0; i < KEY_NUM; i++) {
		free_irq(imx6uirq.irqkeydesc[i].irqnum, &imx6uirq);
		gpio_free(imx6uirq.irqkeydesc[i].gpio);
	}
	cdev_del(&imx6uirq.cdev);
	unregister_chrdev_region(imx6uirq.devid, IMX6UIRQ_CNT);
	device_destroy(imx6uirq.class, imx6uirq.devid);
    class_destroy(imx6uirq.class);
}

module_init(imx6uirq_init);
module_exit(imx6uirq_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("LiNing");