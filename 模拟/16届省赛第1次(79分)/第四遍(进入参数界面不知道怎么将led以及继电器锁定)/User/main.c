#include <STC15F2K60S2.H>
#include "Key.h"
#include "Led.h"
#include "Seg.h"
#include "Distance.h"	
#include "Init.h"
#include "onewire.h"
#include "iic.h"

#define abs(x) (x<0)?(x):(x)
unsigned char Key_Val,Key_Down,Key_Up,Key_Old;
unsigned char Key_Slow_Down;
unsigned char Seg_Slow_Down;
unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};
unsigned char Seg_Pos;
unsigned char ucLed[8] = {0,0,0,0,0,0,0,0};
unsigned int Distance_Slow_Down;
unsigned char AD_Slow_Down;
unsigned int Distance;
unsigned char Seg_Disp_Mode;//0-环境状态界面 1-运动检测状态 2-参数设置状态 3-统计数据状态
unsigned char Para_Mode;//0-温度参数 1-距离参数
unsigned char Temperature;
unsigned char Light_Level;
float AD_Value;
unsigned char Sport_Mode;//1-静止 2-徘徊 3-跑动
unsigned char Tempera_Para = 30;
unsigned char Tempera_Para_Ctrol = 0;
unsigned char Distance_Para = 30;
unsigned char Distance_Para_Ctrol = 0;
unsigned int Relay_Count = 0;
unsigned char i;
bit Relay_Work;//继电器工作状态 0-不工作 1-工作
unsigned char Near_Flag;//接近标志
unsigned char Hot_Flag;//高温标志
static unsigned int Relay_Old_Flag = 0;
unsigned int Relay_Flag;
unsigned char Timer_200ms;
unsigned int Timer_2s;
bit Long_Flag;
bit Led_Relay_Lock;
unsigned char ucLed_Lock[8];//保存锁定时LED状态


void Key_Proc()
{
	if(Key_Slow_Down<10)return;
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
			if(Seg_Disp_Mode == 2)//进入参数界面
			{
				Led_Relay_Lock = 1;//开启锁定
				for(i=0;i<8;i++)
				{
					ucLed_Lock[i] = ucLed[i];
					Relay_Old_Flag = Relay_Flag;
				}
			}
			else
			{
				Led_Relay_Lock = 0;
			}
			if(Seg_Disp_Mode == 1)
			{
				Tempera_Para_Ctrol = Tempera_Para;
				Distance_Para_Ctrol = Distance_Para;
			}
			if(Seg_Disp_Mode == 3)
			{
				Tempera_Para = Tempera_Para_Ctrol;
				Distance_Para = Distance_Para_Ctrol;
			}
		break;
		case 5:
			if(Seg_Disp_Mode == 2)//参数界面下
			{
				Para_Mode ^= 1;
			}
		break;
		case 8:
			if(Seg_Disp_Mode == 2)//参数界面下
			{
				if(Para_Mode == 0)//温度参数下
				{
					Tempera_Para_Ctrol = (Tempera_Para_Ctrol == 80)?80:Tempera_Para_Ctrol+1;
				}
				else if(Para_Mode == 1)//距离参数下
				{
					Distance_Para_Ctrol = (Distance_Para_Ctrol == 80)?80:Distance_Para_Ctrol+5;
				}
			}
		break;
		case 9:
			if(Seg_Disp_Mode == 2)//参数界面下
			{
				if(Para_Mode == 0)//温度参数下
				{
					Tempera_Para_Ctrol = (Tempera_Para_Ctrol == 20)?20:Tempera_Para_Ctrol-1;
				}
				else if(Para_Mode == 1)//距离参数下
				{
					Distance_Para_Ctrol = (Distance_Para_Ctrol == 20)?20:Distance_Para_Ctrol-5;
				}
			}
		break;
	}
	if(Key_Val == 17)
	{
		Long_Flag = 1;
	}
	if(Seg_Disp_Mode==3)
	{
		if(Timer_2s>=2000&&Long_Flag==1)
		{
			Long_Flag = 0;
			Timer_2s = 0;
			Relay_Count = 0;
		}
	}
}
void Seg_Proc()
{
	if(Seg_Slow_Down<100)return;
	Seg_Slow_Down = 0;
	Temperature = rd_temperature();
	switch(Seg_Disp_Mode)
	{
		case 0://环境状态界面
			Seg_Buf[0] = 11;//C 
			Seg_Buf[1] = Temperature / 10 % 10;
			Seg_Buf[2] = Temperature % 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			Seg_Buf[5] = 10;
			Seg_Buf[6] = 12;//n
      Seg_Buf[7] = Light_Level;
		break;
		case 1://运动检测状态
			Seg_Buf[0] = 13;//L 
			Seg_Buf[1] = Sport_Mode;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			Seg_Buf[5] = Distance / 100 % 10;
			Seg_Buf[6] = Distance / 10 % 10;
      Seg_Buf[7] = Distance % 10;			
		break;
		case 2://参数设置状态
		  Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			Seg_Buf[5] = 10;
			Seg_Buf[0] = 14;//P
		if(Para_Mode == 0)
		{
			Seg_Buf[1] = 11;//C
			Seg_Buf[6] = Tempera_Para_Ctrol / 10 % 10;
      Seg_Buf[7] = Tempera_Para_Ctrol % 10;			
		}
		else if(Para_Mode == 1)
		{
			Seg_Buf[1] = 13;//L
			Seg_Buf[6] = Distance_Para_Ctrol / 10 % 10;
      Seg_Buf[7] = Distance_Para_Ctrol % 10;						
		}			
		break;
		case 3://统计数据状态
			Seg_Buf[0] = 12;//n 
			Seg_Buf[1] = 11;//C
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = Relay_Count / 1000 % 10;
			Seg_Buf[5] = Relay_Count / 100 % 10;
			Seg_Buf[6] = Relay_Count / 10 % 10;
      Seg_Buf[7] = Relay_Count % 10;	
			for(i=4;i<7&&Seg_Buf[i]==0;i++)
				Seg_Buf[i] = 10;
		break;
	}
	
}

