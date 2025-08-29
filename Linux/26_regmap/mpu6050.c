#include "linux/init.h"
#include "linux/module.h"
#include "linux/printk.h"
#include "linux/i2c.h"
#include "linux/miscdevice.h"
#include "linux/fs.h"
#include "linux/delay.h"
#include "asm-generic/uaccess.h"
#include "linux/regmap.h"
#include "linux/slab.h"
#include "mpu6050_reg.h"

#define NEWCHRDEV_MINOR 255         /* 次设备号（让MISC自动分配） */
#define NEWCHRDEV_NAME  "mpu6050"   /* 名子 */

typedef struct{
    struct i2c_client *client;      /* i2C设备 */
    struct regmap *regmap;
    struct regmap_config regmap_config;
    uint16_t AX; 
    uint16_t AY; 
    uint16_t AZ; 
    uint16_t GX; 
    uint16_t GY; 
    uint16_t GZ;
    struct miscdevice miscdev;      // 添加miscdevice成员
}newchrdev_t;
 
static int mpu6050_init(newchrdev_t *dev)
{
    int ret;
    
    if (!dev || !dev->regmap) {
        printk("Device or regmap is NULL\n");
        return -EINVAL;
    }
    
    /* 复位MPU6050 */
    
    /* 配置MPU6050 */
    ret = regmap_write(dev->regmap, MPU6050_PWR_MGMT_1, 0x01);
    ret |= regmap_write(dev->regmap, MPU6050_PWR_MGMT_2, 0x00);
    ret |= regmap_write(dev->regmap, MPU6050_SMPLRT_DIV, 0x09);
    ret |= regmap_write(dev->regmap, MPU6050_CONFIG, 0x06);
    ret |= regmap_write(dev->regmap, MPU6050_GYRO_CONFIG, 0x18);
    ret |= regmap_write(dev->regmap, MPU6050_ACCEL_CONFIG, 0x18);
    
    if (ret) {
        printk("Failed to configure MPU6050\n");
        return ret;
    }
    
    msleep(100);
    return 0;
}

void ap3216c_read_data(newchrdev_t *dev)
{
    u8 buffer[14];
    int ret;
    
    if (!dev || !dev->regmap) {
        printk("Device or regmap is NULL\n");
        return;
    }
    
    ret = regmap_bulk_read(dev->regmap, MPU6050_ACCEL_XOUT_H, buffer, 14);
    if (ret < 0) {
        printk("Failed to read sensor data: %d\n", ret);
        return;
    }
    
    dev->AX = (buffer[0] << 8) | buffer[1];
    dev->AY = (buffer[2] << 8) | buffer[3];
    dev->AZ = (buffer[4] << 8) | buffer[5];
    dev->GX = (buffer[8] << 8) | buffer[9];
    dev->GY = (buffer[10] << 8) | buffer[11];
    dev->GZ = (buffer[12] << 8) | buffer[13];
}

static int ap3216c_open(struct inode *inode, struct file *filp)
{
    // 直接从 miscdevice 获取设备结构
    struct miscdevice *miscdev = filp->private_data;
    newchrdev_t *dev = container_of(miscdev, newchrdev_t, miscdev);
    
    filp->private_data = dev;
    return mpu6050_init(dev);
}

static ssize_t ap3216c_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
    short data[6];
    newchrdev_t *dev = (newchrdev_t*)filp->private_data;

    if (!dev) {
        return -EINVAL;
    }

    ap3216c_read_data(dev);

    data[0] = dev->AX;
    data[1] = dev->AY;
    data[2] = dev->AZ;
    data[3] = dev->GX;
    data[4] = dev->GY;
    data[5] = dev->GZ;
    
    if (copy_to_user(buf, data, sizeof(data))) {
        return -EFAULT;
    }
    
    return sizeof(data);
}

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

static int ap3216c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret = 0;
    unsigned int val;
    newchrdev_t *dev;
    
    printk("MPU6050 probe started\n");
    
    dev = devm_kzalloc(&client->dev, sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;
    
    dev->client = client;
    
    /* 初始化regmap配置 */
    dev->regmap_config.reg_bits = 8;
    dev->regmap_config.val_bits = 8;
    dev->regmap_config.max_register = 0x75;
    
    /* 创建regmap */
    dev->regmap = devm_regmap_init_i2c(client, &dev->regmap_config);
    if (IS_ERR(dev->regmap)) {
        ret = PTR_ERR(dev->regmap);
        printk("Failed to init regmap: %d\n", ret);
        return ret;
    }
    
    /* 设置驱动数据 */
    dev_set_drvdata(&client->dev, dev);
    
    mpu6050_init(dev);
    /* 检查设备ID - 使用正确的类型 */
    ret = regmap_read(dev->regmap, MPU6050_WHO_AM_I, &val);
    if (ret) {
        printk("Failed to read WHO_AM_I: %d\n", ret);
        return ret;
    }
    
    printk("MPU6050 WHO_AM_I = 0x%02x\n", (u8)val);
    
    if ((u8)val != 0x68) {
        printk("Invalid MPU6050 ID: 0x%02x\n", (u8)val);
        return -ENODEV;
    }
    
    /* 初始化misc设备 */
    dev->miscdev.minor = NEWCHRDEV_MINOR;
    dev->miscdev.name = NEWCHRDEV_NAME;
    dev->miscdev.fops = &ap3216c_ops;
    
    /* 注册MISC设备 */
    ret = misc_register(&dev->miscdev);
    if(ret < 0){
        printk("ap3216c misc device register failed!\n");
        return ret;
    }
    
    printk("MPU6050 probe successful\n");
    return 0;
}

static int ap3216c_remove(struct i2c_client *client)
{
    newchrdev_t *dev = dev_get_drvdata(&client->dev);
    
    if (dev) {
        misc_deregister(&dev->miscdev);
        // regmap 由 devm 管理，不需要手动释放
    }
    
    return 0;
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

static int __init ap3216c_init(void)
{
    int ret = 0;
    ret = i2c_add_driver(&ap3216c_driver);
    return ret;
}

static void __exit ap3216c_exit(void)
{
    i2c_del_driver(&ap3216c_driver);
}

module_init(ap3216c_init);
module_exit(ap3216c_exit);
MODULE_LICENSE("GPL");