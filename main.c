#include "led.h"
#include "delay.h"
#include "sys.h"
#include "usart.h"
#include "lora_cfg.h" 
#include "usart2.h"	
#include "lora_app.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
//改阈值
void lora_threshold(void) 
{
	u16 i=0;
	u16 len=0;
	u8 a1[30]={0};
	u8 a2[30]={0};
	u8 a3[30]={0};
	u8 a4[30]={0};
	a1[0]=a2[0]=a3[0]=a4[0]=0x00;
	a1[1]=a2[1]=a3[1]=a4[1]=0x00;
	a1[2]=0x15;
	a2[2]=0x16;
	a3[2]=0x17;
	a4[2]=0x18;
	if(USART_RX_STA&0x8000) 
	{
		len = USART_RX_STA&0X3FFF;
		USART_RX_BUF[len]=0;
		//添加结束符
		USART_RX_STA=0;
		for (i=0;i<len;i++)//数据写到发送BUFF 
		{
			while(USART_GetFlagStatus(USART1,USART_FLAG_TC)==RESET);
			//循环发送,直到发送完毕   
			USART_SendData(USART1, USART_RX_BUF[i]);
		}
		for (i=0;i<len;i++) 
		{
			a1[3+i] = USART_RX_BUF[i];
			a2[3+i] = USART_RX_BUF[i];
			a3[3+i] = USART_RX_BUF[i];
			a4[3+i] = USART_RX_BUF[i];
		}
		for (i=0;i<(len+3);i++) 
		{
			while(USART_GetFlagStatus(USART2,USART_FLAG_TC)==RESET);
			//循环发送,直到发送完毕   
			USART_SendData(USART2,a1[i]);
		}
		for (i=0;i<(len+3);i++) 
		{
			while(USART_GetFlagStatus(USART2,USART_FLAG_TC)==RESET);
			//循环发送,直到发送完毕   
			USART_SendData(USART2,a2[i]);
		}
		//delay_ms(1000);
		for (i=0;i<(len+3);i++) 
		{
			while(USART_GetFlagStatus(USART2,USART_FLAG_TC)==RESET);
			//循环发送,直到发送完毕   
			USART_SendData(USART2,a3[i]);
		}
		for (i=0;i<(len+3);i++) 
		{
			while(USART_GetFlagStatus(USART2,USART_FLAG_TC)==RESET);
			//循环发送,直到发送完毕   
			USART_SendData(USART2,a4[i]);
		}
	}
}
//主函数
int main(void) 
{
	int i;
	int len=0;
	int addr1[]={0x00,0x00,0x15,0x01};
	int addr2[]={0x00,0x00,0x16,0x02};
	int addr3[]={0x00,0x00,0x17,0x03};
	int addr4[]={0x00,0x00,0x18,0x04};
	NVIC_Configuration();
	delay_init();
	//延时函数初始化	  
	LED_Init();
	uart_init(9600);
	//串口1初始化为9600	 
	LoRa_Init();
	LoRa_Set();
	Lora_mode=2;
	//标记"发送状态"
	while(1) 
	{
		delay_ms(500);
		for (i = 0; i < 4; i++)                                                    ///主机发送信号 
		{
			while((USART2->SR&0X40)==0);
			//循环发送,直到发送完毕   
			USART2->DR = addr1[i];
		}
		delay_ms(500);
		Lora_mode=1;
		LoRa_ReceData();
		delay_ms(500);
		for (i = 0; i < 4; i++)                                                    ///主机发送信号 
		{
			while((USART2->SR&0X40)==0);
			//循环发送,直到发送完毕   
			USART2->DR = addr2[i];
		}
		delay_ms(500);
		Lora_mode=1;
		LoRa_ReceData();
		delay_ms(500);
		for (i = 0; i < 4; i++)                                                    ///主机发送信号 
		{
			while((USART2->SR&0X40)==0);
			//循环发送,直到发送完毕   
			USART2->DR = addr3[i];
		}
		delay_ms(500);
		Lora_mode=1;
		LoRa_ReceData();
		delay_ms(500);
		for (i = 0; i < 4; i++)                                                    ///主机发送信号 
		{
			while((USART2->SR&0X40)==0);
			//循环发送,直到发送完毕   
			USART2->DR = addr4[i];
		}
		delay_ms(500);
		Lora_mode=1;
		LoRa_ReceData();
	}
}