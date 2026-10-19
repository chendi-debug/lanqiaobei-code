/*头文件声明区*/
#include <STC15F2K60S2.H>
#include "Init.h"
#include "Seg.h"
#include "Key.h"
#include "Led.h"
#include "ultrasound.h"
#include "DS1302.h"


unsigned char ucLed[] = {0,0,0,0,0,0,0,0};
unsigned char Seg_Buf[] = {10,10,10,10,10,10,10,10};
unsigned char Seg_Pos;
unsigned char Seg_Slow_Down;//数码管减速变量
unsigned char Key_Val,Key_Down,Key_Up,Key_Old;
unsigned char Key_Slow_Down;//按键减速变量
unsigned char Seg_Disp_Mode;//数码管显示变量 0-数据界面 1-时间界面 2-记录界面
unsigned int Distance;//距离变量
unsigned char i;
unsigned char Rtc_Buf[] = {23,59,50};
unsigned char Record_Hour[3] = {0,0,0};//记录1-3的小时
unsigned char Record_Min[3] = {0,0,0};//记录1-3的分钟
unsigned char Record_Sec[3] = {0,0,0};//记录1-3的秒钟


unsigned char Rtc_Index = 0; // 索引0=1，1=2，2=3（对应题目1/2/3）
bit Trigger_Flag = 0; // 触发状态标记：0=未触发，1=已触发（避免持续触发重复记录）
unsigned char Record_Count = 0; // 记录次数（0-3，判断是否有记录）
unsigned char j;



void Key_Proc()
{
	if(Key_Slow_Down < 10)return;
	Key_Slow_Down = 0;
	
	Key_Val = Key_Read();
	Key_Down = Key_Val&(Key_Val^Key_Old);
	Key_Up = ~Key_Val&(Key_Val ^ Key_Old);	
	Key_Old = Key_Val;
	switch(Key_Down)
	{
		case 4:
			if(++Seg_Disp_Mode == 3)
				Seg_Disp_Mode = 0;
			if(Seg_Disp_Mode == 2)
			{
				Rtc_Index = 0;
			}
		break;
		case 5:
			if(Seg_Disp_Mode == 2)//只有在记录界面才有效
			{
				if(++Rtc_Index == 3)
					Rtc_Index = 0;
			}
		break;
		case 8:
			if(Seg_Disp_Mode == 2)
			{
				for(j=0;j<3;j++)
				{
					Record_Hour[j] = 0;
					Record_Min[j] = 0;
					Record_Sec[j] = 0;
				}
				Record_Count = 0;
				Rtc_Index = 0;
			}
		break;
		case 9:
			Trigger_Flag = 0;
			Beep(0);
		
		break;
	}
	
}
void Seg_Proc()
{
	if(Seg_Slow_Down < 100)return;
	Seg_Slow_Down = 0;
	
	Read_Rtc(Rtc_Buf);

	
	switch(Seg_Disp_Mode)
	{
		case 0://数据界面 L 1110 0011  1100 0111 0xc7
			Seg_Buf[0] = 11;//L
			Seg_Buf[1] = Seg_Buf[2] = Seg_Buf[3] = Seg_Buf[4] = 10;
			Seg_Buf[5] = Distance / 100 % 10;
			Seg_Buf[6] = Distance / 10 % 10;		
			Seg_Buf[7] = Distance % 10;

		break;
		case 1://时间界面 - 1111 1101  1011 1111
			Seg_Buf[0] = Rtc_Buf[0] / 10 % 10;
			Seg_Buf[1] = Rtc_Buf[0] % 10;
			Seg_Buf[2] = 12;//-
			Seg_Buf[3] = Rtc_Buf[1] / 10 % 10;
			Seg_Buf[4] = Rtc_Buf[1] % 10;
			Seg_Buf[5] = 12;
			Seg_Buf[6] = Rtc_Buf[2] / 10 % 10;
			Seg_Buf[7] = Rtc_Buf[2] % 10;
		
		break;
		case 2://记录界面 A 0001 0001
			Seg_Buf[0] = 13;//A
			Seg_Buf[1] = Rtc_Index+1;
		
			// 第二步：默认显示“-”（保底，无记录时显示）
			Seg_Buf[2] = 12;//-（时十位）
			Seg_Buf[3] = 12;//-（时个位）
			Seg_Buf[4] = 12;//-（分十位）
			Seg_Buf[5] = 12;//-（分个位）
			Seg_Buf[6] = 12;//-（秒十位）
			Seg_Buf[7] = 12;//-（秒个位）	
		
			//第3步
			if(Rtc_Index < Record_Count)
			{
				// 用独立数组取值，无二维索引，避免错误
				Seg_Buf[2] = Record_Hour[Rtc_Index] / 10 % 10; // 时十位
				Seg_Buf[3] = Record_Hour[Rtc_Index] % 10;      // 时个位
				Seg_Buf[4] = Record_Min[Rtc_Index] / 10 % 10;  // 分十位
				Seg_Buf[5] = Record_Min[Rtc_Index] % 10;       // 分个位
				Seg_Buf[6] = Record_Sec[Rtc_Index] / 10 % 10;  // 秒十位
				Seg_Buf[7] = Record_Sec[Rtc_Index] % 10;       // 秒个位
				
				// 容错：时间值异常时仍显示“-”，避免黑屏
				if(Record_Hour[Rtc_Index] > 23) {Seg_Buf[2]=12; Seg_Buf[3]=12;}
				if(Record_Min[Rtc_Index] > 59)  {Seg_Buf[4]=12; Seg_Buf[5]=12;}
				if(Record_Sec[Rtc_Index] > 59)  {Seg_Buf[6]=12; Seg_Buf[7]=12;}
			}		
		
		break;
	}
	
}
void Led_Proc()
{
	
	// 先清空所有LED，再按当前界面点亮对应LED
	for(i=0; i<8; i++)
			ucLed[i] = 0; // 0=熄灭（根据你的底层定义，若1=点亮则反过来）
	
	switch(Seg_Disp_Mode)
	{
			case 0: ucLed[0] = 1; break; // L1（第0位）点亮（数据界面）
			case 1: ucLed[1] = 1; break; // L2（第1位）点亮（时间界面）
			case 2: ucLed[2] = 1; break; // L3（第2位）点亮（记录界面）
	}
	Led_Disp(ucLed);
	
	
}

