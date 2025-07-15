/**
 ****************************************************************************************************
 * @file        touch.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.1
 * @date        2023-05-29
 * @brief       触摸屏 驱动代码
 *   @note      支持电阻/电容式触摸屏
 *              触摸屏驱动（支持ADS7843/7846/UH7843/7846/XPT2046/TSC2046/GT9147/GT9271/FT5206/GT1151等）代码
 *
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 探索者 F407开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 * 修改说明
 * V1.0 20211025
 * 第一次发布
 * V1.1 20230529
 * 1，新增对ST7796 3.5寸屏 GT1151的支持
 * 2，新增对ILI9806 4.3寸屏 GT1151的支持
 ****************************************************************************************************
 */

#include "stdio.h"
#include "stdlib.h"
#include "lcd.h"
#include "touch.h"
#include "24cxx.h"
#include "math.h"

_m_tp_dev tp_dev =
{
    tp_init,
    tp_scan,
    tp_adjust,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};
//默认为touchtype=0的数据.
uint8_t CMD_RDX=0XD0;
uint8_t CMD_RDY=0X90;

/**
 * @brief       SPI写数据
 *   @note      向触摸屏IC写入1 byte数据
 * @param       data: 要写入的数据
 * @retval      无
 */
static void tp_write_byte(uint8_t data)
{
    uint8_t count = 0;

    for (count = 0; count < 8; count++)
    {
        if (data & 0x80)    /* 发送1 */
        {
            T_MOSI(1);
        }
        else                /* 发送0 */
        {
            T_MOSI(0);
        }

        data <<= 1;
        T_CLK(0);
				for_delay_us(1);
        T_CLK(1);           /* 上升沿有效 */
    }
}

/**
 * @brief       SPI读数据
 *   @note      从触摸屏IC读取adc值
 * @param       cmd: 指令
 * @retval      读取到的数据,ADC值(12bit)
 */
static uint16_t tp_read_ad(uint8_t cmd)
{
    uint8_t count = 0;
    uint16_t num = 0;
    
    T_CLK(0);           /* 先拉低时钟 */
    T_MOSI(0);          /* 拉低数据线 */
    T_CS(0);            /* 选中触摸屏IC */
    tp_write_byte(cmd); /* 发送命令字 */
    for_delay_us(6);        /* ADS7846的转换时间最长为6us */
    T_CLK(0);
    for_delay_us(1);
    T_CLK(1);           /* 给1个时钟，清除BUSY */
    for_delay_us(1);
    T_CLK(0);

    for (count = 0; count < 16; count++)    /* 读出16位数据,只有高12位有效 */
    {
        num <<= 1;
        T_CLK(0);       /* 下降沿有效 */
        for_delay_us(1);
        T_CLK(1);

        if (T_MISO) num++;
    }

    num >>= 4;          /* 只有高12位有效. */
    T_CS(1);            /* 释放片选 */
    return num;
}

/* 电阻触摸驱动芯片 数据采集 滤波用参数 */
#define TP_READ_TIMES   5       /* 读取次数 */
#define TP_LOST_VAL     1       /* 丢弃值 */

/**
 * @brief       读取一个坐标值(x或者y)
 *   @note      连续读取TP_READ_TIMES次数据,对这些数据升序排列,
 *              然后去掉最低和最高TP_LOST_VAL个数, 取平均值
 *              设置时需满足: TP_READ_TIMES > 2*TP_LOST_VAL 的条件
 *
 * @param       cmd : 指令
 *   @arg       0XD0: 读取X轴坐标(@竖屏状态,横屏状态和Y对调.)
 *   @arg       0X90: 读取Y轴坐标(@竖屏状态,横屏状态和X对调.)
 *
 * @retval      读取到的数据(滤波后的), ADC值(12bit)
 */
