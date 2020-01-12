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
// �豸������ʼ��(�����豸������lora_cfg.h����) 
_LoRa_CFG LoRa_CFG =
{
	.addr		= LORA_ADDR,    // �豸��ַ 
	.power		= LORA_POWER,   // ���书�� 
	.chn		= LORA_CHN,     // �ŵ� 
	.wlrate		= LORA_RATE,    // �������� 
	.wltime		= LORA_WLTIME,  // ˯��ʱ�� 
	.mode		= LORA_MODE,    // ����ģʽ 
	.mode_sta	= LORA_STA,     // ����״̬ 
	.bps		= LORA_TTLBPS,  // ���������� 
	.parity		= LORA_TTLPAR   // У��λ���� 
};

// ȫ�ֲ��� 
EXTI_InitTypeDef	EXTI_InitStructure;
NVIC_InitTypeDef	NVIC_InitStructure;

// �豸����ģʽ(���ڼ�¼�豸״̬) 
u8 Lora_mode = 0;       // 0:����ģʽ 1:����ģʽ 2:����ģʽ 
// ��¼�ж�״̬ 
static u8 Int_mode = 0; // 0���ر� 1:������ 2:�½��� 
 // AUX�ж�����
 // mode:���õ�ģʽ 0:�ر� 1:������ 2:�½���
 
void Aux_Int( u8 mode )
{
	if ( !mode )
	{
		EXTI_InitStructure.EXTI_LineCmd		= DISABLE;                      // �ر� 
		NVIC_InitStructure.NVIC_IRQChannelCmd	= DISABLE;
	}else{
		if ( mode == 1 )
			EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;          //������ 
		else if ( mode == 2 )
			EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;         //�½��� 

		EXTI_InitStructure.EXTI_LineCmd		= ENABLE;
		NVIC_InitStructure.NVIC_IRQChannelCmd	= ENABLE;
	}
	Int_mode = mode;                                                    // ��¼�ж�ģʽ 
	EXTI_Init( &EXTI_InitStructure );
	NVIC_Init( &NVIC_InitStructure );
}

// LORA_AUX�жϷ����� 
void EXTI4_IRQHandler( void )
{
	if ( EXTI_GetITStatus( EXTI_Line4 ) )
	{
		if ( Int_mode == 1 )                            //������(����:��ʼ�������� ����:���ݿ�ʼ���) 
		{
			if ( Lora_mode == 1 )                         // ����ģʽ 
			{
				USART2_RX_STA = 0;                          // ���ݼ�����0 
			}
			Int_mode = 2;                                 // �����½��� 
		}  else if ( Int_mode == 2 )                    // �½���(����:�����ѷ����� ����:�����������) 
		{
			if ( Lora_mode == 1 )                         // ����ģʽ 
			{
				USART2_RX_STA |= 1 << 15;                   // ���ݼ��������� 
			}else if ( Lora_mode == 2 )                   // ����ģʽ(�������ݷ������) 
			{
			Lora_mode = 1;                                // �������ģʽ 
			}
			Int_mode	= 1;                                // ���������� 
			LED0		= 1;                                  // DS0�� 
		}
		Aux_Int( Int_mode );                            // ���������жϱ��� 
		EXTI_ClearITPendingBit( EXTI_Line4 );           // ���LINE4�ϵ��жϱ�־λ 
	}
}

 // LoRaģ���ʼ��
 // ����ֵ:0,���ɹ�
 //        1,���ʧ��
 
u8 LoRa_Init( void )
{
	u8			retry	= 0;
	u8			temp	= 1;
	GPIO_InitTypeDef	GPIO_InitStructure;
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA, ENABLE );                 // ʹ��PB,PE�˿�ʱ�� 
	GPIO_InitStructure.GPIO_Pin	= GPIO_Pin_11;                              // LORA_MD0 
	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_Out_PP;                       // ������� 
	GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;                       // IO���ٶ�Ϊ50MHz 
	GPIO_Init( GPIOA, &GPIO_InitStructure );                                // ������� ��IO���ٶ�Ϊ50MHz 
	GPIO_InitStructure.GPIO_Pin	= GPIO_Pin_4;                               // LORA_AUX 
	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_IPD;                          // �������� 
	GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;                       // IO���ٶ�Ϊ50MHz 
	GPIO_Init( GPIOA, &GPIO_InitStructure );                                // �����趨������ʼ��GPIOA.4 
	GPIO_EXTILineConfig( GPIO_PortSourceGPIOA, GPIO_PinSource4 );
	EXTI_InitStructure.EXTI_Line	= EXTI_Line4;
	EXTI_InitStructure.EXTI_Mode	= EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;                  //�����ش��� 
	EXTI_InitStructure.EXTI_LineCmd = DISABLE;                              // �ж��߹ر� 
	EXTI_Init( &EXTI_InitStructure );                                       // ����EXTI_InitStruct��ָ���Ĳ�����ʼ������EXTI�Ĵ��� 

	NVIC_InitStructure.NVIC_IRQChannel			= EXTI4_IRQn;                   // LORA_AUX 
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority	= 0x02;           // ��ռ���ȼ�2�� 
	NVIC_InitStructure.NVIC_IRQChannelSubPriority		= 0x03;                 // �����ȼ�3 
	NVIC_InitStructure.NVIC_IRQChannelCmd			= DISABLE;                    // �ر��ⲿ�ж�ͨ�� 
	NVIC_Init( &NVIC_InitStructure );
	LORA_MD0	= 0;
	LORA_AUX	= 0;
	while ( LORA_AUX )                                                      // ȷ��LORAģ���ڿ���״̬��(LORA_AUX=0) 
	{
	}
	usart2_init( 115200 );                                                  // ��ʼ������2 

	LORA_MD0 = 1;                                                           // ����ATģʽ 
	delay_ms( 40 );
	retry = 3;
	while ( retry-- )
	{
	}
	if ( retry == 0 )
		temp = 1;                                                             // ���ʧ�� 
	return(temp);
}


