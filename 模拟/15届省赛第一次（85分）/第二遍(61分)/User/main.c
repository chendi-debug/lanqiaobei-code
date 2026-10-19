/*头文件声明区*/
#include <STC15F2K60S2.H>
#include "Key.h"
#include "Seg.h"
#include "Led.h"
#include "Init.h"
#include "iic.h"
#include "ds1302.h"

/*变量声明区*/
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量
unsigned char Key_Slow_Down;//按键减速专用变量
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char ucLed[8]={0,0,0,0,0,0,0};//Led显示数据存放数组
unsigned char DA_Slow_Down;


unsigned char Seg_Disp_Mode;//0-频率界面 1-参数界面 2-时间界面 3-回显界面
unsigned char Para_Disp_Mode;//0-超限参数界面 1-校准值参数界面
unsigned char Echo_Mode;//0-频率回显 1-时间回显
signed int Freq;
unsigned int Timer_1s;
unsigned char i;
signed int Freq_Para = 2000;//超限参数
signed int Calibration_value = 0;//校准值
#define abs(x) (x<0)?-(x):(x)
unsigned char RTC_Timer[] = {13,03,05};
unsigned int Freq_Max;//回显最大值
unsigned char RTC_MaxTimer[] = {0,0,0};//发生的时间
unsigned char Time_200ms;

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
			if(++Seg_Disp_Mode == 4)Seg_Disp_Mode = 0;
			Para_Disp_Mode = 0;
			Echo_Mode = 0;
		break;
		case 5:
			if(Seg_Disp_Mode == 1)//在参数界面下
			{
				Para_Disp_Mode ^= 1;
			}
			if(Seg_Disp_Mode == 3)//在回显界面下
			{
				Echo_Mode ^= 1;
			}
		break;
		case 8://加
			if(Para_Disp_Mode == 0)//超限参数界面
			{
				Freq_Para = (Freq_Para == 9000) ? 9000 :Freq_Para+1000;
			}
			if(Para_Disp_Mode == 1)//校准值参数界面
			{
				Calibration_value = (Calibration_value == 900)?900:Calibration_value+100;
			}
		break;
		case 9://减
			if(Para_Disp_Mode == 0)//超限参数界面
			{
				Freq_Para = (Freq_Para == 1000) ? 1000 :Freq_Para-1000;
			}
			if(Para_Disp_Mode == 1)//校准值参数界面
			{
				Calibration_value = (Calibration_value == -900)?-900:Calibration_value-100;
			}
		break;
	}
}




