#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>

#define CHARDEV_BASE_MAJOR  200             /* 主设备号 */
#define CHARDEV_BASE_NAME   "chrdev_base"   /* 设备名称 */

static char readbuf[100]
static char writebuf[100]
static char kerneldata[] = {"kernel data!"};

/*
 * @description		: 打开设备
 * @param - inode 	: 传递给驱动的inode
 * @param - filp 	: 设备文件，file结构体有个叫做private_data的成员变量
 * 					  一般在open的时候将private_data指向设备结构体。
 * @return 			: 0 成功;其他 失败
 */
static int chrdevbase_open(struct inode *inode, struct file *filp)
{
	//printk("chrdevbase open!\r\n");
	return 0;
}

static int chrdevbase_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	int ret = 0;
    memcpy(readbuf, kerneldata, sizeof(kerneldata));
    ret = copy_to_user(buf, readbuf, cnt);
    if(ret == 0){
		printk("kernel senddata ok!\r\n");
	}else{
		printk("kernel senddata failed!\r\n");
	}

	return 0;
}

static ssize_t chrdevbase_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
	int retvalue = 0;
	/* 接收用户空间传递给内核的数据并且打印出来 */
	retvalue = copy_from_user(writebuf, buf, cnt);
	if(retvalue == 0){
		printk("kernel recevdata:%s\r\n", writebuf);
	}else{
		printk("kernel recevdata failed!\r\n");
	}
	
	// printk("chrdevbase write!\r\n");
	return 0;
}

static int chrdevbase_release(struct inode *inode, struct file *filp)
{
	//printk("chrdevbase open!\r\n");
	return 0;
}

/*
 * 设备操作函数结构体
 */
static struct file_operations chrdev_fops = {
	.owner = THIS_MODULE,	
	.open = chrdevbase_open,
	.read = chrdevbase_read,
	.write = chrdevbase_write,
	.release = chrdevbase_release,
};

/*
 * @description	: 驱动入口函数 
 * @param 		: 无
 * @return 		: 0 成功;其他 失败
 */
static void __init chr_dev_base_init()
{
    int reg = 0;
    /* 注册字符设备驱动 */
    reg = register_chrdev(CHARDEV_BASE_MAJOR, CHARDEV_BASE_NAME, &chrdev_fops);
    if (reg < 0)
    {
        printk("chrdevbase driver register failed\r\n");
        return 1;
    } else {
        printk("chrdevbase init OK!\r\n");
        return 0;
    }

}

/*
 * @description	: 驱动出口函数
 * @param 		: 无
 * @return 		: 无
 */
static void __exit chr_dev_base_exit()
{
    /* 注销字符设备驱动 */
    unregister_chrdev(CHARDEV_BASE_MAJOR, CHARDEV_BASE_NAME);
}
/* 
 * 将上面两个函数指定为驱动的入口和出口函数 
 */
module_init(chr_dev_base_init);
module_exit(chr_dev_base_exit);

/* 
 * LICENSE和作者信息
 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("LiNing");
