/* ili9341.h */
#ifndef __ILI9341_H
#define __ILI9341_H

#include "main.h"
#include "stm32f4xx_hal.h"

#define LCD_BASE    ((uint32_t)(0x60000000 | 0x0001 << 16))
#define LCD_REG     (*((volatile uint16_t *)(LCD_BASE)))
#define LCD_DATA    (*((volatile uint16_t *)(LCD_BASE + 2)))

void ILI9341_Init(void);
void LCD_WriteReg(uint16_t reg);
void LCD_WriteData(uint16_t data);
void LCD_WriteRegData(uint16_t reg, uint16_t data);
void LCD_Clear(uint16_t color);

#endif

