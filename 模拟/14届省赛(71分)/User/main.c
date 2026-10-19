/*头文件声明区*/
#include <STC15F2K60S2.H>
#include "Key.h"
#include "Seg.h"
#include "Led.h"
#include "Init.h"
#include "Uart.h"
#include "ultrasound.h"
#include "ds1302.h"  
#include "onewire.h"
#include "iic.h"

/*变量声明区*/
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量
unsigned char Key_Slow_Down;//按键减速专用变量
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Point[8]={0,0,0,0,0,0,0,0};//数码管小数点数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char ucLed[8]={0,0,0,0,0,0,0};//Led显示数据存放数组


unsigned char Seg_Disp_Mode ;//数码管显示变量 0-时间界面 1-回显界面 2-参数界面 3-温湿度界面 
unsigned char Rtc_Timer[] = {13,03,05};//实时时间变量
unsigned char Echo_Disp_Mode = 0;//回显界面子界面 0-温度回显 1-湿度回显 2-时间回显

/*温度相关*/
unsigned char Temperature;      //温度变量
unsigned char Temperature_Para = 30;//温度参数
unsigned char Temperature_Max = 0 ; //温度最大值
unsigned int Temperature_Avg = 0;  //温度平均值
float Temperature_Sum = 0;  // 温度总和（用于计算平均）

/*湿度相关*/
float Humidity;           //湿度值
float Humidity_Max = 0;        //最高湿度
float Humidity_Avg = 0;            //湿度平均值
float Humidity_Sum = 0;     // 湿度总和（用于计算平均）
bit Humidity_Error;//湿度错误标志

// 频率测量变量
unsigned int Timer_1s;         //定时器0计时1秒变量
unsigned int Freq;             //NE555频率测量值
unsigned int Timer_2s;//两秒计时变量s9长按检测


/*LED闪烁*/
unsigned char Timer_100ms;//100毫秒计时变量
bit L4_LED_Flag = 0;//led4闪烁标志位


bit Light_State = 0;        // 光照状态：1=亮，0=暗
bit Last_Light_State = 0;   // 上一次光照状态
unsigned char Ad_Val;//阈值变量

/*采集相关变量*/
unsigned int Collect_Flag_Timer;//采集标志位
unsigned char Trigger_Time[] = {0,0}; // 最近触发时间（时、分，BCD码）
unsigned char Trigger_Count = 0; // 触发次数
bit Collect_Flag = 0;       // 采集触发标志

// 历史数据变量
float Last_Temp = 0;        // 上一次采集温度
float Last_Hum = 0;         // 上一次采集湿度

/*键盘处理函数*/
void Key_Proc()
{
	if(Key_Slow_Down < 10)return;
	Key_Slow_Down=0;//键盘减速程序
	
	Key_Val =Key_Read();//实时读取键码值
	Key_Down = Key_Val & (Key_Old ^ Key_Val);//判断按键是否按下
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);//捕捉按键上升沿
	Key_Old = Key_Val;//辅助扫描变量

	// 温湿度界面期间所有按键无效
//	if (Seg_Disp_Mode == 3) return;
	
	switch(Key_Down)
	{
		case 4:
			if(++Seg_Disp_Mode==4)
				Seg_Disp_Mode = 0;
			Echo_Disp_Mode = 0;
			
		break;
		case 5:
			if(Seg_Disp_Mode == 1)//在回显界面下
			{
				if(++Echo_Disp_Mode==3)
					Echo_Disp_Mode = 0;
			}
		break;
		case 8://+
			if(Seg_Disp_Mode == 2)//在参数界面下
			{
				Temperature_Para = (Temperature_Para == 99) ? 99 : Temperature_Para+1;
			}				
		break;
		case 9://-
			if(Seg_Disp_Mode == 2)//在参数界面下
			{
				Temperature_Para = (Temperature_Para == 0) ? 0 : Temperature_Para-1;
			}
			if(Seg_Disp_Mode == 1)//时间回显界面下
			{
				//长按2s清空数据
			}
		break;		
	}
	
	
}

