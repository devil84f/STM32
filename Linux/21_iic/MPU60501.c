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
#include <linux/i2c.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include "mpu6050_reg.h"

#define DEV_NAME "mpu6050"
#define DEV_CNT 1

static dev_t mpu6050_devno;
static struct cdev mpu6050_chr_dev;
static struct class *class_mpu6050;
static struct device *device_mpu6050;
static struct i2c_client *mpu6050_client;

static int i2c_write_mpu6050(struct i2c_client *mpu6050_client, u8 address, u8 data)
{
	int error = 0;
	u8 write_data[2];
	struct i2c_msg send_msg;	//待发送的数据结构体
	
	write_data[0] = address;	//设置要发送的数据
	write_data[1] = data;
	
	/*发送数据要写入地址reg*/
	send_msg.addr = mpu6050_client->addr;	//mpu6050在i2c总线上的地址
	send_msg.flags = 0;
	send_msg.buf = write_data;				//写入的首地址
	send_msg.len = 2;						//msg长度
 
	error = i2c_transfer(mpu6050_client->adapter, &send_msg, 1);
	if (error != 1)	{
		printk(KERN_DEBUG "i2c transfer error\n");
		return -1;
	}
	return 0;
 
}
 
static int i2c_read_mpu6050(struct i2c_client *mpu6050_client, u8 address, void *data, u32 length)
{
	int error = 0;
	u8 address_data = address;
	struct i2c_msg mpu6050_msg[2];
 
	/*设置读取位置msg*/
	mpu6050_msg[0].addr = mpu6050_client->addr; //mpu6050在 iic 总线上的地址
	mpu6050_msg[0].flags = 0;					//标记为发送数据
	mpu6050_msg[0].buf = &address_data;			//写入的首地址
	mpu6050_msg[0].len = 1;						//写入长度
 
	/*设置读取位置msg*/
	mpu6050_msg[1].addr = mpu6050_client->addr; //mpu6050在 iic 总线上的地址
	mpu6050_msg[1].flags = I2C_M_RD;			//标记为读取数据
	mpu6050_msg[1].buf = data;					//读取得到的数据保存位置
	mpu6050_msg[1].len = length;				//读取长度
 
	error = i2c_transfer(mpu6050_client->adapter, mpu6050_msg, 2);
	if (error != 2) {
		printk(KERN_DEBUG "\n i2c read mpu6050 error\n");
		return -1;
	}
	return 0;
}
 
static void mpu6050_init(void)
{
    i2c_write_mpu6050(mpu6050_client, MPU6050_PWR_MGMT_1, 0x00);
    i2c_write_mpu6050(mpu6050_client, MPU6050_PWR_MGMT_2, 0x00);
    i2c_write_mpu6050(mpu6050_client, MPU6050_SMPLRT_DIV, 0x09);
    i2c_write_mpu6050(mpu6050_client, MPU6050_CONFIG, 0x06);
    i2c_write_mpu6050(mpu6050_client, MPU6050_GYRO_CONFIG, 0x18);
    i2c_write_mpu6050(mpu6050_client, MPU6050_ACCEL_CONFIG, 0x18);
}

static int mpu6050_open(struct inode *inode, struct file *filp)
{
	mpu6050_init();
	return 0;
}
 
static int mpu6050_read(struct file *filp, char __user *buf, size_t cnt, loff_t *off)
{
	char data_H;
	char data_L;
	int error;
	//保存mpu6050转换得到的原始数据
	short mpu6050_result[6] = {0};
 
	/*读取3轴加速度原始值*/
	i2c_read_mpu6050(mpu6050_client, MPU6050_ACCEL_XOUT_H, &data_H, 1);
	i2c_read_mpu6050(mpu6050_client, MPU6050_ACCEL_XOUT_L, &data_L, 1);
	mpu6050_result[0] = data_H << 8;
	mpu6050_result[0] += data_L;
 
	i2c_read_mpu6050(mpu6050_client, MPU6050_ACCEL_YOUT_H, &data_H, 1);
	i2c_read_mpu6050(mpu6050_client, MPU6050_ACCEL_YOUT_L, &data_L, 1);
	mpu6050_result[1] = data_H << 8;
    mpu6050_result[1] += data_L;
 
	i2c_read_mpu6050(mpu6050_client, MPU6050_ACCEL_ZOUT_H, &data_H, 1);
	i2c_read_mpu6050(mpu6050_client, MPU6050_ACCEL_ZOUT_L, &data_L, 1);
	mpu6050_result[2] = data_H << 8;
	mpu6050_result[2] += data_L;
 
	/*读取3轴角速度原始值*/
	i2c_read_mpu6050(mpu6050_client, MPU6050_GYRO_XOUT_H, &data_H, 1);
	i2c_read_mpu6050(mpu6050_client, MPU6050_GYRO_XOUT_L, &data_L, 1);
	mpu6050_result[3] = data_H << 8;
	mpu6050_result[3] += data_L;
 
	i2c_read_mpu6050(mpu6050_client, MPU6050_GYRO_YOUT_H, &data_H, 1);
	i2c_read_mpu6050(mpu6050_client, MPU6050_GYRO_YOUT_L, &data_L, 1);
	mpu6050_result[4] = data_H << 8;
	mpu6050_result[4] += data_L;
 
	i2c_read_mpu6050(mpu6050_client, MPU6050_GYRO_ZOUT_H, &data_H, 1);
	i2c_read_mpu6050(mpu6050_client, MPU6050_GYRO_ZOUT_L, &data_L, 1);
	mpu6050_result[5] = data_H << 8;
	mpu6050_result[5] += data_L;
 
 
	printk("kernel:AX=%d, AY=%d, AZ=%d \n",(int)mpu6050_result[0],(int)mpu6050_result[1],(int)mpu6050_result[2]);
	printk("kernel:GX=%d, GY=%d, GZ=%d \n \n",(int)mpu6050_result[3],(int)mpu6050_result[4],(int)mpu6050_result[5]);
 
	/*将读取得到的数据拷贝到用户空间*/
	//error = copy_to_user(buf, mpu6050_result, sizeof(mpu6050_result));
	printk("cnt:%d\n", cnt);
	error = copy_to_user(buf, mpu6050_result, cnt);
 
	if(error != 0)
	{
		printk("copy_to_user error:%x!", error);
		return -1;
	}
 
	return 0;
}
 
