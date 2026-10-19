#include <STC15F2K60S2.H>
#include "Init.h"
#include "Seg.h"
#include "Key.h"
#include "Led.h"
#include "Distance.h"
#include "iic.h"
#include "onewire.h"

unsigned char Key_Val,Key_Down,Key_Up,Key_Old;
unsigned char Key_Slow_Down;
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};
unsigned char Seg_Slow_Down;
unsigned char Seg_Pos;
unsigned char ucLed[8]={0,0,0,0,0,0,0,0};
unsigned char Seg_Disp_Mode;//0-测距 1-参数 2-工厂
unsigned char Para_Mode;//0-距离参数 1-温度参数
unsigned char Set_Mode;//0-校准值 1-介质 2-DAC输出
unsigned int Temperature;
signed int Distance;
bit Distacne_Mode=0;//0-cm 1-m
unsigned char Distacne_Para=40;
unsigned char Temperature_Para=30;
signed char Cal_Value=0;
#define abs(x) (x<0)?-(x):(x)
unsigned char i;
unsigned char Distance_Slow_Down;
unsigned int Speed=340;
float Da_Value=10;
unsigned char Tiemr_100ms;
bit Value_Flag=0;
unsigned int Timer_6s;
bit Start_Mode;
unsigned int Timer_2s;
bit S8_Flag;
unsigned int Timer_1s;
bit s8_Long_Flag;

void Key_Proc()
{
	if(Value_Flag==1)
		return;
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
		Para_Mode=0;
		Set_Mode=0;
		break;
		case 5:
			if(Seg_Disp_Mode==0)//测距
			{
				Distacne_Mode ^=1;
			}
			if(Seg_Disp_Mode == 1)//参数
			{
				Para_Mode ^=1;
			}
			if(Seg_Disp_Mode==2)//在工厂模式下
			{
				if(++Set_Mode==3)Set_Mode=0;
			}
		break;
		case 8:
			S8_Flag=1;
		if(S8_Flag==1&&Timer_1s>=1000)
		{
			S8_Flag=0;
			Timer_1s=0;
			s8_Long_Flag=1;
		}
		else
		{
			Timer_1s=0;
			s8_Long_Flag=0;
			S8_Flag=0;
			if(Seg_Disp_Mode==0&&Key_Down==8)//在测距界面下
			{
				Value_Flag=1;
			}
			if(Seg_Disp_Mode==1&&Para_Mode==0)//距离参数
			{
				Distacne_Para=(Distacne_Para==90)?90:Distacne_Para+10;
			}
			if(Seg_Disp_Mode==1&&Para_Mode==1)//温度参数
			{
				Temperature_Para=(Temperature_Para==80)?80:Temperature_Para+1;
			}
			if(Seg_Disp_Mode==2&&Set_Mode==0)//校准值设置
			{
				Cal_Value=(Cal_Value==90)?90:Cal_Value+5;
			}
			if(Seg_Disp_Mode==2&&Set_Mode==1)//介质设置
			{
				Speed=(Speed==9990)?9990:Speed+10;
			}
			if(Seg_Disp_Mode==2&&Set_Mode==2)//DAC输出设置
			{
				Da_Value=(Da_Value==20)?20:Da_Value+1;
			}
		}
		break;
		case 9:
			if(Seg_Disp_Mode==1&&Para_Mode==0)//距离参数
			{
				Distacne_Para=(Distacne_Para==10)?10:Distacne_Para-10;
			}
			if(Seg_Disp_Mode==1&&Para_Mode==1)//温度参数
			{
				Temperature_Para=(Temperature_Para==0)?0:Temperature_Para-1;
			}
			if(Seg_Disp_Mode==2&&Set_Mode==0)//校准值设置
			{
				Cal_Value=(Cal_Value==-90)?-90:Cal_Value-5;
			}
			if(Seg_Disp_Mode==2&&Set_Mode==1)//介质设置
			{
				Speed=(Speed==10)?10:Speed-10;
			}
			if(Seg_Disp_Mode==2&&Set_Mode==2)//DAC输出设置
			{
				Da_Value=(Da_Value==1)?1:Da_Value-1;
			}
		break;
	}
	if(Key_Val==17)
	{
		Start_Mode=1;
	}
	if(Start_Mode==1&&Timer_2s>=2000)
	{
		Seg_Disp_Mode=0;
		Start_Mode=0;
		Timer_2s=0;
		Distacne_Para=40;
		Temperature_Para=30;
		Cal_Value=0;
		Speed=340;
		Da_Value=10;		
	}
}

