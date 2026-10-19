#include <STC15F2K60S2.H>
#include "Init.h"
#include "Led.h"
#include "Key.h"
#include "Seg.h"

unsigned char Key_Val,Key_Down,Key_Up,Key_Old;
unsigned char Key_Slow_Down;
unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};
unsigned char Seg_Pos;
unsigned char Seg_Slow_Down;
unsigned char ucLed[8] = {0,0,0,0,0,0,0,0};
unsigned char Led_Disp_Mode;//0-模式1 1-模式2
unsigned char i;


void Key_Proc()
{
	if(Key_Slow_Down < 10)return;
		Key_Slow_Down = 0;
	Key_Val = Key_Read();
	Key_Down = Key_Val & (Key_Old ^ Key_Val);
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);
	Key_Old = Key_Val;
	
	switch(Key_Down)
	{
		case 6:
			if(++Led_Disp_Mode == 4)
				Led_Disp_Mode = 0;
		break;
	}
	
}
	
void Seg_Proc()
{
	if(Seg_Slow_Down < 80)return;
		Seg_Slow_Down = 0;	

}
void Led_Proc()
{
	if(Led_Disp_Mode == 0)//模式1
	{
		for(i=0;i<8;i++)
		{
			
			ucLed[i] = 1;
		}
	}		
	Led_Disp(ucLed);
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
	Seg_Slow_Down++;
	Key_Slow_Down++;
	
	if(++Seg_Pos == 8)Seg_Pos = 0;
	if(Seg_Buf[Seg_Pos] >= '.')
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos]-'.',1);
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
		
}

void main()
{
	Sys_Init();
	Timer1_Init();
	
	
	while(1)
	{
		Key_Proc();
		Led_Proc();
		Seg_Proc();
		
	}
}