void LoRa_Set( void )
{
	u8	sendbuf[20];
	u8	lora_addrh, lora_addrl = 0;

	usart2_set( LORA_TTLBPS_115200, LORA_TTLPAR_8N1 );                              // ��������ģʽǰ����ͨ�Ų����ʺ�У��λ(115200 8λ���� 1λֹͣ ������У�飩 
	usart2_rx( 1 );                                                                 // ��������3���� 

	while ( LORA_AUX );                                                                       // �ȴ�ģ����� 
	LORA_MD0 = 1;                                                                   // ��������ģʽ 
	delay_ms( 40 );
	Lora_mode = 0;                                                                  // ���"����ģʽ" 
	lora_addrh	= (LoRa_CFG.addr >> 8) & 0xff;
	lora_addrl	= LoRa_CFG.addr & 0xff;
	sprintf( (char *) sendbuf, "AT+ADDR=%02x,%02x", lora_addrh, lora_addrl );       // �����豸��ַ 
	sprintf( (char *) sendbuf, "AT+WLRATE=%d,%d", LoRa_CFG.chn, LoRa_CFG.wlrate );  // �����ŵ��Ϳ������� 
	sprintf( (char *) sendbuf, "AT+TPOWER=%d", LoRa_CFG.power );                    // ���÷��书�� 
	sprintf( (char *) sendbuf, "AT+CWMODE=%d", LoRa_CFG.mode );                     // ���ù���ģʽ 
	sprintf( (char *) sendbuf, "AT+TMODE=%d", LoRa_CFG.mode_sta );                  // ���÷���״̬ 
	sprintf( (char *) sendbuf, "AT+WLTIME=%d", LoRa_CFG.wltime );                   // ����˯��ʱ�� 
	sprintf( (char *) sendbuf, "AT+UART=%d,%d", LoRa_CFG.bps, LoRa_CFG.parity );    // ���ô��ڲ����ʡ�����У��λ 
	LORA_MD0 = 0;                                                                   // �˳�����,����ͨ�� 
	delay_ms( 40 );
	while ( LORA_AUX );                                                                       // �ж��Ƿ����(ģ����������ò���) 
	USART2_RX_STA	= 0;
	Lora_mode	= 1;                                                            // ���"����ģʽ" 
	usart2_set( LoRa_CFG.bps, LoRa_CFG.parity );                                    // ����ͨ��,����ͨ�Ŵ�������(�����ʡ�����У��λ) 
	Aux_Int( 1 );                                                                   // ����LORA_AUX�������ж� 
}
// Loraģ��������� 
u8	Dire_Date[] = { 0x00, 0x00, 0x14, 0x01, 0x01 };
u8	date[30] = { 0 };                                                               // �������� 
u8	Tran_Data[30] = { 0 };                                                          // ͸������ 
u8	buf[4] = { 0x00, 0x00, 0x00, 0x00 };
#define Dire_DateLen sizeof(Dire_Date) / sizeof(Dire_Date[0])
// LoRa����������� 
extern int	distance_now, distance_threshold;
char		buff[30][100];
// Loraģ��������� 
void LoRa_ReceData( void )
{
	int	i	= 0;
	//���������� 
	if ( USART2_RX_STA & 0x8000 )
	{
		USART2_RX_STA	= 0;
		i		= (int) USART2_RX_BUF[0] - 1;
		if ( buf[i] != USART2_RX_BUF[1] )
		{
			sprintf( buff[0], "{\"ParkingNo\":%x, \"status\":%x}", USART2_RX_BUF[0], USART2_RX_BUF[1] );
			while ( USART_GetFlagStatus( USART1, USART_FLAG_TC ) == RESET );                           // ѭ������,ֱ��������� 
			USART1_Send_String( (char *) buff[0] );
			buf[i] = USART2_RX_BUF[1];
		}
		strcpy( (char *) USART2_RX_BUF, "" );
		LoRa_Set();
		Lora_mode = 2;
	}
}


