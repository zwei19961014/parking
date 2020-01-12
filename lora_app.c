#include "lora_app.h"
#include "lora_cfg.h"
#include "usart2.h"
#include "usart.h"
#include "string.h"
#include "led.h"
#include "delay.h"
#include "stdio.h"
#include "text.h"
#include "stdlib.h"
// 设备参数初始化(具体设备参数见lora_cfg.h定义) 
_LoRa_CFG LoRa_CFG =
{
	.addr		= LORA_ADDR,    // 设备地址 
	.power		= LORA_POWER,   // 发射功率 
	.chn		= LORA_CHN,     // 信道 
	.wlrate		= LORA_RATE,    // 空中速率 
	.wltime		= LORA_WLTIME,  // 睡眠时间 
	.mode		= LORA_MODE,    // 工作模式 
	.mode_sta	= LORA_STA,     // 发送状态 
	.bps		= LORA_TTLBPS,  // 波特率设置 
	.parity		= LORA_TTLPAR   // 校验位设置 
};

// 全局参数 
EXTI_InitTypeDef	EXTI_InitStructure;
NVIC_InitTypeDef	NVIC_InitStructure;

// 设备工作模式(用于记录设备状态) 
u8 Lora_mode = 0;       // 0:配置模式 1:接收模式 2:发送模式 
// 记录中断状态 
static u8 Int_mode = 0; // 0：关闭 1:上升沿 2:下降沿 
 // AUX中断设置
 // mode:配置的模式 0:关闭 1:上升沿 2:下降沿
 
void Aux_Int( u8 mode )
{
	if ( !mode )
	{
		EXTI_InitStructure.EXTI_LineCmd		= DISABLE;                      // 关闭 
		NVIC_InitStructure.NVIC_IRQChannelCmd	= DISABLE;
	}else{
		if ( mode == 1 )
			EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;          //上升沿 
		else if ( mode == 2 )
			EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;         //下降沿 

		EXTI_InitStructure.EXTI_LineCmd		= ENABLE;
		NVIC_InitStructure.NVIC_IRQChannelCmd	= ENABLE;
	}
	Int_mode = mode;                                                    // 记录中断模式 
	EXTI_Init( &EXTI_InitStructure );
	NVIC_Init( &NVIC_InitStructure );
}

// LORA_AUX中断服务函数 
void EXTI4_IRQHandler( void )
{
	if ( EXTI_GetITStatus( EXTI_Line4 ) )
	{
		if ( Int_mode == 1 )                            //上升沿(发送:开始发送数据 接收:数据开始输出) 
		{
			if ( Lora_mode == 1 )                         // 接收模式 
			{
				USART2_RX_STA = 0;                          // 数据计数清0 
			}
			Int_mode = 2;                                 // 设置下降沿 
		}  else if ( Int_mode == 2 )                    // 下降沿(发送:数据已发送完 接收:数据输出结束) 
		{
			if ( Lora_mode == 1 )                         // 接收模式 
			{
				USART2_RX_STA |= 1 << 15;                   // 数据计数标记完成 
			}else if ( Lora_mode == 2 )                   // 发送模式(串口数据发送完毕) 
			{
			Lora_mode = 1;                                // 进入接收模式 
			}
			Int_mode	= 1;                                // 设置上升沿 
			LED0		= 1;                                  // DS0灭 
		}
		Aux_Int( Int_mode );                            // 重新设置中断边沿 
		EXTI_ClearITPendingBit( EXTI_Line4 );           // 清除LINE4上的中断标志位 
	}
}

 // LoRa模块初始化
 // 返回值:0,检测成功
 //        1,检测失败
 
u8 LoRa_Init( void )
{
	u8			retry	= 0;
	u8			temp	= 1;
	GPIO_InitTypeDef	GPIO_InitStructure;
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA, ENABLE );                 // 使能PB,PE端口时钟 
	GPIO_InitStructure.GPIO_Pin	= GPIO_Pin_11;                              // LORA_MD0 
	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_Out_PP;                       // 推挽输出 
	GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;                       // IO口速度为50MHz 
	GPIO_Init( GPIOA, &GPIO_InitStructure );                                // 推挽输出 ，IO口速度为50MHz 
	GPIO_InitStructure.GPIO_Pin	= GPIO_Pin_4;                               // LORA_AUX 
	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_IPD;                          // 下拉输入 
	GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;                       // IO口速度为50MHz 
	GPIO_Init( GPIOA, &GPIO_InitStructure );                                // 根据设定参数初始化GPIOA.4 
	GPIO_EXTILineConfig( GPIO_PortSourceGPIOA, GPIO_PinSource4 );
	EXTI_InitStructure.EXTI_Line	= EXTI_Line4;
	EXTI_InitStructure.EXTI_Mode	= EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;                  //上升沿触发 
	EXTI_InitStructure.EXTI_LineCmd = DISABLE;                              // 中断线关闭 
	EXTI_Init( &EXTI_InitStructure );                                       // 根据EXTI_InitStruct中指定的参数初始化外设EXTI寄存器 

	NVIC_InitStructure.NVIC_IRQChannel			= EXTI4_IRQn;                   // LORA_AUX 
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority	= 0x02;           // 抢占优先级2， 
	NVIC_InitStructure.NVIC_IRQChannelSubPriority		= 0x03;                 // 子优先级3 
	NVIC_InitStructure.NVIC_IRQChannelCmd			= DISABLE;                    // 关闭外部中断通道 
	NVIC_Init( &NVIC_InitStructure );
	LORA_MD0	= 0;
	LORA_AUX	= 0;
	while ( LORA_AUX )                                                      // 确保LORA模块在空闲状态下(LORA_AUX=0) 
	{
	}
	usart2_init( 115200 );                                                  // 初始化串口2 

	LORA_MD0 = 1;                                                           // 进入AT模式 
	delay_ms( 40 );
	retry = 3;
	while ( retry-- )
	{
	}
	if ( retry == 0 )
		temp = 1;                                                             // 检测失败 
	return(temp);
}


