/**
 ****************************************************************************************************
 * @file        touch.c
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.1
 * @date        2023-05-29
 * @brief       ������ ��������
 *   @note      ֧�ֵ���/����ʽ������
 *              ������������֧��ADS7843/7846/UH7843/7846/XPT2046/TSC2046/GT9147/GT9271/FT5206/GT1151�ȣ�����
 *
 * @license     Copyright (c) 2020-2032, ������������ӿƼ����޹�˾
 ****************************************************************************************************
 * @attention
 *
 * ʵ��ƽ̨:����ԭ�� ̽���� F407������
 * ������Ƶ:www.yuanzige.com
 * ������̳:www.openedv.com
 * ��˾��ַ:www.alientek.com
 * �����ַ:openedv.taobao.com
 *
 * �޸�˵��
 * V1.0 20211025
 * ��һ�η���
 * V1.1 20230529
 * 1��������ST7796 3.5���� GT1151��֧��
 * 2��������ILI9806 4.3���� GT1151��֧��
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
//Ĭ��Ϊtouchtype=0������.
uint8_t CMD_RDX=0XD0;
uint8_t CMD_RDY=0X90;

/**
 * @brief       SPIд����
 *   @note      ������ICд��1 byte����
 * @param       data: Ҫд�������
 * @retval      ��
 */
static void tp_write_byte(uint8_t data)
{
    uint8_t count = 0;

    for (count = 0; count < 8; count++)
    {
        if (data & 0x80)    /* ����1 */
        {
            T_MOSI(1);
        }
        else                /* ����0 */
        {
            T_MOSI(0);
        }

        data <<= 1;
        T_CLK(0);
				for_delay_us(1);
        T_CLK(1);           /* ��������Ч */
    }
}

/**
 * @brief       SPI������
 *   @note      �Ӵ�����IC��ȡadcֵ
 * @param       cmd: ָ��
 * @retval      ��ȡ��������,ADCֵ(12bit)
 */
static uint16_t tp_read_ad(uint8_t cmd)
{
    uint8_t count = 0;
    uint16_t num = 0;
    
    T_CLK(0);           /* ������ʱ�� */
    T_MOSI(0);          /* ���������� */
    T_CS(0);            /* ѡ�д�����IC */
    tp_write_byte(cmd); /* ���������� */
    for_delay_us(6);        /* ADS7846��ת��ʱ���Ϊ6us */
    T_CLK(0);
    for_delay_us(1);
    T_CLK(1);           /* ��1��ʱ�ӣ����BUSY */
    for_delay_us(1);
    T_CLK(0);

    for (count = 0; count < 16; count++)    /* ����16λ����,ֻ�и�12λ��Ч */
    {
        num <<= 1;
        T_CLK(0);       /* �½�����Ч */
        for_delay_us(1);
        T_CLK(1);

        if (T_MISO) num++;
    }

    num >>= 4;          /* ֻ�и�12λ��Ч. */
    T_CS(1);            /* �ͷ�Ƭѡ */
    return num;
}

/* ���败������оƬ ���ݲɼ� �˲��ò��� */
#define TP_READ_TIMES   5       /* ��ȡ���� */
#define TP_LOST_VAL     1       /* ����ֵ */

/**
 * @brief       ��ȡһ������ֵ(x����y)
 *   @note      ������ȡTP_READ_TIMES������,����Щ������������,
 *              Ȼ��ȥ����ͺ����TP_LOST_VAL����, ȡƽ��ֵ
 *              ����ʱ������: TP_READ_TIMES > 2*TP_LOST_VAL ������
 *
 * @param       cmd : ָ��
 *   @arg       0XD0: ��ȡX������(@����״̬,����״̬��Y�Ե�.)
 *   @arg       0X90: ��ȡY������(@����״̬,����״̬��X�Ե�.)
 *
 * @retval      ��ȡ��������(�˲����), ADCֵ(12bit)
 */