static uint16_t tp_read_xoy(uint8_t cmd)
{
    uint16_t i, j;
    uint16_t buf[TP_READ_TIMES];
    uint16_t sum = 0;
    uint16_t temp;

    for (i = 0; i < TP_READ_TIMES; i++)     /* 先读取TP_READ_TIMES次数据 */
    {
        buf[i] = tp_read_ad(cmd);
				//printf("X:%x Y:%x \r\n", buf[i], buf[i]);
    }

    for (i = 0; i < TP_READ_TIMES - 1; i++) /* 对数据进行排序 */
    {
        for (j = i + 1; j < TP_READ_TIMES; j++)
        {
            if (buf[i] > buf[j])   /* 升序排列 */
            {
                temp = buf[i];
                buf[i] = buf[j];
                buf[j] = temp;
            }
        }
    }

    sum = 0;

    for (i = TP_LOST_VAL; i < TP_READ_TIMES - TP_LOST_VAL; i++)   /* 去掉两端的丢弃值 */
    {
        sum += buf[i];  /* 累加去掉丢弃值以后的数据. */
    }

    temp = sum / (TP_READ_TIMES - 2 * TP_LOST_VAL); /* 取平均值 */
    return temp;
}

/**
 * @brief       读取x, y坐标
 * @param       x,y: 读取到的坐标值
 * @retval      无
 */
uint8_t tp_read_xy(uint16_t *x, uint16_t *y)
{
    uint16_t xval, yval;
		xval=tp_read_xoy(CMD_RDX);
		yval=tp_read_xoy(CMD_RDY);	  

    *x = xval;
    *y = yval;
		return 1;//读数成功
}

/* 连续两次读取X,Y坐标的数据误差最大允许值 */
#define TP_ERR_RANGE    50      /* 误差范围 */

/**
 * @brief       连续读取2次触摸IC数据, 并滤波
 *   @note      连续2次读取触摸屏IC,且这两次的偏差不能超过ERR_RANGE,满足
 *              条件,则认为读数正确,否则读数错误.该函数能大大提高准确度.
 *
 * @param       x,y: 读取到的坐标值
 * @retval      0, 失败; 1, 成功;
 */
static uint8_t tp_read_xy2(uint16_t *x, uint16_t *y)
{
    uint16_t x1, y1;
    uint16_t x2, y2;
		uint8_t flag;    
    flag=tp_read_xy(&x1,&y1);   
    if(flag==0)return(0);
		//printf("X:%x Y:%x \r\n", tp_dev.x[0], tp_dev.y[0]);
    flag=tp_read_xy(&x2,&y2);	   
    if(flag==0)return(0);   
    if(((x2<=x1&&x1<x2+TP_ERR_RANGE)||(x1<=x2&&x2<x1+TP_ERR_RANGE))//前后两次采样在+-50内
    &&((y2<=y1&&y1<y2+TP_ERR_RANGE)||(y1<=y2&&y2<y1+TP_ERR_RANGE)))
    {
        *x=(x1+x2)/2;
        *y=(y1+y2)/2;
        return 1;
    }else return 0;
}

/******************************************************************************************/
/* 与LCD部分有关的函数, 用来校准用的 */

/**
 * @brief       画一个校准用的触摸点(十字架)
 * @param       x,y   : 坐标
 * @param       color : 颜色
 * @retval      无
 */
static void tp_draw_touch_point(uint16_t x, uint16_t y, uint16_t color)
{
    lcd_draw_line(x - 12, y, x + 13, y, color); /* 横线 */
    lcd_draw_line(x, y - 12, x, y + 13, color); /* 竖线 */
    lcd_draw_point(x + 1, y + 1, color);
    lcd_draw_point(x - 1, y + 1, color);
    lcd_draw_point(x + 1, y - 1, color);
    lcd_draw_point(x - 1, y - 1, color);
    lcd_draw_circle(x, y, 6, color);            /* 画中心圈 */
}

/**
 * @brief       画一个大点(2*2的点)
 * @param       x,y   : 坐标
 * @param       color : 颜色
 * @retval      无
 */
void tp_draw_big_point(uint16_t x, uint16_t y, uint16_t color)
{
    lcd_draw_point(x, y, color);       /* 中心点 */
    lcd_draw_point(x + 1, y, color);
    lcd_draw_point(x, y + 1, color);
    lcd_draw_point(x + 1, y + 1, color);
}

/******************************************************************************************/

/**
 * @brief       触摸按键扫描
 * @param       mode: 坐标模式
 *   @arg       0, 屏幕坐标;
 *   @arg       1, 物理坐标(校准等特殊场合用)
 *
 * @retval      0, 触屏无触摸; 1, 触屏有触摸;
 */
