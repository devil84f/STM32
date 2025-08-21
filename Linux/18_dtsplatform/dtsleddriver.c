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
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define NEWLED_CNT		1			/* 设备数量 */
#define NEWLED_NAME		"dtsplatled" 	/* 设备名字 */
#define LEDOFF 			0			/* 关灯    */
#define LEDON 			1			/* 开灯    */

struct platled_dev {
	dev_t devid;					/* 设备号 	 */
	struct cdev cdev;				/* 字符设备  */
	struct class *class;			/* 类 		*/
	struct device *device;			/* 设备 	*/
	int major;						/* 主设备号  */
	int minor;						/* 次设备号  */
	struct device_node *nd;			/* 设备节点  */
	int ledgpio;					/* gpio号   */
};		

struct platled_dev dtsled; 			/* LED设备  */

/*
 * @description		: LED打开/关闭
 * @param - sta 	: LEDON(0) 打开LED，LEDOFF(1) 关闭LED
 * @return 			: 无
 */
void led_switch(u8 sta)
{
	if(sta == LEDON) {
		gpio_set_value(dtsled.ledgpio, 0);
	}else if(sta == LEDOFF) {
		gpio_set_value(dtsled.ledgpio, 1);
	}	
}

/*
 * @description		: 打开设备
 * @param - inode 	: 传递给驱动的inode
 * @param - filp 	: 设备文件，file结构体有个叫做private_data的成员变量
 * 					  一般在open的时候将private_data指向设备结构体。
 * @return 			: 0 成功;其他 失败
 */
static int led_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &dtsled;
	return 0;
}

/*
 * @description		: 从设备读取数据 
 * @param - filp 	: 要打开的设备文件(文件描述符)
 * @param - buf 	: 返回给用户空间的数据缓冲区
 * @param - cnt 	: 要读取的数据长度
 * @param - offt 	: 相对于文件首地址的偏移
 * @return 			: 读取的字节数，如果为负值，表示读取失败
 */
static ssize_t led_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	return 0;
}

/*
 * @description		: 向设备写数据 
 * @param - filp 	: 设备文件，表示打开的文件描述符
 * @param - buf 	: 要写给设备写入的数据
 * @param - cnt 	: 要写入的数据长度
 * @param - offt 	: 相对于文件首地址的偏移
 * @return 			: 写入的字节数，如果为负值，表示写入失败
 */
static ssize_t led_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
	int retvalue;
	unsigned char databuf[1];
	unsigned char ledstat;

	retvalue = copy_from_user(databuf, buf, cnt);
	if(retvalue < 0) {
		printk("kernel write failed!\r\n");
		return -EFAULT;
	}

	ledstat = databuf[0];		/* 获取状态值 */

	if(ledstat == LEDON) {	
		led_switch(LEDON);		/* 打开LED灯 */
	} else if(ledstat == LEDOFF) {
		led_switch(LEDOFF);	/* 关闭LED灯 */
	}
	return 0;
}

/*
 * @description		: 关闭/释放设备
 * @param - filp 	: 要关闭的设备文件(文件描述符)
 * @return 			: 0 成功;其他 失败
 */
static int led_release(struct inode *inode, struct file *filp)
{
	return 0;
}

/* 设备操作函数 */
static struct file_operations dtsled_fops = {
	.owner = THIS_MODULE,
	.open = led_open,
	.read = led_read,
	.write = led_write,
	.release = 	led_release,
};

static int leddriver_probe(struct platform_device *dev)
{
	printk("led driver and device has matched!\r\n");

	/* 6、创建设备号 */
	if(dtsled.major){
		dtsled.devid = MKDEV(dtsled.major, 0);
		register_chrdev_region(dtsled.devid, NEWLED_CNT, NEWLED_NAME);
	} else {
		alloc_chrdev_region(&dtsled.devid, 0, NEWLED_CNT, NEWLED_NAME);
		dtsled.major = MAJOR(dtsled.devid);
		dtsled.minor = MINOR(dtsled.devid);
	}
	printk("dtsled major = %d, minor = %d\r\n", dtsled.major, dtsled.minor);

	/* 7、初始化cdev */
	dtsled.cdev.owner = THIS_MODULE;
	cdev_init(&dtsled.cdev, &dtsled_fops);

	/* 8、添加cdev */
	cdev_add(&dtsled.cdev, dtsled.devid, NEWLED_CNT);

	/* 7、创建类 */
	dtsled.class = class_create(THIS_MODULE, NEWLED_NAME);
	if(IS_ERR(dtsled.class)){
		return PTR_ERR(dtsled.class);
	}

	/* 7、创建设备 */
	dtsled.device = device_create(dtsled.class, NULL, dtsled.devid, NULL, NEWLED_NAME);
	if(IS_ERR(dtsled.device)){
		return PTR_ERR(dtsled.device);
	}

	dtsled.nd = of_find_node_by_path("/gpioled");
	if(dtsled.nd == NULL) {
		printk("gpio node not find!\r\n");
		return -EINVAL;
	}

	dtsled.ledgpio = of_get_named_gpio(dtsled.nd, "led-gpio", 0);
	if(dtsled.ledgpio < 0) {
		printk("led-gpio not find!\r\n");
		return -EINVAL;
	}

	gpio_request(dtsled.ledgpio, "ledgpio");
	gpio_direction_output(dtsled.ledgpio, 1);
	return 0;
}

static int leddriver_remove(struct platform_device *dev)
{
	gpio_set_value(dtsled.ledgpio, 1);
	gpio_free(dtsled.ledgpio);
	/* 注销字符设备驱动 */
	cdev_del(&dtsled.cdev);
	unregister_chrdev_region(dtsled.devid, NEWLED_CNT);
	device_destroy(dtsled.class, dtsled.devid);
	class_destroy(dtsled.class);

	return 0;
}

static const struct of_device_id led_of_match[] = {
	{.compatible	= "atkmini-gpioled"},
	{ /* sentinel */ },
};

static struct platform_driver led_driver = {
	.driver 			= {
		.name 			= "imx6ul-led",
		.of_match_table = led_of_match,
	},
	.probe 				= leddriver_probe,
	.remove 			= leddriver_remove,
};

/*
 * @description	: 驱动模块加载函数
 * @param 		: 无
 * @return 		: 无
 */
static int __init leddriver_init(void)
{
	return platform_driver_register(&led_driver);
}

/*
 * @description	: 驱动模块卸载函数
 * @param 		: 无
 * @return 		: 无
 */
static void __exit leddriver_exit(void)
{
	platform_driver_unregister(&led_driver);
}

module_init(leddriver_init);
module_exit(leddriver_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("LiNing");
