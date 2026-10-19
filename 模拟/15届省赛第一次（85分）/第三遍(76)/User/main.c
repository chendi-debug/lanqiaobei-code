#include <STC15F2K60S2.H>
#include "Led.h"
#include "Init.h"
#include "Seg.h"
#include "Key.h"
#include "ds1302.h"
#include "iic.h"

#define abs(x) (x<0)?-(x):(x)

unsigned char Key_Down,Key_Up,Key_Val,Key_Old;
unsigned char Key_Slow_Down;
unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};
unsigned char Seg_Pos;
unsigned char Seg_Slow_Down;
unsigned char AD_Slow_Down;
unsigned char ucLed[8] = {0,0,0,0,0,0,0,0};
unsigned char Seg_Disp_Mode =0;//0-频率界面 1-参数界面 2-时间界面 3-回显界面 
bit Para_Mode;//0-超限 1-校准值
unsigned int Timer_1s;
unsigned int Freq;
unsigned char i;
float Freq_Para = 2000;
signed int Calibration_value = 0;
unsigned char RTC_Timer[3] = {13,03,05};
bit Echo_Mode;//0-频率 1-时间
unsigned char Echo_RTC[3] = {0,0,0};
unsigned int Echo_Freq = 0;
unsigned int L1Timer_400ms;
unsigned int L2Timer_400ms;

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
		case 4:
			if(++Seg_Disp_Mode == 4)
				Seg_Disp_Mode = 0;	
			Para_Mode = 0;
			Echo_Mode = 0;
		break;
		case 5:
			if(Seg_Disp_Mode == 1)//参数界面
			{
				Para_Mode ^= 1;
			}
			if(Seg_Disp_Mode == 3)
			{
				Echo_Mode ^= 1;
			}
		break;
		case 8:
			if(Seg_Disp_Mode == 1)//在参数界面下
			{
				if(Para_Mode == 0)//超限
				{
					Freq_Para = (Freq_Para == 9000)?9000:Freq_Para+1000;
				}
				else if(Para_Mode == 1)//校准值
				{
					Calibration_value = (Calibration_value==900)?900:Calibration_value+100;
				}
			}
		break;
		case 9:
			if(Seg_Disp_Mode == 1)//在参数界面下
			{
				if(Para_Mode == 0)//超限
				{
					Freq_Para = (Freq_Para == 1000)?1000:Freq_Para-1000;
				}
				else if(Para_Mode == 1)//校准值
				{
					Calibration_value = (Calibration_value==-900)?-900:Calibration_value-100;
				}
			}
		break;	
	}
	
	
}

void Seg_Proc()
{
	if(Seg_Slow_Down < 100)return;
		Seg_Slow_Down = 0;
	Read_Rtc(RTC_Timer);
	switch(Seg_Disp_Mode)
	{
		case 0://频率界面
			Seg_Buf[0] = 11;//F 
			Seg_Buf[1] = 10;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = (Freq+Calibration_value)>64000?10:(Freq+Calibration_value) / 10000 % 10;
			Seg_Buf[4] = (Freq+Calibration_value)>64000?10:(Freq+Calibration_value) / 1000 % 10;
			Seg_Buf[5] = (Freq+Calibration_value)>64000?10:(Freq+Calibration_value) / 100 % 10;
			Seg_Buf[6] = (Freq+Calibration_value)>64000?16:(Freq+Calibration_value) / 10 % 10;
			Seg_Buf[7] = (Freq+Calibration_value)>64000?16:(Freq+Calibration_value) % 10;

//				Seg_Buf[3] = Freq / 10000 % 10;
//				Seg_Buf[4] = Freq / 1000 % 10;
//				Seg_Buf[5] = Freq / 100 % 10;
//				Seg_Buf[6] = Freq / 10 % 10;
//				Seg_Buf[7] = Freq % 10;
		
			for(i=3;i<7&&Seg_Buf[i]==0;i++)
				Seg_Buf[i] = 10;
		break;
		case 1://参数界面
			if(Para_Mode == 0)//超限界面
			{
				Seg_Buf[0] = 12;//P 
				Seg_Buf[1] = 1;
				Seg_Buf[2] = 10;
				Seg_Buf[3] = 10;
				Seg_Buf[4] = (unsigned int)Freq_Para / 1000 % 10;
				Seg_Buf[5] = (unsigned int)Freq_Para / 100 % 10;
				Seg_Buf[6] = (unsigned int)Freq_Para / 10 % 10;
				Seg_Buf[7] = (unsigned int)Freq_Para % 10;				
			}
			else if(Para_Mode == 1)//校准值界面
			{
				if(Calibration_value>0)
				{
					Seg_Buf[0] = 12;//P 
					Seg_Buf[1] = 2;
					Seg_Buf[2] = 10;
					Seg_Buf[3] = 10;
					Seg_Buf[4] = 10;
					Seg_Buf[5] = Calibration_value / 100 % 10;
					Seg_Buf[6] = Calibration_value / 10 % 10;
					Seg_Buf[7] = Calibration_value % 10;	
				}
				else if(Calibration_value<0)
				{
					Seg_Buf[0] = 12;//P 
					Seg_Buf[1] = 2;
					Seg_Buf[2] = 10;
					Seg_Buf[3] = 10;
					Seg_Buf[4] = 13;//-
					Seg_Buf[5] = abs(Calibration_value / 100 % 10);
					Seg_Buf[6] = abs(Calibration_value / 10 % 10);
					Seg_Buf[7] = abs(Calibration_value % 10);						
				}
				else if(Calibration_value == 0)
				{
					Seg_Buf[0] = 12;//P 
					Seg_Buf[1] = 2;
					Seg_Buf[2] = 10;
					Seg_Buf[3] = 10;
					Seg_Buf[4] = 10;//-
					Seg_Buf[5] = 10;
					Seg_Buf[6] = 10;
					Seg_Buf[7] = 0;		
				}
			}
		break;
		case 2://时间界面
					Seg_Buf[0] = RTC_Timer[0] / 10 % 10;//P 
					Seg_Buf[1] = RTC_Timer[0] % 10;
					Seg_Buf[2] = 13;
					Seg_Buf[3] = RTC_Timer[1] / 10 % 10;
					Seg_Buf[4] = RTC_Timer[1] % 10;//-
					Seg_Buf[5] = 13;
					Seg_Buf[6] = RTC_Timer[2] / 10 % 10;
					Seg_Buf[7] = RTC_Timer[2] % 10;				
		break;
		case 3://回显界面
			if(Echo_Mode==0)//频率回显
			{
				Seg_Buf[0] = 14;//H 
				Seg_Buf[1] = 11;//F
				Seg_Buf[2] = 10;
				Seg_Buf[3] = Echo_Freq / 10000 % 10;
				Seg_Buf[4] = Echo_Freq / 1000 % 10;
				Seg_Buf[5] = Echo_Freq / 100 % 10;
				Seg_Buf[6] = Echo_Freq / 10 % 10;
				Seg_Buf[7] = Echo_Freq % 10;	
				for(i=3;i<7&&Seg_Buf[i]==0;i++)
					Seg_Buf[i] = 10;
			}
			else if(Echo_Mode == 1)//时间回显
			{	
				Seg_Buf[0] = 14;//H
				Seg_Buf[1] = 15;//A 
				Seg_Buf[2] = Echo_RTC[0] / 10 % 10;
				Seg_Buf[3] = Echo_RTC[0] % 10;
				Seg_Buf[4] = Echo_RTC[1] / 10 % 10;
				Seg_Buf[5] = Echo_RTC[1] % 10;
				Seg_Buf[6] = Echo_RTC[2] / 10 % 10;
				Seg_Buf[7] = Echo_RTC[2] % 10;				
			}
		break;
	}

}

