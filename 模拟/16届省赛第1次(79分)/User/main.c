#include <STC15F2K60S2.H>
#include "Seg.h"
#include "Key.h"
#include "Led.h"
#include "iic.h"
#include "ultrasound.h"
#include "onewire.h"
#include "math.h"


#define abs(x) (x<0) ? -(x) : (x)
unsigned char Key_Val,Key_Down,Key_Up,Key_Old;
unsigned char Key_Slow_Down;//按键减速变量
unsigned char Seg_Pos;
unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};
unsigned char Seg_Slow_Down;//数码管减速变量
unsigned char ucLed[8] = {0,0,0,0,0,0,0,0};
unsigned char Led_Slow_Down;
unsigned int Distance_Slow_Down;//距离测量减速变量

/*显示相关*/
unsigned char Seg_Disp_Mode;//数码管显示模式 0-环境状态界面 1- 运动检测界面 2-参数设置界面 3-统计数据界面 
unsigned char Para_Disp_Mode;//参数显示界面模式 0-温度参数界面 1-距离参数界面
unsigned char Sport_Mode;//运动状态变量

signed char Temperature;//温度变量
unsigned char Light_Level = 0;//光强等级 0-N1 1-N2 2-N3 3-N4
unsigned char Voltage;//光敏电阻电压

/*距离相关*/
unsigned char Distance;//距离变量
unsigned char Distance_Old;//上一次的距离变量
float D_Distance;//两次测量差值 当然有正有负用abs

/*参数相关*/
unsigned char Distance_Para = 30;//距离参数变量
unsigned char Temperature_Para = 30;//温度参数

/*继电器相关*/
unsigned int Relay_Count;//继电器吸合次数统计

unsigned int Timer_2s;//两秒变量
unsigned char Time2s_Flag;//2秒变量标志位
bit State_Change_Flag;//状态更新标志
bit Hot_Temp_Flag = 0;//高温标志位
bit Distance_Proximity_Flag = 0;//接近标志位
bit Run_State;//跑动的状态
bit Stay_State;//静止的状态
bit ParaLysis_State;//徘徊的状态
bit First_Get_Distance;//第一次获取距离的值
bit Led_Blink_Flag;//LED闪烁标志位
unsigned char Blink_Time_100ms;

