/*头文件声明区*/
#include <STC15F2K60S2.H>
#include "Key.h"
#include "Seg.h"
#include "Led.h"
#include "Init.h"
#include "ds1302.h"
#include "iic.h"
#include "onewire.h"

/*变量声明区*/
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量
unsigned char Key_Slow_Down;//按键减速专用变量
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char ucLed[8]={0,0,0,0,0,0,0};//Led显示数据存放数组
unsigned char AD_Slow_Down;
unsigned char Tempera_Slow_Down;

unsigned char Seg_Disp_Mode;//0-时间界面 1-回显界面 2-参数界面 3-温湿度界面
unsigned char Echo_Disp_Mode;//0-温度回显 1-湿度回显 2-时间回显
unsigned char RTC_Timer[] = {13,03,05};
unsigned char Echo_Hour = 0;
unsigned char Echo_Min = 0;
unsigned int Freq;
unsigned int Time_1s;
bit Error_Value = 0;//无效值

unsigned char Hum_Value;//湿度
unsigned char Hum_Max;//最大湿度值
unsigned int Hum_Avg;//湿度平均值

unsigned char Temperature;
unsigned int Tempera_Avg;//温度平均值
unsigned char Tempera_Max;//最大温度
unsigned char Temperature_Para = 30;//温度参数

unsigned char Value_Collect_Flag;//采集标志位

unsigned char AD_Value;
static unsigned int State_Old;
unsigned int State_Current;
unsigned int Record_Count;
unsigned int Collect_Count;
unsigned char Lock = 0;
unsigned int Time_3s;
unsigned char Seg_Disp_Temp;
unsigned char Seg_Disp_Mode_Backup;
bit Long_Press;//长按标志
unsigned int Timer_2s;
unsigned char Timer_L4;
bit L6_Flag;

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
			if(++Seg_Disp_Mode == 3)
				Seg_Disp_Mode = 0;
			Echo_Disp_Mode = 0;
		break;
		case 5:
			if(++Echo_Disp_Mode == 3)
				Echo_Disp_Mode = 0;
		break;
		case 8://加
			if(Seg_Disp_Mode == 2)
			{
				Temperature_Para = (Temperature_Para == 99)?99:Temperature_Para+1;
			}
		break;
		case 9://减
			if(Seg_Disp_Mode == 2)
			{
				Temperature_Para = (Temperature_Para == 0)?0:Temperature_Para-1;
			}
		break;
	}
	switch(Key_Val)
	{
		case 9:
		  if(Echo_Disp_Mode == 2)//时间回显子界面下
			{
				Long_Press = 1;
			}
		break;
	}
	if(Key_Up == 9)
	{
		Timer_2s = 0;
		Record_Count = 0;
		Tempera_Max = 0;
		Tempera_Avg = 0;
		Hum_Max = 0;
		Hum_Avg = 0;
		Echo_Hour = 0;
		Echo_Min = 0;
	}
}




/*信息处理函数*/
void Seg_Proc()
{
	if(Seg_Slow_Down < 100) return;
	Seg_Slow_Down = 0;//数码管减速程序

	Read_Rtc(RTC_Timer);
	
	switch(Seg_Disp_Mode)
	{
		case 0://时间界面
			Seg_Buf[0] = RTC_Timer[0] / 10 % 10;
			Seg_Buf[1] = RTC_Timer[0] % 10;
			Seg_Buf[2] = 11;//-
			Seg_Buf[3] = RTC_Timer[1] / 10 % 10;
			Seg_Buf[4] = RTC_Timer[1] % 10;
			Seg_Buf[5] = 11;//-
			Seg_Buf[6] = RTC_Timer[2] / 10 % 10;
			Seg_Buf[7] = RTC_Timer[2] % 10;
		break;
		case 1://回显界面
			switch(Echo_Disp_Mode)
			{
				case 0://温度回显
					Seg_Buf[0] = 12;//C
					Seg_Buf[1] = 10;
					Seg_Buf[2] = (Record_Count==0)?10:Tempera_Max / 10 % 10;
					Seg_Buf[3] = (Record_Count==0)?10:Tempera_Max % 10;
					Seg_Buf[4] = (Record_Count==0)?10:11;//-
					Seg_Buf[5] = (Record_Count==0)?10:(Tempera_Avg*10) / 100 % 10;
					Seg_Buf[6] = (Record_Count==0)?10:(Tempera_Avg*10) / 10 % 10 + '.';
					Seg_Buf[7] = (Record_Count==0)?10:(Tempera_Avg*10) % 10;
				break;
				case 1://湿度回显
					Seg_Buf[0] = 16;//H
					Seg_Buf[1] = 10;
					Seg_Buf[2] = (Record_Count==0)?10:Hum_Max / 10 % 10;
					Seg_Buf[3] = (Record_Count==0)?10:Hum_Max % 10;
					Seg_Buf[4] = (Record_Count==0)?10:11;//-
					Seg_Buf[5] = (Record_Count==0)?10:(Hum_Avg*10) / 100 % 10;
					Seg_Buf[6] = (Record_Count==0)?10:(Hum_Avg*10) / 10 % 10 + '.';
					Seg_Buf[7] = (Record_Count==0)?10:(Hum_Avg*10) % 10;					
				break;
				case 2://时间回显
					Seg_Buf[0] = 17;//F 
					Seg_Buf[1] = Record_Count / 10 % 10;
					Seg_Buf[2] = Record_Count % 10;
					Seg_Buf[3] = (Record_Count==0)?10:Echo_Hour / 10 % 10;
					Seg_Buf[4] = (Record_Count==0)?10:Echo_Hour % 10;
					Seg_Buf[5] = (Record_Count==0)?10:11;//-
					Seg_Buf[6] = (Record_Count==0)?10:Echo_Min / 10 % 10;
					Seg_Buf[7] = (Record_Count==0)?10:Echo_Min % 10;					
				break;				
			}				
		break;
		case 2://参数界面
			Seg_Buf[0] = 15;//P
			Seg_Buf[1] = 10;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			Seg_Buf[5] = 10;
			Seg_Buf[6] = Temperature_Para / 10 % 10;
			Seg_Buf[7] = Temperature_Para % 10;			
		break;
		case 3:
			Seg_Buf[0] = 13;//E
			Seg_Buf[1] = 10;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = Temperature / 10 % 10;
			Seg_Buf[4] = Temperature % 10;
			Seg_Buf[5] = 11;//-
			Seg_Buf[6] = (Error_Value==1)?14:Hum_Value / 10 % 10;
			Seg_Buf[7] = (Error_Value==1)?14:Hum_Value % 10;			
		break;
	}

}



