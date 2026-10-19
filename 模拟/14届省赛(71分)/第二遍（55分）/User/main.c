/*头文件声明区*/
#include <STC15F2K60S2.H>
#include "Key.h"
#include "Seg.h"
#include "Led.h"
#include "Init.h"
#include "onewire.h"
#include "ds1302.h"
#include "iic.h"


/*变量声明区*/
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量
unsigned char Key_Slow_Down;//按键减速专用变量
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char ucLed[8]={0,0,0,0,0,0,0};//Led显示数据存放数组
unsigned char AD_SLow_Down;
unsigned char Seg_Disp_Mode;//0-时间界面 1-回显界面 2-参数界面
unsigned char	Echo_Mode;//回显界面 0-温度回显 1-湿度回显 2-时间回显
unsigned char RTC_Timer[] = {13,03,05};

float Temperature;//温度变量
unsigned char Temperature_Para = 30;//温度参数变量
unsigned char Temperature_Max;
float Temperature_Sum = 0;//温度累加
float Temperature_Avg = 0;//平均温度

unsigned char Humidity_Max = 0;     // 最高湿度
float  Humidity_Sum = 0;     // 湿度累加
float Humidity_Avg = 0;     // 平均湿度

unsigned char Collect_Count = 0;    // 采集次数
unsigned char Echo_Hour = 0;        // 触发时的时
unsigned char Echo_Min  = 0;        // 触发时的分

float Humidity;//湿度变量
unsigned int Freq;
unsigned int Time_1s;
bit Invalid_humidity;//无效湿度标志位
float AD_Value;//AD值
bit Value_Collect_Flag = 0;//数据采集标志位
unsigned int Time_3s;

unsigned char Last_Light_State = 0;//0-亮 1-暗
unsigned char Seg_Disp_Mode_Backup = 0;//触发采集前的界面备份
unsigned char Collect_Slow_Down;//采集减速专用变量
unsigned int  S9_Press_Time = 0;//S9长按计时
unsigned char Time_100ms;//0.1秒

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
			if(++Seg_Disp_Mode == 3)Seg_Disp_Mode = 0;
			Echo_Mode = 0;
		break;
		case 5:
			if(++Echo_Mode == 3)Echo_Mode = 0;
		break;
		case 8://加
			if(Seg_Disp_Mode == 2)//参数界面
			{
				Temperature_Para = (Temperature_Para == 99) ? 99 :Temperature_Para + 1;
			}
		break;
		case 9://减 按下瞬间
			if(Seg_Disp_Mode == 2)//参数界面
			{
				Temperature_Para = (Temperature_Para == 0) ? 0 :Temperature_Para - 1;
			}
			S9_Press_Time = 0;//按下时清零开始计时
		break;
	}
	//S9长按计时：Key_Old里S9对应位还是1说明还在按着
	if((Key_Old & 9) == 9 && Seg_Disp_Mode == 1 && Echo_Mode == 2)
		S9_Press_Time++;

	//S9松开时判断是否超过2秒
	switch(Key_Up)
	{
		case 9:
			if(Seg_Disp_Mode == 1 && Echo_Mode == 2 && S9_Press_Time >= 200)
			{
				Collect_Count    = 0;
				Temperature_Max  = 0;
				Temperature_Sum  = 0;
				Temperature_Avg  = 0;
				Humidity_Max     = 0;
				Humidity_Sum     = 0;
				Humidity_Avg     = 0;
				Echo_Hour        = 0;
				Echo_Min         = 0;
			}
			S9_Press_Time = 0;
		break;
	}
}




