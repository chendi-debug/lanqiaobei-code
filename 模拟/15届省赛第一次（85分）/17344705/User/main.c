#include <STC15F2K60S2.H>
#include "Init.h"
#include "Led.h"
#include "Key.h"
#include "Seg.h"
#include "ds1302.h"
#include "Distance.h"
#include "iic.h"


unsigned char Key_Val,Key_Down,Key_Up,Key_Old;
unsigned char Key_Slow_Down;
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};
unsigned char Seg_Pos;
unsigned char Seg_Slow_Down;
unsigned char ucLed[8]={0,0,0,0,0,0,0,0};
unsigned char Distance_Slow_Down;
unsigned char Distance;
unsigned char ucRtc[3]={13,03,05};
unsigned char AD_Slow_Down;
unsigned int AD_Value;
unsigned char Seg_Disp_Mode;//0-频率界面 1-参数界面 2-时间界面 3-回显界面 
bit Para_Mode;//0-超限参数 1-校准值参数
bit Echo_Mode;//0-频率回显 1-时间回显
signed int Freq;
unsigned int Timer_1s;
unsigned char i;
unsigned int Freq_Para=2000;
unsigned int Freq_Para_Ctrol=0;
signed int Cal_Value=0;
signed int Cal_Value_Ctrol=0;
#define abs(x) (x<0)?-(x):(x)
unsigned int Freq_Max;
unsigned char Echo_Rtc[3]={0,0,0};
unsigned int Timer_400ms;
unsigned int Timer_400ms_L2;


void Key_Proc()
{
	if(Key_Slow_Down<10)return;
		Key_Slow_Down=0;
	Key_Val=Key_Read();
	Key_Down=Key_Val&(Key_Old^Key_Val);
	Key_Up=~Key_Val&(Key_Old^Key_Val);
	Key_Old=Key_Val;
	
	switch(Key_Down)
	{
		case 4:
			if(++Seg_Disp_Mode==4)Seg_Disp_Mode=0;
			Para_Mode=0;
			Echo_Mode=0;
		if(Seg_Disp_Mode==1&&Para_Mode==1)//校准值参数
		{
			Cal_Value_Ctrol=Cal_Value;
			
		}
		else if(Seg_Disp_Mode==1&&Para_Mode==0)//超限参数
		{
			Freq_Para_Ctrol=Freq_Para;
			
		}
		else if(Seg_Disp_Mode==2)
		{
			Cal_Value=Cal_Value_Ctrol;
			Freq_Para=Freq_Para_Ctrol;
		}
		break;
		case 5:
			if(Seg_Disp_Mode==1)
			{
				Para_Mode ^=1;
			}
			if(Seg_Disp_Mode==3)
			{
				Echo_Mode ^=1;
			}
		break;
		case 8:
			if(Seg_Disp_Mode==1&&Para_Mode==0)//超限参数
			{
				Freq_Para_Ctrol=(Freq_Para_Ctrol==9000)?9000:Freq_Para_Ctrol+1000;
			}
			if(Seg_Disp_Mode==1&&Para_Mode==1)//校准值参数
			{
				Cal_Value_Ctrol=(Cal_Value_Ctrol==900)?900:Cal_Value_Ctrol+100;
			}
		break;
		case 9:
			if(Seg_Disp_Mode==1&&Para_Mode==0)//超限参数
			{
				Freq_Para_Ctrol=(Freq_Para_Ctrol==1000)?1000:Freq_Para_Ctrol-1000;
			}
			if(Seg_Disp_Mode==1&&Para_Mode==1)//校准值参数
			{
				Cal_Value_Ctrol=(Cal_Value_Ctrol==-900)?-900:Cal_Value_Ctrol-100;
			}
		break;
	}
	
}
void Seg_Proc()
{
	if(Seg_Slow_Down<90)return;
		Seg_Slow_Down=0;
	Read_Rtc(ucRtc);
	
	switch(Seg_Disp_Mode)
	{
		case 0://频率界面
			Seg_Buf[0]=11;//F
			Seg_Buf[1]=10;
			Seg_Buf[2]=10;
			Seg_Buf[3]=abs((Freq+Cal_Value)<0)?10:abs((Freq+Cal_Value)/10000%10);
			Seg_Buf[4]=abs((Freq+Cal_Value)<0)?10:abs((Freq+Cal_Value)/1000%10);
			Seg_Buf[5]=abs((Freq+Cal_Value)<0)?10:abs((Freq+Cal_Value)/100%10);
			Seg_Buf[6]=abs((Freq+Cal_Value)<0)?14:abs((Freq+Cal_Value)/10%10);
			Seg_Buf[7]=abs((Freq+Cal_Value)<0)?14:abs((Freq+Cal_Value)%10);
		for(i=3;i<7&&Seg_Buf[i]==0;i++)
			Seg_Buf[i]=10;
		break;
		case 1://参数界面
			if(Para_Mode==0)//超限参数
			{
				Seg_Buf[0]=12;//P
				Seg_Buf[1]=1;
				Seg_Buf[2]=10;
				Seg_Buf[3]=10;
				Seg_Buf[4]=Freq_Para_Ctrol/1000%10;
				Seg_Buf[5]=Freq_Para_Ctrol/100%10;
				Seg_Buf[6]=Freq_Para_Ctrol/10%10;
				Seg_Buf[7]=Freq_Para_Ctrol%10;				
			}
			else if(Para_Mode==1)//校准值参数
			{
				Seg_Buf[0]=12;//P
				Seg_Buf[1]=2;
				Seg_Buf[2]=10;
				Seg_Buf[3]=10;
				if(Cal_Value_Ctrol>0)
				{
					Seg_Buf[4]=10;
					Seg_Buf[5]=Cal_Value_Ctrol/100%10;
					Seg_Buf[6]=Cal_Value_Ctrol/10%10;
					Seg_Buf[7]=Cal_Value_Ctrol%10;	
				}	
				else if(Cal_Value_Ctrol<0)
				{
					Seg_Buf[4]=13;//-
					Seg_Buf[5]=abs(Cal_Value_Ctrol/100%10);
					Seg_Buf[6]=abs(Cal_Value_Ctrol/10%10);
					Seg_Buf[7]=abs(Cal_Value_Ctrol%10);					
				}
				else if(Cal_Value_Ctrol==0)
				{
					Seg_Buf[4]=10;
					Seg_Buf[5]=10;
					Seg_Buf[6]=10;
					Seg_Buf[7]=0;					
				}
			}
		break;
		case 2://时间界面
				Seg_Buf[0]=ucRtc[0]/10%10;
				Seg_Buf[1]=ucRtc[0]%10;
				Seg_Buf[2]=13;
				Seg_Buf[3]=ucRtc[1]/10%10;
				Seg_Buf[4]=ucRtc[1]%10;
				Seg_Buf[5]=13;
				Seg_Buf[6]=ucRtc[2]/10%10;
				Seg_Buf[7]=ucRtc[2]%10;			
		break;		
		case 3://回显界面
			if(Echo_Mode==0)//频率回显
			{
				Seg_Buf[0]=15;//H 
				Seg_Buf[1]=11;//F
				Seg_Buf[2]=10;
				Seg_Buf[3]=Freq_Max/10000%10;
				Seg_Buf[4]=Freq_Max/1000%10;
				Seg_Buf[5]=Freq_Max/100%10;
				Seg_Buf[6]=Freq_Max/10%10;
				Seg_Buf[7]=Freq_Max%10;
				for(i=3;i<7&&Seg_Buf[i]==0;i++)
				{
					Seg_Buf[i]=10;
				}				
			}
			else
			{
				Seg_Buf[0]=15;//H 
				Seg_Buf[1]=16;//A
				Seg_Buf[2]=Echo_Rtc[0]/10%10;
				Seg_Buf[3]=Echo_Rtc[0]%10;
				Seg_Buf[4]=Echo_Rtc[1]/10%10;
				Seg_Buf[5]=Echo_Rtc[1]%10;
				Seg_Buf[6]=Echo_Rtc[2]/10%10;
				Seg_Buf[7]=Echo_Rtc[2]%10;						
			}
		break;	
	}

} 
void Led_Proc()
{
	
	if(Seg_Disp_Mode==0)
	{
		ucLed[0]=(Timer_400ms<200)?1:0;
	}
	else
		ucLed[0]=0;
	
	if(Freq>Freq_Para)
	{
		ucLed[1]=(Timer_400ms_L2<200)?1:0;
	}
	else
		ucLed[1]=0;	
	if((Freq+Cal_Value)<0)
	{
		ucLed[1]=1;
	}
	
	Led_Disp(ucLed);
	
	if(Freq_Max<Freq)
	{
		Freq_Max=Freq;
		Echo_Rtc[0]=ucRtc[0];
		Echo_Rtc[1]=ucRtc[1];
		Echo_Rtc[2]=ucRtc[2];
	}
}

