#ifndef _lcd_H
#define _lcd_H

#include "system.h"

#define LCD_WIDTH   320
#define LCD_HEIGHT  480

#define LCD_BLACK   0x0000
#define LCD_WHITE   0xFFFF
#define LCD_RED     0xF800
#define LCD_GREEN   0x07E0
#define LCD_BLUE    0x001F
#define LCD_YELLOW  0xFFE0

void LCD_Init(void);
void LCD_Clear(u16 color);
void LCD_SetWindow(u16 x1, u16 y1, u16 x2, u16 y2);
void LCD_WritePixel(u16 color);
void LCD_DrawTestBars(void);
u16 LCD_ReadRegValue(u16 reg);

#endif