static uint8_t tp_scan(uint8_t mode)
{
    if (T_PEN == 0)     /* 有按键按下 */
    {
        if (mode)       /* 读取物理坐标, 无需转换 */
        {
            tp_read_xy2(&tp_dev.x[0], &tp_dev.y[0]);
        }
        else if (tp_read_xy2(&tp_dev.x[0], &tp_dev.y[0]))     /* 读取屏幕坐标, 需要转换 */
        {
           tp_dev.x[0]=tp_dev.xfac*tp_dev.x[0]+tp_dev.xc;//将结果转换为屏幕坐标
					 tp_dev.y[0]=tp_dev.yfac*tp_dev.y[0]+tp_dev.yc; 
        }

        if ((tp_dev.sta & TP_PRES_DOWN) == 0)   /* 之前没有被按下 */
        {
            tp_dev.sta = TP_PRES_DOWN | TP_CATH_PRES;   /* 按键按下 */
            tp_dev.x[4]=tp_dev.x[0];   /* 记录第一次按下时的坐标 */
            tp_dev.y[4]=tp_dev.y[0];
        }
    }
    else
    {
        if (tp_dev.sta & TP_PRES_DOWN)      /* 之前是被按下的 */
        {
            tp_dev.sta &= ~(1<<7);    /* 标记按键松开 */
        }
        else     /* 之前就没有被按下 */
        {
            tp_dev.x[4]=0;
						tp_dev.y[4]=0;
						tp_dev.x[0]=0xffff;
						tp_dev.y[0]=0xffff;
        }
    }

    return tp_dev.sta & TP_PRES_DOWN; /* 返回当前的触屏状态 */
}

/* TP_SAVE_ADDR_BASE定义触摸屏校准参数保存在EEPROM里面的位置(起始地址)
 * 占用空间 : 13字节.
 */
#define TP_SAVE_ADDR_BASE   40

/**
 * @brief       保存校准参数
 *   @note      参数保存在EEPROM芯片里面(24C02),起始地址为TP_SAVE_ADDR_BASE.
 *              占用大小为13字节
 * @param       无
 * @retval      无
 */
void tp_save_adjust_data(void)
{
  uint8_t *p = (uint8_t *)&tp_dev.xfac;   /* 指向首地址 */

    /* p指向tp_dev.xfac的地址, p+4则是tp_dev.yfac的地址
     * p+8则是tp_dev.xoff的地址,p+10,则是tp_dev.yoff的地址
     * 总共占用12个字节(4个参数)
     * p+12用于存放标记电阻触摸屏是否校准的数据(0X0A)
     * 往p[12]写入0X0A. 标记已经校准过.
     */
    at24cxx_write(TP_SAVE_ADDR_BASE, p, 12);                /* 保存12个字节数据(xfac,yfac,xc,yc) */
    at24cxx_write_one_byte(TP_SAVE_ADDR_BASE + 12, 0X0A);   /* 保存校准值 */
}

/**
 * @brief       获取保存在EEPROM里面的校准值
 * @param       无
 * @retval      0，获取失败，要重新校准
 *              1，成功获取数据
 */
uint8_t tp_get_adjust_data(void)
{
    uint8_t *p = (uint8_t *)&tp_dev.xfac;
    uint8_t temp = 0;

    /* 由于我们是直接指向tp_dev.xfac地址进行保存的, 读取的时候,将读取出来的数据
     * 写入指向tp_dev.xfac的首地址, 就可以还原写入进去的值, 而不需要理会具体的数
     * 据类型. 此方法适用于各种数据(包括结构体)的保存/读取(包括结构体).
     */
    at24cxx_read(TP_SAVE_ADDR_BASE, p, 12);                 /* 读取12字节数据 */
    temp = at24cxx_read_one_byte(TP_SAVE_ADDR_BASE + 12);   /* 读取校准状态标记 */

    if (temp == 0X0A)
    {
        return 1;
    }

    return 0;
}

/* 提示字符串 */
char *const TP_REMIND_MSG_TBL = "Please use the stylus click the cross on the screen.The cross will always move until the screen adjustment is completed.";

/**
 * @brief       触摸屏校准代码
 *   @note      使用五点校准法(具体原理请百度)
 *              本函数得到x轴/y轴比例因子xfac/yfac及物理中心坐标值(xc,yc)等4个参数
 *              我们规定: 物理坐标即AD采集到的坐标值,范围是0~4095.
 *                        逻辑坐标即LCD屏幕的坐标, 范围为LCD屏幕的分辨率.
 *
 * @param       无
 * @retval      无
 */