void Seg_Proc()
{
	if(Seg_Slow_Down<90)return;
		Seg_Slow_Down=0;

	
	switch(Seg_Disp_Mode)
	{
		case 0://测距界面
			Seg_Buf[0]=Temperature/100%10;
			Seg_Buf[1]=Temperature/10%10+'.';
			Seg_Buf[2]=Temperature%10;
			Seg_Buf[3]=12;//-
			Seg_Buf[4]=10;
		if(Distacne_Mode==0)//cm
		{
			Seg_Buf[5]=((Distance+Cal_Value)<=-10)?12:abs((Distance+Cal_Value)/100%10);
			Seg_Buf[6]=(Seg_Buf[5]==0&&(Distance+Cal_Value)<0)?12:abs((Distance+Cal_Value)/10%10);		
			Seg_Buf[7]=abs((Distance+Cal_Value)%10);	
			for(i=5;i<7&&Seg_Buf[i]==0;i++)
				Seg_Buf[i]=10;
		}
		else 
		{
			Seg_Buf[5]=(Distance+Cal_Value)/100%10+'.';
			Seg_Buf[6]=(Distance+Cal_Value)/10%10;		
			Seg_Buf[7]=(Distance+Cal_Value)%10;				
		}

		break;
		case 1://参数界面
			if(Para_Mode==0)//距离参数
			{
				Seg_Buf[0]=11;//P
				Seg_Buf[1]=1;
			  Seg_Buf[2]=Key_Val/10%10;
				Seg_Buf[3]=Key_Val%10;
				Seg_Buf[4]=10;
				Seg_Buf[5]=10;
				Seg_Buf[6]=Distacne_Para/10%10;
				Seg_Buf[7]=Distacne_Para%10;				
			}
			else
			{
				Seg_Buf[0]=11;//P
				Seg_Buf[1]=2;
			  Seg_Buf[2]=10;
				Seg_Buf[3]=10;
				Seg_Buf[4]=10;
				Seg_Buf[5]=10;
				Seg_Buf[6]=Temperature_Para/10%10;
				Seg_Buf[7]=Temperature_Para%10;					
			}
		break;
		case 2://工厂模式
			switch(Set_Mode)
			{
				case 0://校准值
					Seg_Buf[0]=13;//F 
					Seg_Buf[1]=1;
					Seg_Buf[2]=10;
					Seg_Buf[3]=10;
					Seg_Buf[4]=10;	
					if(Cal_Value>0)
					{
						Seg_Buf[5]=10;
						Seg_Buf[6]=(Seg_Buf[6]==0)?10:Cal_Value/10%10;
						Seg_Buf[7]=Cal_Value%10;		
					}		
					else if(Cal_Value<0)
					{
						Seg_Buf[5]=(Cal_Value>-10)?10:12;
						Seg_Buf[6]=(Seg_Buf[5]==10)?12:abs(Cal_Value/10%10);
						Seg_Buf[7]=abs(Cal_Value%10);						
					}
					else if(Cal_Value==0)
					{
						Seg_Buf[5]=10;
						Seg_Buf[6]=10;
						Seg_Buf[7]=0;							
					}
				break;
				case 1://介质设置界面
					Seg_Buf[0]=13;//F 
					Seg_Buf[1]=2;
					Seg_Buf[2]=10;
					Seg_Buf[3]=10;
					Seg_Buf[4]=Speed/1000%10;					
					Seg_Buf[5]=Speed/100%10;					
					Seg_Buf[6]=Speed/10%10;					
					Seg_Buf[7]=Speed%10;					
					for(i=4;i<7&&Seg_Buf[i]==0;i++)
						Seg_Buf[i]=10;
				break;
				case 2://DAC输出设置界面
					Seg_Buf[0]=13;//F 
					Seg_Buf[1]=3;
					Seg_Buf[2]=10;
					Seg_Buf[3]=10;
					Seg_Buf[4]=10;					
					Seg_Buf[5]=10;					
					Seg_Buf[6]=(unsigned char)Da_Value/10%10+'.';					
					Seg_Buf[7]=(unsigned char)Da_Value%10;						
				break;
			}				
		break;
	}
}
void Led_Proc()
{
	if(Seg_Disp_Mode==0)//测距界面
	{
		if(Distance>=255)
		{
			for(i=0;i<8;i++)
				ucLed[i]=1;
		}
		else if(Distance)
		{
			if(Distance>=0)
			{
				for(i=0;i<8;i++)
				{
				ucLed[i]=((Distance+Cal_Value)>>i)&0x01;
				}
			}
			else 
			{
				for(i=0;i<8;i++)
				{
				ucLed[i]=0;
				}				
			}
		}
	}
	if(Seg_Disp_Mode==1)
	{
		ucLed[7]=1;
	}
	if(Seg_Disp_Mode==2)//工厂模式
	{
		ucLed[0]=(Tiemr_100ms<100)?1:0;
	}
	else
		ucLed[0]=0;
	Led_Disp(ucLed);
	
	/*Da相关*/
	if(Distance<10)
		Da_Write((Da_Value/10)*51.0);
	else if(Distance>90)
		Da_Write(5*51.0);
	else if(Distance<90&&Distance>10)
	{
		Da_Write(((float)(((float)(5-Da_Value)/80)*Distance+1.125*Da_Value-0.625))*51.0f);		
	}
}
void Distance_Proc()
{
	if(Distance_Slow_Down<140)return;
	Distance_Slow_Down=0;
		Temperature=rd_temperature()*10;
	Distance = Ut_Wave_Data();
	

	if((Distacne_Para-5<=(Distance+Cal_Value)<=Distacne_Para+5)&&(Temperature/10)<=Temperature_Para)
	{
		Relay(1);
	}
	else
		Relay(0);

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
	Distance_Slow_Down++;
	
	if(++Seg_Pos==8)Seg_Pos=0;
	if(Seg_Buf[Seg_Pos]>='.')
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos]-'.',1);
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
		
	
	if(++Tiemr_100ms==200&&Seg_Disp_Mode==2)
		Tiemr_100ms=0;
	
	if(Value_Flag==1)
	{
		if(++Timer_6s==6000)
		{
			Timer_6s=0;
			Value_Flag=0;
		}
	}
	
	if(Start_Mode==1&&++Timer_2s==2000)
	{
		Timer_2s=2000;
	}
	
	if(S8_Flag)
	{
		if(++Timer_1s==1000)
		{
			Timer_1s=1000;
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
	rd_temperature();
	Delay750ms();

	while(1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		Distance_Proc();
	}
}
