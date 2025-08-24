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
#include <linux/i2c.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include "mpu6050_reg.h"

#define MPU6050_CNT     1           /* 驱动数量 */
#define MPU6050_NAME    "mpu6050"   /* 驱动名称 */

struct mpu6050_data {
        int16_t AX; 
        int16_t AY; 
        int16_t AZ; 
        int16_t GX; 
        int16_t GY; 
        int16_t GZ;
};
/* MPU6050设备结构体 */
struct mpu6050_dev {
    void *private_data;
    uint8_t ID;
    struct mpu6050_data data;
};

static struct mpu6050_dev mpu6050;

/*
 * @description : 从mpu6050读取多个寄存器数据
 * @param-dev   : 设备
 * @param-reg   : 要读取的首地址
 * @param-val   : 读取到的数据
 * @param-len   : 读取的长度
 * @return      : 0->成功；其他->失败；
 */
static int mpu6050_read_regs(struct mpu6050_dev *dev, u8 reg, void *val, int len)
{
    int ret;
    struct i2c_msg msg[2];
    struct i2c_client *client = (struct i2c_client *)dev->private_data;
    
    /* msg[0]为发送要读取的首地址 */
    msg[0].addr     = client->addr; /* mpu6050地址   */
    msg[0].flags    = 0;            /* 标记为写操作   */
    msg[0].len      = 1;            /* reg长度       */
    msg[0].buf      = &reg;         /* 读取的首地址   */

    /* msg[1]读取数据 */
    msg[1].addr     = client->addr; /* mpu6050地址   */
    msg[1].flags    = I2C_M_RD;     /* 标记为读操作   */
    msg[1].len      = len;          /* 读取长度      */
    msg[1].buf      = val;          /* 读取的缓冲区   */

    ret = i2c_transfer(client->adapter, msg, 2);
    if (ret == 2){
        ret = 0;
    } else {
        printk("i2c rd failed=%d reg=%02x len=%d\n", ret, reg, len);
        ret = -EREMOTEIO;
    }

    return ret;
}

/*
 * @description : 向mpu6050多个寄存器写入数据
 * @param-dev   : 设备
 * @param-reg   : 要写的首地址
 * @param-buf   : 写入的数据
 * @param-len   : 写入的长度
 * @return      : 0->成功；其他->失败；
 */
static int mpu6050_write_regs(struct mpu6050_dev *dev, u8 reg, u8 *buf, int len)
{
    int ret;
    u8 b[256];
    struct i2c_msg msg;
    struct i2c_client *client = (struct i2c_client *)dev->private_data;
    
    b[0] = reg;
    memcpy(&b[1],buf,len);

    /* msg写入 */
    msg.addr     = client->addr; /* mpu6050地址   */
    msg.flags    = 0;            /* 标记为写操作   */
    msg.len      = len + 1;      /* reg长度       */
    msg.buf      = b;           /* 读取的首地址   */

    ret = i2c_transfer(client->adapter, &msg, 1);
    if (ret == 1){
        ret = 0;
    } else {
        printk("i2c wr failed=%d reg=%02x len=%d addr=%02x\n",ret, reg, len, client->addr);
        ret = -EREMOTEIO;
    }

    return ret;
}

/*
 * @description : 从mpu6050单个寄存器读取数据
 * @param-dev   : 设备
 * @param-reg   : 要读取的首地址
 * @return      : 读取到的数据
 */
static u8 mpu6050_read_reg(struct mpu6050_dev *dev, u8 reg)
{
    u8 data = 0;

    mpu6050_read_regs(dev, reg, &data, 1);
    return data;
#if 0
    struct i2c_client *client = (struct i2c_client *)dev->private_data;
    return i2c_smbus_read_byte_data(client, reg);
#endif
}

/*
 * @description : 向mpu6050单个寄存器写入数据
 * @param-dev   : 设备
 * @param-reg   : 要写入的首地址
 * @param-data  : 写入的数据
 * @return      : 0->成功；其他->失败；
 */
static void mpu6050_write_reg(struct mpu6050_dev *dev, u8 reg, u8 data)
{
    u8 buf = 0;
	buf = data;
    mpu6050_write_regs(dev, reg, &buf, 1);
}

/*
 * @description : mpu6050初始化
 * @param       : 无
 * @return      : 0->成功；其他->失败；
 */

static int mpu6050_init(struct mpu6050_dev *dev)
{
    // 初始化配置：
	// 配置电源管理寄存器1
	// 此处配置为：不复位，接触睡眠，不需要循环，温度传感器不失能
    mpu6050_write_reg(dev, MPU6050_PWR_MGMT_1, 0x00);
    mpu6050_write_reg(dev, MPU6050_PWR_MGMT_2, 0x00);
	// 配置采样率分频
	// 此处配置为：决定数据输出快慢，数越小越快，此处为10分频
    mpu6050_write_reg(dev, MPU6050_SMPLRT_DIV, 0x09);
	// 配置配置寄存器
	// 此处配置为：不需要外部同步，低通滤波器最平滑滤波
    mpu6050_write_reg(dev, MPU6050_CONFIG, 0x06);
	// 配置陀螺仪配置寄存器
	// 此处配置为：不自测，满量程选择最大量程
    mpu6050_write_reg(dev, MPU6050_GYRO_CONFIG, 0x18);
	// 配置加速度计配置寄存器
	// 此处配置为：不自测，满量程选择最大量程，不使用高通滤波器
    mpu6050_write_reg(dev, MPU6050_ACCEL_CONFIG, 0x18);

    return 0;
}

