#include "camera_stream.h"
#include "lcd.h"
#include "led.h"
#include "usart.h"
#include "SysTick.h"

#define CAMERA_PACKET_HEAD1      0xA5
#define CAMERA_PACKET_HEAD2      0x5A
#define CAMERA_PACKET_TYPE_LINE  0x01
#define CAMERA_LINE_BYTES        (CAMERA_FRAME_W * 2)

static u8 CameraStream_ReadByte(void)
{
	return USART1_ReadByte();
}

static u8 camera_first_frame_seen = 0;

static void CameraStream_DrawLine(u8 line_index, u16 *row)
{
	u16 x, sy;
	u16 left = 0;
	u16 top = line_index * 2;

	LCD_SetWindow(left, top, left + CAMERA_FRAME_W * 2 - 1, top + 1);
	for(sy = 0; sy < 2; sy++)
	{
		for(x = 0; x < CAMERA_FRAME_W; x++)
		{
			LCD_WritePixel(row[x]);
			LCD_WritePixel(row[x]);
		}
	}
}

static void CameraStream_ProcessLinePacket(void)
{
	u8 packet_type;
	u8 line_index;
	u8 length_hi;
	u8 length_lo;
	u8 checksum;
	u8 checksum_recv;
	u8 hi, lo;
	u16 length;
	u16 x;
	u16 row[CAMERA_FRAME_W];

	packet_type = CameraStream_ReadByte();
	line_index = CameraStream_ReadByte();
	length_hi = CameraStream_ReadByte();
	length_lo = CameraStream_ReadByte();
	length = ((u16)length_hi << 8) | length_lo;
	checksum = packet_type + line_index + length_hi + length_lo;

	if(packet_type != CAMERA_PACKET_TYPE_LINE || line_index >= CAMERA_FRAME_H || length != CAMERA_LINE_BYTES)
	{
		for(x = 0; x < length; x++)
		{
			(void)CameraStream_ReadByte();
		}
		(void)CameraStream_ReadByte();
		return;
	}

	for(x = 0; x < CAMERA_FRAME_W; x++)
	{
		hi = CameraStream_ReadByte();
		lo = CameraStream_ReadByte();
		checksum += hi + lo;
		row[x] = ((u16)hi << 8) | lo;
	}

	checksum_recv = CameraStream_ReadByte();
	if(checksum == checksum_recv)
	{
		if(line_index == 0)
		{
			LED1 = !LED1;
			if(camera_first_frame_seen == 0)
			{
				camera_first_frame_seen = 1;
				LCD_Clear(LCD_RED);
				delay_ms(2000);
				LCD_Clear(LCD_BLACK);
			}
		}
		CameraStream_DrawLine(line_index, row);
		if(line_index == CAMERA_FRAME_H - 1)
		{
			LED2 = !LED2;
		}
	}
}

void CameraStream_Process(void)
{
	if(CameraStream_ReadByte() != CAMERA_PACKET_HEAD1)
	{
		return;
	}
	if(CameraStream_ReadByte() != CAMERA_PACKET_HEAD2)
	{
		return;
	}

	CameraStream_ProcessLinePacket();
}


