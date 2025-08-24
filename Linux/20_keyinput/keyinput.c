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
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define KEYINPUT_CNT    1
#define KEYINPUT_NAME   "keyinput"

struct keyinput_dev {
    struct device_node *nd;
    struct timer_list timer;
    struct input_dev *inputdev;
    int keygpio;
    int irq;
    int value;
    irqreturn_t (*handler)(int, void *);
};

static struct keyinput_dev keyinput;

static irqreturn_t keyinput_irq(int irq, void *dev_id)
{
    struct keyinput_dev *dev = (struct keyinput_dev *)dev_id;
	dev->timer.data = (volatile long)dev_id;
	mod_timer(&dev->timer, jiffies + msecs_to_jiffies(10));	/* 10ms定时 */
	return IRQ_RETVAL(IRQ_HANDLED);
}

void keyinput_timer(unsigned long arg)
{
	unsigned char value;
	struct keyinput_dev *dev = (struct keyinput_dev *)arg;

	value = gpio_get_value(dev->keygpio); 	/* 读取IO值 */
	if(value == 0){ 						/* 按下按键 */
		/* 上报按键值 */
		input_report_key(dev->inputdev, dev->value, 1);/* 最后一个参数表示按下还是松开，1为按下，0为松开 */
		input_sync(dev->inputdev);
	} else { 									/* 按键松开 */
		input_report_key(dev->inputdev, dev->value, 0);
		input_sync(dev->inputdev);
	}	
}

static int keyinput_probe(struct platform_device *pdev) 
{
    int ret = 0;
    printk("beep driver and device has matched!\r\n");
    /* 在设备树查找节点 */
    keyinput.nd = pdev->dev.of_node;
    if (keyinput.nd == NULL) {
        printk("keyinput node not find!\r\n");
        return -EINVAL;
    }
    /* 提取GPIO */
    keyinput.keygpio = of_get_named_gpio(keyinput.nd, "key-gpio", 0);
    if (keyinput.keygpio < 0) {
        printk("keyinput gpio not find!\r\n");
        return -EINVAL;
    }
    /* 初始化GPIO */
    gpio_request(keyinput.keygpio, KEYINPUT_NAME);
    gpio_direction_input(keyinput.keygpio);
    /* 申请中断 */
    keyinput.irq = irq_of_parse_and_map(keyinput.nd, 0);
    if (!keyinput.irq) {
        return -EINVAL;
    }
    keyinput.value = KEY_0;
    keyinput.handler = keyinput_irq;
    ret = request_irq(keyinput.irq, keyinput.handler, IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING, KEYINPUT_NAME, &keyinput); // 中断号 中断处理函数 中断标志 中断名 *dev
    if (ret < 0) {
        printk("irq %d request failed!\r\n", keyinput.irq);
			return -EFAULT;
    }
    /* 创建定时器 */
    init_timer(&keyinput.timer);
    keyinput.timer.function = keyinput_timer;
    /* 申请input_dev */
    keyinput.inputdev = input_allocate_device();
    keyinput.inputdev->name = KEYINPUT_NAME;
    /* 初始化input_dev，设置产生哪些事件 */
	__set_bit(EV_KEY, keyinput.inputdev->evbit);	/* 设置产生按键事件          */
	__set_bit(EV_REP, keyinput.inputdev->evbit);	/* 重复事件，比如按下去不放开，就会一直输出信息 		 */

	/* 初始化input_dev，设置产生哪些按键 */
	__set_bit(KEY_0, keyinput.inputdev->keybit);
    ret = input_register_device(keyinput.inputdev);	
    if (ret) {
		printk("register input device failed!\r\n");
		return ret;
	}
    return 0;
}

static int keyinput_remove(struct platform_device *pdev) 
{
    gpio_free(keyinput.keygpio);
    /* 删除定时器 */
	del_timer_sync(&keyinput.timer);	/* 删除定时器 */
		
	/* 释放中断 */
	free_irq(keyinput.irq, &keyinput);
	/* 释放input_dev */
	input_unregister_device(keyinput.inputdev);
	input_free_device(keyinput.inputdev);
    printk("keyinput removed\r\n");
    return 0;
}

static const struct of_device_id keyinput_of_match[] = {
    { .compatible = "atkmini-key" },
    { /* sentinel */ },
};

static struct platform_driver keydriver = {
    .driver     = {
        .name           = "keyinput",
        .of_match_table = keyinput_of_match,
    },
    .probe              = keyinput_probe,
    .remove             = keyinput_remove,
};

static int __init keyinput_init(void)
{
    return(platform_driver_register(&keydriver));
}

static void __exit keyinput_exit(void)
{
    platform_driver_unregister(&keydriver);
}

module_init(keyinput_init);
module_exit(keyinput_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LiNing");