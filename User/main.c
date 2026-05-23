#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "lcd.h"
#include "camera_stream.h"

/*******************************************************************************
* Function Name  : main
* Description    : Receive camera frames from USART1 and display them on TFT LCD.
*******************************************************************************/
int main(void)
{
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	LED_Init();
	USART1_Init(230400);

	LCD_Init();
	LCD_Clear(LCD_RED);
	delay_ms(500);
	LCD_Clear(LCD_GREEN);
	delay_ms(500);
	LCD_DrawTestBars();
	USART1_SendString("CAM READY\r\n");

	while(1)
	{
		CameraStream_Process();
	}
}
