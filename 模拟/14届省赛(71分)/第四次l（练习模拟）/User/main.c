#include <STC15F2K60S2.H>
#include "Init.h"
#include "Seg.h"
#include "Key.h"
#include "Led.h"
#include "onewire.h"
#include "ds1302.h"
#include "iic.h"

unsigned char Key_Val,Key_Down,Key_Up,Key_Old;
unsigned char Key_Slow_Down;
unsigned char Seg_Pos;
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};
unsigned char Seg_Slow_Down;
unsigned char Temperature_Slow_Down;
unsigned char ucLed[8]={0,0,0,0,0,0,0,0};
unsigned char Seg_Disp_Mode;//0-时间界面 1-回显界面 2-参数界面 3-温湿度界面
unsigned char Rtc_Timer[3]={13,03,05};
unsigned char Echo_Mode;//0-温度回显 1-湿度回显 2-时间回显
unsigned char Temperature;
unsigned char Temperature_Max;
float Temperature_Avg;
unsigned char Hum_Value;
unsigned char Hum_Value_Max;
float Hum_Value_Avg;
unsigned char Relay_Count;
unsigned char Echo_Rtc[2]={0,0};
unsigned char Temperature_Para=30;
unsigned char Temperature_Para_Ctrol=0;
unsigned int Freq;
unsigned int Timer_1s;
bit Error_Value_Flag=0;
unsigned char AD_Slow_Down;
unsigned int Ad_Value;
bit Light_Flag;
bit Disp_Goto;
bit Timer_3s_Flag=0;
unsigned int Timer_3s;
unsigned char Seg_Disp_old=0;
bit Long_Flag=0;
unsigned int Timer_2s;
unsigned char Timer_L4;
bit L4_Flag;


void Key_Proc()
{
	if(Key_Slow_Down<10)return;
		Key_Slow_Down=0;
	
	Key_Val=Key_Read();
	Key_Down=Key_Val&(Key_Val^Key_Old);
	Key_Up=~Key_Val&(Key_Val^Key_Old);
	Key_Old=Key_Val;
	
	switch(Key_Down)
	{
		case 4:
			if(++Seg_Disp_Mode==3)Seg_Disp_Mode=0;
		Echo_Mode=0;
		if(Seg_Disp_Mode==2)
			Temperature_Para_Ctrol=Temperature_Para;
		else if(Seg_Disp_Mode==0)
			Temperature_Para=Temperature_Para_Ctrol;
		break;
		case 5:
			if(Seg_Disp_Mode==1)
			{
				if(++Echo_Mode==3)
					Echo_Mode=0;
			}
		break;
		case 8:
			if(Seg_Disp_Mode==2)//参数界面
			{
				Temperature_Para_Ctrol=(Temperature_Para_Ctrol==99)?99:Temperature_Para_Ctrol+1;
			}
		break;
		case 9:
			if(Seg_Disp_Mode==2)//参数界面
			{
				Temperature_Para_Ctrol=(Temperature_Para_Ctrol==0)?0:Temperature_Para_Ctrol-1;
			}
			if(Seg_Disp_Mode==1&&Echo_Mode==2)//时间回显
			{
				Long_Flag=1;
			}
		break;
	}
	if(Timer_2s==2000&&Long_Flag)
	{
		Timer_2s=0;
		Long_Flag=0;
		Relay_Count=0;
	}
	
}


void Seg_Proc()
{
	if(Seg_Slow_Down<70)return;
		Seg_Slow_Down=0;
	Read_Rtc(Rtc_Timer);
	switch(Seg_Disp_Mode)
	{
		case 0://时间界面
			Seg_Buf[0]=Rtc_Timer[0]/10%10;
			Seg_Buf[1]=Rtc_Timer[0]%10;
			Seg_Buf[2]=11;//-
			Seg_Buf[3]=Rtc_Timer[1]/10%10;
			Seg_Buf[4]=Rtc_Timer[1]%10;
			Seg_Buf[5]=11;
			Seg_Buf[6]=Rtc_Timer[2]/10%10;
			Seg_Buf[7]=Rtc_Timer[2]%10;		
		
		break;
		case 1://回显界面
			switch(Echo_Mode)
			{
				case 0://温度
					Seg_Buf[0]=12;//C
					Seg_Buf[1]=10;
					Seg_Buf[2]=(Relay_Count==0)?10:Temperature_Max / 10 % 10;
					Seg_Buf[3]=(Relay_Count==0)?10:Temperature_Max % 10;
					Seg_Buf[4]=(Relay_Count==0)?10:11;//-
					Seg_Buf[5]=(Relay_Count==0)?10:(unsigned int)Temperature_Avg / 100 % 10;
					Seg_Buf[6]=(Relay_Count==0)?10:(unsigned int)Temperature_Avg / 10 % 10+'.';
					Seg_Buf[7]=(Relay_Count==0)?10:(unsigned int)Temperature_Avg % 10;					
				break;
				case 1://湿度
					Seg_Buf[0]=17;//H
					Seg_Buf[1]=10;
					Seg_Buf[2]=(Relay_Count==0)?10:Hum_Value_Max / 10 % 10;
					Seg_Buf[3]=(Relay_Count==0)?10:Hum_Value_Max % 10;
					Seg_Buf[4]=(Relay_Count==0)?10:11;
					Seg_Buf[5]=(Relay_Count==0)?10:(unsigned int)Hum_Value_Avg / 100 % 10;
					Seg_Buf[6]=(Relay_Count==0)?10:(unsigned int)Hum_Value_Avg / 10 % 10+'.';
					Seg_Buf[7]=(Relay_Count==0)?10:(unsigned int)Hum_Value_Avg % 10;					
				break;
				case 2://时间
					Seg_Buf[0]=13;//F
					Seg_Buf[1]=Relay_Count/10%10;
					Seg_Buf[2]=Relay_Count%10;
					Seg_Buf[3]=(Relay_Count==0)?10:Echo_Rtc[0] / 10 % 10;
					Seg_Buf[4]=(Relay_Count==0)?10:Echo_Rtc[0] % 10;
					Seg_Buf[5]=(Relay_Count==0)?10:11;
					Seg_Buf[6]=(Relay_Count==0)?10:Echo_Rtc[1] / 10 % 10;
					Seg_Buf[7]=(Relay_Count==0)?10:Echo_Rtc[1] % 10;						
				break;
			}
		break;
		case 2://参数界面
			Seg_Buf[0]=14;//P
			Seg_Buf[1]=10;
			Seg_Buf[2]=10;
			Seg_Buf[3]=10;
			Seg_Buf[4]=10;
			Seg_Buf[5]=10;
			Seg_Buf[6]=Temperature_Para_Ctrol / 10 % 10;
			Seg_Buf[7]=Temperature_Para_Ctrol % 10;			
		break;
		case 3:
			Seg_Buf[0]=15;//E
			Seg_Buf[1]=10;
			Seg_Buf[2]=10;
			Seg_Buf[3]=Temperature/10%10;
			Seg_Buf[4]=Temperature%10;
			Seg_Buf[5]=11;
			Seg_Buf[6]=(Error_Value_Flag==1)?16:Hum_Value / 10 % 10;
			Seg_Buf[7]=(Error_Value_Flag==1)?16:Hum_Value % 10;			
		break;
	}
}



