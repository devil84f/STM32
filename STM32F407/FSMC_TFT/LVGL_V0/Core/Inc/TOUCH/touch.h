/**
 ****************************************************************************************************
 * @file        touch.h
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

#ifndef __TOUCH_H__
#define __TOUCH_H__

#include "main.h"

/******************************************************************************************/

#define KEY0        HAL_GPIO_ReadPin(K0_GPIO_Port, K0_Pin)     /* ��ȡKEY0���� */
#define KEY1        HAL_GPIO_ReadPin(K1_GPIO_Port, K1_Pin)     /* ��ȡKEY1���� */
#define KEY2        HAL_GPIO_ReadPin(K2_GPIO_Port, K2_Pin)     /* ��ȡKEY2���� */
#define WK_UP       HAL_GPIO_ReadPin(WKUP_GPIO_Port, WKUP_Pin)     /* ��ȡWKUP���� */


#define KEY0_PRES    1              /* KEY0���� */
#define KEY1_PRES    2              /* KEY1���� */
#define KEY2_PRES    3              /* KEY2���� */
#define WKUP_PRES    4              /* KEY_UP����(��WK_UP) */

/******************************************************************************************/
/* ���败��������IC T_PEN/T_CS/T_MISO/T_MOSI/T_SCK ���� ���� */
#define T_MISO_GPIO_PORT                GPIOB
#define T_MISO_GPIO_PIN                 GPIO_PIN_14

#define T_MOSI_GPIO_PORT                GPIOB
#define T_MOSI_GPIO_PIN                 GPIO_PIN_15

#define T_CLK_GPIO_PORT                 GPIOB
#define T_CLK_GPIO_PIN                  GPIO_PIN_13

/******************************************************************************************/

/* ���败������������ */
#define T_PEN           HAL_GPIO_ReadPin(LCD_T_PEN_GPIO_Port, LCD_T_PEN_Pin)             /* T_PEN */
#define T_MISO          HAL_GPIO_ReadPin(T_MISO_GPIO_PORT, T_MISO_GPIO_PIN)           /* T_MISO */

#define T_MOSI(x)     do{ x ? \
                          HAL_GPIO_WritePin(T_MOSI_GPIO_PORT, T_MOSI_GPIO_PIN, GPIO_PIN_SET) : \
                          HAL_GPIO_WritePin(T_MOSI_GPIO_PORT, T_MOSI_GPIO_PIN, GPIO_PIN_RESET); \
                      }while(0)     /* T_MOSI */

#define T_CLK(x)      do{ x ? \
                          HAL_GPIO_WritePin(T_CLK_GPIO_PORT, T_CLK_GPIO_PIN, GPIO_PIN_SET) : \
                          HAL_GPIO_WritePin(T_CLK_GPIO_PORT, T_CLK_GPIO_PIN, GPIO_PIN_RESET); \
                      }while(0)     /* T_CLK */

#define T_CS(x)       do{ x ? \
                          HAL_GPIO_WritePin(LCD_T_CS_GPIO_Port, LCD_T_CS_Pin, GPIO_PIN_SET) : \
                          HAL_GPIO_WritePin(LCD_T_CS_GPIO_Port, LCD_T_CS_Pin, GPIO_PIN_RESET); \
                      }while(0)     /* T_CS */


#define TP_PRES_DOWN    0x80  /* ���������� */
#define TP_CATH_PRES    0x40  /* �а��������� */
#define CT_MAX_TOUCH    5      /* ������֧�ֵĵ���,�̶�Ϊ5�� */

/* ������������ */
typedef struct
{
    uint8_t (*init)(void);      /* ��ʼ�������������� */
    uint8_t (*scan)(uint8_t);   /* ɨ�败����.0,��Ļɨ��;1,��������; */
    void (*adjust)(void);       /* ������У׼ */
    uint16_t x[CT_MAX_TOUCH];   /* ��ǰ���� */
    uint16_t y[CT_MAX_TOUCH];   /* �����������10������,����������x[0],y[0]����:�˴�ɨ��ʱ,����������,��
                                 * x[9],y[9]�洢��һ�ΰ���ʱ������.
                                 */

    uint16_t sta;               /* �ʵ�״̬
                                 * b15:����1/�ɿ�0;
                                 * b14:0,û�а�������;1,�а�������.
                                 * b13~b10:����
                                 * b9~b0:���ݴ��������µĵ���(0,��ʾδ����,1��ʾ����)
                                 */

    /* 5��У׼������У׼����(����������ҪУ׼) */
    float xfac;                 /* 5��У׼��x����������� */
    float yfac;                 /* 5��У׼��y����������� */
    short xc;                   /* ����X��������ֵ(ADֵ) */
    short yc;                   /* ����Y��������ֵ(ADֵ) */

    /* �����Ĳ���,��������������������ȫ�ߵ�ʱ��Ҫ�õ�.
     * b0:0, ����(�ʺ�����ΪX����,����ΪY�����TP)
     *    1, ����(�ʺ�����ΪY����,����ΪX�����TP)
     * b1~6: ����.
     * b7:0, ������
     *    1, ������
     */
    uint8_t touchtype;
} _m_tp_dev;

extern _m_tp_dev tp_dev;        /* ������������touch.c���涨�� */

/* ����ɨ�躯�� */
uint8_t key_scan(uint8_t mode);     /* ����ɨ�躯�� */

/* ���������� */

static void tp_write_byte(uint8_t data);                /* �����оƬд��һ������ */
static uint16_t tp_read_ad(uint8_t cmd);                /* ��ȡADת��ֵ */
static uint16_t tp_read_xoy(uint8_t cmd);               /* ���˲��������ȡ(X/Y) */
uint8_t tp_read_xy(uint16_t *x, uint16_t *y);       /* ˫�����ȡ(X+Y) */
static uint8_t tp_read_xy2(uint16_t *x, uint16_t *y);   /* ����ǿ�˲���˫���������ȡ */
static void tp_draw_touch_point(uint16_t x, uint16_t y, uint16_t color);    /* ��һ������У׼�� */
static void tp_adjust_info_show(uint16_t xy[5][2], double px, double py);   /* ��ʾУ׼��Ϣ */

uint8_t tp_init(void);                 /* ��ʼ�� */
static uint8_t tp_scan(uint8_t mode);  /* ɨ�� */
void tp_adjust(void);                  /* ������У׼ */
void tp_save_adjust_data(void);        /* ����У׼���� */
uint8_t tp_get_adjust_data(void);      /* ��ȡУ׼���� */
void tp_draw_big_point(uint16_t x, uint16_t y, uint16_t color); /* ��һ����� */

void load_draw_dialog(void);
void lcd_draw_bline(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t size, uint16_t color);
uint8_t key_scan(uint8_t mode);
void rtp_test(void);

#endif

















