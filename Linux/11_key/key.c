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
#include <linux/semaphore.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define KEY_CNT 	1
#define KEY_NAME	"key"

#define KEY0_VALUE	0XF0
#define INVAKEY		0X00

struct key_dev {
	struct cdev cdev;
	struct class *class;
	struct device_node *nd;
	struct device *device;
	int key_gpio;
	int major;
	int minor;
	dev_t devid;
	atomic_t key_value;
};

struct key_dev key;

static int key_io_init(void)
{
	key.nd = of_find_node_by_path("/key");
	if (key.nd == NULL) {
		printk("can't find key node!\r\n");
		return -EINVAL;
	}

	key.key_gpio = of_get_named_gpio(key.nd, "key-gpio", 0);
	if (key.key_gpio < 0) {
		printk("can't get key-gpio!\r\n");
	}
	printk("key_gpio = %d!\r\n", key.key_gpio);

	gpio_request(key.key_gpio, "key0");
	gpio_direction_input(key.key_gpio);

	return 0;
}

static int key_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &key;
	return 0;
}

static ssize_t key_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	int ret = 0;
	unsigned char value;

	struct key_dev *dev = filp->private_data;
	if(gpio_get_value(dev->key_gpio) == 0) {
		while(!gpio_get_value(dev->key_gpio));
		atomic_set(&dev->key_value, KEY0_VALUE);
	} else {
		atomic_set(&dev->key_value, INVAKEY);
	}

	value = atomic_read(&dev->key_value);
	ret = copy_to_user(buf, &value, sizeof(value));
	return ret;
}

static struct file_operations key_fops = {
	.owner = THIS_MODULE,
	.open = key_open,
	.read = key_read,
};

static int __init my_key_init(void)
{
	if(key.major) {
		key.devid = MKDEV(key.major, 0);
		register_chrdev_region(key.devid, KEY_CNT, KEY_NAME);
	} else {
		alloc_chrdev_region(&key.devid, 0, KEY_CNT, KEY_NAME);
		key.major = MAJOR(key.devid);
		key.minor = MINOR(key.devid);
	}
	printk("key_dev_id = %d\r\n", key.devid);

	key.cdev.owner = THIS_MODULE;
	cdev_init(&key.cdev, &key_fops);
	cdev_add(&key.cdev, key.devid, KEY_CNT);

	key.class = class_create(THIS_MODULE, KEY_NAME);
	if (IS_ERR(key.class)) {
		return PTR_ERR(key.class);
	}

	key.device = device_create(key.class, NULL, key.devid, NULL, KEY_NAME);
	if (IS_ERR(key.device)) {
		return PTR_ERR(key.device);
	}

	if(!key_io_init()) {
		printk("gpio init!\r\n");
	}
	return 0;
}

static void __exit my_key_exit(void)
{
	cdev_del(&key.cdev);
	unregister_chrdev_region(key.devid, KEY_CNT);
	device_destroy(key.class, key.devid);
	class_destroy(key.class);
	gpio_free(key.key_gpio);
}

module_init(my_key_init);
module_exit(my_key_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("LiNing");