void tp_adjust(void)
{
    uint16_t pos_temp[4][2];     /* 物理坐标缓存值 */
    uint8_t  cnt = 0;
    uint16_t d1,d2;
		uint32_t tem1,tem2;
		double fac;
    uint16_t outtime = 0;
    cnt = 0;

    lcd_clear(WHITE);       /* 清屏 */
    lcd_show_string(40, 40, 160, 100, 16, TP_REMIND_MSG_TBL, RED); /* 显示提示信息 */
    tp_draw_touch_point(20, 20, RED);   /* 画点1 */
    tp_dev.sta = 0;         /* 消除触发信号 */
		tp_dev.xfac=0;//xfac用来标记是否校准过,所以校准之前必须清掉!以免错误	
    while (1)               /* 如果连续10秒钟没有按下,则自动退出 */
    {
        tp_dev.scan(1);     /* 扫描物理坐标 */
			  
        if ((tp_dev.sta & 0xc0) == TP_CATH_PRES)   /* 按键按下了一次(此时按键松开了.) */
        {
            outtime = 0;
            tp_dev.sta &= ~(1<<6);    /* 标记按键已经被处理过了. */

            pos_temp[cnt][0] = tp_dev.x[0];      /* 保存X物理坐标 */
            pos_temp[cnt][1] = tp_dev.y[0];      /* 保存Y物理坐标 */
            cnt++;

            switch (cnt)
            {
                case 1:
                    tp_draw_touch_point(20, 20, WHITE);                 /* 清除点1 */
                    tp_draw_touch_point(lcddev.width - 20, 20, RED);    /* 画点2 */
                    break;

                case 2:
                    tp_draw_touch_point(lcddev.width - 20, 20, WHITE);  /* 清除点2 */
                    tp_draw_touch_point(20, lcddev.height - 20, RED);   /* 画点3 */
                    break;

                case 3:
                    tp_draw_touch_point(20, lcddev.height - 20, WHITE); /* 清除点3 */
                    tp_draw_touch_point(lcddev.width - 20, lcddev.height - 20, RED);    /* 画点4 */
                    break;

                case 4:
										tem1=abs(pos_temp[0][0]-pos_temp[1][0]);//x1-x2
										tem2=abs(pos_temp[0][1]-pos_temp[1][1]);//y1-y2
										tem1*=tem1;
										tem2*=tem2;
										d1=sqrt(tem1+tem2);//得到1,2的距离
										
										tem1=abs(pos_temp[2][0]-pos_temp[3][0]);//x3-x4
										tem2=abs(pos_temp[2][1]-pos_temp[3][1]);//y3-y4
										tem1*=tem1;
										tem2*=tem2;
										d2=sqrt(tem1+tem2);//得到3,4的距离
										fac=(float)d1/d2;
										if(fac<0.95||fac>1.05||d1==0||d2==0)//不合格
										{
											cnt=0;
											tp_draw_touch_point(lcddev.width-20,lcddev.height-20,WHITE);	//清除点4
											tp_draw_touch_point(20,20,RED);								//画点1
											printf("12 34错误 \n");
											// tp_adjust_info_show(pos_temp[0][0],pos_temp[0][1],pos_temp[1][0],pos_temp[1][1],pos_temp[2][0],pos_temp[2][1],pos_temp[3][0],pos_temp[3][1],fac*100);//显示数据   
											continue;
										}
										tem1=abs(pos_temp[0][0]-pos_temp[2][0]);//x1-x3
										tem2=abs(pos_temp[0][1]-pos_temp[2][1]);//y1-y3
										tem1*=tem1;
										tem2*=tem2;
										d1=sqrt(tem1+tem2);//得到1,3的距离
										
										tem1=abs(pos_temp[1][0]-pos_temp[3][0]);//x2-x4
										tem2=abs(pos_temp[1][1]-pos_temp[3][1]);//y2-y4
										tem1*=tem1;
										tem2*=tem2;
										d2=sqrt(tem1+tem2);//得到2,4的距离
										fac=(float)d1/d2;
										if(fac<0.95||fac>1.05)//不合格
										{
											cnt=0;
											tp_draw_touch_point(lcddev.width-20,lcddev.height-20,WHITE);	//清除点4
											tp_draw_touch_point(20,20,RED);								//画点1
											printf("13 24错误 \n");
											//TP_Adj_Info_Show(pos_temp[0][0],pos_temp[0][1],pos_temp[1][0],pos_temp[1][1],pos_temp[2][0],pos_temp[2][1],pos_temp[3][0],pos_temp[3][1],fac*100);//显示数据   
											continue;
										}//正确了
										//对角线相等
										tem1=abs(pos_temp[1][0]-pos_temp[2][0]);//x1-x3
										tem2=abs(pos_temp[1][1]-pos_temp[2][1]);//y1-y3
										tem1*=tem1;
										tem2*=tem2;
										d1=sqrt(tem1+tem2);//得到1,4的距离
						
										tem1=abs(pos_temp[0][0]-pos_temp[3][0]);//x2-x4
										tem2=abs(pos_temp[0][1]-pos_temp[3][1]);//y2-y4
										tem1*=tem1;
										tem2*=tem2;
										d2=sqrt(tem1+tem2);//得到2,3的距离
										fac=(float)d1/d2;
										if(fac<0.95||fac>1.05)//不合格
										{
											cnt=0;
											tp_draw_touch_point(lcddev.width-20,lcddev.height-20,WHITE);	//清除点4
											tp_draw_touch_point(20,20,RED);								//画点1
											printf("14 23错误 \n");
											//TP_Adj_Info_Show(pos_temp[0][0],pos_temp[0][1],pos_temp[1][0],pos_temp[1][1],pos_temp[2][0],pos_temp[2][1],pos_temp[3][0],pos_temp[3][1],fac*100);//显示数据   
											continue;
										}//正确了
										//计算结果
										tp_dev.xfac=(float)(lcddev.width-40)/(pos_temp[1][0]-pos_temp[0][0]);//得到xfac		 
										tp_dev.xc=(lcddev.width-tp_dev.xfac*(pos_temp[1][0]+pos_temp[0][0]))/2;//得到xoff
												
										tp_dev.yfac=(float)(lcddev.height-40)/(pos_temp[2][1]-pos_temp[0][1]);//得到yfac
										tp_dev.yc=(lcddev.height-tp_dev.yfac*(pos_temp[2][1]+pos_temp[0][1]))/2;//得到yoff  
										if(abs(tp_dev.xfac)>2||abs(tp_dev.yfac)>2)//触屏和预设的相反了.
										{
											cnt=0;
											tp_draw_touch_point(lcddev.width-20,lcddev.height-20,WHITE);	//清除点4
											tp_draw_touch_point(20,20,RED);								//画点1
											tp_dev.touchtype=!tp_dev.touchtype;//修改触屏类型.
											if(tp_dev.touchtype)//X,Y方向与屏幕相反
											{
												CMD_RDX=0XD0;
												CMD_RDY=0X90; 
											}else				   //X,Y方向与屏幕相同
											{
												CMD_RDX=0X90;
												CMD_RDY=0XD0;	 
											}			
											printf("屏幕相反 \n");
											continue;
										}
										g_point_color = BLUE;
                    lcd_clear(WHITE);   /* 清屏 */
                    lcd_show_string(35, 110, lcddev.width, lcddev.height, 16, "Touch Screen Adjust OK!", BLUE); /* 校正完成 */
                    HAL_Delay(1000);
                    tp_save_adjust_data();

                    lcd_clear(WHITE);   /* 清屏 */
                    return; /* 校正完成 */
            }
        }

        HAL_Delay(10);
        outtime++;

        if (outtime > 1000)
        {
            tp_get_adjust_data();
            break;
        }
    }

}

