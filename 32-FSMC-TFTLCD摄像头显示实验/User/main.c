#include "system.h"
#include "SysTick.h"
#include "usart.h"
#include "led.h"
#include "tftlcd.h"

#define CAMERA_HEAD1       0xA5
#define CAMERA_HEAD2       0x5A
#define CAMERA_LINE_TYPE   0x01
#define CAMERA_FRAME_W     160
#define CAMERA_FRAME_H     120
#define CAMERA_LINE_BYTES  (CAMERA_FRAME_W * 2)

static u8 Camera_ReadByte(void)
{
	while((USART1->SR & 0x20) == 0);
	return (u8)(USART1->DR & 0xFF);
}

static void Camera_DrawLine(u8 line_index, u16 *row)
{
	u16 x, sy;
	u16 y = line_index * 2;

	LCD_Set_Window(0, y, CAMERA_FRAME_W * 2 - 1, y + 1);
	for(sy = 0; sy < 2; sy++)
	{
		for(x = 0; x < CAMERA_FRAME_W; x++)
		{
			LCD_WriteData_Color(row[x]);
			LCD_WriteData_Color(row[x]);
		}
	}
}

static void Camera_ProcessLinePacket(void)
{
	u8 packet_type;
	u8 line_index;
	u8 length_hi;
	u8 length_lo;
	u8 checksum;
	u8 checksum_recv;
	u8 hi;
	u8 lo;
	u16 length;
	u16 x;
	u16 row[CAMERA_FRAME_W];

	packet_type = Camera_ReadByte();
	line_index = Camera_ReadByte();
	length_hi = Camera_ReadByte();
	length_lo = Camera_ReadByte();
	length = ((u16)length_hi << 8) | length_lo;
	checksum = packet_type + line_index + length_hi + length_lo;

	if(packet_type != CAMERA_LINE_TYPE || line_index >= CAMERA_FRAME_H || length != CAMERA_LINE_BYTES)
	{
		for(x = 0; x < length; x++)
		{
			(void)Camera_ReadByte();
		}
		(void)Camera_ReadByte();
		return;
	}

	for(x = 0; x < CAMERA_FRAME_W; x++)
	{
		hi = Camera_ReadByte();
		lo = Camera_ReadByte();
		checksum += hi + lo;
		row[x] = ((u16)hi << 8) | lo;
	}

	checksum_recv = Camera_ReadByte();
	if(checksum == checksum_recv)
	{
		Camera_DrawLine(line_index, row);
		if(line_index == 0)
		{
			LED1 = !LED1;
		}
		if(line_index == CAMERA_FRAME_H - 1)
		{
			LED2 = !LED2;
		}
	}
}

static void Camera_Process(void)
{
	if(Camera_ReadByte() != CAMERA_HEAD1)
	{
		return;
	}
	if(Camera_ReadByte() != CAMERA_HEAD2)
	{
		return;
	}
	Camera_ProcessLinePacket();
}

int main(void)
{
	HAL_Init();
	SystemClock_Init(RCC_PLL_MUL9);
	SysTick_Init(72);
	USART1_Init(230400);
	HAL_NVIC_DisableIRQ(USART1_IRQn);
	__HAL_UART_DISABLE_IT(&UART1_Handler, UART_IT_RXNE);
	LED_Init();
	TFTLCD_Init();

	FRONT_COLOR = BLACK;
	LCD_ShowString(10, 10, tftlcd_data.width, tftlcd_data.height, 24, "Camera ready");
	LCD_ShowString(10, 40, tftlcd_data.width, tftlcd_data.height, 16, "Run camera_to_tft.py COM5");

	while(1)
	{
		Camera_Process();
	}
}
