/*头文件声明区*/
#include <STC15F2K60S2.H>
#include "Key.h"
#include "Seg.h"
#include "Led.h"
#include "Init.h"
#include "ultrasound.h"
#include "ds1302.h"  		
#include "iic.h"

/*变量声明区*/
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量
unsigned char Key_Slow_Down;//按键减速专用变量
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Point[8]={0,0,0,0,0,0,0,0};//数码管小数点数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char ucLed[8]={0,0,0,0,0,0,0};//Led显示数据存放数组

unsigned char Seg_Disp_Mode;//数码管显示模式 0- 数据显示界面 1-参数设置界面
unsigned char Display_Mode;//数据显示界面分界面 0-时间数据显示界面 1-距离数据显示界面 2-数据记录显示界面
unsigned char Data_Display_Mode;//数据记录显示界面分界面 0-最大值 1-平均值 2-最小值
unsigned char Para_Set_Mode;//参数设置界面 0-采集时间设置界面 1-距离参数设置界面
bit Distance_Data_Mode;//距离数据显示格式 0-触发模式 1-定时模式
pdata unsigned char Distance_Max = 0;//距离最大值变量
pdata unsigned char Distance_Avg = 0;//距离平均值变量
pdata unsigned char Distance_Min = 0xFFFF;//距离最小值变量

pdata unsigned char Rtc_Timer[] = {20,20,01};//初始时间
pdata unsigned int Distance;//距离变量
unsigned char i;

#define MAX_RECORD 30//最大记录数
pdata unsigned int Distance_Record[MAX_RECORD];//距离记录数组（扩大10倍）
unsigned char Record_Count = 0;//已记录数据个数
bit Data_Updata_Flag = 0;//数据更新标志

pdata char Collect_Time = 2;//采集时间参数
pdata unsigned char Distance_Param = 20;//距离参数
pdata unsigned char Temp_Collect_Time = 2;//采集时间临时缓存(参数调整时用)
pdata unsigned char Temp_Distance_Param = 20;//距离参数临时缓存
bit Trigger_Flag = 0;//触发模式采集标志（光敏电阻触发）
idata unsigned char Timer_Count = 0;//定时模式计数
idata unsigned char Light_State = 1;//光敏电阻状态（1-亮，0-暗）
idata unsigned char Last_Light_State = 1;//上一次光敏电阻状态
pdata float AD_Val;
pdata	unsigned long Sum;

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
		case 4://大界面切换按键
			Seg_Disp_Mode ^=1;
			if(Seg_Disp_Mode == 1)//进入参数设置界面下默认当前为采集时间设置。
			{
				Para_Set_Mode = 0;
				Temp_Collect_Time = Collect_Time; //为什么要缓存？？？？？
				Temp_Distance_Param = Distance_Param;				
			}
			else if(Seg_Disp_Mode == 0)//进入数据显示界面 默认当前为时间数据显示
			{
				Display_Mode = 0;
				Collect_Time = Temp_Collect_Time;//能不能理解为控制值与实际值
				Distance_Param = Temp_Distance_Param;
			}
		break;
		case 5://切换数据显示界面
			if(Seg_Disp_Mode == 0)
			{
				if(++Display_Mode == 3)
					Display_Mode = 0;
				if(Display_Mode == 2)//进入数据记录界面默认显示最大值
					Data_Display_Mode = 0;
			}
			else if(Seg_Disp_Mode == 1)//参数设置界面下切换参数
			{
				Para_Set_Mode ^=1;
			}
		break;
		case 8://“模式”按键
			if(Display_Mode == 1)//在测距数据显示界面下，按下S8按键，切换触发和定时模式
			{
				Distance_Data_Mode ^=1;
			}
			else if(Display_Mode == 2)//在数据记录显示界面下，按下S8按键，按照最大值、最小值、平均值
			{
				if(++Data_Display_Mode == 3)
				{
					Data_Display_Mode = 0;
				}
			}
		break;
	}
}