void Led_Proc()
{
	if(Led_Relay_Lock)
	{
		Led_Disp(ucLed_Lock);
		return;
	}
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
	if(Sport_Mode==1)//静止
	{
		ucLed[7] = 0;
	}
	else if(Sport_Mode == 2)//徘徊
	{
		ucLed[7] = 1;
	}
	else if(Sport_Mode == 3)//跑动
	{
		ucLed[7] = (Timer_200ms<100)?1:0;
	}
	
	Led_Disp(ucLed);
}
void AD_Proc()
{
	//如果锁定直接跳过
	if(Led_Relay_Lock)
	{
		return;
	}
	if(AD_Slow_Down<20)return;
		AD_Slow_Down = 0;
	AD_Value = Ad_Read(0x41)/51.0;//                                          别忘了改
	
	if(AD_Value>3.0)
		Light_Level = 1;
	else if(2.0<=AD_Value&&AD_Value<3.0)
		Light_Level = 2;
	else if(0.5<=AD_Value&&AD_Value<2.0)
		Light_Level = 3;
	else if(AD_Value<0.5)
		Light_Level = 4;
	
	/*继电器相关*/
	/*继电器相关*/
	if(Distance<Distance_Para)
		Near_Flag = 1;
	else
		Near_Flag = 0;
	
	if(Temperature>Tempera_Para)
		Hot_Flag = 1;
	else
		Hot_Flag = 0;
	
	if(Hot_Flag==1&&Near_Flag==1)
	{
		Relay(1);
		Relay_Flag = 1;
	}
	else
	{
		Relay(0);
		Relay_Flag = 0;
	}
	if(Relay_Old_Flag==0 && Relay_Flag ==1 )
	{
		Relay_Count++;
	}
	Relay_Old_Flag = Relay_Flag;
		
}

void Distance_Proc()
{
	float D_Distance;
	static unsigned char Last_Distance;
	if(Distance_Slow_Down < 1000)return;
		Distance_Slow_Down = 0;
	Distance = Ut_Wave_Data();
	D_Distance = abs(Distance - Last_Distance);
	Last_Distance = Distance;
	
	if(D_Distance<5)
	{
		Sport_Mode = 1;
	}
	else if(5<=D_Distance&&D_Distance<10)
	{
		Sport_Mode = 2;
	}
	else if(D_Distance>=10)
	{
		Sport_Mode = 3;	
	}
	/*测距相关*/
	

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
	Distance_Slow_Down++;
	AD_Slow_Down++;
	
	if(++Seg_Pos==8)Seg_Pos = 0;
	if(Seg_Buf[Seg_Pos]>='.')
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos]-'.',1);
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
		
	if(++Timer_200ms==200)
		Timer_200ms = 0;
	
	if(Long_Flag)
	{
		if(++Timer_2s==2000)
			Timer_2s = 2000;
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
	PCA_Init();
	Timer1_Init();
	rd_temperature();
	Delay750ms();
	
	while(1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		Distance_Proc();
		AD_Proc();
	}
}