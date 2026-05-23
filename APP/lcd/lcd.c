#include "lcd.h"
#include "SysTick.h"

typedef struct
{
	vu16 *reg;
	vu16 *ram;
} LCD_BusDef;

static LCD_BusDef lcd_bus[] =
{
	{(vu16 *)(0x60000000 | 0x00000000), (vu16 *)(0x60000000 | 0x00000002)}, /* NE1, RS=A0  */
	{(vu16 *)(0x60000000 | 0x000000FE), (vu16 *)(0x60000000 | 0x00000100)}, /* NE1, RS=A6  */
	{(vu16 *)(0x60000000 | 0x000007FE), (vu16 *)(0x60000000 | 0x00000800)}, /* NE1, RS=A10 */
	{(vu16 *)(0x60000000 | 0x0001FFFE), (vu16 *)(0x60000000 | 0x00020000)}, /* NE1, RS=A16 */
	{(vu16 *)(0x60000000 | 0x0007FFFE), (vu16 *)(0x60000000 | 0x00080000)}, /* NE1, RS=A18 */
	{(vu16 *)(0x64000000 | 0x000007FE), (vu16 *)(0x64000000 | 0x00000800)}, /* NE2, RS=A10 */
	{(vu16 *)(0x68000000 | 0x000007FE), (vu16 *)(0x68000000 | 0x00000800)}, /* NE3, RS=A10 */
	{(vu16 *)(0x6C000000 | 0x000007FE), (vu16 *)(0x6C000000 | 0x00000800)}, /* NE4, RS=A10 */
	{(vu16 *)(0x6C000000 | 0x0001FFFE), (vu16 *)(0x6C000000 | 0x00020000)}, /* NE4, RS=A16 */
	{(vu16 *)(0x6C000000 | 0x0007FFFE), (vu16 *)(0x6C000000 | 0x00080000)}, /* NE4, RS=A18 */
};

#define LCD_BUS_COUNT (sizeof(lcd_bus) / sizeof(lcd_bus[0]))
static u8 lcd_active_bus = 0;

static void LCD_WriteReg(u16 reg)
{
	*lcd_bus[lcd_active_bus].reg = reg;
}

static void LCD_WriteData(u16 data)
{
	*lcd_bus[lcd_active_bus].ram = data;
}

static void LCD_WriteCmdData(u16 reg, u16 data)
{
	LCD_WriteReg(reg);
	LCD_WriteData(data);
}

static void LCD_WriteData8(u8 data)
{
	LCD_WriteData(data);
}

static void LCD_FSMC_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	FSMC_NORSRAMInitTypeDef FSMC_NORSRAMInitStructure;
	FSMC_NORSRAMTimingInitTypeDef readWriteTiming;

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE |
	                       RCC_APB2Periph_GPIOF | RCC_APB2Periph_GPIOG, ENABLE);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_4 | GPIO_Pin_5 |
	                              GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 |
	                              GPIO_Pin_11 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 |
	                              GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 |
	                              GPIO_Pin_15;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 |
	                              GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_12 | GPIO_Pin_13 |
	                              GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_Init(GPIOF, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 |
	                              GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_9 | GPIO_Pin_10 |
	                              GPIO_Pin_12;
	GPIO_Init(GPIOG, &GPIO_InitStructure);

	readWriteTiming.FSMC_AddressSetupTime = 0x01;
	readWriteTiming.FSMC_AddressHoldTime = 0x00;
	readWriteTiming.FSMC_DataSetupTime = 0x0F;
	readWriteTiming.FSMC_BusTurnAroundDuration = 0x00;
	readWriteTiming.FSMC_CLKDivision = 0x00;
	readWriteTiming.FSMC_DataLatency = 0x00;
	readWriteTiming.FSMC_AccessMode = FSMC_AccessMode_A;

	FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM1;
	FSMC_NORSRAMInitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Disable;
	FSMC_NORSRAMInitStructure.FSMC_MemoryType = FSMC_MemoryType_SRAM;
	FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_16b;
	FSMC_NORSRAMInitStructure.FSMC_BurstAccessMode = FSMC_BurstAccessMode_Disable;
	FSMC_NORSRAMInitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;
	FSMC_NORSRAMInitStructure.FSMC_WrapMode = FSMC_WrapMode_Disable;
	FSMC_NORSRAMInitStructure.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;
	FSMC_NORSRAMInitStructure.FSMC_WriteOperation = FSMC_WriteOperation_Enable;
	FSMC_NORSRAMInitStructure.FSMC_WaitSignal = FSMC_WaitSignal_Disable;
	FSMC_NORSRAMInitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Disable;
	FSMC_NORSRAMInitStructure.FSMC_WriteBurst = FSMC_WriteBurst_Disable;
	FSMC_NORSRAMInitStructure.FSMC_ReadWriteTimingStruct = &readWriteTiming;
	FSMC_NORSRAMInitStructure.FSMC_WriteTimingStruct = &readWriteTiming;
	FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure);
	FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM1, ENABLE);
	FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM2;
	FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure);
	FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM2, ENABLE);
	FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM3;
	FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure);
	FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM3, ENABLE);
	FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM4;
	FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure);
	FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM4, ENABLE);
}