/*按键处理函数*/
void Key_Proc()
{
	if(Key_Slow_Down < 10)return;
	Key_Slow_Down=0;//键盘减速程序
	
	
	Key_Val = Key_Read();
	Key_Down = Key_Val & (Key_Old ^ Key_Val);
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);
	Key_Old = Key_Val;
	
	switch(Key_Down)
	{
		case 4://界面切换按键
			if(++Seg_Disp_Mode == 4)
				Seg_Disp_Mode = 0;
			if(Seg_Disp_Mode == 2)//在参数界面下
				Para_Disp_Mode = 0;//默认处于温度参数子界面
		break;
		case 5:
			if(Seg_Disp_Mode == 2)//处于参数界面
			{
				Para_Disp_Mode ^= 1;
			}
		break;
		case 8://加
			if(Para_Disp_Mode == 0)//在温度参数下
			{
				Temperature_Para = (Temperature_Para == 80) ? 80 :Temperature_Para+1;
			}
			if(Para_Disp_Mode == 1)//在距离参数下
			{
				Distance_Para = (Distance_Para == 80) ? 80 :Distance_Para+5;
			}
		break;
		case 9://减
			if(Para_Disp_Mode == 0)//在温度参数下
			{
				Temperature_Para = (Temperature_Para == 20) ? 20 :Temperature_Para-1;
			}
			if(Para_Disp_Mode == 1)//在距离参数下
			{
				Distance_Para = (Distance_Para == 20) ? 20 :Distance_Para-5;
			}
		break;
		case 17:
			if(Seg_Disp_Mode == 3)//处于统计界面下
			{
				Time2s_Flag = 1;
			}
			else
				Time2s_Flag = 0;
		break;
	}
	
}
/*数码管显示函数*/
void Seg_Proc()
{
	if(Seg_Slow_Down < 80) return;
		Seg_Slow_Down = 0;
	
	Temperature = rd_temperature();
	Distance = Ut_Wave_Data();

	if(Temperature > Temperature_Para)//高温判定
		Hot_Temp_Flag = 1;
	else
		Hot_Temp_Flag = 0;
	switch(Seg_Disp_Mode)
	{
		case 0://环境状态界面
			Seg_Buf[0] = 12;//C
			Seg_Buf[1] = Temperature / 10 % 10;
			Seg_Buf[2] = Temperature % 10;	
			Seg_Buf[3] = 10;
      Seg_Buf[4] = 10;
			Seg_Buf[5] = 10;
		  Seg_Buf[6] = 14;//n 00010011 11001000 0xc8
			Seg_Buf[7] = Light_Level;
		break;
		case 1://运动检测界面
			Seg_Buf[0] = 13;//L
			Seg_Buf[1] = Sport_Mode + 1;// 1-静止 2-徘徊 3-跑动
			Seg_Buf[2] = 10;
      Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			Seg_Buf[5] = Distance / 100 % 10;
			Seg_Buf[6] = Distance / 10 % 10;
			Seg_Buf[7] = Distance % 10;		
		break;
		case 2://参数设置界面
			switch(Para_Disp_Mode)
			{
				case 0://温度参数界面
					Seg_Buf[0] = 11;//P
					Seg_Buf[1] = 12;//C
					Seg_Buf[2] = 10;
					Seg_Buf[3] = 10;
					Seg_Buf[4] = 10;
					Seg_Buf[5] = 10;
					Seg_Buf[6] = Temperature_Para / 10 % 10;
					Seg_Buf[7] = Temperature_Para % 10;
				break;
				case 1://距离参数界面
					Seg_Buf[0] = 11;//P
					Seg_Buf[1] = 13;//L
					Seg_Buf[2] = 10;
					Seg_Buf[3] = 10;
					Seg_Buf[4] = 10;
					Seg_Buf[5] = 10;
					Seg_Buf[6] = Distance_Para / 10 % 10;
					Seg_Buf[7] = Distance_Para % 10;
				break;
			}				
		break;
		case 3://统计数据界面
			Seg_Buf[0] = 14;//N
			Seg_Buf[1] = 12;//C
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = Relay_Count / 1000 % 10;
			Seg_Buf[5] = Relay_Count / 100 % 10;
			Seg_Buf[6] = Relay_Count / 10 % 10 ;
			Seg_Buf[7] = Relay_Count % 10;			
		break;		
	}
}
void AD_Proc()
{
	/*AD_DA*/
	Voltage = Ad_Read(0x43) / 51.0f;//0x43是滑动变阻器记得该0x41
	if(Voltage >= 3.0)
	{
		Light_Level = 1;//强光
	}
	else if(Voltage >=2.0 && Voltage < 3.0)
	{
		Light_Level = 2;
	}
	else if(Voltage >= 0.5 && Voltage < 2.0)
	{
		Light_Level = 3;
	}
	else if(Voltage < 0.5)
	{
		Light_Level = 4;
	}
	if(Hot_Temp_Flag == 1 && Distance_Proximity_Flag == 1)
	{
		Relay(1);
		Relay_Count++;	
		Relay(0);
	}
	else
	{
		Relay(0);	
	}
}
/*其他显示函数*/
void Led_Proc()
{
	if(Led_Slow_Down != 5) return;
		Led_Slow_Down = 0;
		ucLed[0] =ucLed[1] =ucLed[2] =ucLed[3] = 0;
	//注意:当设备触发“接近”判定时，L1-L4 指示灯有四种工作状态，
	if(Distance_Proximity_Flag == 1)
	{
		switch(Light_Level)
		{
			case 1: ucLed[0] = 1; break; // 等级1：仅L1亮
			case 2: ucLed[0] = ucLed[1] = 1; break; // 等级2：L1+L2亮
			case 3: ucLed[0] = ucLed[1] = ucLed[2] = 1; break; // 等级3：L1-L3亮
			case 4: ucLed[0] = ucLed[1] = ucLed[2] = ucLed[3] = 1; break; // 等级4：全亮
		}
	}
	if(Sport_Mode == 1)//徘徊
	{
		ucLed[7] = 1;
	}
	else if(Sport_Mode == 2)
	{
		ucLed[7] = (Led_Blink_Flag == 1);
	}

	Led_Disp(ucLed);
}
void Get_Distance()
{
	unsigned char Temp_Motion_State;//运动状态暂时值!!!!!!!!!!(巧妙)
	if(Distance_Slow_Down<999)return;//1秒采集
		Distance_Slow_Down = 0;	
	
	Distance = Ut_Wave_Data();
	D_Distance = abs(Distance - Distance_Old);
	Distance_Old = Distance;	
	if(Distance < Distance_Para) //测量距离小于参数距离
		Distance_Proximity_Flag = 1; 
	if(!State_Change_Flag)//状态未锁定
	{
		if(D_Distance < 5.0)//静止
			Temp_Motion_State = 0;
		else if(D_Distance >= 5.0 && D_Distance < 10)//徘徊
			Temp_Motion_State = 1;
		else if(D_Distance >= 10)//跑动
			Temp_Motion_State = 2;
		State_Change_Flag = (Temp_Motion_State != Sport_Mode);
		Sport_Mode = Temp_Motion_State;
	}
		
	if(Temperature > Temperature_Para)//高温
	{		
		Hot_Temp_Flag = 1;	
	}
	else
		Hot_Temp_Flag = 0;
	

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
	Led_Slow_Down++;
	Distance_Slow_Down++;
	
	
	Seg_Pos = (++Seg_Pos) % 8;
	if(Seg_Buf[Seg_Pos] > 20)
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos] - ',',1);
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);	
	
	if(Time2s_Flag == 1)
	{
		if(++Timer_2s == 2000)
		{
			Timer_2s = 0;
			Time2s_Flag = 0;
			Relay_Count = 0;
		}
	}
	if(Sport_Mode == 2 && ++Blink_Time_100ms == 1000)
	{
		Led_Blink_Flag = 1;
		Blink_Time_100ms = 0;
	}
	else 
	{
		Led_Blink_Flag = 0;
	}

}

void main()
{
	Timer1_Init();
	PCA_Init();
	while(1)
	{
		Get_Distance();
		Key_Proc();
		Seg_Proc();
		AD_Proc();
		Led_Proc();
		
	}
}