/*数据采集函数（亮→暗触发）*/
void Data_Collect() 
{
    if (Collect_Flag_Timer < 2999) return; // 3秒内禁止重复触发

    // 检测光照状态变化（亮→暗）
    if(Light_State == 1)
		{
			return;
		}
		if(Last_Light_State == 1 && Light_State == 0) 
		{
			Collect_Flag = 1;
			Trigger_Count++; // 触发次数+1
			if (Trigger_Count > 99) Trigger_Count = 99; // 最大99次

			// 读取当前时间作为触发时间
			Read_Rtc(Rtc_Timer);
			Trigger_Time[0] = Rtc_Timer[0]; // 时（BCD码）
			Trigger_Time[1] = Rtc_Timer[1]; // 分（BCD码）

			// 读取温度（DS18B20）
			Temperature = rd_temperature();
			if(Temperature < 0) Temperature = 0; // 题目要求不考虑负温度
			if(Temperature > 99) Temperature = 99; // 限制显示范围

			// 读取湿度（NE555频率转换）
			if(Freq >= 200 && Freq <= 2000)// 有效频率范围 
				Humidity = (Freq - 200) * 0.0444 + 10; // 200Hz=10%，2000Hz=90%线性转换
			else
				Humidity = -1; // 标记无效湿度
			// 数据统计（仅有效数据）
			if (Humidity != -1) {
					// 温度统计
					if (Temperature > Temperature_Max || Trigger_Count == 1) {
							Temperature_Max = Temperature;
					}
					Temperature_Sum += Temperature;

					// 湿度统计
					if (Humidity > Humidity_Max || Trigger_Count == 1) {
							Humidity_Max = Humidity;
					}
					Humidity_Sum += Humidity;

					// 保存本次数据作为历史数据
					Last_Temp = Temperature;
					Last_Hum = Humidity;
			}

			// 切换到温湿度界面，3秒后返回原界面
			Seg_Disp_Mode = 3;
			Collect_Flag_Timer = 3000; // 3秒倒计时（1ms计数）
	}
	
    Last_Light_State = Light_State;
}        

/*信息处理函数*/
void Seg_Proc()
{
	if(Seg_Slow_Down < 80) return;
	Seg_Slow_Down = 0;//数码管减速程序
	Read_Rtc(Rtc_Timer);	
	
	Temperature = rd_temperature();
	
	
	if(Humidity_Max < Humidity)
	{
		Humidity_Max = Humidity;
	}
	if(Temperature_Max < Temperature)
	{
		Temperature_Max = Temperature;
	}

	
	
	switch(Seg_Disp_Mode)
	{
		case 0://时间界面 - 1111 1101 1011 1111 0xbf
			Seg_Buf[0] = Rtc_Timer[0] / 10 ;
			Seg_Buf[1] = Rtc_Timer[0] % 10;
			Seg_Buf[2] = 11;//-
			Seg_Buf[3] = Rtc_Timer[1] / 10;
			Seg_Buf[4] = Rtc_Timer[1] % 10;
			Seg_Buf[5] = 11;//-
			Seg_Buf[6] = Rtc_Timer[2] / 10 % 10;
			Seg_Buf[7] = Rtc_Timer[2] % 10;
		
		break;
		case 1://回显界面
			switch(Echo_Disp_Mode)
			{
				case 0://温度回显 C 0110 0011 1100 0110 0xc6
					Seg_Buf[0] = 12;//C
					Seg_Buf[1] = 10;//熄灭
//				if(Trigger_Count > 0) //如果有触发
//				{
					Seg_Buf[2] = Temperature_Max / 10 % 10;
					Seg_Buf[3] = Temperature_Max % 10;
					Seg_Buf[4] = 11;//-
					Seg_Buf[5] = (unsigned char)Temperature_Avg / 10 % 10;
					Seg_Buf[6] = (unsigned char)Temperature_Avg % 10 + '.';		
					Seg_Buf[7] = (unsigned int)(Temperature_Avg * 10) % 10;		
//				}
//				else
//				{
//					Seg_Buf[2] = 10;
//					Seg_Buf[3] = 10;
//					Seg_Buf[4] = 10;//-
//					Seg_Buf[5] = 10;
//					Seg_Buf[6] = 10;
//					Seg_Buf[7] = 10;
//				}
				break;
				case 1://湿度回显 H 1001 0001 10001001 0x89
//					if(Trigger_Count > 0 && Humidity != -1)
//					{
						Seg_Buf[0] = 13;//H
						Seg_Buf[1] = 10;
						Seg_Buf[2] = (unsigned char)Humidity_Max / 10 % 10;
						Seg_Buf[3] = (unsigned char)Humidity_Max % 10;	
						//Humidity_Avg = Humidity_Sum / Trigger_Count;
						Seg_Buf[5] = (unsigned char)Humidity_Avg / 10;
						Seg_Buf[6] = (unsigned char)Humidity_Avg % 10 + '.';
						Seg_Buf[7] = (unsigned int)(Humidity_Avg * 10) % 10;				
//					}						
				break;
				case 2://时间回显 F 0111 0001 1000 1110 0x8e
					Seg_Buf[0] = 17;//F
				break;
			}
		break;
		case 2://参数界面 P 0011 0001 1000 1100   0x8c
			Seg_Buf[0] = 15;//P
			Seg_Buf[1] = 10;
		  Seg_Buf[2] = 10;
		  Seg_Buf[3] = 10;
		  Seg_Buf[4] = 10;
		  Seg_Buf[5] = 10;
		  Seg_Buf[6] = Temperature_Para / 10 % 10;
		  Seg_Buf[7] = Temperature_Para % 10;
		
		break;
		case 3://温湿度界面   E 0110 0001 1000 0110 0x86
			Seg_Buf[0] = 16;//E
			Seg_Buf[1] = 10;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = Temperature / 10 % 10;
			Seg_Buf[4] = Temperature % 10;
			Seg_Buf[5] = 11;
		if(Humidity_Error == 1)
		{
			Seg_Buf[6] = 14;
			Seg_Buf[7] = 14;
		}
		else
		{
			Seg_Buf[6] = (unsigned char)Humidity / 10 % 10;
			Seg_Buf[7] = (unsigned char)Humidity % 10;
		}
		break;
	}

}



