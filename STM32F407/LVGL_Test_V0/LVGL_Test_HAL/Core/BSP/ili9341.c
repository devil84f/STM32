/* ili9341.c */
#include "ili9341.h"

void LCD_WriteReg(uint16_t reg) {
    LCD_REG = reg;
}

void LCD_WriteData(uint16_t data) {
    LCD_DATA = data;
}

void LCD_WriteRegData(uint16_t reg, uint16_t data) {
    LCD_WriteReg(reg);
    LCD_WriteData(data);
}

void ILI9341_Init(void) {
    HAL_Delay(100);

    LCD_WriteReg(0x01); // Software Reset
    HAL_Delay(100);

    LCD_WriteReg(0x28); // Display OFF

    LCD_WriteReg(0xCF);
    LCD_WriteData(0x00);
    LCD_WriteData(0x83);
    LCD_WriteData(0x30);

    LCD_WriteReg(0xED);
    LCD_WriteData(0x64);
    LCD_WriteData(0x03);
    LCD_WriteData(0x12);
    LCD_WriteData(0x81);

    LCD_WriteReg(0xE8);
    LCD_WriteData(0x85);
    LCD_WriteData(0x01);
    LCD_WriteData(0x79);

    LCD_WriteReg(0xCB);
    LCD_WriteData(0x39);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x00);
    LCD_WriteData(0x34);
    LCD_WriteData(0x02);

    LCD_WriteReg(0xF7);
    LCD_WriteData(0x20);

    LCD_WriteReg(0xEA);
    LCD_WriteData(0x00);
    LCD_WriteData(0x00);

    LCD_WriteReg(0xC0); // Power control 1
    LCD_WriteData(0x26);

    LCD_WriteReg(0xC1); // Power control 2
    LCD_WriteData(0x11);

    LCD_WriteReg(0xC5); // VCOM control 1
    LCD_WriteData(0x35);
    LCD_WriteData(0x3E);

    LCD_WriteReg(0xC7); // VCOM control 2
    LCD_WriteData(0xBE);

    LCD_WriteReg(0x36); // Memory Access Control
    LCD_WriteData(0x48);

    LCD_WriteReg(0x3A); // Pixel Format Set
    LCD_WriteData(0x55); // 16bit/pixel

    LCD_WriteReg(0xB1); // Frame Rate Control
    LCD_WriteData(0x00);
    LCD_WriteData(0x1B);

    LCD_WriteReg(0xF2); // Enable 3G
    LCD_WriteData(0x08);

    LCD_WriteReg(0x26); // Gamma Set
    LCD_WriteData(0x01);

    LCD_WriteReg(0xE0); // Positive Gamma Correction
    LCD_WriteData(0x1F); LCD_WriteData(0x1A); LCD_WriteData(0x18); LCD_WriteData(0x0A);
    LCD_WriteData(0x0F); LCD_WriteData(0x06); LCD_WriteData(0x45); LCD_WriteData(0x87);
    LCD_WriteData(0x32); LCD_WriteData(0x0A); LCD_WriteData(0x07); LCD_WriteData(0x02);
    LCD_WriteData(0x07); LCD_WriteData(0x05); LCD_WriteData(0x00);

    LCD_WriteReg(0xE1); // Negative Gamma Correction
    LCD_WriteData(0x00); LCD_WriteData(0x25); LCD_WriteData(0x27); LCD_WriteData(0x05);
    LCD_WriteData(0x10); LCD_WriteData(0x09); LCD_WriteData(0x3A); LCD_WriteData(0x78);
    LCD_WriteData(0x4D); LCD_WriteData(0x05); LCD_WriteData(0x18); LCD_WriteData(0x0D);
    LCD_WriteData(0x38); LCD_WriteData(0x3A); LCD_WriteData(0x1F);

    LCD_WriteReg(0x11); // Sleep OUT
    HAL_Delay(120);

    LCD_WriteReg(0x29); // Display ON
    LCD_WriteReg(0x2C); // Memory Write
}

void LCD_Clear(uint16_t color) {
    uint32_t i;

    LCD_WriteReg(0x2A); // Column addr set
    LCD_WriteData(0x00);
    LCD_WriteData(0x00);
    LCD_WriteData(0x00);
    LCD_WriteData(0xEF);

    LCD_WriteReg(0x2B); // Row addr set
    LCD_WriteData(0x00);
    LCD_WriteData(0x00);
    LCD_WriteData(0x01);
    LCD_WriteData(0x3F);

    LCD_WriteReg(0x2C); // Memory write
    for(i = 0; i < 240*320; i++) {
        LCD_WriteData(color);
    }
}

