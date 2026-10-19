#include <STC15F2K60S2.H>
#include "Init.h"
#include "Seg.h"
#include "Led.h"
#include "Key.h"
#include "Distance.h"
#include "onewire.h"
#include "ds1302.h"
#include "Uart.h"
#include "string.h"
#include "stdio.h"

unsigned char Key_Val,Key_Down,Key_Up,Key_Old;
unsigned char Key_Slow_Down;
unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};
unsigned char Seg_Pos;
unsigned char Seg_Slow_Down;
unsigned char ucLed[8] = {1,1,1,1,1,1,1,1};
unsigned int Distance;
unsigned char Distance_Slow_Down;
unsigned char Temperature_Slow_Down;
unsigned char Temperature;
unsigned char ucRtc[3]={23,55,54};
idata unsigned char Uart_Buf[10];			//串口接收缓冲区
idata unsigned char Uart_Rx_Index;					//串口接收索引
idata unsigned char Uart_Recv_Tick=0;					//串口接收时间标志
bit Uart_Rx_Flag=0;						//串口接收标志位
unsigned char Value[]={88,66};


void Key_Proc()
{
	if(Key_Slow_Down<10)return;
		Key_Slow_Down=0;
	Key_Val = Key_Read();
	Key_Down = Key_Val&(Key_Val^Key_Old);
	Key_Up = ~Key_Val&(Key_Val^Key_Old);
	Key_Old=Key_Val;
	
}
void Seg_Proc()
{
	if(Seg_Slow_Down<120)return;
		Seg_Slow_Down = 0;
	Read_Rtc(ucRtc);
	Seg_Buf[0] = Distance / 100 % 10;
	Seg_Buf[1] = Distance / 10 % 10;
	Seg_Buf[2] = Distance % 10;
	Seg_Buf[3] = ucRtc[1] / 10 % 10;
	Seg_Buf[4] = ucRtc[1] % 10;
	Seg_Buf[5] = ucRtc[2] / 10 % 10;
	Seg_Buf[6] = ucRtc[2] % 10;

}
void Distance_Proc()
{
	if(Distance_Slow_Down<90)return;
	Distance_Slow_Down=0;
	Distance = Ut_Wave_Data();
	
}
void Tempera_Proc()
{
	if(Temperature_Slow_Down<40)return;
	Temperature_Slow_Down=0;
	Temperature = rd_Temperature();
}
void Uart_Proc()
{
	if(Uart_Rx_Index==0)
		return;
	if(Uart_Recv_Tick>=10)
	{
		Uart_Recv_Tick=0;
		Uart_Rx_Flag=0;
		
		if(strcmp(Uart_Buf,"#")==0)
			printf("(%bu,%bu)",91,51);
		
		memset(Uart_Buf,0,10);
		Uart_Rx_Index=0;
	}
}
void Led_Proc()
{
	
}
void Timer1_Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0xBF;			//定时器时钟12T模式
	TMOD &= 0x0F;			//设置定时器模式
	TL1 = 0x18;				//设置定时初始值
	TH1 = 0xFC;				//设置定时初始值
	TF1 = 0;				//清除TF1标志
	TR1 = 1;				//定时器1开始计时
	ET1 = 1;				//使能定时器1中断
	EA =1;
}
void Timer1_Isr(void) interrupt 3
{
	if(Uart_Rx_Flag)
		Uart_Recv_Tick++;
	Key_Slow_Down++;
	Seg_Slow_Down++;
	Distance_Slow_Down++;
	Temperature_Slow_Down++;
	
	if(++Seg_Pos==8)Seg_Pos=0;
	if(Seg_Buf[Seg_Pos]>='.')
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos]-'.',1);
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
		
}
void Uart1_Isr(void) interrupt 4
{

	if (RI)				//检测串口1接收中断
	{
		Uart_Buf[Uart_Rx_Index++]=SBUF;
				Uart_Recv_Tick=0;
				Uart_Rx_Flag=1;

		RI=0;
		if(Uart_Rx_Index>10)
		{
			Uart_Rx_Flag = 0;
			Uart_Recv_Tick=0;
			Uart_Rx_Index=0;
			memset(Uart_Buf,0,10);
		}
	}
}
void Delay750ms(void)	//@12.000MHz
{
	unsigned char data i, j, k;

	i = 35;
	j = 51;
	k = 182;
	do
	{
		do
		{
			while (--k);
		} while (--j);
	} while (--i);
}

void main()
{

	Sys_Init();
	Timer1_Init();
	Set_Rtc(ucRtc);
	rd_Temperature();
	Delay750ms();
	Uart1_Init();
	while(1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		Distance_Proc();
		Tempera_Proc();
		Uart_Proc();
	}
}
