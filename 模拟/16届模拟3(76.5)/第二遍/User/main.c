#include <STC15F2K60S2.H>
#include "Led.h"
#include "Seg.h"
#include "Key.h"
#include "Init.h"
#include "iic.h"
#include "string.h"
#include "Uart.h"
#include "stdio.h"
#include "Distance.h"
unsigned char Key_Down,Key_Up,Key_Old,Key_Val;
unsigned char Key_Slow_Down;
unsigned char Seg_Slow_Down;
unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};
unsigned char Seg_Pos;
unsigned char ucLed[8] = {0,0,0,0,0,0,0,0};
unsigned char Ad_Slow_Down;
unsigned char Ad_Value;
unsigned char Seg_Disp_Mode;//0-湿度界面 1-参数界面 2-时间界面 
idata unsigned char Uart_Buf[10] = {0}; //串口接收缓冲区
idata unsigned char Uart_Rx_Index;    //串口接收索引值
idata unsigned char Uart_Recv_Tick;			//串口接收时间标志
idata unsigned char Uart_Rx_Flag;
unsigned char Current_Value[2] = {200,55};
unsigned int Distance;



void Key_Proc()
{
	if(Key_Slow_Down<10)return;
	Key_Slow_Down=0;
	
	Key_Val=Key_Read();
	Key_Down=Key_Val&(Key_Val^Key_Old);
	Key_Up=~Key_Val&(Key_Val^Key_Old);
	Key_Old=Key_Val;
	
	
}
void Seg_Proc()
{
	if(Seg_Slow_Down<120)return;
	Seg_Slow_Down=0;
	Distance = Ut_Wave_Data();
	
	Seg_Buf[0] = Distance / 100 % 10;
	Seg_Buf[1] = Distance / 10 % 10;
	Seg_Buf[2] = Distance % 10;

}
void Led_Proc()
{
	Led_Disp(ucLed);

}

void Ad_Proc()
{
	if(Ad_Slow_Down<60)return;
	Ad_Slow_Down = 0;
	Ad_Value = Ad_Read(0x43);

	

}

void Uart_Proc()
{
	if(Uart_Rx_Index==0)
		return;//无数据，直接返回
	if(Uart_Recv_Tick>=10)//如果接收超时，对数据读取，清空缓冲区
	{
		Uart_Recv_Tick=0;
		Uart_Rx_Flag=0;
		memset(Uart_Buf,0,Uart_Rx_Index);//清空接收数据
		Uart_Rx_Index=0;
	}
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
	EA = 1;
}
void Timer1_Isr(void) interrupt 3
{
	Key_Slow_Down++;
	Seg_Slow_Down++;
	Ad_Slow_Down++;
	
	if(Uart_Rx_Flag)
		Uart_Recv_Tick++;
	if(++Seg_Pos==8)Seg_Pos=0;
	if(Seg_Buf[Seg_Pos]>'.')
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos]-'.',1);
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
		
}
void Uart1_Isr(void) interrupt 4
{
	if (TI)				//检测串口1发送中断
	{
		TI = 0;			//清除串口1发送中断请求位
	}
	if (RI)				//如果接收到数据
	{
		Uart_Rx_Flag = 1;
		Uart_Recv_Tick = 0;
		Uart_Buf[Uart_Rx_Index++]=SBUF;
		RI = 0;			//清除串口1接收中断请求位
		if(Uart_Rx_Index>=10)
		{
			Uart_Rx_Index = 0;//防止溢出
			memset(Uart_Buf,0,10);
		}
	}
}
void main()
{
	Sys_Init();
	Timer1_Init();
	Uart1_Init();
	PCA_Init();
	while(1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		Ad_Proc();
		Uart_Proc();
	}
}