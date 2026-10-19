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
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char ucLed[8]={0,0,0,0,0,0,0};//Led显示数据存放数组
unsigned char Distance_Slow_Down;
unsigned char i;
unsigned char AD_Slow_Down;


unsigned char Seg_Disp_Mode;// 0-数据显示界面 1-参数设置界面
unsigned char Seg_Data_Mode;//0-时间界面 1-距离界面 2-数据记录界面
unsigned char Para_Set_Mode;//0-采集时间参数界面 1-距离参数设置界面
unsigned char Distance_Mode;//测距模式 0-触发模式 1-定时模式
unsigned char RTC_Time[] = {20,20,01};
unsigned int Distance;
unsigned char Data_Record_Mode;//0-最大值 1-平均值 2-最小值
unsigned int Distance_Max = 0;   //距离最大值
unsigned int Distance_Avg = 0;   //距离平均值
unsigned int Distance_Min = 0xffff;//距离最小值
unsigned long Distance_Sum = 0;  //距离累加和
unsigned char Distance_Count = 0;//采集次数
unsigned char AD_Value;
unsigned char Flash_Flag;//光线标志
unsigned char Flash_Flag_Old;//上一次光线
//unsigned char Distance_Collect_Flag;//距离采集标志位
unsigned char Diatance_Para = 20;//距离参数
unsigned char Time_Para = 2;//采集时间参数

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
		case 4:
			if(++Seg_Disp_Mode==2)Seg_Disp_Mode=0;		
		break;
		case 5:
			if(Seg_Disp_Mode == 0)//在数据界面下
			{
				if(++Seg_Data_Mode == 3)Seg_Data_Mode = 0;		
			}
			if(Seg_Disp_Mode == 1)//参数界面下
			{
				if(++Para_Set_Mode == 2)Para_Set_Mode = 0;
			}
		break;
		case 8:
			if(Seg_Data_Mode == 1)//距离数据显示界面
			{
				if(++Distance_Mode == 2)Distance_Mode=0;
			}
			if(Seg_Data_Mode == 2)//数据记录显示界面
			{
				if(++Data_Record_Mode == 3)Data_Record_Mode = 0;
			}
		break;
		case 9:
			//if()
		break;
	}
}




