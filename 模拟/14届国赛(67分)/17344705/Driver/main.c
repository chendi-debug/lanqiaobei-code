#include <STC15F2K60S2.H>
#include "Init.h"
#include "Key.h"
#include "Seg.h"
#include "Led.h"
#include "Uart.h"



void Timer1_Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0xBF;			//定时器时钟12T模式
	TMOD &= 0x0F;			//设置定时器模式
	TL1 = 0x18;				//设置定时初始值
	TH1 = 0xFC;				//设置定时初始值
	TF1 = 0;				//清除TF1标志
	TR1 = 1;				//定时器1开始计时
	ET1 = 1;				//使能定时器1中断
	EA=1;
}
void Timer1_Isr(void) interrupt 3
{
}
void Uart1_Isr(void) interrupt 4
{

	if (RI)				//检测串口1接收中断
	{
		RI = 0;			//清除串口1接收中断请求位
	}
}
void main()
{
	Sys_Init();
	Timer1_Init();
	Seg_Disp(1,1,1);
	while(1)
	{
		
	}
}