void Led_Proc()
{
	if(Seg_Disp_Mode == 0)
	{
		ucLed[0] = (L1Timer_400ms<200)?1:0;
	}
	else
	ucLed[0] = 0;

	if(abs((Freq+Calibration_value)<0))
		ucLed[1] = 1;
	else if((Freq+Calibration_value)>Freq_Para)
	{
		ucLed[1] = (L2Timer_400ms<200)?1:0;
	}	
	else
		ucLed[1] = 0;
	Led_Disp(ucLed);
}

void AD_DA(void)
{
	
	if(AD_Slow_Down < 80)return;
		AD_Slow_Down = 0;
	
	/*回显相关*/
	if(Echo_Freq < Freq)
	{
		Echo_Freq = Freq;
		for(i=0;i<3;i++)
		{
			Echo_RTC[i]=RTC_Timer[i];
		}
	}
	/*AD相关*/
	if((Freq+Calibration_value)<500)
		Da_Write(1*51.0);
	else if((Freq+Calibration_value) > Freq_Para)
		Da_Write(5*51.0);	
	else if(500<(Freq+Calibration_value)&&(Freq+Calibration_value)<Freq_Para)
	{
		Da_Write((4/(Freq_Para-500)*(Freq+Calibration_value)+1-(2000/(Freq_Para-500)))*51.0);
	}		
}
void Timer0_Init(void)		//1毫秒@12.000MHz                                                                                                    
{
	AUXR &= 0x7F;			//定时器时钟12T模式
	TMOD &= 0xF0;			//设置定时器模式
	TMOD |= 0x05;			//设置计数器模式
	TL0 = 0x00;				//设置定时初始值
	TH0 = 0x00;				//设置定时初始值
	TF0 = 0;				//清除TF0标志
	TR0 = 1;				//定时器0开始计时
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
	AD_Slow_Down++;
	
	
	
	if(++Seg_Pos==8)Seg_Pos=0;
	if(Seg_Buf[Seg_Pos]>='.')
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos]-'.',1);
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
	
	if(++Timer_1s == 1000)
	{
		Timer_1s = 0;
		Freq = (TH0 << 8) | TL0;
		TH0 = TL0 = 0;
	}
	if(++L1Timer_400ms == 400&&Seg_Disp_Mode==0)
		L1Timer_400ms = 0;
		
	if(++L2Timer_400ms == 400)
		L2Timer_400ms = 0;
}
void main()
{
	Sys_Init();
	Timer1_Init();
	Timer0_Init();
	Set_Rtc(RTC_Timer);
	
	
	while(1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		AD_DA();
	}
}