void Ultrasound_Proc()
{

	Distance = Ut_Wave_Data();
	
	if(Distance < 30)
	{
		Beep(1);
		if(Trigger_Flag == 0)//仅第一次触发时记录
		{
			//1.队列移位：旧数据后移

			Record_Hour[2] = Record_Hour[1];
			Record_Min[2] = Record_Min[1];
			Record_Sec[2] = Record_Sec[1];	

			Record_Hour[1] = Record_Hour[0];
			Record_Min[1] = Record_Min[0];
			Record_Sec[1] = Record_Sec[0];				
		
			//2.新触发时间存入索引1(Rtc_Record)
			Read_Rtc(Rtc_Buf);//先读当前时间
			Record_Hour[0] = Rtc_Buf[0]; // 小时
			Record_Min[0] = Rtc_Buf[1];  // 分钟
			Record_Sec[0] = Rtc_Buf[2];  // 秒数
			
			//3.更新记录状态
			Trigger_Flag = 1;
			if(Record_Count < 3) 
				Record_Count++;		
		}	
	}
	else
	{
		Trigger_Flag = 0;
		Beep(0);
	}
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
	EA = 1;					//开启
}

void Timer1_Isr(void) interrupt 3
{
	Seg_Slow_Down++;
	Key_Slow_Down++;
	
	
	if(++Seg_Pos == 8)Seg_Pos = 0;
	if(Seg_Buf[Seg_Pos]>='.')
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos]-'.',1);
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
}

void main()
{
	System_Init();
	Timer1_Init();
	Set_Rtc(Rtc_Buf);
	PCA_Init();
	Beep(1);
	while(1)
	{
		Seg_Proc();
		Key_Proc();
		Led_Proc();
		Ultrasound_Proc();
	}
}