/*其他显示函数*/
void Led_Proc()
{
	ucLed[0] = (Seg_Disp_Mode == 0);
	ucLed[1] = (Seg_Disp_Mode == 1);
	ucLed[2] = (Seg_Disp_Mode == 3);
	ucLed[3] = (L4_LED_Flag == 1);
	Led_Disp(ucLed);
	
	
	/*阈值判断*/
	Ad_Val = Ad_Read(0x43);
	if(Ad_Val > 120) 
		Light_State = 1;
	else 
		Light_State = 0;
 
	
	/*湿度相关*/
	if(Freq < 200)
		Humidity_Error = 1;//湿度为10
	else if(Freq > 2000)
		Humidity_Error = 1;
	else 
	{
		Humidity_Error = 0;
		Humidity = 0.044 * Freq + 1.2;
	}

	
	
	if((Temperature > Temperature_Para) && (++Timer_100ms == 200))//采集温度>温度参数
	{
		Timer_100ms = 0;
		L4_LED_Flag = 1;
	}
	else
	{
		L4_LED_Flag = 0;
	}
}

/*定时器0*/
void Timer0_Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0x7F;			//定时器时钟12T模式
	TMOD &= 0xF0;			//设置定时器模式
	TMOD |= 0x05;			//设置计数器模式
	TL0 = 0x00;				//设置定时初始值
	TH0 = 0x00;				//设置定时初始值
	TF0 = 0;				//清除TF0标志
	TR0 = 1;				//定时器0开始计时
	ET0 = 1;
	EA = 1;
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


/*定时器1中断服务函数*/
void Timer1_Isr(void) interrupt 3
{
	Key_Slow_Down++;
	Seg_Slow_Down++;

	
	
	Collect_Flag_Timer++;
	if(++Seg_Pos == 8) Seg_Pos = 0;//数码管显示专用
	if(Seg_Buf[Seg_Pos]>='.')
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos] - '.',1);
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
	if(++Timer_1s == 1000)
	{
		Timer_1s = 0;
		Freq = (TH0 << 8) | TL0;
		TL0 = TH0 = 0;	
	}
}
/*750ms延时*/
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

/* Main */
void main()
{

	Sys_Init();
	Timer1_Init();
	Timer0_Init();
	Set_Rtc(Rtc_Timer);	
	rd_temperature();
	Delay750ms();

	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();

	}
}