void Led_Proc()
{
	ucLed[0]=(Seg_Disp_Mode==0);//时间界面
	ucLed[1]=(Seg_Disp_Mode==1);//回显界面
	ucLed[2]=(Seg_Disp_Mode==3);//温湿度界面
	if(Temperature>Temperature_Para)
	{
		L4_Flag=1;
		ucLed[3]=(Timer_L4<100)?1:0;
	}
	else
	{
		L4_Flag=0;
		ucLed[3]=0;
	}	
	Led_Disp(ucLed);
}
void Temperature_Proc()
{
	static float Temperature_Sum;
	static float Hum_Sum;
	
	if(Temperature_Slow_Down<100)return;
		Temperature_Slow_Down=0;

	if(Disp_Goto)//亮到暗
	{
		Timer_3s_Flag=1;
		Seg_Disp_old = Seg_Disp_Mode;
		Seg_Disp_Mode=3;
		Disp_Goto=0;
		Relay_Count++;
		Echo_Rtc[0]=Rtc_Timer[0];
		Echo_Rtc[1]=Rtc_Timer[1];


			/*温度相关*/
			Temperature=rd_temperature();
			if(Temperature_Max<Temperature)
				Temperature_Max=Temperature;
			Temperature_Sum += Temperature;
			Temperature_Avg = Temperature_Sum/Relay_Count*10;

			/*湿度相关*/
			if(Freq<200||Freq>2000)
			{
				Error_Value_Flag=1;//无效数据
			}
			else
			{
				Error_Value_Flag=0;
				Hum_Value=0.044*Freq+1.2;
			}
			if(Hum_Value_Max<Hum_Value)
				Hum_Value_Max=Hum_Value;
			Hum_Sum+=Hum_Value;
			Hum_Value_Avg=Hum_Sum/Relay_Count*10;

			if(Error_Value_Flag==1)
			{
				ucLed[4]=1;
			}
			else
				ucLed[4]=0;
	}

	
}
void Ad_Proc()
{
	static unsigned int Last_State;
	if(AD_Slow_Down<30)return;
		AD_Slow_Down=0;
	Ad_Value=Ad_Read(0x41)*100/51;

	if(Ad_Value>=120)
	{
		Light_Flag=1;
	}
	else
	{
		Light_Flag=0;
	}
	if(Last_State==1&&Light_Flag==0)
	{
		Disp_Goto=1;
	}
	Last_State = Light_Flag;
	
}
void Timer0_Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0x7F;			//定时器时钟12T模式
	TMOD &= 0xF0;			//设置定时器模式
	TMOD |= 0x05;			//设置定时器模式
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
	EA=1;
}
void Timer1_Isr(void) interrupt 3
{
	Seg_Slow_Down++;
	Key_Slow_Down++;
	AD_Slow_Down++;
	Temperature_Slow_Down++;
	
	if(L4_Flag)
	{
		if(++Timer_L4==200)
			Timer_L4=0;
	}
	if(++Seg_Pos==8)Seg_Pos=0;
	if(Seg_Buf[Seg_Pos]>='.')
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos]-'.',1);
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
	
	if(++Timer_1s==1000)
	{
		Timer_1s=0;
		Freq=(TH0<<8)|TL0;
		TH0=TL0=0;
	}
	
		if(Timer_3s_Flag==1)
	{
		if(++Timer_3s==3000)
		{
			Timer_3s_Flag=0;
			Timer_3s=0;
			Seg_Disp_Mode=Seg_Disp_old;
		}
	}
	if(Long_Flag)
	{
		if(++Timer_2s==2000)
		{
			Timer_2s=2000;
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
	Timer0_Init();
	Set_Rtc(Rtc_Timer);
	rd_temperature();
	Delay750ms();
	

	while(1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		Ad_Proc();
		Temperature_Proc();
	}
}