/**
 * @brief       触摸屏初始化
 * @param       无
 * @retval      0,没有进行校准
 *              1,进行过校准
 */
uint8_t tp_init(void)
{ 
    GPIO_InitTypeDef gpio_init_struct;
    
    tp_dev.touchtype = 0;                   /* 默认设置(电阻屏 & 竖屏) */
    tp_dev.touchtype |= lcddev.dir & 0X01;  /* 根据LCD判定是横屏还是竖屏 */
	
		__HAL_RCC_GPIOB_CLK_ENABLE();    /* T_PEN脚时钟使能 */
		__HAL_RCC_GPIOC_CLK_ENABLE();     /* T_CS脚时钟使能 */

		gpio_init_struct.Pin = LCD_T_PEN_Pin;
		gpio_init_struct.Mode = GPIO_MODE_INPUT;                 /* 输入 */
		gpio_init_struct.Pull = GPIO_PULLUP;                     /* 上拉 */
		gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;      /* 高速 */
		HAL_GPIO_Init(LCD_T_PEN_GPIO_Port, &gpio_init_struct);       /* 初始化T_PEN引脚 */

		gpio_init_struct.Pin = T_MISO_GPIO_PIN;
		HAL_GPIO_Init(T_MISO_GPIO_PORT, &gpio_init_struct);      /* 初始化T_MISO引脚 */

		gpio_init_struct.Pin = T_MOSI_GPIO_PIN;
		gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;             /* 推挽输出 */
		gpio_init_struct.Pull = GPIO_PULLUP;                     /* 上拉 */
		gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;      /* 高速 */
		HAL_GPIO_Init(T_MOSI_GPIO_PORT, &gpio_init_struct);      /* 初始化T_MOSI引脚 */

		gpio_init_struct.Pin = T_CLK_GPIO_PIN;
		HAL_GPIO_Init(T_CLK_GPIO_PORT, &gpio_init_struct);       /* 初始化T_CLK引脚 */

		gpio_init_struct.Pin = LCD_T_CS_Pin;
		HAL_GPIO_Init(LCD_T_CS_GPIO_Port, &gpio_init_struct);        /* 初始化T_CS引脚 */
	
		tp_read_xy(&tp_dev.x[0], &tp_dev.y[0]); /* 第一次读取初始化 */
		at24cxx_init();         /* 初始化24CXX */
		
		if (tp_get_adjust_data())
		{
				return 0;           /* 已经校准 */
		}
		else                    /* 未校准? */
		{
				lcd_clear(WHITE);   /* 清屏 */
				tp_adjust();        /* 屏幕校准 */
				tp_save_adjust_data();
		}

    return 1;
}


