/*头文件声明区*/
#include <STC15F2K60S2.H>
#include "Key.h"
#include "Seg.h"
#include "Led.h"
#include "Init.h"
#include "Uart.h"
#include "ultrasound.h"
#include "onewire.h"
#include "iic.h"
/*变量声明区*/
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量
unsigned char Key_Slow_Down;//按键减速专用变量
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char ucLed[8]={0,0,0,0,0,0,0};//Led显示数据存放数组
unsigned char Seg_Disp_Mode;//0-环境状态界面 1-运动检测界面 2-参数界面 3-统计数据界面
unsigned char Para_Disp_Mode;//参数设置子界面 0-温度参数界面  1-距离参数
unsigned char Temperature;//温度变量
unsigned char Flash_Level = 0;//光强等级 
unsigned char Sport_State;//运动状态 1-静止 2-徘徊 3-跑动
unsigned int Distance;//距离参数
unsigned char Temperature_Para = 30;//温度参数
unsigned char Distance_Para = 30;//距离参数
unsigned int Relay_Count;//继电器吸合次数
unsigned char AD_Slow_Down;
unsigned char i;
float AD_Val;//AD值
bit Hot_Temperature;//高温
unsigned int Distance_Slow_Down;
#define abs(x) (x<0) ? -(x) : (x)

bit State_Change_Flag;//状态更新标志
bit Near_Flag;//接近标志位

unsigned char Timer_100ms;
bit Flash_Flag;//L8闪烁标志位

unsigned int Timer_2000ms;//2秒变量
bit Long_Press;//长按标志
/*键盘处理函数*/

void Key_Proc()
{
	if(Key_Slow_Down < 10)return;
	Key_Slow_Down=0;//键盘减速程序
	
	Key_Val =Key_Read();//实时读取键码值
	Key_Down = Key_Val & (Key_Old ^ Key_Val);//判断按键是否按下
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);//捕捉按键上升沿

	
	switch(Key_Val)
	{
		case 17://清零计数
			Long_Press = 1;
		if(Timer_2000ms > 2000)
		{
			Relay_Count = 0;
			Long_Press = 0;
		}
		break;
	}
	Key_Old = Key_Val;//辅助扫描变量
	switch(Key_Down)
	{
		case 4:
			if(++Seg_Disp_Mode == 4)Seg_Disp_Mode = 0;
			Para_Disp_Mode = 0;
		break;
		case 5:
			if(Seg_Disp_Mode == 2)//在参数界面下
			{
				if(++Para_Disp_Mode == 2)Para_Disp_Mode = 0;
			}
		break;
		case 8:
			if(Para_Disp_Mode == 0)//温度参数下
			{
				Temperature_Para = (Temperature_Para == 80) ? 80 : Temperature_Para+1;
			}
			if(Para_Disp_Mode == 1)//距离参数下
			{
				Distance_Para = (Distance_Para == 80) ? 80 : Distance_Para+5;
			}
		break;
		case 9:
			if(Para_Disp_Mode == 0)//温度参数下
			{
				Temperature_Para = (Temperature_Para == 20) ? 20 : Temperature_Para-1;
			}
			if(Para_Disp_Mode == 1)//距离参数下
			{
				Distance_Para = (Distance_Para == 20) ? 20 : Distance_Para-5;
			}
		break;
	}

}




/*信息处理函数*/
void Seg_Proc()
{
	if(Seg_Slow_Down < 80) return;
	Seg_Slow_Down = 0;//数码管减速程序
	
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
			Seg_Buf[7] = Flash_Level;
//			Seg_Buf[3] = Key_Val /10 % 10;
//			Seg_Buf[4] =Key_Val % 10;
		break;
		case 1://运动检测界面
			Seg_Buf[0] = 13;//L 
			Seg_Buf[1] = Sport_State;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			Seg_Buf[5] = Distance / 100 % 10;
			Seg_Buf[6] = Distance / 10 % 10;//n
			Seg_Buf[7] = Distance % 10;	
		break;
		case 2://参数界面
			if(Para_Disp_Mode == 0)//温度参数界面	
			{				
				Seg_Buf[0] = 14;//P  
				Seg_Buf[1] = 11;//C
				Seg_Buf[2] = 10;
				Seg_Buf[3] = 10;
				Seg_Buf[4] = 10;
				Seg_Buf[5] = 10;
				Seg_Buf[6] = Temperature_Para / 10 % 10;
				Seg_Buf[7] = Temperature_Para % 10;	
			}
			else if(Para_Disp_Mode == 1) 
			{
				Seg_Buf[0] = 14;//P  
				Seg_Buf[1] = 13;//L
				Seg_Buf[2] = 10;
				Seg_Buf[3] = 10;
				Seg_Buf[4] = 10;
				Seg_Buf[5] = 10;
				Seg_Buf[6] = Distance_Para / 10 % 10;
				Seg_Buf[7] = Distance_Para % 10;						
			}
		break;
		case 3://统计数据界面
				Seg_Buf[0] = 12;//n  
				Seg_Buf[1] = 11;//c
				Seg_Buf[2] = 10;
				Seg_Buf[3] = 10;
				Seg_Buf[4] = 10;
				Seg_Buf[5] = Relay_Count / 100 % 10;
				Seg_Buf[6] = Relay_Count / 10 % 10;
				Seg_Buf[7] = Relay_Count % 10;	
				for(i = 5;i<7 && Seg_Buf[i] == 0;i++)
					Seg_Buf[i] = 10; 
		break;
	}

}