void AD_Proc()
{
	if(AD_Slow_Down<60)return;
		AD_Slow_Down=0;
	if((Freq+Cal_Value)<500)
	{
		Da_Write(1*51.0f);
	}
	else if((Freq+Cal_Value)>Freq_Para)
	{
		Da_Write(5*51.0f);		
	}
	else 
	{
		Da_Write(((float)(Freq+Cal_Value)*4/(Freq_Para-500)+1-(float)2000/(Freq_Para-500))*51.0f);
	}
}
void Distance_Proc()
{
	if(Distance_Slow_Down<120)return;
		Distance_Slow_Down=0;
	
	Distance=Ut_Wave_Data();
	
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

void Timer1_Isr(void) interrupt 3
{
	Key_Slow_Down++;
	Seg_Slow_Down++;
	Distance_Slow_Down++;
	AD_Slow_Down++;
	
	if(++Timer_400ms==400&&Seg_Disp_Mode==0)
	{
		Timer_400ms=0;
	}
	if(Freq>Freq_Para&&++Timer_400ms_L2==400)
	{
		Timer_400ms_L2=0;
	}
	if(++Seg_Pos==8)Seg_Pos=0;
	if(Seg_Buf[Seg_Pos]>='.')
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos]-'.',1);
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
		
	if(++Timer_1s==1000)
	{
		Timer_1s=0;
		Freq=TH0<<8|TL0;
		TH0=TL0=0;	
	}
}
void main()
{
	Sys_Init();
	Timer1_Init();
	Timer0_Init();
	Set_Rtc(ucRtc);
	while(1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		AD_Proc();
		Distance_Proc();
	}
}