/*信息处理函数*/
void Seg_Proc()
{
	if(Seg_Slow_Down < 120) return;
	Seg_Slow_Down = 0;//数码管减速程序

	Read_Rtc(RTC_Timer);

	//温湿度界面优先显示
	if(Value_Collect_Flag == 1)
	{
		Seg_Buf[0] = 16;//E
		Seg_Buf[1] = 10;
		Seg_Buf[2] = 10;
		Seg_Buf[3] = (unsigned char)Temperature / 10 % 10;
		Seg_Buf[4] = (unsigned char)Temperature % 10;
		Seg_Buf[5] = 11;//-
		Seg_Buf[6] = (Invalid_humidity == 0) ? (unsigned int)Humidity / 10 % 10 : 17;
		Seg_Buf[7] = (Invalid_humidity == 0) ? (unsigned int)Humidity % 10 : 17;
		return;//直接返回，不走下面switch
	}

	switch(Seg_Disp_Mode)
	{
		case 0://时间界面 -
			Seg_Buf[2] = Seg_Buf[5] = 11;//-
			Seg_Buf[0] = RTC_Timer[0] / 10 % 10;
			Seg_Buf[1] = RTC_Timer[0] % 10;
			Seg_Buf[3] = RTC_Timer[1] / 10 % 10;
			Seg_Buf[4] = RTC_Timer[1] % 10;
			Seg_Buf[6] = RTC_Timer[2] / 10 % 10;
			Seg_Buf[7] = RTC_Timer[2] % 10;
		break;
		case 1://回显界面
			switch(Echo_Mode)
			{
				case 0://温度回显 C
					Seg_Buf[0] = 12;//C
					Seg_Buf[1] = 10;
					Seg_Buf[2] = (Collect_Count != 0)?Temperature_Max / 10 % 10: 10;
					Seg_Buf[3] = (Collect_Count != 0)?Temperature_Max % 10 : 10;
					Seg_Buf[4] = (Collect_Count != 0)?11:10;//-
					Seg_Buf[5] = (Collect_Count != 0)?(unsigned int)(Temperature_Avg * 10) / 100 % 10 : 10;
					Seg_Buf[6] = (Collect_Count != 0)?(unsigned int)(Temperature_Avg * 10) / 10 % 10 + '.' : 10;
					Seg_Buf[7] = (Collect_Count != 0)?(unsigned int)(Temperature_Avg * 10) % 10 : 10;
				break;                           
				case 1://湿度回显 H              
					Seg_Buf[0] = 13;//H            
					Seg_Buf[1] = 10;               
					Seg_Buf[2] = (Collect_Count != 0)?Humidity_Max / 10 % 10 : 10;
					Seg_Buf[3] = (Collect_Count != 0)?Humidity_Max % 10 : 10;
					Seg_Buf[4] = (Collect_Count != 0)?11 : 10;//-
					Seg_Buf[5] = (Collect_Count != 0)?(unsigned int)(Humidity_Avg * 10) / 100 % 10 : 10;
					Seg_Buf[6] = (Collect_Count != 0)?(unsigned int)(Humidity_Avg * 10) / 10 % 10 + '.': 10;
					Seg_Buf[7] = (Collect_Count != 0)?(unsigned int)(Humidity_Avg * 10) % 10 : 10;
				break;
				case 2://时间回显 F
					Seg_Buf[0] = 14;//F
				Seg_Buf[1] = Collect_Count / 10 % 10;
				Seg_Buf[2] = Collect_Count % 10;
				Seg_Buf[3] = (Collect_Count == 1)?Echo_Hour / 10 % 10 : 10;
				Seg_Buf[4] = (Collect_Count == 1)?Echo_Hour % 10 : 10;
				Seg_Buf[5] = (Collect_Count == 1)?11: 10;//-
				Seg_Buf[6] = (Collect_Count == 1)?Echo_Min / 10 % 10 : 10;
				Seg_Buf[7] = (Collect_Count == 1)?Echo_Min % 10 : 10;
				break;
			}
		break;
		case 2://参数界面 15 P
				Seg_Buf[0] = 15;//P
				Seg_Buf[1] = 10;
				Seg_Buf[2] = 10;
				Seg_Buf[3] = 10;
				Seg_Buf[4] = 10;
				Seg_Buf[5] = 10;
				Seg_Buf[6] = Temperature_Para / 10 % 10;
				Seg_Buf[7] = Temperature_Para % 10;
		break;
	}
}

