#include <STC15F2K60S2.H>
#include "Init.h"
#include "Seg.h"
#include "Led.h"
#include "Key.h"
#include "Distance.h"
#include "onewire.h"
#include "iic.h"

unsigned char Key_Val,Key_Down,Key_Up,Key_Old;
unsigned char Key_Slow_Down;
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};
unsigned char Seg_Pos;
unsigned char Seg_Slow_Down;
unsigned char ucLed[8]={0,0,0,0,0,0,0,0};
unsigned int Distance_Slow_Down;
unsigned char Seg_Disp_Mode;//0-环境状态界面 1-运动检测 2-参数设置 3-统计数据
unsigned char Para_Mode;//0-温度参数 1-距离参数
unsigned char Temperature;
unsigned char Light_Level=0;
unsigned char Sport_Mode;//1-静止 2-徘徊 3-跑动
unsigned char Distance;
unsigned char Temperature_Para=30;
unsigned char Temperature_Para_Ctrol=0;
unsigned char Distance_Para=30;
unsigned char Distance_Para_Ctrol=0;
unsigned int Relay_Count;
unsigned char i;
unsigned int Ad_Value;
bit Hot_Flag;
bit Near_Flag;
#define abs(x) (x<0)?(x):(x)
static unsigned char Last_Sport_Mode;
bit Lock_Flag;
unsigned int Timer_3s;
unsigned char Timer_200ms;
bit Relay_Flag;
static unsigned char Last_Relay_Flag;
bit Long_Press;
unsigned int Timer_2s;


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
			if(++Seg_Disp_Mode==4)Seg_Disp_Mode=0;
			Para_Mode=0;
		if(Seg_Disp_Mode==2)
		{
			Temperature_Para_Ctrol=Temperature_Para;
			Distance_Para_Ctrol=Distance_Para;
		}
		if(Seg_Disp_Mode==3)
		{
			Temperature_Para=Temperature_Para_Ctrol;
			Distance_Para=Distance_Para_Ctrol;			
		}
		break;
		case 5:
			if(Seg_Disp_Mode==2)//参数界面
			{
				Para_Mode ^=1;
			}
		break;
		case 8:
			if(Seg_Disp_Mode==2&&Para_Mode==0)
			{
				Temperature_Para_Ctrol=(Temperature_Para_Ctrol==80)?80:Temperature_Para_Ctrol+1;
			}
			if(Seg_Disp_Mode==2&&Para_Mode==1)
			{
				Distance_Para_Ctrol=(Distance_Para_Ctrol==80)?80:Distance_Para_Ctrol+5;
			}
		break;
		case 9:
			if(Seg_Disp_Mode==2&&Para_Mode==0)
			{
				Temperature_Para_Ctrol=(Temperature_Para_Ctrol==20)?20:Temperature_Para_Ctrol-1;
			}
			if(Seg_Disp_Mode==2&&Para_Mode==1)
			{
				Distance_Para_Ctrol=(Distance_Para_Ctrol==20)?20:Distance_Para_Ctrol-5;
			}
		break;
	}
	switch(Key_Val)
	{
		case 17:
			Long_Press=1;
		break;
	}
	
	if(Timer_2s>=2000&&Long_Press==1)
	{
		Long_Press=0;
		Timer_2s=0;
		Relay_Count=0;
	}
}

