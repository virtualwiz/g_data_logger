#include "stm32f10x_usart.h"
#include "stdarg.h"
#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "lcd.h"
#include "led.h"
#include "mpu6050.h"
#include "mmc_sd.h"
#include "malloc.h"
#include "key.h"
#include "timer.h"


#define SD_ON
//#define UART_ON

void LCD_DrawIndex(){
	LCD_Clear(WHITE);
	POINT_COLOR = BLACK;
	LCD_ShowString(5,0,240,12,12,"Copyright(C) 2017 Owlet Industries");
	LCD_ShowString(5,12,240,24,24,"SD Data Logger 1.0");
}
char displayBuff[10];
void LCD_RefreshData(){
	char displayBuff[10];
				POINT_COLOR = MAGENTA;
		sprintf(displayBuff,"%.3f     ",Roll);
		LCD_ShowString(100,LINE2,240,24,24,displayBuff);
		sprintf(displayBuff,"%.3f     ",Pitch);
		LCD_ShowString(100,LINE3,240,24,24,displayBuff);
		sprintf(displayBuff,"%.3f     ",Yaw);
		LCD_ShowString(100,LINE4,240,24,24,displayBuff);
				POINT_COLOR = DARKBLUE;
		sprintf(displayBuff,"%d     ",G0);
		LCD_ShowString(100,LINE5,240,24,24,displayBuff);
		sprintf(displayBuff,"%d     ",G1);
		LCD_ShowString(100,LINE6,240,24,24,displayBuff);
		sprintf(displayBuff,"%d     ",G2);
		LCD_ShowString(100,LINE7,240,24,24,displayBuff);
						POINT_COLOR = BROWN;
		sprintf(displayBuff,"%d     ",A0);
		LCD_ShowString(100,LINE8,240,24,24,displayBuff);
		sprintf(displayBuff,"%d     ",A1);
		LCD_ShowString(100,LINE9,240,24,24,displayBuff);
		sprintf(displayBuff,"%d     ",A2);
		LCD_ShowString(100,LINE10,240,24,24,displayBuff);
}

	int sector = 0;

void TIM3_IRQHandler(void)   //TIM3中断
{
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) //检查指定的TIM中断发生与否:TIM 中断源 
		{
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update  );  //清除TIMx的中断待处理位:TIM 中断源 
		sprintf(displayBuff,"%d",sector);
		LCD_ShowString(100,LINE11,240,24,24,displayBuff);
		}
}

u8 sdDataBuff[512]={0x11,0x22,0x33,0x44,0x55};
u8 sdDeadbeef[4]={0xDE,0xAD,0xBE,0xEF};
char *sdPtr;

void DataUploader()
{
	int cyc;
	LCD_ShowString(5,LINE7,240,24,24,"***DATA UPLOAD***");
	LCD_ShowString(5,LINE11,240,24,24,"Sect#");
	TIM3_Int_Init(4999,7199);
	LCD_ShowString(5,LINE9,240,24,24,"Ready to transfer.");
	delay_ms(1000);
	while(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_15));
	LCD_ShowString(5,LINE9,240,24,24,"Transmitting...   ");
	
	while(1)
	{
		LED0 = 0;
		SD_ReadDisk(sdDataBuff,sector,1);
				sdPtr = (char *)&Roll; for(cyc=0;cyc<4;cyc++) sdPtr[cyc]=sdDataBuff[cyc+0];
				sdPtr = (char *)&Pitch; for(cyc=0;cyc<4;cyc++) sdPtr[cyc]=sdDataBuff[cyc+4];
				sdPtr = (char *)&Yaw; for(cyc=0;cyc<4;cyc++) sdPtr[cyc]=sdDataBuff[cyc+8];
				sdPtr = (char *)&G0; for(cyc=0;cyc<4;cyc++) sdPtr[cyc]=sdDataBuff[cyc+12];
				sdPtr = (char *)&G1; for(cyc=0;cyc<4;cyc++) sdPtr[cyc]=sdDataBuff[cyc+16];
				sdPtr = (char *)&G2; for(cyc=0;cyc<4;cyc++) sdPtr[cyc]=sdDataBuff[cyc+20];
				sdPtr = (char *)&A0; for(cyc=0;cyc<4;cyc++) sdPtr[cyc]=sdDataBuff[cyc+24];
				sdPtr = (char *)&A1; for(cyc=0;cyc<4;cyc++) sdPtr[cyc]=sdDataBuff[cyc+28];
				sdPtr = (char *)&A2; for(cyc=0;cyc<4;cyc++) sdPtr[cyc]=sdDataBuff[cyc+32];
		LED0 = 1;
		if(sdDataBuff[0] == 0xDE && sdDataBuff[1] == 0xAD && sdDataBuff[2] == 0xBE && sdDataBuff[3] == 0xEF)
		{
			sector++;
			LCD_ShowString(5,LINE9,240,24,24,"K1 -> Next segment");
			while(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_15));
			LCD_ShowString(5,LINE9,240,24,24,"Transmitting...   ");
		}
		else
		{
			LED1 = 0;
			printf("%d\t%f\t%f\t%f\t%d\t%d\t%d\t%d\t%d\t%d\t\n",sector,Roll,Pitch,Yaw,G0,G1,G2,A0,A1,A2);
			LED1 = 1;
		}
		sector++;
	}
}

