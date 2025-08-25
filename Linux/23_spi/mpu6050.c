#include "linux/init.h"
#include "linux/module.h"
#include "linux/printk.h"
#include "linux/i2c.h"
#include "linux/miscdevice.h"
#include "linux/fs.h"
#include "linux/delay.h"
#include "asm-generic/uaccess.h"
#include "mpu6050_reg.h"

#define NEWCHRDEV_MINOR 255         /* 次设备号（让MISC自动分配） */
#define NEWCHRDEV_NAME  "mpu6050"   /* 名子 */

typedef struct{
    struct i2c_client *client;      /* i2C设备 */
    uint16_t AX; 
    uint16_t AY; 
    uint16_t AZ; 
    uint16_t GX; 
    uint16_t GY; 
    uint16_t GZ;
}newchrdev_t;

newchrdev_t newchrdev;

static int i2c_write_mpu6050(struct i2c_client *mpu6050_client, u8 address, u8 data)
{
    int error = 0;
    u8 write_data[2];
    struct i2c_msg send_msg;
    
    printk("Writing to MPU6050: addr=0x%02x, reg=0x%02x, data=0x%02x\n", 
           mpu6050_client->addr, address, data);
    
    write_data[0] = address;
    write_data[1] = data;
    
    send_msg.addr = mpu6050_client->addr;
    send_msg.flags = 0;
    send_msg.buf = write_data;
    send_msg.len = 2;
    
    error = i2c_transfer(mpu6050_client->adapter, &send_msg, 1);
    if (error != 1) {
        printk("i2c_transfer failed: error=%d\n", error);
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
    printk("\n i2c read mpu6050 error %d\n", error);
	if (error != 2) {
		printk(KERN_DEBUG "\n i2c read mpu6050 error\n");
		return -1;
	}
	return 0;
}
 
static void mpu6050_init(newchrdev_t *dev)
{
    /* 复位MPU6050 */
    i2c_write_mpu6050(dev->client, MPU6050_PWR_MGMT_1, 0x01);
    i2c_write_mpu6050(dev->client, MPU6050_PWR_MGMT_2, 0x00);
    i2c_write_mpu6050(dev->client, MPU6050_SMPLRT_DIV, 0x09);
    i2c_write_mpu6050(dev->client, MPU6050_CONFIG, 0x06);
    i2c_write_mpu6050(dev->client, MPU6050_GYRO_CONFIG, 0x18);
    i2c_write_mpu6050(dev->client, MPU6050_ACCEL_CONFIG, 0x18);
    msleep(100);  /* 等待复位完成 */
}

/*
 * @description	: 读取AP3216C的数据，读取原始数据，包括ALS,PS和IR, 注意！
 *				: 如果同时打开ALS,IR+PS的话两次数据读取的时间间隔要大于112.5ms
 * @param - ir	: ir数据
 * @param - ps 	: ps数据
 * @param - ps 	: als数据 
 * @return 		: 无。
 */
void ap3216c_read_data(newchrdev_t *dev)
{
    u8 buffer[14]; // 从0x3B开始读取14个寄存器
    int ret;
    
    // 一次性读取所有传感器数据寄存器（0x3B到0x48）
    ret = i2c_read_mpu6050(dev->client, MPU6050_ACCEL_XOUT_H, buffer, 14);
    if (ret < 0) {
        printk("Failed to read sensor data\n");
    }
    
    // 解析数据
    dev->AX = (buffer[0] << 8) | buffer[1];
    dev->AY = (buffer[2] << 8) | buffer[3];
    dev->AZ = (buffer[4] << 8) | buffer[5];

    dev->GX = (buffer[8] << 8) | buffer[9];
    dev->GY = (buffer[10] << 8) | buffer[11];
    dev->GZ = (buffer[12] << 8) | buffer[13];
}

/*
 * @description		: 打开设备
 * @param - inode 	: 传递给驱动的inode
 * @param - filp 	: 设备文件，file结构体有个叫做private_data的成员变量
 * 					  一般在open的时候将private_data指向设备结构体。
 * @return 			: 0 成功;其他 失败
 */
static int ap3216c_open(struct inode *inode, struct file *filp)
{
    /* 1、设置私有数据 */
    filp->private_data = &newchrdev;
    /* 2、初始化ap3216c */
    mpu6050_init(&newchrdev);
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
static ssize_t ap3216c_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
    short data[6];
    long err = 0;

    newchrdev_t *dev = (newchrdev_t*)filp->private_data;

    ap3216c_read_data(dev);

	data[0] = dev->AX;
    data[1] = dev->AY;
    data[2] = dev->AZ;
    data[3] = dev->GX;
    data[4] = dev->GY;
    data[5] = dev->GZ;
	err = copy_to_user(buf, data, sizeof(data));
	return sizeof(data);
}

/*
 * @description		: 关闭/释放设备
 * @param - filp 	: 要关闭的设备文件(文件描述符)
 * @return 			: 0 成功;其他 失败
 */
static int ap3216c_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static const struct file_operations ap3216c_ops = {
    .owner   = THIS_MODULE,
    .open = ap3216c_open,
    .read = ap3216c_read,
    .release = ap3216c_release,
};

/* MISC设备结构体 */
static struct miscdevice ap3216c_miscdev = {
	.minor = NEWCHRDEV_MINOR,
	.name = NEWCHRDEV_NAME,
	.fops = &ap3216c_ops,
};

 /*
  * @description     : i2c驱动的probe函数，当驱动与
  *                    设备匹配以后此函数就会执行
  * @param - client  : i2c设备
  * @param - id      : i2c设备ID
  * @return          : 0，成功;其他负值,失败
  */
static int ap3216c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret = 0;
    u8 val;
    
    /* 保存client */
    newchrdev.client = client;
    mpu6050_init(&newchrdev);
    /* 读取WHO_AM_I寄存器(0x75)，MPU6050应返回0x68 */
    ret = i2c_read_mpu6050(client, 0x75, &val, 1);
    if (ret < 0) {
        printk("Failed to read WHO_AM_I register\n");
        return ret;
    }
    printk("MPU6050 WHO_AM_I = 0x%02x\n", val);
    
    /* 注册MISC设备 */
    ret = misc_register(&ap3216c_miscdev);
    if(ret < 0){
        printk("ap3216c misc device register failed!\r\n");
        return -EFAULT;
    }
    
    return 0;
}