static uint16_t tp_read_xoy(uint8_t cmd)
{
    uint16_t i, j;
    uint16_t buf[TP_READ_TIMES];
    uint16_t sum = 0;
    uint16_t temp;

    for (i = 0; i < TP_READ_TIMES; i++)     /* �ȶ�ȡTP_READ_TIMES������ */
    {
        buf[i] = tp_read_ad(cmd);
				//printf("X:%x Y:%x \r\n", buf[i], buf[i]);
    }

    for (i = 0; i < TP_READ_TIMES - 1; i++) /* �����ݽ������� */
    {
        for (j = i + 1; j < TP_READ_TIMES; j++)
        {
            if (buf[i] > buf[j])   /* �������� */
            {
                temp = buf[i];
                buf[i] = buf[j];
                buf[j] = temp;
            }
        }
    }

    sum = 0;

    for (i = TP_LOST_VAL; i < TP_READ_TIMES - TP_LOST_VAL; i++)   /* ȥ�����˵Ķ���ֵ */
    {
        sum += buf[i];  /* �ۼ�ȥ������ֵ�Ժ������. */
    }

    temp = sum / (TP_READ_TIMES - 2 * TP_LOST_VAL); /* ȡƽ��ֵ */
    return temp;
}

/**
 * @brief       ��ȡx, y����
 * @param       x,y: ��ȡ��������ֵ
 * @retval      ��
 */
uint8_t tp_read_xy(uint16_t *x, uint16_t *y)
{
    uint16_t xval, yval;
		xval=tp_read_xoy(CMD_RDX);
		yval=tp_read_xoy(CMD_RDY);	  

    *x = xval;
    *y = yval;
		return 1;//�����ɹ�
}

/* �������ζ�ȡX,Y�������������������ֵ */
#define TP_ERR_RANGE    50      /* ��Χ */

/**
 * @brief       ������ȡ2�δ���IC����, ���˲�
 *   @note      ����2�ζ�ȡ������IC,�������ε�ƫ��ܳ���ERR_RANGE,����
 *              ����,����Ϊ������ȷ,�����������.�ú����ܴ�����׼ȷ��.
 *
 * @param       x,y: ��ȡ��������ֵ
 * @retval      0, ʧ��; 1, �ɹ�;
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
    if(((x2<=x1&&x1<x2+TP_ERR_RANGE)||(x1<=x2&&x2<x1+TP_ERR_RANGE))//ǰ�����β�����+-50��
    &&((y2<=y1&&y1<y2+TP_ERR_RANGE)||(y1<=y2&&y2<y1+TP_ERR_RANGE)))
    {
        *x=(x1+x2)/2;
        *y=(y1+y2)/2;
        return 1;
    }else return 0;
}

/******************************************************************************************/
/* ��LCD�����йصĺ���, ����У׼�õ� */

/**
 * @brief       ��һ��У׼�õĴ�����(ʮ�ּ�)
 * @param       x,y   : ����
 * @param       color : ��ɫ
 * @retval      ��
 */
static void tp_draw_touch_point(uint16_t x, uint16_t y, uint16_t color)
{
    lcd_draw_line(x - 12, y, x + 13, y, color); /* ���� */
    lcd_draw_line(x, y - 12, x, y + 13, color); /* ���� */
    lcd_draw_point(x + 1, y + 1, color);
    lcd_draw_point(x - 1, y + 1, color);
    lcd_draw_point(x + 1, y - 1, color);
    lcd_draw_point(x - 1, y - 1, color);
    lcd_draw_circle(x, y, 6, color);            /* ������Ȧ */
}

/**
 * @brief       ��һ�����(2*2�ĵ�)
 * @param       x,y   : ����
 * @param       color : ��ɫ
 * @retval      ��
 */
void tp_draw_big_point(uint16_t x, uint16_t y, uint16_t color)
{
    lcd_draw_point(x, y, color);       /* ���ĵ� */
    lcd_draw_point(x + 1, y, color);
    lcd_draw_point(x, y + 1, color);
    lcd_draw_point(x + 1, y + 1, color);
}

/******************************************************************************************/

/**
 * @brief       ��������ɨ��
 * @param       mode: ����ģʽ
 *   @arg       0, ��Ļ����;
 *   @arg       1, ��������(У׼�����ⳡ����)
 *
 * @retval      0, �����޴���; 1, �����д���;
 */
