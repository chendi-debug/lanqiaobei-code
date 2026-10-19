#include <STC15F2K60S2.H>
#include "Led.h"
#include "Seg.h"
#include "Key.h"
#include "iic.h"
#include "onewire.h"
#include "Init.h"

unsigned char Key_Val,Key_Down,Key_Up,Key_Old;
unsigned char Key_Slow_Down;//按键减速变量
unsigned char Seg_Slow_Down;//数码管减速变量
unsigned char Seg_Pos;//数码管位
unsigned char Seg_Buf[] = {10,10,10,10,10,10,10,10};
unsigned char ucLed[] = {0,0,0,0,0,0,0,0};


unsigned char Seg_Disp_Mode;//数码管显示变量 0-检测界面 1-阈值界面 
unsigned char Threshold_Disp_Mode;//阈值显示界面 0-温度阈值 1-光照阈值
float Volate;//电压值
unsigned int Lux_Value;//光照度
unsigned char temperature;//温度变量
unsigned char Temperature_Thre = 30;//温度阈值
unsigned int Lux_Thre = 100;//光照阈值



/*按键处理函数*/
void Key_Proc()
{
	if(Key_Slow_Down < 10)return;
		Key_Slow_Down = 0;
	Key_Val = Key_Read();
	Key_Down = Key_Val & (Key_Val ^ Key_Old);
	Key_Up = ~Key_Val & (Key_Val ^ Key_Old);
	Key_Old = Key_Val;
	
	switch(Key_Down)
	{
		case 4:
			Seg_Disp_Mode ^= 1;
			
			if(Seg_Disp_Mode == 1)//默认在温度阈值界面
			{
				Threshold_Disp_Mode = 0;
			}
		break;
		case 5:
			if(Seg_Disp_Mode == 1)
			{
				Threshold_Disp_Mode ^= 1;
			}
		break;
		case 8:
			if(Seg_Disp_Mode == 1)
			{
				if(Threshold_Disp_Mode == 0)//在温度阈值界面下
				{
					Temperature_Thre = (Temperature_Thre == 50)?50:Temperature_Thre+5;
				}
				if(Threshold_Disp_Mode == 1)//在光照阈值界面下
				{
					Lux_Thre = (Lux_Thre == 300)?300:Lux_Thre+20;
				}
			}
		break;
		case 9:
			if(Seg_Disp_Mode == 1)
			{
				if(Threshold_Disp_Mode == 0)//在温度阈值界面下
				{
					Temperature_Thre = (Temperature_Thre == 10)?10:Temperature_Thre-5;
				}
				if(Threshold_Disp_Mode == 1)//在光照阈值界面下
				{
					Lux_Thre = (Lux_Thre == 100)?100:Lux_Thre-20;
				}
			}
		break;
			
	}
	
}
/*数码管显示函数*/
void Seg_Proc()
{
	if(Seg_Slow_Down < 80)return;
		Seg_Slow_Down = 0;	

  temperature = rd_temperature();
	switch(Seg_Disp_Mode)
	{
		case 0://检测界面
			Seg_Buf[0] = 11;//C
			Seg_Buf[1] = temperature / 10 % 10;
			Seg_Buf[2] = temperature % 10;	
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 12;//H 1001 0001 1000 1001 0x89
			Seg_Buf[5] = Lux_Value / 100 % 10;
			Seg_Buf[6] = Lux_Value / 10 % 10;
			Seg_Buf[7] = Lux_Value % 10;
			if(Seg_Buf[5] == 0)
				Seg_Buf[5] = 10;
		break;
		case 1://阈值界面
			switch(Threshold_Disp_Mode)
			{
				case 0://温度阈值P 
					Seg_Buf[0] = 13;//P
					Seg_Buf[1] = 11;//C	
					Seg_Buf[2] = 10; 
					Seg_Buf[3] = 10;
					Seg_Buf[4] = 10; 
					Seg_Buf[5] = 10;	
					Seg_Buf[6] = Temperature_Thre / 10 % 10;
					Seg_Buf[7] = Temperature_Thre % 10;
				break;
				case 1://光照阈值
					Seg_Buf[0] = 13;//P
					Seg_Buf[1] = 12;//H
					Seg_Buf[2] = 10; 
					Seg_Buf[3] = 10;
					Seg_Buf[4] = 10; 
					Seg_Buf[5] = Lux_Thre / 100 % 10;	
					Seg_Buf[6] = Lux_Thre / 10 % 10;
					Seg_Buf[7] = Lux_Thre % 10;					
				break;
			}
		break;
	}
}

/*其他显示函数*/
void Led_Proc()
{
	ucLed[0] = (Seg_Disp_Mode == 0);
	ucLed[1] = (Seg_Disp_Mode == 1);
	ucLed[7] = ((temperature > Temperature_Thre) && (Lux_Value > Lux_Thre));
	Led_Disp(ucLed);
	
	/*AD*/
	Volate = Ad_Read(0x43) / 51.0;
	if(Volate <= 0.3)
	{
		Lux_Value = 10;
	}
	else if(Volate >= 5)
	{
		Lux_Value = 500;
	}
	else 
	{
		Lux_Value = 104.7 * (Volate) - 23.5;
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
	
	Seg_Pos = (++Seg_Pos) % 8;
	if(Seg_Buf[Seg_Pos] > 20)
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos] - '.',1);
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
		
}

void main()
{
	System_Init();	
	Timer1_Init();

	
	while(1)
	{
		Led_Proc();
		Seg_Proc();
		Key_Proc();
	}
}
