/*头文件声明区*/
#include <STC15F2K60S2.H>
#include "Key.h"
#include "Seg.h"
#include "Led.h"
#include "Init.h"
#include "Uart.h"
#include "ultrasound.h"
#include "ds1302.h"

/*变量声明区*/
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量
unsigned char Key_Slow_Down;//按键减速专用变量
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char ucLed[8]={0,0,0,0,0,0,0};//Led显示数据存放数组
unsigned char Seg_Disp_Mode;//0-数据界面 1-时间界面 2-记录界面

unsigned char Distance;//距离变量
unsigned char i;
unsigned char RTC_Timer[] = {23,59,06};//时钟变量
unsigned char Trigger_Slow_Down;

unsigned char Record_Hour[3] = {0,0,0};//记录1-3的小时
unsigned char Record_Min[3] = {0,0,0};//记录1-3的分钟
unsigned char Record_Sec[3] = {0,0,0};//记录1-3的秒钟

unsigned char RTC_Index = 0;//索引 0=1 1=2 2=3
bit Trigger_Flag = 0;//触发标志位 0=未触发 1=已触发
unsigned char Record_Count;
unsigned char j;



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
		case 4://界面切换按键
			Seg_Disp_Mode = (++Seg_Disp_Mode % 3);
		if(Seg_Disp_Mode == 2)//记录界面
		{
			RTC_Index = 0;
		}
		break;
		case 5:
			RTC_Index = (++RTC_Index % 3);
		break;
		case 8:
			if(Seg_Disp_Mode == 2)//记录界面
			{
				for(j=0;j<3;j++)
				{
					Record_Hour[j] = 0;
					Record_Min[j] = 0;
					Record_Sec[j] = 0;
				}
				Record_Count = 0;
				RTC_Index = 0;
			}
		break;
		case 9:
			Trigger_Flag = 0;
			Relay(0);
		break;
	}
}




/*信息处理函数*/
void Seg_Proc()
{
	if(Seg_Slow_Down < 100) return;
	Seg_Slow_Down = 0;//数码管减速程序
	
	Distance = Ut_Wave_Data();
	Read_Rtc(RTC_Timer);
	
	switch(Seg_Disp_Mode)
	{
		case 0://数据界面 L 1110 0011 1100 0111 0xc7
			Seg_Buf[0] = 11;//L
			Seg_Buf[1] = 10;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			Seg_Buf[5] = Distance / 100 % 10;
			Seg_Buf[6] = Distance / 10 % 10;
			Seg_Buf[7] = Distance % 10;
		for(i=5;i<7 && Seg_Buf[i] == 0;i++)
			Seg_Buf[i] = 10;
		break;
		case 1://时间界面 - 1111 1101 1011 1111 0xbf
			Seg_Buf[0] = RTC_Timer[0] / 10 % 10;
			Seg_Buf[1] = RTC_Timer[0] % 10;
			Seg_Buf[2] = 12;//-
			Seg_Buf[3] = RTC_Timer[1] / 10 % 10;
			Seg_Buf[4] = RTC_Timer[1] % 10;
			Seg_Buf[5] = 12;//-
			Seg_Buf[6] = RTC_Timer[2] / 10 % 10;
			Seg_Buf[7] = RTC_Timer[2] % 10;			
		break;
		case 2://记录界面
			Seg_Buf[0] = 13;//A 0001 0001 1000 1000 0x88
			Seg_Buf[1] = RTC_Index + 1;
	
			Seg_Buf[2] = 12;
		  Seg_Buf[3] = 12;
		  Seg_Buf[4] = 12;
		  Seg_Buf[5] = 12;
		  Seg_Buf[6] = 12;
		  Seg_Buf[7] = 12;
			
		if(Record_Count > 0 && Record_Count > RTC_Index)
		{
			Seg_Buf[2] = Record_Hour[RTC_Index] / 10 % 10;
			Seg_Buf[3] = Record_Hour[RTC_Index] % 10;	
			Seg_Buf[4] = Record_Min[RTC_Index] / 10 % 10;
			Seg_Buf[5] = Record_Min[RTC_Index] % 10;	
			Seg_Buf[6] = Record_Sec[RTC_Index] / 10 % 10;
			Seg_Buf[7] = Record_Sec[RTC_Index] % 10;				
		}
		break;
	}
}



/*其他显示函数*/
void Led_Proc()
{
	ucLed[0] = (Seg_Disp_Mode == 0);
	ucLed[1] = (Seg_Disp_Mode == 1);
	ucLed[2] = (Seg_Disp_Mode == 2);
	
	Led_Disp(ucLed);
	
}

void Trigger_Proc()
{
	if(Trigger_Slow_Down < 30) return;
		Trigger_Slow_Down = 0;
	if(Distance < 30)
	{
		Relay(1);
		if(Trigger_Flag == 0)//仅第一次触发时记录,等距离恢复到30cm以上后再次触发
		{
			Record_Hour[2] = Record_Hour[1];
			Record_Min[2] = Record_Min[1];	
			Record_Sec[2] = Record_Sec[1];	

			Record_Hour[1] = Record_Hour[0];
			Record_Min[1] = Record_Min[0];	
			Record_Sec[1] = Record_Sec[0];	

			Record_Hour[0] = RTC_Timer[0];
			Record_Min[0] = RTC_Timer[1];	
			Record_Sec[0] = RTC_Timer[2];	
			
			//更新状态
			Trigger_Flag = 1;
			if(Record_Count < 3)
				Record_Count++;
			
		}
	}
	else if(Distance >= 30)
	{
		Trigger_Flag = 0;
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


/*定时器0中断服务函数*/
void Timer1_Isr(void) interrupt 3
{
	Key_Slow_Down++;
	Seg_Slow_Down++;
	Trigger_Slow_Down++;
	
	
	if(++Seg_Pos == 8) Seg_Pos = 0;//数码管显示专用
	if(Seg_Buf[Seg_Pos]>='.')
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos] - '.',1);
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
	
}

/* Main */
void main()
{
	Sys_Init();
	PCA_Init();
	Timer1_Init();
	Set_Rtc(RTC_Timer);
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		Trigger_Proc();
	}
}



