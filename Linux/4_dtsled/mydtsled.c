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
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define NEWLED_CNT		1			/* 设备数量 */
#define NEWLED_NAME		"mydtsled" 	/* 设备名字 */
#define LEDOFF 	0					/* 关灯    */
#define LEDON 	1					/* 开灯    */

/* 映射后的寄存器虚拟地址指针 */
static void __iomem *IMX6U_CCM_CCGR1;
static void __iomem *SW_MUX_GPIO1_IO03;
static void __iomem *SW_PAD_GPIO1_IO03;
static void __iomem *GPIO1_DR;
static void __iomem *GPIO1_GDIR;

/* 设备结构体 */
struct mydtsled_dev {
    struct cdev c_dev;      /* 字符设备  */
    dev_t dev_num;          /* 设备号    */
    struct class *class;    /* 类       */
    struct device *device;  /* 设备     */
    int major;              /* 主设备号  */
    int minor;              /* 次设备号  */
    struct device_node *nd; /* 设备节点  */
};

struct mydtsled_dev mydtsled;

/*
 * @description		: LED打开/关闭
 * @param - sta 	: LEDON(0) 打开LED，LEDOFF(1) 关闭LED
 * @return 			: 无
 */
void led_switch(u8 sta)
{
	u32 val = 0;
	if(sta == LEDON) {
		val = readl(GPIO1_DR);
		val &= ~(1 << 3);	
		writel(val, GPIO1_DR);
	}else if(sta == LEDOFF) {
		val = readl(GPIO1_DR);
		val|= (1 << 3);	
		writel(val, GPIO1_DR);
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
	filp->private_data = &mydtsled;
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
static struct file_operations mydtsled_fops = {
	.owner = THIS_MODULE,
	.open = led_open,
	.read = led_read,
	.write = led_write,
	.release = 	led_release,
};

/* 驱动安装、卸载函数实现 */
/*
 * @description	: 驱动入口函数
 * @param 		: 无
 * @return 		: 无
 */
static int __init my_dts_led_init(void)
{
    u32 val = 0;
    int ret;
    const char *str;
    u32 regdata[14];
    struct property *proper;

    /* 1、从设备树获取设备节点 */
    mydtsled.nd = of_find_node_by_path("/atkmini");
    if(mydtsled.nd == NULL) {
        printk("atkmini node can not found!\r\n");
		return -EINVAL;
    } else {
        printk("atkmini node has been found!\r\n");
    }

    /* 2、获取 compatible 属性内容 */
    proper = of_find_property(mydtsled.nd, "compatible", NULL);
    if(proper == NULL) {
		printk("compatible property find failed\r\n");
	} else {
		printk("compatible = %s\r\n", (char*)proper->value);
	}
    ret = of_property_read_string(mydtsled.nd, "compatible", &str);
    if(ret < 0) {
		printk("compatible read failed!\r\n");
	} else {
		printk("compatible = %s\r\n",str);
	}

    /* 3、获取 status 属性内容 */
    ret = of_property_read_string(mydtsled.nd, "status", &str);
    if(ret < 0) {
		printk("status read failed!\r\n");
	} else {
		printk("status = %s\r\n",str);
	}
    proper = of_find_property(mydtsled.nd, "reg", NULL);
    if(proper == NULL) {
		printk("of_find_property reg property find failed\r\n");
	} else {
		const u32 *reg_data = proper->value; // 使用一个有类型的指针，更清晰、安全
        int num_regs = proper->length / sizeof(u32); // 计算 u32 值的数量
        int i;

        printk("reg property found, length = %d bytes, contains %d u32 values:\r\n", 
            proper->length, num_regs);

        for(i = 0; i < num_regs; i++) {
            // 使用新指针进行访问，并打印换行符以改善格式
            printk("%#X ", reg_data[i]);
        }
        printk("\r\n"); // 在 Linux 内核中，通常使用 \n 而不是 \r\n
	}

    /* 4、获取 reg 属性内容 */
    ret = of_property_read_u32_array(mydtsled.nd, "reg", regdata, 10);
    if (ret < 0) {
        printk("reg read failed!\r\n");
	} else {
		u8 i = 0;
		printk("reg data:\r\n");
		for(i = 0; i < 10; i++)
			printk("%#X ", regdata[i]);
		printk("\r\n");
	}

    /* 5、初始化 LED */
    IMX6U_CCM_CCGR1 = of_iomap(mydtsled.nd, 0);
    SW_MUX_GPIO1_IO03 = of_iomap(mydtsled.nd, 1);
    SW_PAD_GPIO1_IO03 = of_iomap(mydtsled.nd, 2);
    GPIO1_DR = of_iomap(mydtsled.nd, 3);
    GPIO1_GDIR = of_iomap(mydtsled.nd, 4);

    /* 1、初始化LED */
	/* 2、使能GPIO1时钟 */
	val = readl(IMX6U_CCM_CCGR1);
	val &= ~(3 << 26);	/* 清楚以前的设置 */
	val |= (3 << 26);	/* 设置新值 */
	writel(val, IMX6U_CCM_CCGR1);

	/* 3、设置GPIO1_IO03的复用功能，将其复用为
	 *    GPIO1_IO03，最后设置IO属性。
	 */
	writel(5, SW_MUX_GPIO1_IO03);
	
	/*寄存器SW_PAD_GPIO1_IO03设置IO属性
	 *bit 16:0 HYS关闭
	 *bit [15:14]: 00 默认下拉
     *bit [13]: 0 kepper功能
     *bit [12]: 1 pull/keeper使能
     *bit [11]: 0 关闭开路输出
     *bit [7:6]: 10 速度100Mhz
     *bit [5:3]: 110 R0/6驱动能力
     *bit [0]: 0 低转换率
	 */
	writel(0x10B0, SW_PAD_GPIO1_IO03);

	/* 4、设置GPIO1_IO03为输出功能 */
	val = readl(GPIO1_GDIR);
	val &= ~(1 << 3);	/* 清除以前的设置 */
	val |= (1 << 3);	/* 设置为输出 */
	writel(val, GPIO1_GDIR);

	/* 5、默认关闭LED */
	val = readl(GPIO1_DR);
	val |= (1 << 3);	
	writel(val, GPIO1_DR);

    if(mydtsled.major) {
        mydtsled.dev_num = MKDEV(mydtsled.major, 0);
        register_chrdev_region(mydtsled.dev_num, NEWLED_CNT, NEWLED_NAME);
    } else {
        alloc_chrdev_region(&mydtsled.dev_num, 0, NEWLED_CNT, NEWLED_NAME);
		mydtsled.major = MAJOR(mydtsled.dev_num);
		mydtsled.minor = MINOR(mydtsled.dev_num);
    }
    printk("mydtsled major = %d, minor = %d\r\n", mydtsled.major, mydtsled.minor);

    /* 7、初始化cdev */
	mydtsled.c_dev.owner = THIS_MODULE;
	cdev_init(&mydtsled.c_dev, &mydtsled_fops);

    /* 8、添加cdev */
	cdev_add(&mydtsled.c_dev, mydtsled.dev_num, NEWLED_CNT);

    /* 7、创建类 */
	mydtsled.class = class_create(THIS_MODULE, NEWLED_NAME);
	if(IS_ERR(mydtsled.class)){
		return PTR_ERR(mydtsled.class);
	}

	/* 7、创建设备 */
	mydtsled.device = device_create(mydtsled.class, NULL, mydtsled.dev_num, NULL, NEWLED_NAME);
	if(IS_ERR(mydtsled.device)){
		return PTR_ERR(mydtsled.device);
	}

    return 0;
}

/*
 * @description	: 驱动出口函数
 * @param 		: 无
 * @return 		: 无
 */
static void __exit my_dts_led_exit(void)
{
    /* 取消映射 */
	iounmap(IMX6U_CCM_CCGR1);
	iounmap(SW_MUX_GPIO1_IO03);
	iounmap(SW_PAD_GPIO1_IO03);
	iounmap(GPIO1_DR);
	iounmap(GPIO1_GDIR);

	/* 注销字符设备驱动 */
	cdev_del(&mydtsled.c_dev);
	unregister_chrdev_region(mydtsled.dev_num, NEWLED_CNT);
	device_destroy(mydtsled.class, mydtsled.dev_num);
	class_destroy(mydtsled.class);
}

/* 注册驱动安装、卸载函数 */
module_init(my_dts_led_init);
module_exit(my_dts_led_exit);

/* 添加LICENSE和AUTHOR信息 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("LiNing");