static int mpu6050_release(struct inode *inode, struct file *filp)
{
	printk("\nmpu6050_release \n");
	return 0;
}
 
static struct file_operations mpu6050_chr_dev_fops = {
	.owner = THIS_MODULE,
	.open = mpu6050_open,
	.read = mpu6050_read,
	.release = mpu6050_release,
};

static int mpu6050_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = -1;
	printk(KERN_EMERG "\t match success !!\n");
	
	//动态分配来获取设备编号， 次设备号为0.可通过cat /proc/devices的方式查看
	//DEV_CNT为1，当前只申请一个设备编号
	ret = alloc_chrdev_region(&mpu6050_devno, 0, DEV_CNT, DEV_NAME);
	if (ret < 0) {
		printk("fail to alloc mpu6050_devno\n");
		goto alloc_err;
	}
	
	//关联字符设备结构体cdev与文件操作结构体
	mpu6050_chr_dev_fops.owner = THIS_MODULE;
	cdev_init(&mpu6050_chr_dev, &mpu6050_chr_dev_fops);
 
	//将设备添加进cdev_map列表中
	ret = cdev_add(&mpu6050_chr_dev, mpu6050_devno, DEV_CNT);
	if (ret < 0) {
		printk("fail to add cdev\n");
		goto add_err;
	}
 
	class_mpu6050 = class_create(THIS_MODULE, DEV_NAME);
 
	device_mpu6050 = device_create(class_mpu6050, NULL, mpu6050_devno, NULL, DEV_NAME);
	mpu6050_client = client;
	return 0;
 
add_err:
	// 添加设备失败时，需要注销设备号
	unregister_chrdev_region(mpu6050_devno, DEV_CNT);
	printk("\n error! \n");
alloc_err:
 
	return -1;
}
 
static int mpu6050_remove(struct i2c_client *client)
{
	device_destroy(class_mpu6050, mpu6050_devno);
	class_destroy(class_mpu6050);
	cdev_del(&mpu6050_chr_dev);	//清除设备号
	unregister_chrdev_region(mpu6050_devno, DEV_CNT);	//取消注册字符设备
	return 0;
}

/*定义ID 匹配表*/
static const struct i2c_device_id gtp_device_id[] = {
	{"alientek,mpu6050", 0},
	{}
};
 
/*定义设备树匹配表*/
static const struct of_device_id mpu6050_of_match_table[] = {
	{.compatible = "alientek,mpu6050"},
	{/*预留*/}
};
 
/*定义i2c总线设备结构体*/
struct i2c_driver mpu6050_driver = {
	.probe 		= mpu6050_probe,
	.remove 	= mpu6050_remove,
	.id_table 	= gtp_device_id,
	.driver 	= {
		.name = "mpu6050",
		.owner	= THIS_MODULE,
		.of_match_table = mpu6050_of_match_table,
	},
};

/*驱动初始化函数*/
static int __init mpu6050_driver_init(void)
{
	int ret;
	pr_info("mpu6050_driver_init\n");
	ret = i2c_add_driver(&mpu6050_driver);
	return ret;
}
 
/*驱动注销函数*/
static void __exit mpu6050_driver_exit(void)
{
	pr_info("mpu6050_driver_exit\n");
	i2c_del_driver(&mpu6050_driver);
}

module_init(mpu6050_driver_init);
module_exit(mpu6050_driver_exit);
 
MODULE_LICENSE("GPL");