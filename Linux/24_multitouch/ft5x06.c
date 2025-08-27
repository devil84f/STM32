#include <linux/module.h>
#include <linux/ratelimit.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/input/mt.h>
#include <linux/input/touchscreen.h>
#include <linux/input/edt-ft5x06.h>
#include <linux/i2c.h>

#define FT5X06_NAME             "ft5x06"
#define FT5X06_CNT              1

#define GT_CTRL_REG 	        0X8040  /* GT9147控制寄存器         */
#define GT_MODSW_REG 	        0X804D  /* GT9147模式切换寄存器        */
#define GT_9xx_CFGS_REG 	    0X8047  /* GT9147配置起始地址寄存器    */
#define GT_1xx_CFGS_REG 	    0X8050  /* GT1151配置起始地址寄存器    */
#define GT_CHECK_REG 	        0X80FF  /* GT9147校验和寄存器       */
#define GT_PID_REG 		        0X8140  /* GT9147产品ID寄存器       */

#define GT_GSTID_REG 	        0X814E  /* GT9147当前检测到的触摸情况 */
#define GT_TP1_REG 		        0X814F  /* 第一个触摸点数据地址 */
#define GT_TP2_REG 		        0X8157	/* 第二个触摸点数据地址 */
#define GT_TP3_REG 		        0X815F  /* 第三个触摸点数据地址 */
#define GT_TP4_REG 		        0X8167  /* 第四个触摸点数据地址  */
#define GT_TP5_REG 		        0X816F	/* 第五个触摸点数据地址   */
#define MAX_SUPPORT_POINTS      5       /* 最多5点电容触摸 */

/* 设备结构体 */
struct ft5x06dev {
    struct device_node *nd;
    struct input_dev *input;
    struct i2c_client *client;
    void *private_dat;
    int irq_gpio, res_gpio;
    int irq_num;
};

static struct ft5x06dev ft5x06_dev;

static int ft5x06_read_regs(struct i2c_client *client, u8 reg,
                             u8 *buf, u8 len)
{
    int ret = 0;
    struct i2c_msg msg[2] = {
        {
            .addr   = client->addr,
            .flags  = 0,
            .buf    = &reg,
            .len    = 1,
        },
        {
            .addr   = client->addr,
            .flags  = I2C_M_RD,
            .buf    = buf,
            .len    = len,
        },
    };

    ret = i2c_transfer(client->adapter, msg, 2);
    if (ret != 2) {
        dev_err(&client->dev, "i2c read failed: %d\n", ret);
        return (ret < 0) ? ret : -EIO;
    }

    return 0;
}

static int ft5x06_write_regs(struct i2c_client *client, u8 reg,
                             const u8 *buf, u8 len)
{
    int ret = 0;
    u8 b[256];
    struct i2c_msg msg;

    if (len > 255)
        return -EINVAL;

    b[0] = reg;
    memcpy(&b[1], buf, len);

    msg.addr = client->addr;
    msg.flags = 0;
    msg.buf = b;
    msg.len = len + 1;

    ret = i2c_transfer(client->adapter, &msg, 1);
    if (ret != 1) {
        dev_err(&client->dev, "i2c write failed: %d\n", ret);
        return (ret < 0) ? ret : -EIO;
    }

    return 0;
}

static int ft5x06_reset(struct i2c_client *client, struct ft5x06dev *dev)
{
    int ret = 0;

	if (gpio_is_valid(dev->res_gpio)) {  		/* 检查IO是否有效 */
		/* 申请复位IO，并且默认输出低电平 */
		ret = devm_gpio_request_one(&client->dev,	
					dev->res_gpio, GPIOF_OUT_INIT_LOW,
					"edt-ft5x06 reset");
		if (ret) {
			return ret;
		}

		msleep(5);
		gpio_set_value(dev->res_gpio, 1);	/* 输出高电平，停止复位 */
		msleep(300);
	}

	return 0;
}

static irqreturn_t ft5x06_handler(int irq, void *dev_id)
{
    struct ft5x06dev *dev = dev_id;
    u8 rdbuf[29];
	int i, type, x, y, id;
	int offset, tplen;
	bool down;

	offset = 1; 	/* 偏移1，也就是0X02+1=0x03,从0X03开始是触摸值 */
	tplen = 6;		/* 一个触摸点有6个寄存器来保存触摸值 */

	memset(rdbuf, 0, sizeof(rdbuf));		/* 清除 */
    ft5x06_read_regs(dev->client, FT5X06_TD_STATUS_REG, rdbuf, FT5X06_READLEN);

    /* 上报每一个触摸点坐标 */
	for (i = 0; i < MAX_SUPPORT_POINTS; i++) {
		u8 *buf = &rdbuf[i * tplen + offset];

		/* 以第一个触摸点为例，寄存器TOUCH1_XH(地址0X03),各位描述如下：
		 * bit7:6  Event flag  0:按下 1:释放 2：接触 3：没有事件
		 * bit5:4  保留
		 * bit3:0  X轴触摸点的11~8位。
		 */
		type = buf[0] >> 6;     /* 获取触摸类型 */
		if (type == TOUCH_EVENT_RESERVED)
			continue;
 
		/* 我们所使用的触摸屏和FT5X06是反过来的 */
		x = ((buf[2] << 8) | buf[3]) & 0x0fff;
		y = ((buf[0] << 8) | buf[1]) & 0x0fff;
		
		/* 以第一个触摸点为例，寄存器TOUCH1_YH(地址0X05),各位描述如下：
		 * bit7:4  Touch ID  触摸ID，表示是哪个触摸点
		 * bit3:0  Y轴触摸点的11~8位。
		 */
		id = (buf[2] >> 4) & 0x0f;
		down = type != TOUCH_EVENT_UP;

		input_mt_slot(dev->input, id);
		input_mt_report_slot_state(dev->input, MT_TOOL_FINGER, down);

		if (!down)
			continue;

		input_report_abs(dev->input, ABS_MT_POSITION_X, x);
		input_report_abs(dev->input, ABS_MT_POSITION_Y, y);
	}

	input_mt_report_pointer_emulation(dev->input, true);
	input_sync(dev->input);

    return IRQ_HANDLED;
}