static uint8_t tp_scan(uint8_t mode)
{
    if (T_PEN == 0)     /* �а������� */
    {
        if (mode)       /* ��ȡ��������, ����ת�� */
        {
            tp_read_xy2(&tp_dev.x[0], &tp_dev.y[0]);
        }
        else if (tp_read_xy2(&tp_dev.x[0], &tp_dev.y[0]))     /* ��ȡ��Ļ����, ��Ҫת�� */
        {
           tp_dev.x[0]=tp_dev.xfac*tp_dev.x[0]+tp_dev.xc;//�����ת��Ϊ��Ļ����
					 tp_dev.y[0]=tp_dev.yfac*tp_dev.y[0]+tp_dev.yc; 
        }

        if ((tp_dev.sta & TP_PRES_DOWN) == 0)   /* ֮ǰû�б����� */
        {
            tp_dev.sta = TP_PRES_DOWN | TP_CATH_PRES;   /* �������� */
            tp_dev.x[4]=tp_dev.x[0];   /* ��¼��һ�ΰ���ʱ������ */
            tp_dev.y[4]=tp_dev.y[0];
        }
    }
    else
    {
        if (tp_dev.sta & TP_PRES_DOWN)      /* ֮ǰ�Ǳ����µ� */
        {
            tp_dev.sta &= ~(1<<7);    /* ��ǰ����ɿ� */
        }
        else     /* ֮ǰ��û�б����� */
        {
            tp_dev.x[4]=0;
						tp_dev.y[4]=0;
						tp_dev.x[0]=0xffff;
						tp_dev.y[0]=0xffff;
        }
    }

    return tp_dev.sta & TP_PRES_DOWN; /* ���ص�ǰ�Ĵ���״̬ */
}

/* TP_SAVE_ADDR_BASE���崥����У׼����������EEPROM�����λ��(��ʼ��ַ)
 * ռ�ÿռ� : 13�ֽ�.
 */
#define TP_SAVE_ADDR_BASE   40

/**
 * @brief       ����У׼����
 *   @note      ����������EEPROMоƬ����(24C02),��ʼ��ַΪTP_SAVE_ADDR_BASE.
 *              ռ�ô�СΪ13�ֽ�
 * @param       ��
 * @retval      ��
 */
void tp_save_adjust_data(void)
{
  uint8_t *p = (uint8_t *)&tp_dev.xfac;   /* ָ���׵�ַ */

    /* pָ��tp_dev.xfac�ĵ�ַ, p+4����tp_dev.yfac�ĵ�ַ
     * p+8����tp_dev.xoff�ĵ�ַ,p+10,����tp_dev.yoff�ĵ�ַ
     * �ܹ�ռ��12���ֽ�(4������)
     * p+12���ڴ�ű�ǵ��败�����Ƿ�У׼������(0X0A)
     * ��p[12]д��0X0A. ����Ѿ�У׼��.
     */
    at24cxx_write(TP_SAVE_ADDR_BASE, p, 12);                /* ����12���ֽ�����(xfac,yfac,xc,yc) */
    at24cxx_write_one_byte(TP_SAVE_ADDR_BASE + 12, 0X0A);   /* ����У׼ֵ */
}

/**
 * @brief       ��ȡ������EEPROM�����У׼ֵ
 * @param       ��
 * @retval      0����ȡʧ�ܣ�Ҫ����У׼
 *              1���ɹ���ȡ����
 */
uint8_t tp_get_adjust_data(void)
{
    uint8_t *p = (uint8_t *)&tp_dev.xfac;
    uint8_t temp = 0;

    /* ����������ֱ��ָ��tp_dev.xfac��ַ���б����, ��ȡ��ʱ��,����ȡ����������
     * д��ָ��tp_dev.xfac���׵�ַ, �Ϳ��Ի�ԭд���ȥ��ֵ, ������Ҫ���������
     * ������. �˷��������ڸ�������(�����ṹ��)�ı���/��ȡ(�����ṹ��).
     */
    at24cxx_read(TP_SAVE_ADDR_BASE, p, 12);                 /* ��ȡ12�ֽ����� */
    temp = at24cxx_read_one_byte(TP_SAVE_ADDR_BASE + 12);   /* ��ȡУ׼״̬��� */

    if (temp == 0X0A)
    {
        return 1;
    }

    return 0;
}