static int mpu6050_read_data(struct mpu6050_dev *dev)
{
    u8 buffer[14]; // 从0x3B开始读取14个寄存器
    int ret;
    
    // 一次性读取所有传感器数据寄存器（0x3B到0x48）
    ret = mpu6050_read_regs(dev, MPU6050_ACCEL_XOUT_H, buffer, 14);
    if (ret < 0) {
        printk("Failed to read sensor data\n");
        return ret;
    }
    
    // 解析数据
    dev->data.AX = (buffer[0] << 8) | buffer[1];
    dev->data.AY = (buffer[2] << 8) | buffer[3];
    dev->data.AZ = (buffer[4] << 8) | buffer[5];

    dev->data.GX = (buffer[8] << 8) | buffer[9];
    dev->data.GY = (buffer[10] << 8) | buffer[11];
    dev->data.GZ = (buffer[12] << 8) | buffer[13];

    return 0;
}

/*
 * @description : mpu6050打开
 * @param       : 无
 * @return      : 0->成功；其他->失败；
 */
static int mpu6050_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &mpu6050;
    /* 初始化MPU6050 */
    mpu6050_init(&mpu6050);

    return 0;
}

/*
 * @description : 从设备读取数据
 * @param-filp  : 文件描述符
 * @param-buf   : 返回给用户的数据缓冲区
 * @param-cnt   : 要读取的数据长度
 * @param-off   : 相对于文件首地址偏移
 * @return      : 读取的字节数；其他负值->失败；
 */
static ssize_t mpu6050_read(struct file *filp, char __user *buf, size_t cnt, loff_t *off)
{
    uint16_t data[6];
    int size;
    struct mpu6050_dev *dev = filp->private_data;
    mpu6050_read_data(dev);
    data[0] = dev->data.AX;
    data[1] = dev->data.AY;
    data[2] = dev->data.AZ;
    data[3] = dev->data.GX;
    data[4] = dev->data.GY;
    data[5] = dev->data.GZ;
    size = sizeof(data);
    if (copy_to_user(buf, data, size) != 0) {
        return -EFAULT;
    }
    return size; // 返回实际读取的字节数
}

/*
 * @description : mpu6050释放、关闭
 * @param       : 无
 * @return      : 0->成功；其他->失败；
 */
static int mpu6050_release(struct inode *inode, struct file *filp)
{
    return 0;
}

/* MPU6050操作函数结构体 */
static struct file_operations mpu6050_fops= {
    .owner      = THIS_MODULE,
    .open       = mpu6050_open,
    .read       = mpu6050_read,
    .release    = mpu6050_release,
};

/* MISC设备结构体 */
static struct miscdevice mpu6050_miscdev = {
    .minor  = MISC_DYNAMIC_MINOR,
    .name   = MPU6050_NAME,
    .fops   = &mpu6050_fops,
};

/*
 * @description : I2C驱动的probe函数，驱动与设备匹配后自动执行
 * @param-client: i2c设备
 * @param-id    : 设备ID
 * @return      : 0->成功；其他->失败；
 */
static int mpu6050_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret;
    
    printk(KERN_INFO "mpu6050: probing device at address 0x%02x\n", client->addr);
    printk(KERN_INFO "mpu6050: device and driver matched!\n");
    
    ret = misc_register(&mpu6050_miscdev); 
    if(ret < 0){ 
        printk(KERN_ERR "mpu6050: misc_register failed with error %d\n", ret); 
        return ret; 
    }
    
    mpu6050.private_data = client;

    return 0;
}

/*
 * @description : I2C驱动的remove函数，驱动卸载后自动执行
 * @param-client: i2c设备
 * @return      : 0->成功；其他->失败；
 */
static int mpu6050_remove(struct i2c_client *client)
{
    int ret;
    ret = misc_deregister(&mpu6050_miscdev); 
    if(ret < 0){ 
        printk("mpu6050_miscdev deregister failed!\r\n"); 
        return -EFAULT; 
    } 

    return 0;
}

/*定义ID 匹配表*/
static const struct i2c_device_id gtp_device_id[] = {
	{"alientek,mpu6050", 0},
	{},
};

/* 设备树匹配列表 */
static const struct of_device_id mpu6050_of_match[] = {
    {.compatible = "alientek,mpu6050"},
    { /* Sentinel */ },
};

/* I2C驱动结构体 */
static struct i2c_driver mpu6050_driver = {
    .driver     = {
        .owner  = THIS_MODULE,
        .name   = MPU6050_NAME,
        .of_match_table = mpu6050_of_match,
    },
    .id_table 	= gtp_device_id,
    .probe      = mpu6050_probe,
    .remove     = mpu6050_remove,
};

/*
 * @description : 驱动入口函数，注册MPU6050的I2C驱动
 * @param       : 无
 * @return      : 无
 */
static int __init mpu6050_driver_init(void)
{
    int ret;
    
    ret = i2c_add_driver(&mpu6050_driver);
    if (ret) {
        printk(KERN_ERR "mpu6050: i2c_add_driver failed with error %d\n", ret);
        return ret;
    }
    
    return 0;
}

/*
 * @description : 驱动出口函数
 * @param       : 无
 * @return      : 无
 */
static void __exit mpu6050_driver_exit(void)
{
    pr_info("mpu6050_driver_exit\n");
    i2c_del_driver(&mpu6050_driver);
}

module_init(mpu6050_driver_init);
module_exit(mpu6050_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LiNing");