/*信息处理函数*/
void Seg_Proc()
{
	if(Seg_Slow_Down < 100) return;
	Seg_Slow_Down = 0;//数码管减速程序
	
	Read_Rtc(RTC_Timer);
	if(Freq_Max < Freq)
	{
		Freq_Max = Freq;
		RTC_MaxTimer[0] = RTC_Timer[0];
		RTC_MaxTimer[1] = RTC_Timer[1];
		RTC_MaxTimer[2] = RTC_Timer[2];		
	}
	switch(Seg_Disp_Mode)
	{
		case 0://频率界面
			Seg_Buf[0] = 11;//F 
			Seg_Buf[1] = 10;
			Seg_Buf[2] = 10;		
		if((Freq+Calibration_value) < 0)
		{
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			Seg_Buf[5] = 10;
			Seg_Buf[6] = 13;//L 
			Seg_Buf[7] = 13;//L
		}
		else
		{
			Seg_Buf[3] = (Freq+Calibration_value) / 10000 % 10;
			Seg_Buf[4] = (Freq+Calibration_value) / 1000 % 10;
			Seg_Buf[5] = (Freq+Calibration_value) / 100 % 10;
			Seg_Buf[6] = (Freq+Calibration_value) / 10 % 10;
			Seg_Buf[7] = (Freq+Calibration_value) % 10;		
			for(i=3;i<7&&Seg_Buf[i]==0;i++)
				Seg_Buf[i] = 10;			
		}

		break;
		case 1://参数界面
			Seg_Buf[0] = 12;//P
			Seg_Buf[1] = Para_Disp_Mode + 1;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			if(Para_Disp_Mode == 0)		
			{
				Seg_Buf[4] = Freq_Para / 1000 % 10;
				Seg_Buf[5] = Freq_Para / 100 % 10;
				Seg_Buf[6] = Freq_Para / 10 % 10;
				Seg_Buf[7] = Freq_Para % 10;
			}
			else
			{
				Seg_Buf[4] = (abs(Calibration_value) < 0) ? 14 : 10;
				Seg_Buf[5] = abs(Calibration_value / 100 % 10);
				Seg_Buf[6] = abs(Calibration_value / 10 % 10);
				Seg_Buf[7] = abs(Calibration_value % 10);
				if(Calibration_value == 0)
				{
				Seg_Buf[4] = 10;
				Seg_Buf[5] = 10;
				Seg_Buf[6] = 10;
				Seg_Buf[7] = 0;					
				}
			}
		break;
		case 2://时间界面
			Seg_Buf[0] = RTC_Timer[0] / 10 % 10;
			Seg_Buf[1] = RTC_Timer[0] % 10;	
			Seg_Buf[2] = 14;//-
			Seg_Buf[3] = RTC_Timer[1] % 10;	
			Seg_Buf[4] = RTC_Timer[1] / 10 % 10;
			Seg_Buf[5] = 14;//-	
			Seg_Buf[6] = RTC_Timer[2] / 10 % 10;
			Seg_Buf[7] = RTC_Timer[2] % 10;	
		break;
		case 3://回显界面
			Seg_Buf[0] = 15;//H
			if(Echo_Mode == 0)//频率回显
			{
				Seg_Buf[1] = 11;//F
				Seg_Buf[2] = 10;
				Seg_Buf[3] = Freq_Max / 10000 % 10;				
				Seg_Buf[4] = Freq_Max / 1000 % 10;
				Seg_Buf[5] = Freq_Max / 100 % 10;
				Seg_Buf[6] = Freq_Max / 10 % 10;
				Seg_Buf[7] = Freq_Max % 10;
			for(i=3;i<7&&Seg_Buf[i]==0;i++)
				Seg_Buf[i] = 10;				
			}
			else
			{
				Seg_Buf[1] = 16;//A
				Seg_Buf[2] = RTC_MaxTimer[0] / 10 % 10;
				Seg_Buf[3] = RTC_MaxTimer[0] % 10;
				Seg_Buf[4] = RTC_MaxTimer[1] / 10 % 10;
				Seg_Buf[5] = RTC_MaxTimer[1] % 10;
				Seg_Buf[6] = RTC_MaxTimer[2] / 10 % 10;
				Seg_Buf[7] = RTC_MaxTimer[2] % 10;		
			}
		break;
	}

}



/*其他显示函数*/
void Led_Proc()
{


	
}
/*DA函数*/
void DA_Proc()
{
	if(++DA_Slow_Down < 70)return;
	DA_Slow_Down = 0;
	
	
	if((Freq+Calibration_value)<0)
		Da_Write(0);
	else if((Freq+Calibration_value)<=500)
	{
		Da_Write(1*51.0f);		
	}
	else if((Freq+Calibration_value) >= 500 && (Freq+Calibration_value) < Freq_Para)
	{
		Da_Write((4/(Freq_Para-500))*(Freq+Calibration_value) +1-(200/(Freq_Para-500)) * 51.0f);
	}
	else if((Freq+Calibration_value) > Freq_Para)
	{
		Da_Write(5*51.0f);	
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
	DA_Slow_Down++;
	
	if(++Seg_Pos == 8) Seg_Pos = 0;//数码管显示专用
	if(Seg_Buf[Seg_Pos]>='.')
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos] - '.',1);
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
	
	if(++Timer_1s == 1000)
	{
		Timer_1s = 0;
		Freq = (TH0 << 8) | TL0;
		TH0 = TL0 = 0;	
	}
	if(Seg_Disp_Mode == 0)
	 {
			if(++Time_200ms == 200) ucLed[0] = 1;
			else  ucLed[0] = 0;
	 }
	 if((Freq+Calibration_value) > Freq_Para)
	 {
			if(++Time_200ms == 200) ucLed[1] = 1;
			else  ucLed[1] = 0;		
	 }
	 if((Freq+Calibration_value) <0)
		 ucLed[1] = 1;
	Led_Disp(ucLed);
}

/* Main */
void main()
{
	Sys_Init();
	Timer1_Init();
	Set_Rtc(RTC_Timer);
	Timer0_Init();
	
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		DA_Proc();
	}
}