/*其他显示函数*/
void Led_Proc()
{
	ucLed[0] = (Seg_Disp_Mode == 0);
	ucLed[1] = (Seg_Disp_Mode == 1);
	ucLed[2] = (Seg_Disp_Mode == 3);
	if(Temperature > Temperature_Para)
	{
		ucLed[3] = (Timer_L4 < 100) ? 1 : 0;
	}
	if(Error_Value == 1)
		ucLed[4] = 1;
	else 
		ucLed[4] = 0;
	if(L6_Flag == 1)
		ucLed[5] = 1;
	else
		ucLed[5] = 0;
		
	Led_Disp(ucLed);

}
void AD_Proc()
{
	if(AD_Slow_Down < 120)return;
	AD_Slow_Down = 0;

	AD_Value = Ad_Read(0x41) / 51.0 * 100;

	if(AD_Value < 100)
		State_Current = 1;
	else if(AD_Value > 140)
		State_Current = 0;

	if(State_Current == 1 && State_Old == 0 && Lock == 0)
	{
		Seg_Disp_Mode_Backup = Seg_Disp_Mode;
		Seg_Disp_Mode = 3;
		Value_Collect_Flag = 1;
		Lock = 1;
		Record_Count++;
	}

	State_Old = State_Current;
}
/*温湿度采集函数*/
void Tempera_Proc()
{
	static char Temperature_Old;
	static char Hum_Old;	
	static float Temperaturn_Sum = 0;
	static float Hum_Sum = 0;
	static bit Collected = 0;
	if(Tempera_Slow_Down<50)return;
	Tempera_Slow_Down = 0;

	if(Value_Collect_Flag == 1 && Collected == 0)
	{
	
		if(Freq < 200 || Freq > 2000)
		{
			Error_Value = 1;
		}
		else if(Freq>=200 && Freq<=2000)
		{
			Error_Value = 0;
			Hum_Value = (Freq * 0.044 + 1.2);
		}
		Temperature = rd_temperature();
		Echo_Hour = RTC_Timer[0];
		Echo_Min  = RTC_Timer[1];

		Collect_Count++;
		if(Temperature > Tempera_Max)Tempera_Max = Temperature;
		Temperaturn_Sum += Temperature;
		Tempera_Avg = Temperaturn_Sum / Collect_Count;
		
		
		if((unsigned char)Hum_Value > Hum_Max) Hum_Max = (unsigned char)Hum_Value;
		Hum_Sum += Hum_Value;
		Hum_Avg = Hum_Sum / Collect_Count;

		Collected = 1;
		if(Temperature > Temperature_Old && Hum_Value > Hum_Old)
			L6_Flag = 1;
		else
			L6_Flag = 0;
		Temperature_Old = Temperature;
		Hum_Old = Hum_Value;
		
	}
	if(Value_Collect_Flag == 0)
	{
		Collected = 0;
	}
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
	Tempera_Slow_Down++;
	AD_Slow_Down++;
	
	if(Temperature > Temperature_Para)
	{
		if(++Timer_L4 == 200)Timer_L4 = 0;
	}		
	
	if(++Seg_Pos == 8) Seg_Pos = 0;//数码管显示专用
	if(Seg_Buf[Seg_Pos]>='.')
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos] - '.',1);
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
	
	if(Value_Collect_Flag == 1)
	{
		if(++Time_3s == 3000)
		{
			Time_3s = 0;
			Value_Collect_Flag = 0;
			Lock = 0;
			Seg_Disp_Mode = Seg_Disp_Mode_Backup;
		}
	}
	if(++Time_1s == 1000)
	{
		Time_1s = 0;
		Freq = (TH0 << 8) | TL0;
		TH0 = TL0 = 0;
	}
	if(Long_Press == 1 && Echo_Disp_Mode == 2)
	{
		if(++Timer_2s == 2000)
		{
			Timer_2s = 2000;
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

/* Main */
void main()
{
	Sys_Init();
	Timer1_Init();
	Timer0_Init();
	rd_temperature();
	Delay750ms();
	Set_Rtc(RTC_Timer);

	AD_Value = Ad_Read(0x43) / 51.0 * 100;
	State_Old = (AD_Value > 120) ? 0 : 1;

	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		Tempera_Proc();
		AD_Proc();
	}
}