/**
 * @brief       清空屏幕并在右上角显示"RST"
 * @param       无
 * @retval      无
 */
void load_draw_dialog(void)
{
    lcd_clear(WHITE);                                                /* 清屏 */
    lcd_show_string(lcddev.width - 24, 0, 200, 16, 16, "RST", BLUE); /* 显示清屏区域 */
}

/**
 * @brief       画粗线
 * @param       x1,y1: 起点坐标
 * @param       x2,y2: 终点坐标
 * @param       size : 线条粗细程度
 * @param       color: 线的颜色
 * @retval      无
 */
void lcd_draw_bline(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t size, uint16_t color)
{
    uint16_t t;
    int xerr = 0, yerr = 0, delta_x, delta_y, distance;
    int incx, incy, row, col;

    if (x1 < size || x2 < size || y1 < size || y2 < size)
        return;

    delta_x = x2 - x1; /* 计算坐标增量 */
    delta_y = y2 - y1;
    row = x1;
    col = y1;

    if (delta_x > 0)
    {
        incx = 1; /* 设置单步方向 */
    }
    else if (delta_x == 0)
    {
        incx = 0; /* 垂直线 */
    }
    else
    {
        incx = -1;
        delta_x = -delta_x;
    }

    if (delta_y > 0)
    {
        incy = 1;
    }
    else if (delta_y == 0)
    {
        incy = 0; /* 水平线 */
    }
    else
    {
        incy = -1;
        delta_y = -delta_y;
    }

    if (delta_x > delta_y)
        distance = delta_x; /* 选取基本增量坐标轴 */
    else
        distance = delta_y;

    for (t = 0; t <= distance + 1; t++) /* 画线输出 */
    {
        lcd_fill_circle(row, col, size, color); /* 画点 */
        xerr += delta_x;
        yerr += delta_y;

        if (xerr > distance)
        {
            xerr -= distance;
            row += incx;
        }

        if (yerr > distance)
        {
            yerr -= distance;
            col += incy;
        }
    }
}

