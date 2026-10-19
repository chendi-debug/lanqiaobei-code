/*头文件声明区*/
#include <STC15F2K60S2.H>
#include "Key.h"
#include "Seg.h"
#include "Led.h"
#include "Init.h"
#include "onewire.h"
#include "ds1302.h"


/*变量声明区*/
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量
unsigned char Key_Slow_Down;//按键减速专用变量
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char ucLed[8]={0,0,0,0,0,0,0};//Led显示数据存放数组
unsigned char Seg_Disp_Mode;//0-温度显示界面 1-时间显示界面 2-参数设置界面 
unsigned int Temperature_10x;
unsigned char RTC_Timer[3] = {23,25,20};
unsigned char Tempera_Para = 23;
bit Work_Mode = 0;//0-温度控制 1-时间控制
unsigned char S17_Down;
unsigned int Timer_5s;
unsigned char Realy_Time;
unsigned char L3_Timer;
unsigned char L3_Flag;
unsigned int L1_Timer_5s;
bit L1_Flag;
/*键盘处理函数*/

void Key_Proc()
{
	if(Key_Slow_Down < 10)return;
	Key_Slow_Down=0;//键盘减速程序
	
	Key_Val =Key_Read();//实时读取键码值
	Key_Down = Key_Val & (Key_Old ^ Key_Val);//判断按键是否按下
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);//捕捉按键上升沿
	Key_Old = Key_Val;//辅助扫描变量
	switch(Key_Down)
	{
		case 12:
			if(++Seg_Disp_Mode == 3)
				Seg_Disp_Mode = 0;
		break;
		case 13:
			Work_Mode ^= 1;
		break;
		case 16://加
			if(Seg_Disp_Mode == 2)
				Tempera_Para = (Tempera_Para == 99)?99:Tempera_Para + 1;
		break;
		case 17://减
			if(Seg_Disp_Mode == 2)
				Tempera_Para = (Tempera_Para == 10)?10:Tempera_Para - 1;	
		break;
	}
		if(Seg_Disp_Mode == 1)//时间显示界面下
		{
			if(Key_Val == 17)
				S17_Down = 1;
			else
				S17_Down = 0;
		}
}




/*信息处理函数*/
void Seg_Proc()
{
	if(Seg_Slow_Down < 100) return;
	Seg_Slow_Down = 0;//数码管减速程序
	
	Read_Rtc(RTC_Timer);
	
	Temperature_10x = rd_temperature() * 10;
	
	
	switch(Seg_Disp_Mode)
	{
		case 0://温度显示界面
			Seg_Buf[0] = 11;//U
			Seg_Buf[1] = Seg_Disp_Mode +1;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			Seg_Buf[5] = Temperature_10x / 100 % 10;
			Seg_Buf[6] = Temperature_10x / 10 % 10 + '.';
			Seg_Buf[7] = Temperature_10x % 10;			
		break;
		case 1://时间显示界面
			Seg_Buf[0] = 11;//U
			Seg_Buf[1] = Seg_Disp_Mode +1;
			Seg_Buf[2] = 10;
		if(S17_Down == 0)
		{
			Seg_Buf[3] = RTC_Timer[0] / 10 % 10;
			Seg_Buf[4] = RTC_Timer[0] % 10;
			Seg_Buf[5] = 12;//-
			Seg_Buf[6] = RTC_Timer[1] / 10 % 10;
			Seg_Buf[7] = RTC_Timer[1] % 10;	
		}
		else
		{
			Seg_Buf[3] = RTC_Timer[1] / 10 % 10;
			Seg_Buf[4] = RTC_Timer[1] % 10;
			Seg_Buf[5] = 12;//-
			Seg_Buf[6] = RTC_Timer[2] / 10 % 10;
			Seg_Buf[7] = RTC_Timer[2] % 10;				
		}			
		break;
		case 2://参数设置界面
			Seg_Buf[0] = 11;//U
			Seg_Buf[1] = Seg_Disp_Mode +1;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			Seg_Buf[5] = 10;
			Seg_Buf[6] = Tempera_Para / 10 % 10;
			Seg_Buf[7] = Tempera_Para % 10;				
		break;
	}

}



/*其他显示函数*/
void Led_Proc()
{
	ucLed[1] = (Work_Mode == 0);
	ucLed[2] = (L3_Flag == 1 && ((L3_Timer <100)?1:0));
	if(RTC_Timer[1] == 0 && RTC_Timer[2] == 0)
	{
		ucLed[0] = 1;
		L1_Flag = 1;
	}		
	Led_Disp(ucLed);

	if(Work_Mode == 0)//温度控制
	{
		if(Temperature_10x / 10 > Tempera_Para)
		{
			Relay(1);
			L3_Flag = 1;
		}
		else
		{
		 Relay(0);
			L3_Flag = 0;
		}
	}
	else if(Work_Mode == 1)//时间控制
	{
		if(RTC_Timer[1] == 0 && RTC_Timer[2] == 0)
		{
			Relay(1);
			L3_Flag = 1;
			Realy_Time = 1;
		}
	}
	if(Timer_5s == 5000 && Realy_Time == 0 && Work_Mode == 1)
	{
		Timer_5s = 0;
		 Relay(0);
		L3_Flag = 0;		
	}
}



/* 定时器1中断初始化函数 */
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


/*定时器0中断服务函数*/
void Timer1_Isr(void) interrupt 3
{
	Key_Slow_Down++;
	Seg_Slow_Down++;

	if(++Seg_Pos == 8) Seg_Pos = 0;//数码管显示专用
	if(Seg_Buf[Seg_Pos]>='.')
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos] - '.',1);
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
	
	if(Work_Mode == 1)
	{
		if(Realy_Time == 1)
		{
			if(++Timer_5s == 5000)
			{
				Timer_5s = 5000;
				Realy_Time = 0;
			}
		}
	}
	if(++L3_Timer == 200)L3_Timer = 0;
	
	if(L1_Flag == 1)
	{
		if(++L1_Timer_5s == 5000)
		{
			L1_Timer_5s = 0;
			ucLed[0] = 0;
			L1_Flag = 0;
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

/* Main */
void main()
{
	Sys_Init();
	Timer1_Init();

	rd_temperature();
	Delay750ms();
	Set_Rtc(RTC_Timer);	
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();

	}
}