/*其他显示函数*/
void Led_Proc()
{
	if(Near_Flag)
	{
		if(Flash_Level == 1) {ucLed[0] = 1;ucLed[1] = ucLed[2] = ucLed[3] =0;}
		if(Flash_Level == 2) {ucLed[0] = ucLed[1] = 1;ucLed[2] = ucLed[3] = 0;}
		if(Flash_Level == 3) {ucLed[0] = ucLed[1] = ucLed[2] = 1;ucLed[3] = 0;}
		if(Flash_Level == 4) {ucLed[0] = ucLed[1] = ucLed[2] = ucLed[3] = 1;}
	}
	else
	{
		ucLed[0] = ucLed[1] = ucLed[2] = ucLed[3] = 0;
	}
	
	if(Sport_State == 1)//静止
	{
		ucLed[7] = 0;
	}
	else if(Sport_State == 2)
	{
		ucLed[7] = 1;		
	}
	else if(Sport_State == 3)
	{
//		ucLed[7] = (Flash_Flag == 1) ? 1 : 0;
	}
	Led_Disp(ucLed);
	
}

void AD_Proc()
{
	if(AD_Slow_Down < 90)return;
	AD_Slow_Down = 0;
	AD_Val = Ad_Read(0x41) / 51.0f;//0~5V
	
	if(AD_Val >= 3.0)
		Flash_Level = 1;
	else if(AD_Val >= 2.5 && AD_Val < 3.0)
		Flash_Level = 2;		
	else if(AD_Val >= 0.5 && AD_Val < 2.0)
		Flash_Level = 3;	
	else if(AD_Val < 0.5)
		Flash_Level = 4;	

	/*接近判断*/
	if(Distance < Distance_Para)
		Near_Flag = 1;//触发接近
	else
		Near_Flag = 0;
	
}

void Distance_Proc()
{
	float D_Distance;//两次测量的差值
//	unsigned char Temp_Motion_State;//运动状态暂时值
	unsigned char Distance_Old;
	if(Distance_Slow_Down < 1000)return;
	Distance_Slow_Down = 0;

	Distance = Ut_Wave_Data();
	D_Distance = abs(Distance - Distance_Old);
	Distance_Old = Distance;
	
	if(!State_Change_Flag)//未锁定
	{
		if(D_Distance < 5.0)
			Sport_State = 1;//静止
		else if(D_Distance >= 5.0 && D_Distance < 10)
			Sport_State = 2;//徘徊
		else if(D_Distance >= 10)
			Sport_State = 3;//跑动			
	}
	/*高温判断*/
	if(Temperature > Temperature_Para)
		Hot_Temperature = 1;
	else 
		Hot_Temperature = 0;	
	
	/*接近判断*/
	if(Distance < Distance_Para)
		Near_Flag = 1;//触发接近
	else
		Near_Flag = 0;	

		/*继电器吸合判断*/
	if((Hot_Temperature == 1) && (Near_Flag == 1))
	{
		Relay(1);
		Relay_Count++;
	}
	else
	{
		Relay(0);		
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


/*定时器0中断服务函数*/
void Timer1_Isr(void) interrupt 3
{
	Key_Slow_Down++;
	Seg_Slow_Down++;
	AD_Slow_Down++;
	Distance_Slow_Down++;
	
	
	if(++Seg_Pos == 8) Seg_Pos = 0;//数码管显示专用
	if(Seg_Buf[Seg_Pos]>='.')
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos] - '.',1);
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
	
	
	if(++Timer_100ms == 100)
	{
		Timer_100ms = 0;
		Flash_Flag = 1;
	}
	else 
		Flash_Flag = 0;
	if(Long_Press == 1)
	{
		if(++Timer_2000ms == 2000)
			Timer_2000ms = 2001;
	}
	else
		Timer_2000ms = 0;
}

/* Main */
void main()
{
	Sys_Init();
		Timer1_Init();
rd_temperature();
	Delay750ms();

	PCA_Init();

	
	while (1)
	{

		Seg_Proc();
				Key_Proc();
		Led_Proc();
		AD_Proc();
		Distance_Proc();
	}
}