/**
 * @brief       按键扫描函数
 * @note        该函数有响应优先级(同时按下多个按键): WK_UP > KEY2 > KEY1 > KEY0!!
 * @param       mode:0 / 1, 具体含义如下:
 *   @arg       0,  不支持连续按(当按键按下不放时, 只有第一次调用会返回键值,
 *                  必须松开以后, 再次按下才会返回其他键值)
 *   @arg       1,  支持连续按(当按键按下不放时, 每次调用该函数都会返回键值)
 * @retval      键值, 定义如下:
 *              KEY0_PRES, 1, KEY0按下
 *              KEY1_PRES, 2, KEY1按下
 *              KEY2_PRES, 3, KEY2按下
 *              WKUP_PRES, 4, WKUP按下
 */
uint8_t key_scan(uint8_t mode)
{
    static uint8_t key_up = 1;  /* 按键按松开标志 */
    uint8_t keyval = 0;

    if (mode) key_up = 1;       /* 支持连按 */

    if (key_up && (KEY0 == 0 || KEY1 == 0 || KEY2 == 0 || WK_UP == 1))  /* 按键松开标志为1, 且有任意一个按键按下了 */
    {
        HAL_Delay(10);           /* 去抖动 */
        key_up = 0;

        if (KEY0 == 0)  keyval = KEY0_PRES;

        if (KEY1 == 0)  keyval = KEY1_PRES;

        if (KEY2 == 0)  keyval = KEY2_PRES;

        if (WK_UP == 1) keyval = WKUP_PRES;
    }
    else if (KEY0 == 1 && KEY1 == 1 && KEY2 == 1 && WK_UP == 0)         /* 没有任何按键按下, 标记按键松开 */
    {
        key_up = 1;
    }

    return keyval;              /* 返回键值 */
}

/**
 * @brief       电阻触摸屏测试函数
 * @param       无
 * @retval      无
 */
//void rtp_test(void)
//{
//    uint8_t key;
//    uint8_t i = 0;

//    while (1)
//    {
//        key = key_scan(0);
//        tp_dev.scan(0);
//				//printf("LVGL测试 \n");
//        if (tp_dev.sta & TP_PRES_DOWN)  /* 触摸屏被按下 */
//        {
//            if (tp_dev.x[0] < lcddev.width && tp_dev.y[0] < lcddev.height)
//            {
//                if (tp_dev.x[0] > (lcddev.width - 24) && tp_dev.y[0] < 16)
//                {
//                    load_draw_dialog(); /* 清除 */
//                }
//                else
//                {
//                    tp_draw_big_point(tp_dev.x[0], tp_dev.y[0], RED);   /* 画点 */
//                }
//            }
//        }
//        else 
//        {
//            HAL_Delay(10);       /* 没有按键按下的时候 */
//        }

//        if (key == KEY0_PRES)   /* KEY0按下,则执行校准程序 */
//        {
//            lcd_clear(WHITE);   /* 清屏 */
//            tp_adjust();        /* 屏幕校准 */
//            // tp_save_adjust_data();
//            load_draw_dialog();
//        }

//        i++;

//        if (i % 20 == 0) HAL_GPIO_TogglePin(LED0_GPIO_Port,LED0_Pin);
//    }
//}

void rtp_test(void)
{
	uint8_t key;
	uint8_t i=0;	  
	while(1)
	{
	 	key=key_scan(0);
		tp_dev.scan(0); 		 
		if(tp_dev.sta & TP_PRES_DOWN)			//触摸屏被按下
		{	
		 	if(tp_dev.x[0]<lcddev.width&&tp_dev.y[0]<lcddev.height)
			{	
				if(tp_dev.x[0]>(lcddev.width-24)&&tp_dev.y[0]<16){
					printf("LVGL测试 \n");
					load_draw_dialog();//清除
				}
				else {
					printf("X:%x Y:%x \r\n", tp_dev.x[0], tp_dev.y[0]);
					tp_draw_big_point(tp_dev.x[0],tp_dev.y[0],RED);		//画图	 
				}					
			}
		}else HAL_Delay(10);	//没有按键按下的时候 	    
		if(key==KEY0_PRES)	//KEY0按下,则执行校准程序
		{
			lcd_clear(WHITE);	//清屏
		  tp_adjust();  		//屏幕校准 
			tp_save_adjust_data();	 
			load_draw_dialog();
		}
		i++;
		if(i%20==0) HAL_GPIO_TogglePin(LED0_GPIO_Port,LED0_Pin);
	}
}