void LoRa_Set( void )
{
	u8	sendbuf[20];
	u8	lora_addrh, lora_addrl = 0;

	usart2_set( LORA_TTLBPS_115200, LORA_TTLPAR_8N1 );                              // 进入配置模式前设置通信波特率和校验位(115200 8位数据 1位停止 无数据校验） 
	usart2_rx( 1 );                                                                 // 开启串口3接收 

	while ( LORA_AUX );                                                                       // 等待模块空闲 
	LORA_MD0 = 1;                                                                   // 进入配置模式 
	delay_ms( 40 );
	Lora_mode = 0;                                                                  // 标记"配置模式" 
	lora_addrh	= (LoRa_CFG.addr >> 8) & 0xff;
	lora_addrl	= LoRa_CFG.addr & 0xff;
	sprintf( (char *) sendbuf, "AT+ADDR=%02x,%02x", lora_addrh, lora_addrl );       // 设置设备地址 
	sprintf( (char *) sendbuf, "AT+WLRATE=%d,%d", LoRa_CFG.chn, LoRa_CFG.wlrate );  // 设置信道和空中速率 
	sprintf( (char *) sendbuf, "AT+TPOWER=%d", LoRa_CFG.power );                    // 设置发射功率 
	sprintf( (char *) sendbuf, "AT+CWMODE=%d", LoRa_CFG.mode );                     // 设置工作模式 
	sprintf( (char *) sendbuf, "AT+TMODE=%d", LoRa_CFG.mode_sta );                  // 设置发送状态 
	sprintf( (char *) sendbuf, "AT+WLTIME=%d", LoRa_CFG.wltime );                   // 设置睡眠时间 
	sprintf( (char *) sendbuf, "AT+UART=%d,%d", LoRa_CFG.bps, LoRa_CFG.parity );    // 设置串口波特率、数据校验位 
	LORA_MD0 = 0;                                                                   // 退出配置,进入通信 
	delay_ms( 40 );
	while ( LORA_AUX );                                                                       // 判断是否空闲(模块会重新配置参数) 
	USART2_RX_STA	= 0;
	Lora_mode	= 1;                                                            // 标记"接收模式" 
	usart2_set( LoRa_CFG.bps, LoRa_CFG.parity );                                    // 返回通信,更新通信串口配置(波特率、数据校验位) 
	Aux_Int( 1 );                                                                   // 设置LORA_AUX上升沿中断 
}
// Lora模块参数配置 
u8	Dire_Date[] = { 0x00, 0x00, 0x14, 0x01, 0x01 };
u8	date[30] = { 0 };                                                               // 定向数组 
u8	Tran_Data[30] = { 0 };                                                          // 透传数组 
u8	buf[4] = { 0x00, 0x00, 0x00, 0x00 };
#define Dire_DateLen sizeof(Dire_Date) / sizeof(Dire_Date[0])
// LoRa控制射灯亮灭 
extern int	distance_now, distance_threshold;
char		buff[30][100];
// Lora模块接收数据 
void LoRa_ReceData( void )
{
	int	i	= 0;
	//有数据来了 
	if ( USART2_RX_STA & 0x8000 )
	{
		USART2_RX_STA	= 0;
		i		= (int) USART2_RX_BUF[0] - 1;
		if ( buf[i] != USART2_RX_BUF[1] )
		{
			sprintf( buff[0], "{\"ParkingNo\":%x, \"status\":%x}", USART2_RX_BUF[0], USART2_RX_BUF[1] );
			while ( USART_GetFlagStatus( USART1, USART_FLAG_TC ) == RESET );                           // 循环发送,直到发送完毕 
			USART1_Send_String( (char *) buff[0] );
			buf[i] = USART2_RX_BUF[1];
		}
		strcpy( (char *) USART2_RX_BUF, "" );
		LoRa_Set();
		Lora_mode = 2;
	}
}