static int ft5x06_irq(struct i2c_client *client, struct ft5x06dev *dev)
{
    int ret = 0;

	if (gpio_is_valid(dev->res_gpio)) {  		/* 检查IO是否有效 */
		/* 申请复位IO，并且默认输出低电平 */
		ret = devm_gpio_request_one(&client->dev, dev->res_gpio, GPIOF_IN, "edt-ft5x06 irq");
		if (ret) {
			return ret;
		}
	}

    ret = devm_request_threaded_irq(&client->dev, client->irq, NULL, ft5x06_handler, IRQF_TRIGGER_FALLING | IRQF_ONESHOT, client->name, &ft5x06_dev);
    if (ret) {
		return ret;
	}

	return 0;
}

static int ft5x06_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret = 0;
    u8 data = 0;
    ft5x06_dev.client = client;
    printk("ft5x06 driver and device matched successful\r\n");

    ft5x06_dev.irq_gpio = of_get_named_gpio(client->dev.of_node, "interrupt-gpios", 0);
    ft5x06_dev.res_gpio = of_get_named_gpio(client->dev.of_node, "reset-gpios", 0);

    ret = ft5x06_reset(client, &ft5x06_dev);
    if (ret < 0) {
        printk("ft5x06 reset faild\r\n");
        return ret;
    }

    ret = ft5x06_irq(client, &ft5x06_dev);
    if (ret < 0) {
        printk("ft5x06 request irq faild\r\n");
        return ret;
    }

    data = 0x00;
    ft5x06_write_regs(client, FT5x06_DEVICE_MODE_REG, &data, 1); 	/* 进入正常模式 	*/
	data = 0x01;
    ft5x06_write_regs(client, FT5426_IDG_MODE_REG, &data, 1); 		/* FT5426中断模式	*/

    ft5x06_dev.input = devm_input_allocate_device(&client->dev);
    ft5x06_dev.input->name = client->name;
    ft5x06_dev.input->id.bustype = BUS_I2C;
    ft5x06_dev.input->dev.parent = &client->dev;

    __set_bit(EV_KEY, ft5x06_dev.input->evbit);
	__set_bit(EV_ABS, ft5x06_dev.input->evbit);
	__set_bit(BTN_TOUCH, ft5x06_dev.input->keybit);

    input_set_abs_params(ft5x06_dev.input, ABS_X, 0, 800, 0, 0);
	input_set_abs_params(ft5x06_dev.input, ABS_Y, 0, 480, 0, 0);
	input_set_abs_params(ft5x06_dev.input, ABS_MT_POSITION_X,0, 800, 0, 0);
	input_set_abs_params(ft5x06_dev.input, ABS_MT_POSITION_Y,0, 480, 0, 0);

	input_mt_init_slots(ft5x06_dev.input, MAX_SUPPORT_POINTS, 0);

    ret = input_register_device(ft5x06_dev.input);
    if (ret) {
        dev_err(&client->dev, "Failed to register input device: %d\n", ret);
        return ret;
    }   

    return 0;
}

static int ft5x06_remove(struct i2c_client *client)
{
    input_unregister_device(ft5x06_dev.input);
    return 0;
}

static const struct of_device_id ft5x06_of_match[] = {
    { .compatible = "alientekMini,gt9147" },
    { /* sentinel */ },
};

static const struct i2c_device_id ft5x06_id_table[] = {
    { "alientekMini,gt9147", 0},
    { /* sentinel */ },
};

static struct i2c_driver ft5x06_i2c_driver = {
    .driver     = {
        .owner  = THIS_MODULE,
        .name   = FT5X06_NAME,
        .of_match_table = ft5x06_of_match,
    },
    .id_table   = ft5x06_id_table,
    .probe      = ft5x06_probe,
    .remove     = ft5x06_remove,
};

static int __init ft5x06_driver_init(void)
{
    return i2c_add_driver(&ft5x06_i2c_driver);
}

static void __exit ft5x06_driver_exit(void)
{
    i2c_del_driver(&ft5x06_i2c_driver);
}

module_init(ft5x06_driver_init);
module_exit(ft5x06_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LiNing");