/*温湿度采集*/
void Collect_Proc(void)
{
	unsigned char Current_Light_State;
	if(Collect_Slow_Down < 50) return;//50ms检测一次
	Collect_Slow_Down = 0;

	AD_Value = Ad_Read(0x41) / 51.0;              //先读AD
	Current_Light_State = (AD_Value < 1.2) ? 1 : 0;//再判断状态

	if(Last_Light_State == 0 && Current_Light_State == 1)//亮->暗跳变
	{
		if(Value_Collect_Flag == 0)//3秒锁定期间不重复触发
		{
			Seg_Disp_Mode_Backup = Seg_Disp_Mode;//备份当前界面
			Temperature = rd_temperature();       //立刻采集温度
			Echo_Hour = RTC_Timer[0];             //记录触发时的时
			Echo_Min  = RTC_Timer[1];             //记录触发时的分

			//更新最大值、累加、平均
			Collect_Count++;
			if(Temperature > Temperature_Max) Temperature_Max = Temperature;
			Temperature_Sum += Temperature;
			Temperature_Avg = Temperature_Sum / Collect_Count;

			if((unsigned char)Humidity > Humidity_Max) Humidity_Max = (unsigned char)Humidity;
			Humidity_Sum += (unsigned char)Humidity;
			Humidity_Avg = Humidity_Sum / Collect_Count;

			Time_3s = 0;
			Value_Collect_Flag = 1;
		}
	}
	Last_Light_State = Current_Light_State;
}
/*其他显示函数*/
void Led_Proc()
{
	ucLed[0] = (Seg_Disp_Mode == 0);
	ucLed[1] = (Seg_Disp_Mode == 1);	
	ucLed[2] = (Value_Collect_Flag == 1);
	ucLed[4] = (Invalid_humidity == 1) ? 0 : 1;
//	if(Collect_Count >= 2)
//	{
//		if()
//	}
	Led_Disp(ucLed);
	
}

/*AD相关*/
void AD_Proc(void)
{
	if(AD_SLow_Down < 50)return;
		AD_SLow_Down = 0;
	
	
		/*湿度采集*/
		if(Freq >= 200 && Freq <= 2000)
		{
			Humidity = 0.044*Freq + 1.2;
			Invalid_humidity = 0;
		}
		else if(Freq > 2000 || Freq < 200)//采集到无效数据
			Invalid_humidity = 1;
		/*温度采集*/
		Temperature = rd_temperature();
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
	AD_SLow_Down++;
	Collect_Slow_Down++;
	if(++Seg_Pos == 8) Seg_Pos = 0;//数码管显示专用
	if(Seg_Buf[Seg_Pos]>='.')
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos] - '.',1);
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
	
	if(++Time_1s == 1000)
	{
		Time_1s = 0;
		Freq = (TH0 << 8) + TL0;
		TH0=TL0 = 0;
	}
	if(Value_Collect_Flag == 1)
	{
		if(++Time_3s == 3000)
		{
			Time_3s = 0;
			Value_Collect_Flag = 0;
			Seg_Disp_Mode = Seg_Disp_Mode_Backup;//恢复触发前的界面
		}
	}
	if(++Time_100ms == 100)
	{
		Time_100ms = 0;
		if(Temperature > Temperature_Para)
		{
			ucLed[3] = 1;
		}
	}
	else
	{
		ucLed[3] = 0;	
	}
}

/* Main */
void main()
{
	Sys_Init();
	Timer1_Init();
	Timer0_Init();
	Set_Rtc(RTC_Timer);

	//上电同步光照初始状态，防止虚假触发
	AD_Value = Ad_Read(0x43) / 51.0;
	Last_Light_State = (AD_Value < 1.2) ? 1 : 0;
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		AD_Proc();
		Collect_Proc();
	}
}