int main(void){
	u8 serialBuff[100];
	u8 key = 0;

	int cyc;
	char displayBuff[10];
	u32 sd_size;
	POINT_COLOR = RED;
	delay_init();	    	 //延时函数初始化	  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //设置NVIC中断分组2:2位抢占优先级，2位响应优先级
	uart_init(9600);	 //串口初始化为9600
	LCD_Init();
	LCD_DrawIndex();
	LED_Init();
	KEY_Init();
	uart_init(9600);
	//mem_init();
	
	POINT_COLOR = RED;
#ifdef SD_ON
	LCD_ShowString(5,LINE12+10,240,24,24,"SD Init");
	while(SD_Initialize())//检测不到SD卡
	{
		LCD_ShowString(5,LINE12+10,240,24,24,"Insert SD Card");
		delay_ms(300);					
		LCD_ShowString(5,LINE12+10,240,24,24,"              ");
		delay_ms(300);
	}
#endif
	
	sd_size=SD_GetSectorCount();//得到扇区数
	sprintf(displayBuff,"SD %d MB",sd_size>>11);
	LCD_ShowString(5,LINE12+10,240,24,24,displayBuff);
	delay_ms(1000);
	
	if(!(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_15)))
		DataUploader();
	
	LCD_ShowString(5,LINE12+10,240,24,24,"Starting MPU6050");
	MPU6050_Init();
	LCD_ShowString(5,LINE12+10,240,24,24,"Ready           ");
	
	POINT_COLOR = BLACK;
	LCD_ShowString(5,LINE2,240,24,24,"Roll");
	LCD_ShowString(5,LINE3,240,24,24,"Pitch");
	LCD_ShowString(5,LINE4,240,24,24,"Yaw");
	LCD_ShowString(5,LINE5,240,24,24,"Gyro0");
	LCD_ShowString(5,LINE6,240,24,24,"Gyro1");
	LCD_ShowString(5,LINE7,240,24,24,"Gyro2");
	LCD_ShowString(5,LINE8,240,24,24,"Accel0");
	LCD_ShowString(5,LINE9,240,24,24,"Accel1");
	LCD_ShowString(5,LINE10,240,24,24,"Accel2");	
	LCD_ShowString(5,LINE11,240,24,24,"Sect#");
	TIM3_Int_Init(4999,7199);//10Khz的计数频率，计数到5000为500ms 
	
	while(1){
		key = KEY_Scan(0);
		if(!(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_15)))
		{
<<<<<<< HEAD
=======
			POINT_COLOR = RED;
			LCD_ShowString(5,LINE12+10,240,24,24,"REC             ");
			delay_ms(500);
			LCD_Fill(100,LINE2,240,LINE11,WHITE);
>>>>>>> usart
			LED0 = 0;  //开始采集
			while(1)
			{
				LED1=0;
				MPU6050_Pose();
				LED1=1;
				
//				sprintf(serialBuff,"%d\t%f\t%f\t%f\t%d\t%d\t%d\t%d\t%d\t%d\t\n",sector,Roll,Pitch,Yaw,G0,G1,G2,A0,A1,A2);
//				USART_OUT(USART1,serialBuff);
				
				sdPtr = (char *)&Roll; for(cyc=0;cyc<4;cyc++) sdDataBuff[cyc+0]=sdPtr[cyc];
				sdPtr = (char *)&Pitch; for(cyc=0;cyc<4;cyc++) sdDataBuff[cyc+4]=sdPtr[cyc];
				sdPtr = (char *)&Yaw; for(cyc=0;cyc<4;cyc++) sdDataBuff[cyc+8]=sdPtr[cyc];
				sdPtr = (char *)&G0; for(cyc=0;cyc<4;cyc++) sdDataBuff[cyc+12]=sdPtr[cyc];
				sdPtr = (char *)&G1; for(cyc=0;cyc<4;cyc++) sdDataBuff[cyc+16]=sdPtr[cyc];
				sdPtr = (char *)&G2; for(cyc=0;cyc<4;cyc++) sdDataBuff[cyc+20]=sdPtr[cyc];
				sdPtr = (char *)&A0; for(cyc=0;cyc<4;cyc++) sdDataBuff[cyc+24]=sdPtr[cyc];
				sdPtr = (char *)&A1; for(cyc=0;cyc<4;cyc++) sdDataBuff[cyc+28]=sdPtr[cyc];
				sdPtr = (char *)&A2; for(cyc=0;cyc<4;cyc++) sdDataBuff[cyc+32]=sdPtr[cyc];
				
<<<<<<< HEAD
				sprintf(displayBuff,"%d",sector);
				LCD_ShowString(100,LINE11,240,24,24,displayBuff);
=======
//				sprintf(displayBuff,"%d",sector);
//				LCD_ShowString(100,LINE11,240,24,24,displayBuff);
>>>>>>> usart
				SD_WriteDisk(sdDataBuff,sector,1);
				sector++;
				if(!(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_15)))
				{
					POINT_COLOR = GREEN;
					LCD_ShowString(5,LINE12+10,240,24,24,"STANDBY         ");
					delay_ms(500);
					break;
				}
			}
			LED0 = 1;
			SD_WriteDisk(sdDeadbeef,sector,1);
			sector++;
		}
		LED1=0;
		MPU6050_Pose();
		LED1=1;
		LCD_RefreshData();
		}
}