/*信息处理函数*/
void Seg_Proc()
{
	if(Seg_Slow_Down < 100) return;
	Seg_Slow_Down = 0;//数码管减速程序
	
	Read_Rtc(RTC_Time);
	
	switch(Seg_Disp_Mode)
	{
		case 0://数据显示界面
			switch(Seg_Data_Mode)
			{
				case 0://时间数据显示界面
					Seg_Buf[0] = RTC_Time[0] / 10 % 10;
					Seg_Buf[1] = RTC_Time[0] % 10;
					Seg_Buf[2] = 11;//- 
					Seg_Buf[3] = RTC_Time[1] / 10 % 10;
					Seg_Buf[4] = RTC_Time[1] % 10;
					Seg_Buf[5] = 11;//-
					Seg_Buf[6] = RTC_Time[2] / 10 % 10;
					Seg_Buf[7] = RTC_Time[2] % 10;				
				break;
				case 1://距离数据显示界面 L 
					if(Distance_Mode == 0)//触发模式 C 
					{
						Seg_Buf[0] = 12;//L
						Seg_Buf[1] = 13;//C
						Seg_Buf[2] = 10; 
						Seg_Buf[3] = 10;
						Seg_Buf[4] = 10;
						Seg_Buf[5] = Distance / 100 % 10;
						Seg_Buf[6] = Distance / 10 % 10;
						Seg_Buf[7] = Distance % 10;	
						for(i=5;i<7&&Seg_Buf[i]==0;i++)
							Seg_Buf[i] = 0;
					}
					else if(Distance_Mode == 1)//定时模式
					{
						Seg_Buf[0] = 12;//L
						Seg_Buf[1] = 14;//F 
						Seg_Buf[2] = 10; 
						Seg_Buf[3] = 10;
						Seg_Buf[4] = 10;
						Seg_Buf[5] = Distance / 100 % 10;
						Seg_Buf[6] = Distance / 10 % 10;
						Seg_Buf[7] = Distance % 10;	
						for(i=5;i<7&&Seg_Buf[i]==0;i++)
							Seg_Buf[i] = 0;						
					}
				break;
				case 2://数据记录显示界面
					switch(Data_Record_Mode)
					{
						case 0://最大值
							Seg_Buf[0] = 15;//H 
							Seg_Buf[1] = 16;// 
							Seg_Buf[2] = 10;
							Seg_Buf[3] = 10;
							Seg_Buf[4] = 10;
							Seg_Buf[5] = Distance_Max / 100 % 10;
							Seg_Buf[6] = Distance_Max / 10 % 10;
							Seg_Buf[7] = Distance_Max % 10;
						break;
						case 1://平均值
							Seg_Buf[0] = 15;//H 
							Seg_Buf[1] = 17;//- 
							Seg_Buf[2] = 10;
							Seg_Buf[3] = 10;
							Seg_Buf[4] = 10;
							Seg_Buf[5] = Distance_Avg / 100 % 10;
							Seg_Buf[6] = Distance_Avg / 10 % 10;
							Seg_Buf[7] = Distance_Avg % 10;							
						break;
						case 2://最小值
							Seg_Buf[0] = 15;//H 
							Seg_Buf[1] = 18;//_
							Seg_Buf[2] = 10;
							Seg_Buf[3] = 10;
							Seg_Buf[4] = 10;
							Seg_Buf[5] = Distance_Min / 100 % 10;
							Seg_Buf[6] = Distance_Min / 10 % 10;
							Seg_Buf[7] = Distance_Min % 10;							
						break;
					}
				break;
			}
		break;
		case 1://参数设置界面
			Seg_Buf[0] = 19;//P
			Seg_Buf[1] = Para_Set_Mode + 1;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			Seg_Buf[5] = 10;

			switch(Para_Set_Mode)
			{
				case 0://采集时间设置界面
					Seg_Buf[6] = Time_Para / 10 % 10;
					Seg_Buf[7] = Time_Para % 10;					
				break;
				case 1://距离参数设置界面
					Seg_Buf[6] = Diatance_Para / 10 % 10;
					Seg_Buf[7] = Diatance_Para % 10;					
				break;
			}

			
		break;
	}
	

}

void Distance_Proc(void)
{
	if(++Distance_Slow_Down < 40)return;
		Distance_Slow_Down = 0;
	
	if(Distance_Mode == 0)//触发模式
	{
		if(Flash_Flag == 1)//触发模式距离采集
		{
			Flash_Flag = 0;  // 立即清除，确保只采集一次
			Distance = Ut_Wave_Data();

			if(Distance_Max < Distance)
				Distance_Max = Distance;

			if(Distance_Min > Distance)
				Distance_Min = Distance;

			// 累加计算平均值
			Distance_Sum += Distance;
			Distance_Count++;
			Distance_Avg = Distance_Sum / Distance_Count;
		}
	}
	else if(Distance_Mode == 1)//定时模式
	{
		if((RTC_Time[2] / Time_Para) % Time_Para == 0)//如果能整除
		{
			Flash_Flag = 0;  // 立即清除，确保只采集一次
			Distance = Ut_Wave_Data();

			if(Distance_Max < Distance)
				Distance_Max = Distance;

			if(Distance_Min > Distance)
				Distance_Min = Distance;

			// 累加计算平均值
			Distance_Sum += Distance;
			Distance_Count++;
			Distance_Avg = Distance_Sum / Distance_Count;		
		}
	}
}


/*其他显示函数*/
void Led_Proc()
{
	Led_Disp(ucLed);
	
}

void AD_Proc(void)
{
	if(++AD_Slow_Down < 60)return;
		AD_Slow_Down = 0;
	AD_Value = Ad_Read(0x43) / 51.0f;

	if(AD_Value < 1.2)
	{
		Flash_Flag = 0;
		Flash_Flag_Old = 0;  // 光线消失，复位旧状态，允许下次触发
	}
	else if(AD_Value >= 1.2 && Flash_Flag_Old == 0)
	{
		Flash_Flag = 1;      // 上升沿：只触发一次
		Flash_Flag_Old = 1;  // 锁住，防止重复触发
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
	Distance_Slow_Down++;
	AD_Slow_Down++;
	
	
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
	Set_Rtc(RTC_Time);
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		AD_Proc();
		Distance_Proc();
	}
}