void Seg_Proc()
{
	if(Seg_Slow_Down<90)return;
	  Seg_Slow_Down=0;
	
	Temperature=rd_Tempeature();
	switch(Seg_Disp_Mode)
	{
		case 0://环境状态界面
			Seg_Buf[0]=11;//C
			Seg_Buf[1]=Temperature/10%10;
			Seg_Buf[2]=Temperature%10;
			Seg_Buf[3]=10;
			Seg_Buf[4]=10;
			Seg_Buf[5]=10;
			Seg_Buf[6]=12;//n
			Seg_Buf[7]=Light_Level;
		break;
		case 1://运动检测
			Seg_Buf[0]=13;//L
			Seg_Buf[1]=Sport_Mode;
			Seg_Buf[2]=10;
			Seg_Buf[3]=10;
			Seg_Buf[4]=10;
			Seg_Buf[5]=Distance/100%10;
			Seg_Buf[6]=Distance/10%10;
			Seg_Buf[7]=Distance%10;			
		break;
		case 2://参数设置
			if(Para_Mode==0)//温度参数
			{
				Seg_Buf[0]=14;//P
				Seg_Buf[1]=11;//C
				Seg_Buf[2]=10;
				Seg_Buf[3]=10;
				Seg_Buf[4]=10;
				Seg_Buf[5]=10;
				Seg_Buf[6]=Temperature_Para_Ctrol/10%10;
				Seg_Buf[7]=Temperature_Para_Ctrol%10;		
			}		
			else if(Para_Mode==1)//距离参数
			{
				Seg_Buf[0]=14;//P
				Seg_Buf[1]=13;//L
				Seg_Buf[2]=10;
				Seg_Buf[3]=10;
				Seg_Buf[4]=10;
				Seg_Buf[5]=10;
				Seg_Buf[6]=Distance_Para_Ctrol/10%10;
				Seg_Buf[7]=Distance_Para_Ctrol%10;				
			}
		break;
		case 3://统计数据
			Seg_Buf[0]=12;//n
			Seg_Buf[1]=11;//c
			Seg_Buf[2]=10;
			Seg_Buf[3]=10;
			Seg_Buf[4]=Relay_Count/1000%10;
			Seg_Buf[5]=Relay_Count/100%10;
			Seg_Buf[6]=Relay_Count/10%10;
			Seg_Buf[7]=Relay_Count%10;
			for(i=4;i<7&&Seg_Buf[i]==0;i++)
				Seg_Buf[i]=10;
		break;
	}
}
void Distance_Proc()
{
	 static signed char Last_Distance;
	 signed char D_Distance;
	
	if(Distance_Slow_Down<1000)return;
	Distance_Slow_Down=0;
	if(Near_Flag==1)//接近
	{
		if(Light_Level==1)
		{
			ucLed[0]=1;ucLed[1]=ucLed[2]=ucLed[3]=0;
		}
		else if(Light_Level==2)
		{
			ucLed[0]=ucLed[1]=1;ucLed[2]=ucLed[3]=0;
		}
		else if(Light_Level==3)
		{
			ucLed[0]=ucLed[1]=ucLed[2]=1;ucLed[3]=0;
		}
		else if(Light_Level==4)
		{
			ucLed[0]=ucLed[1]=ucLed[2]=ucLed[3]=1;
		}
	}
	else 
	{
		ucLed[0]=ucLed[1]=ucLed[2]=ucLed[3]=0;
	}	
	Distance=Ut_Wave_Data();
	D_Distance=abs(Distance-Last_Distance);
	Last_Distance=Distance;
	/*接近判断*/
	if(Distance<Distance_Para)
		Near_Flag=1;
	else
		Near_Flag=0;
	
	if(Lock_Flag==1)
		return;
	if(D_Distance<5)
		Sport_Mode=1;
	else if(D_Distance>=5&&D_Distance<10)
		Sport_Mode=2;
	else if(D_Distance>=10)
		Sport_Mode=3;	

	
	if(Last_Sport_Mode!=Sport_Mode)
	{
		Lock_Flag=1;
	}
	else
		Lock_Flag=0;
	Last_Sport_Mode=Sport_Mode;
	
}

void Led_Proc()
{

	if(Sport_Mode==1)//静止
		ucLed[7]=0;
	else if(Sport_Mode==2)
		ucLed[7]=1;		
	else if(Sport_Mode==3)
		ucLed[7]=(Timer_200ms<100)?1:0;
	
	Led_Disp(ucLed);
	
	/*AD相关*/
	Ad_Value=Ad_Read(0x41)/51.0*100;
	
	if(Ad_Value>300)
		Light_Level=1;
	else if(Ad_Value>=200&&Ad_Value<300)
		Light_Level=2;
	else if(Ad_Value>=50&&Ad_Value<200)
		Light_Level=3;	
	else if(Ad_Value<50)
		Light_Level=4;	
	if(Temperature>Temperature_Para)
		Hot_Flag=1;
	else
		Hot_Flag=0;
	
	if(Hot_Flag&&Near_Flag)
	{
		Relay(1);
		Relay_Flag=1;
	}
	else
	{
		Relay_Flag=0;
		Relay(0);
	}
	if(Last_Relay_Flag==0&&Relay_Flag==1)
	{
		Relay_Count++;
	}
		Last_Relay_Flag=Relay_Flag;
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
	Key_Slow_Down++;
	Seg_Slow_Down++;
	Distance_Slow_Down++;
	
	if(++Seg_Pos==8)Seg_Pos=0;
	if(Seg_Buf[Seg_Pos]>='.')
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos]-'.',1);
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
		
	if(Lock_Flag==1)
	{
		if(++Timer_3s==3000)
		{
			Timer_3s=0;
			Lock_Flag=0;
		}
	}
	if(Sport_Mode==3)
	{
	  if(++Timer_200ms==200)
			Timer_200ms=0;
	}
	if(Long_Press)
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
	rd_Tempeature();
	Delay750ms();
	while(1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		Distance_Proc();
	}
}