void LCD_SetWindow(u16 x1, u16 y1, u16 x2, u16 y2)
{
	LCD_WriteReg(0x2A);
	LCD_WriteData(x1 >> 8);
	LCD_WriteData(x1 & 0xFF);
	LCD_WriteData(x2 >> 8);
	LCD_WriteData(x2 & 0xFF);

	LCD_WriteReg(0x2B);
	LCD_WriteData(y1 >> 8);
	LCD_WriteData(y1 & 0xFF);
	LCD_WriteData(y2 >> 8);
	LCD_WriteData(y2 & 0xFF);

	LCD_WriteReg(0x2C);
}

void LCD_WritePixel(u16 color)
{
	LCD_WriteData(color);
}

u16 LCD_ReadRegValue(u16 reg)
{
	LCD_WriteReg(reg);
	delay_ms(1);
	return *lcd_bus[lcd_active_bus].ram;
}

void LCD_Clear(u16 color)
{
	u32 i;
	LCD_SetWindow(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
	for(i = 0; i < (u32)LCD_WIDTH * LCD_HEIGHT; i++)
	{
		LCD_WriteData(color);
	}
}

void LCD_DrawTestBars(void)
{
	u16 x, y, color;
	LCD_SetWindow(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
	for(y = 0; y < LCD_HEIGHT; y++)
	{
		for(x = 0; x < LCD_WIDTH; x++)
		{
			if(y < 80) color = LCD_RED;
			else if(y < 160) color = LCD_GREEN;
			else if(y < 240) color = LCD_BLUE;
			else if(y < 320) color = LCD_YELLOW;
			else if(y < 400) color = LCD_WHITE;
			else color = LCD_BLACK;
			LCD_WriteData(color);
		}
	}
}

static void LCD_ILI9486_Init(void)
{
	LCD_WriteReg(0x01);
	delay_ms(20);
	LCD_WriteReg(0x28);

	LCD_WriteReg(0xF2);
	LCD_WriteData8(0x18);
	LCD_WriteData8(0xA3);
	LCD_WriteData8(0x12);
	LCD_WriteData8(0x02);
	LCD_WriteData8(0xB2);
	LCD_WriteData8(0x12);
	LCD_WriteData8(0xFF);
	LCD_WriteData8(0x10);
	LCD_WriteData8(0x00);

	LCD_WriteReg(0xF8);
	LCD_WriteData8(0x21);
	LCD_WriteData8(0x04);

	LCD_WriteReg(0xF9);
	LCD_WriteData8(0x00);
	LCD_WriteData8(0x08);

	LCD_WriteCmdData(0x36, 0x48);
	LCD_WriteCmdData(0x3A, 0x55);
	LCD_WriteCmdData(0xB4, 0x02);
	LCD_WriteCmdData(0xC1, 0x41);

	LCD_WriteReg(0xC5);
	LCD_WriteData8(0x00);
	LCD_WriteData8(0x53);
	LCD_WriteData8(0x80);

	LCD_WriteReg(0xE0);
	LCD_WriteData8(0x0F);
	LCD_WriteData8(0x1F);
	LCD_WriteData8(0x1C);
	LCD_WriteData8(0x0C);
	LCD_WriteData8(0x0F);
	LCD_WriteData8(0x08);
	LCD_WriteData8(0x48);
	LCD_WriteData8(0x98);
	LCD_WriteData8(0x37);
	LCD_WriteData8(0x0A);
	LCD_WriteData8(0x13);
	LCD_WriteData8(0x04);
	LCD_WriteData8(0x11);
	LCD_WriteData8(0x0D);
	LCD_WriteData8(0x00);

	LCD_WriteReg(0xE1);
	LCD_WriteData8(0x0F);
	LCD_WriteData8(0x32);
	LCD_WriteData8(0x2E);
	LCD_WriteData8(0x0B);
	LCD_WriteData8(0x0D);
	LCD_WriteData8(0x05);
	LCD_WriteData8(0x47);
	LCD_WriteData8(0x75);
	LCD_WriteData8(0x37);
	LCD_WriteData8(0x06);
	LCD_WriteData8(0x10);
	LCD_WriteData8(0x03);
	LCD_WriteData8(0x24);
	LCD_WriteData8(0x20);
	LCD_WriteData8(0x00);

	LCD_WriteReg(0x11);
	delay_ms(120);
	LCD_WriteReg(0x29);
}

void LCD_Init(void)
{
	u8 i;

	LCD_FSMC_Init();
	delay_ms(50);

	for(i = 0; i < LCD_BUS_COUNT; i++)
	{
		lcd_active_bus = i;
		LCD_ILI9486_Init();
		LCD_DrawTestBars();
		delay_ms(80);
	}
	lcd_active_bus = 7;
}