/*信息处理函数*/
void Seg_Proc()
{
	if(Seg_Slow_Down < 100) return;
	Seg_Slow_Down = 0;//数码管减速程序
	
	Read_Rtc(Rtc_Timer);
		
	switch(Seg_Disp_Mode)
	{
		case 0://数据显示界面
			if(Display_Mode == 0)//时间数据显示界面 - 1111 1101 1011 1111 0xbf
			{
				Seg_Buf[0] = Rtc_Timer[0] / 10 % 10;
				Seg_Buf[1] = Rtc_Timer[0] % 10;
				Seg_Buf[2] = 11;
				Seg_Buf[3] = Rtc_Timer[1] / 10 % 10;
				Seg_Buf[4] = Rtc_Timer[1] % 10;
				Seg_Buf[5] = 11;
				Seg_Buf[6] = Rtc_Timer[2] / 10 % 10;
				Seg_Buf[7] = Rtc_Timer[2] % 10;	
			}
			if(Display_Mode == 1)//距离数据显示界面
			{
				Seg_Buf[0] = 12;//L  0xc7
				if(Distance_Data_Mode == 0)//触发模式 C 0110 0011 1100 0110 0xc6
				{
					Seg_Buf[1] = 13;//C
					Seg_Buf[2] = Seg_Buf[3] = Seg_Buf[4] = 10;
					Seg_Buf[5] = Distance / 100 % 10;
					Seg_Buf[6] = Distance / 10 % 10;
					Seg_Buf[7] = Distance % 10;
					for(i=5;i<7 && Seg_Buf[i]==0;i++)
						Seg_Buf[i] = 10;
				}
				else if(Distance_Data_Mode == 1)//定时模式
				{
					Seg_Buf[1] = 14;//F 0111 0001 1000 1110 0x8e
					Seg_Buf[2] = Seg_Buf[3] = Seg_Buf[4] = 10;
					Seg_Buf[5] = Distance / 100 % 10;
					Seg_Buf[6] = Distance / 10 % 10;
					Seg_Buf[7] = Distance % 10;
					for(i=5;i<7 && Seg_Buf[i]==0;i++)
						Seg_Buf[i] = 10;
				}
			}
			if(Display_Mode == 2)//数据记录显示界面
			{
				Seg_Buf[0] = 15;//H  0x89
				if(Data_Display_Mode == 0)//最大值
				{
					Seg_Buf[1] = 16;//  0xfe
					Seg_Buf[2] = Seg_Buf[3] = 10;
					Seg_Buf[4] = (Distance_Max /10) / 1000 % 10;
					Seg_Buf[5] = (Distance_Max /10) / 100 % 10;
					Seg_Buf[6] = (Distance_Max /10) / 10 % 10;
					Seg_Buf[7] = (Distance_Max /10) % 10;	
					for(i=4;i<7 && Seg_Buf[i]== 0;i++)
						Seg_Buf[i] = 10;
 				}
				if(Data_Display_Mode == 1)//平均值
				{
					Seg_Buf[1] = 11;//  -
					Seg_Buf[2] = Seg_Buf[3] = 10;
					Seg_Buf[4] = Distance_Avg / 1000 % 10;
					Seg_Buf[5] = Distance_Avg / 100 % 10;
					Seg_Buf[6] = Distance_Avg / 10 % 10 + '.';
					Seg_Buf[7] = Distance_Avg % 10;		
					for(i=4;i<7 && Seg_Buf[i]== 0;i++)
						Seg_Buf[i] = 10;					
 				}
				if(Data_Display_Mode == 2)//最小值 
				{
					Seg_Buf[1] = 17;//  _  0xf7
					Seg_Buf[2] = Seg_Buf[3] = 10;
					Seg_Buf[4] = (Distance_Min/10) / 1000 % 10;
					Seg_Buf[5] = (Distance_Min/10) / 100 % 10;
					Seg_Buf[6] = (Distance_Min/10) / 10 % 10;
					Seg_Buf[7] = (Distance_Min/10) % 10;		
					for(i=4;i<7 && Seg_Buf[i]== 0;i++)
						Seg_Buf[i] = 10;					
 				}				
			}
		break;
		case 1://参数设置界面
			Seg_Buf[0] = 18;//P 0x8c
		break;
	}
	

}



/*其他显示函数*/
void Led_Proc()
{
	Led_Disp(ucLed);
	
	/*ADDA*/
	AD_Val = Ad_Read(0x41) / 51.0;//读取光敏电阻
	Last_Light_State = Light_State;
	Light_State = (AD_Val > 1.2) ? 1 : 0;
	if(Last_Light_State == 1 && Light_State)
		Trigger_Flag = 1;
	
	
	/*计算最大最小平均值*/
	if(!Data_Updata_Flag) return;//无新数据则不计算
		Data_Updata_Flag  = 0;
	Distance_Max = 0;
	Distance_Min = 0xFFFF;
	for(i=0;i<Record_Count;i++)
	{
		//计算最大值
		if(Distance_Record[i]>Distance_Max)
			Distance_Max = Distance_Record[i];
		//计算最小值
		if(Distance_Record[i] < Distance_Min)
			Distance_Min = Distance_Record[i];
		//计算总和
		Sum += Distance_Record[i];
	}
	//计算平均值
	if(Record_Count > 0)
		Distance_Avg = (Sum * 10 + Record_Count / 2) / Record_Count;
	
}

/*超声波处理函数*/
void Distance_Proc()
{

	Distance = Ut_Wave_Data();
	
	if(Distance_Data_Mode == 0)//触发模式：光线暗触发一次采集
	{
		if(Trigger_Flag)
		{
			Distance = Ut_Wave_Data();//测量距离
			//记录数据(扩大10倍，方便平均值计算)
			if(Record_Count < MAX_RECORD)
			{
				Distance_Record[Record_Count++] = Distance * 10;
				Data_Updata_Flag = 1;
			}		
			Trigger_Flag = 0;//清除触发标志
		}
	}
	else if(Distance_Data_Mode == 1)//定时模式:秒值整除采集时间触发
	{
		Timer_Count++;
		if(Timer_Count >= Collect_Time * 1000)//1ms中断1000次=1s
		{
			Timer_Count = 0;
			Distance = Ut_Wave_Data();//测量距离
			
			//记录数据
			if(Record_Count < MAX_RECORD)
			{
				Distance_Record[Record_Count++] = Distance * 10;
				Data_Updata_Flag = 1;//置位更新标志
			}
		}
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
	Set_Rtc(Rtc_Timer);
	
	
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		Distance_Proc();
	}
}
