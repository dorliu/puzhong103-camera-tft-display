#include "system.h"
#include "SysTick.h"
#include "usart.h"
#include "led.h"
#include "tftlcd.h"
#include "picture.h"



/*******************************************************************************
* 函 数 名         : main
* 函数功能		   : 主函数
* 输    入         : 无
* 输    出         : 无
*******************************************************************************/
int main()
{
	u8 i=0;
	u16 color=0;
	
	HAL_Init();                     //初始化HAL库 
	SystemClock_Init(RCC_PLL_MUL9); //设置时钟,72M
	SysTick_Init(72);
	USART1_Init(115200);
	LED_Init();
	TFTLCD_Init();			//LCD初始化
	
	FRONT_COLOR=BLACK;
	LCD_ShowString(10,10,tftlcd_data.width,tftlcd_data.height,12,"Hello World!");
	LCD_ShowString(10,30,tftlcd_data.width,tftlcd_data.height,16,"Hello World!");
	LCD_ShowString(10,50,tftlcd_data.width,tftlcd_data.height,24,"Hello World!");
	LCD_ShowFontHZ(10, 80,"普中科技");
	LCD_ShowString(10,120,tftlcd_data.width,tftlcd_data.height,24,"www.prechin.cn");
	
	LCD_Fill(10,150,60,180,GRAY);
	color=LCD_ReadPoint(20,160);
	LCD_Fill(100,150,150,180,color);
	printf("color=%x\r\n",color);
	
	LCD_ShowPicture(20,220,200,112,(u8 *)gImage_picture);

	while(1)
	{
		i++;
		if(i%20==0)
		{
			LED1=!LED1;
		}
		
		delay_ms(10);		
	}
}