/* ��ʾ�ַ��� */
char *const TP_REMIND_MSG_TBL = "Please use the stylus click the cross on the screen.The cross will always move until the screen adjustment is completed.";

/**
 * @brief       ������У׼����
 *   @note      ʹ�����У׼��(����ԭ����ٶ�)
 *              �������õ�x��/y���������xfac/yfac��������������ֵ(xc,yc)��4������
 *              ���ǹ涨: �������꼴AD�ɼ���������ֵ,��Χ��0~4095.
 *                        �߼����꼴LCD��Ļ������, ��ΧΪLCD��Ļ�ķֱ���.
 *
 * @param       ��
 * @retval      ��
 */
void tp_adjust(void)
{
    uint16_t pos_temp[4][2];     /* �������껺��ֵ */
    uint8_t  cnt = 0;
    uint16_t d1,d2;
		uint32_t tem1,tem2;
		double fac;
    uint16_t outtime = 0;
    cnt = 0;

    lcd_clear(WHITE);       /* ���� */
    lcd_show_string(40, 40, 160, 100, 16, TP_REMIND_MSG_TBL, RED); /* ��ʾ��ʾ��Ϣ */
    tp_draw_touch_point(20, 20, RED);   /* ����1 */
    tp_dev.sta = 0;         /* ���������ź� */
		tp_dev.xfac=0;//xfac��������Ƿ�У׼��,����У׼֮ǰ�������!�������	
    while (1)               /* �������10����û�а���,���Զ��˳� */
    {
        tp_dev.scan(1);     /* ɨ���������� */
			  
        if ((tp_dev.sta & 0xc0) == TP_CATH_PRES)   /* ����������һ��(��ʱ�����ɿ���.) */
        {
            outtime = 0;
            tp_dev.sta &= ~(1<<6);    /* ��ǰ����Ѿ����������. */

            pos_temp[cnt][0] = tp_dev.x[0];      /* ����X�������� */
            pos_temp[cnt][1] = tp_dev.y[0];      /* ����Y�������� */
            cnt++;

            switch (cnt)
            {
                case 1:
                    tp_draw_touch_point(20, 20, WHITE);                 /* �����1 */
                    tp_draw_touch_point(lcddev.width - 20, 20, RED);    /* ����2 */
                    break;

                case 2:
                    tp_draw_touch_point(lcddev.width - 20, 20, WHITE);  /* �����2 */
                    tp_draw_touch_point(20, lcddev.height - 20, RED);   /* ����3 */
                    break;

                case 3:
                    tp_draw_touch_point(20, lcddev.height - 20, WHITE); /* �����3 */
                    tp_draw_touch_point(lcddev.width - 20, lcddev.height - 20, RED);    /* ����4 */
                    break;

                case 4:
										tem1=abs(pos_temp[0][0]-pos_temp[1][0]);//x1-x2
										tem2=abs(pos_temp[0][1]-pos_temp[1][1]);//y1-y2
										tem1*=tem1;
										tem2*=tem2;
										d1=sqrt(tem1+tem2);//�õ�1,2�ľ���
										
										tem1=abs(pos_temp[2][0]-pos_temp[3][0]);//x3-x4
										tem2=abs(pos_temp[2][1]-pos_temp[3][1]);//y3-y4
										tem1*=tem1;
										tem2*=tem2;
										d2=sqrt(tem1+tem2);//�õ�3,4�ľ���
										fac=(float)d1/d2;
										if(fac<0.95||fac>1.05||d1==0||d2==0)//���ϸ�
										{
											cnt=0;
											tp_draw_touch_point(lcddev.width-20,lcddev.height-20,WHITE);	//�����4
											tp_draw_touch_point(20,20,RED);								//����1
											printf("12 34���� \n");
											// tp_adjust_info_show(pos_temp[0][0],pos_temp[0][1],pos_temp[1][0],pos_temp[1][1],pos_temp[2][0],pos_temp[2][1],pos_temp[3][0],pos_temp[3][1],fac*100);//��ʾ����   
											continue;
										}
										tem1=abs(pos_temp[0][0]-pos_temp[2][0]);//x1-x3
										tem2=abs(pos_temp[0][1]-pos_temp[2][1]);//y1-y3
										tem1*=tem1;
										tem2*=tem2;
										d1=sqrt(tem1+tem2);//�õ�1,3�ľ���
										
										tem1=abs(pos_temp[1][0]-pos_temp[3][0]);//x2-x4
										tem2=abs(pos_temp[1][1]-pos_temp[3][1]);//y2-y4
										tem1*=tem1;
										tem2*=tem2;
										d2=sqrt(tem1+tem2);//�õ�2,4�ľ���
										fac=(float)d1/d2;
										if(fac<0.95||fac>1.05)//���ϸ�
										{
											cnt=0;
											tp_draw_touch_point(lcddev.width-20,lcddev.height-20,WHITE);	//�����4
											tp_draw_touch_point(20,20,RED);								//����1
											printf("13 24���� \n");
											//TP_Adj_Info_Show(pos_temp[0][0],pos_temp[0][1],pos_temp[1][0],pos_temp[1][1],pos_temp[2][0],pos_temp[2][1],pos_temp[3][0],pos_temp[3][1],fac*100);//��ʾ����   
											continue;
										}//��ȷ��
										//�Խ������
										tem1=abs(pos_temp[1][0]-pos_temp[2][0]);//x1-x3
										tem2=abs(pos_temp[1][1]-pos_temp[2][1]);//y1-y3
										tem1*=tem1;
										tem2*=tem2;
										d1=sqrt(tem1+tem2);//�õ�1,4�ľ���
						
										tem1=abs(pos_temp[0][0]-pos_temp[3][0]);//x2-x4
										tem2=abs(pos_temp[0][1]-pos_temp[3][1]);//y2-y4
										tem1*=tem1;
										tem2*=tem2;
										d2=sqrt(tem1+tem2);//�õ�2,3�ľ���
										fac=(float)d1/d2;
										if(fac<0.95||fac>1.05)//���ϸ�
										{
											cnt=0;
											tp_draw_touch_point(lcddev.width-20,lcddev.height-20,WHITE);	//�����4
											tp_draw_touch_point(20,20,RED);								//����1
											printf("14 23���� \n");
											//TP_Adj_Info_Show(pos_temp[0][0],pos_temp[0][1],pos_temp[1][0],pos_temp[1][1],pos_temp[2][0],pos_temp[2][1],pos_temp[3][0],pos_temp[3][1],fac*100);//��ʾ����   
											continue;
										}//��ȷ��
										//������
										tp_dev.xfac=(float)(lcddev.width-40)/(pos_temp[1][0]-pos_temp[0][0]);//�õ�xfac		 
										tp_dev.xc=(lcddev.width-tp_dev.xfac*(pos_temp[1][0]+pos_temp[0][0]))/2;//�õ�xoff
												
										tp_dev.yfac=(float)(lcddev.height-40)/(pos_temp[2][1]-pos_temp[0][1]);//�õ�yfac
										tp_dev.yc=(lcddev.height-tp_dev.yfac*(pos_temp[2][1]+pos_temp[0][1]))/2;//�õ�yoff  
										if(abs(tp_dev.xfac)>2||abs(tp_dev.yfac)>2)//������Ԥ����෴��.
										{
											cnt=0;
											tp_draw_touch_point(lcddev.width-20,lcddev.height-20,WHITE);	//�����4
											tp_draw_touch_point(20,20,RED);								//����1
											tp_dev.touchtype=!tp_dev.touchtype;//�޸Ĵ�������.
											if(tp_dev.touchtype)//X,Y��������Ļ�෴
											{
												CMD_RDX=0XD0;
												CMD_RDY=0X90; 
											}else				   //X,Y��������Ļ��ͬ
											{
												CMD_RDX=0X90;
												CMD_RDY=0XD0;	 
											}			
											printf("��Ļ�෴ \n");
											continue;
										}
										g_point_color = BLUE;
                    lcd_clear(WHITE);   /* ���� */
                    lcd_show_string(35, 110, lcddev.width, lcddev.height, 16, "Touch Screen Adjust OK!", BLUE); /* У����� */
                    HAL_Delay(1000);
                    tp_save_adjust_data();

                    lcd_clear(WHITE);   /* ���� */
                    return; /* У����� */
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
 * @brief       ��������ʼ��
 * @param       ��
 * @retval      0,û�н���У׼
 *              1,���й�У׼
 */
uint8_t tp_init(void)
{ 
    GPIO_InitTypeDef gpio_init_struct;
    
    tp_dev.touchtype = 0;                   /* Ĭ������(������ & ����) */
    tp_dev.touchtype |= lcddev.dir & 0X01;  /* ����LCD�ж��Ǻ����������� */
	
		__HAL_RCC_GPIOB_CLK_ENABLE();    /* T_PEN��ʱ��ʹ�� */
		__HAL_RCC_GPIOC_CLK_ENABLE();     /* T_CS��ʱ��ʹ�� */

		gpio_init_struct.Pin = LCD_T_PEN_Pin;
		gpio_init_struct.Mode = GPIO_MODE_INPUT;                 /* ���� */
		gpio_init_struct.Pull = GPIO_PULLUP;                     /* ���� */
		gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;      /* ���� */
		HAL_GPIO_Init(LCD_T_PEN_GPIO_Port, &gpio_init_struct);       /* ��ʼ��T_PEN���� */

		gpio_init_struct.Pin = T_MISO_GPIO_PIN;
		HAL_GPIO_Init(T_MISO_GPIO_PORT, &gpio_init_struct);      /* ��ʼ��T_MISO���� */

		gpio_init_struct.Pin = T_MOSI_GPIO_PIN;
		gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;             /* ������� */
		gpio_init_struct.Pull = GPIO_PULLUP;                     /* ���� */
		gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;      /* ���� */
		HAL_GPIO_Init(T_MOSI_GPIO_PORT, &gpio_init_struct);      /* ��ʼ��T_MOSI���� */

		gpio_init_struct.Pin = T_CLK_GPIO_PIN;
		HAL_GPIO_Init(T_CLK_GPIO_PORT, &gpio_init_struct);       /* ��ʼ��T_CLK���� */

		gpio_init_struct.Pin = LCD_T_CS_Pin;
		HAL_GPIO_Init(LCD_T_CS_GPIO_Port, &gpio_init_struct);        /* ��ʼ��T_CS���� */
	
		tp_read_xy(&tp_dev.x[0], &tp_dev.y[0]); /* ��һ�ζ�ȡ��ʼ�� */
		at24cxx_init();         /* ��ʼ��24CXX */
		
		if (tp_get_adjust_data())
		{
				return 0;           /* �Ѿ�У׼ */
		}
		else                    /* δУ׼? */
		{
				lcd_clear(WHITE);   /* ���� */
				tp_adjust();        /* ��ĻУ׼ */
				tp_save_adjust_data();
		}

    return 1;
}


/**
 * @brief       �����Ļ�������Ͻ���ʾ"RST"
 * @param       ��
 * @retval      ��
 */
void load_draw_dialog(void)
{
    lcd_clear(WHITE);                                                /* ���� */
    lcd_show_string(lcddev.width - 24, 0, 200, 16, 16, "RST", BLUE); /* ��ʾ�������� */
}

/**
 * @brief       ������
 * @param       x1,y1: �������
 * @param       x2,y2: �յ�����
 * @param       size : ������ϸ�̶�
 * @param       color: �ߵ���ɫ
 * @retval      ��
 */
void lcd_draw_bline(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t size, uint16_t color)
{
    uint16_t t;
    int xerr = 0, yerr = 0, delta_x, delta_y, distance;
    int incx, incy, row, col;

    if (x1 < size || x2 < size || y1 < size || y2 < size)
        return;

    delta_x = x2 - x1; /* ������������ */
    delta_y = y2 - y1;
    row = x1;
    col = y1;

    if (delta_x > 0)
    {
        incx = 1; /* ���õ������� */
    }
    else if (delta_x == 0)
    {
        incx = 0; /* ��ֱ�� */
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
        incy = 0; /* ˮƽ�� */
    }
    else
    {
        incy = -1;
        delta_y = -delta_y;
    }

    if (delta_x > delta_y)
        distance = delta_x; /* ѡȡ�������������� */
    else
        distance = delta_y;

    for (t = 0; t <= distance + 1; t++) /* ������� */
    {
        lcd_fill_circle(row, col, size, color); /* ���� */
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
 * @brief       ����ɨ�躯��
 * @note        �ú�������Ӧ���ȼ�(ͬʱ���¶������): WK_UP > KEY2 > KEY1 > KEY0!!
 * @param       mode:0 / 1, ���庬������:
 *   @arg       0,  ��֧��������(���������²���ʱ, ֻ�е�һ�ε��û᷵�ؼ�ֵ,
 *                  �����ɿ��Ժ�, �ٴΰ��²Ż᷵��������ֵ)
 *   @arg       1,  ֧��������(���������²���ʱ, ÿ�ε��øú������᷵�ؼ�ֵ)
 * @retval      ��ֵ, ��������:
 *              KEY0_PRES, 1, KEY0����
 *              KEY1_PRES, 2, KEY1����
 *              KEY2_PRES, 3, KEY2����
 *              WKUP_PRES, 4, WKUP����
 */
uint8_t key_scan(uint8_t mode)
{
    static uint8_t key_up = 1;  /* �������ɿ���־ */
    uint8_t keyval = 0;

    if (mode) key_up = 1;       /* ֧������ */

    if (key_up && (KEY0 == 0 || KEY1 == 0 || KEY2 == 0 || WK_UP == 1))  /* �����ɿ���־Ϊ1, ��������һ������������ */
    {
        HAL_Delay(10);           /* ȥ���� */
        key_up = 0;

        if (KEY0 == 0)  keyval = KEY0_PRES;

        if (KEY1 == 0)  keyval = KEY1_PRES;

        if (KEY2 == 0)  keyval = KEY2_PRES;

        if (WK_UP == 1) keyval = WKUP_PRES;
    }
    else if (KEY0 == 1 && KEY1 == 1 && KEY2 == 1 && WK_UP == 0)         /* û���κΰ�������, ��ǰ����ɿ� */
    {
        key_up = 1;
    }

    return keyval;              /* ���ؼ�ֵ */
}

/**
 * @brief       ���败�������Ժ���
 * @param       ��
 * @retval      ��
 */
//void rtp_test(void)
//{
//    uint8_t key;
//    uint8_t i = 0;

//    while (1)
//    {
//        key = key_scan(0);
//        tp_dev.scan(0);
//				//printf("LVGL���� \n");
//        if (tp_dev.sta & TP_PRES_DOWN)  /* ������������ */
//        {
//            if (tp_dev.x[0] < lcddev.width && tp_dev.y[0] < lcddev.height)
//            {
//                if (tp_dev.x[0] > (lcddev.width - 24) && tp_dev.y[0] < 16)
//                {
//                    load_draw_dialog(); /* ��� */
//                }
//                else
//                {
//                    tp_draw_big_point(tp_dev.x[0], tp_dev.y[0], RED);   /* ���� */
//                }
//            }
//        }
//        else 
//        {
//            HAL_Delay(10);       /* û�а������µ�ʱ�� */
//        }

//        if (key == KEY0_PRES)   /* KEY0����,��ִ��У׼���� */
//        {
//            lcd_clear(WHITE);   /* ���� */
//            tp_adjust();        /* ��ĻУ׼ */
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
		if(tp_dev.sta & TP_PRES_DOWN)			//������������
		{	
		 	if(tp_dev.x[0]<lcddev.width&&tp_dev.y[0]<lcddev.height)
			{	
				if(tp_dev.x[0]>(lcddev.width-24)&&tp_dev.y[0]<16){
					printf("LVGL���� \n");
					load_draw_dialog();//���
				}
				else {
					printf("X:%x Y:%x \r\n", tp_dev.x[0], tp_dev.y[0]);
					tp_draw_big_point(tp_dev.x[0],tp_dev.y[0],RED);		//��ͼ	 
				}					
			}
		}else HAL_Delay(10);	//û�а������µ�ʱ�� 	    
		if(key==KEY0_PRES)	//KEY0����,��ִ��У׼����
		{
			lcd_clear(WHITE);	//����
		  tp_adjust();  		//��ĻУ׼ 
			tp_save_adjust_data();	 
			load_draw_dialog();
		}
		i++;
		if(i%20==0) HAL_GPIO_TogglePin(LED0_GPIO_Port,LED0_Pin);
	}
}