/*
 * @description     : i2c驱动的remove函数，移除i2c驱动的时候此函数会执行
 * @param - client 	: i2c设备
 * @return          : 0，成功;其他负值,失败
 */
static int ap3216c_remove(struct i2c_client *client)
{
    int ret = 0;
    /* MISC 驱动框架卸载 */
    misc_deregister(&ap3216c_miscdev);
    return ret;
}

/* 传统匹配方式ID列表 */
static const struct i2c_device_id ap3216c_id[] = {
	{"alientek,mpu6050", 0},  
	{},
};

/* 设备树匹配列表 */
static const struct of_device_id ap3216c_of_match[] = {
	{ .compatible = "alientek,mpu6050" },
	{ /* Sentinel */ },
};

/* i2c驱动结构体 */	
static struct i2c_driver ap3216c_driver = {
    .probe = ap3216c_probe,
    .remove = ap3216c_remove,
    .driver = {
        .owner = THIS_MODULE,
            .name = "ap3216c",
            .of_match_table = ap3216c_of_match, 
           },
    .id_table = ap3216c_id,
};

/*
 * @description	: 驱动入口函数
 * @param 		: 无
 * @return 		: 无
 */
static int __init ap3216c_init(void)
{
    int ret = 0;
    ret = i2c_add_driver(&ap3216c_driver);
    return ret;
}

/*
 * @description	: 驱动出口函数
 * @param 		: 无
 * @return 		: 无
 */
static void __exit ap3216c_exit(void)
{
    i2c_del_driver(&ap3216c_driver);
}

/* module_i2c_driver(ap3216c_driver) */

module_init(ap3216c_init);
module_exit(ap3216c_exit);
MODULE_LICENSE("GPL");
