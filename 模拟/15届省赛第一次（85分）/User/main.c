/*头文件声明区*/
#include <STC15F2K60S2.H>
#include "Key.h"
#include "Seg.h"
#include "Led.h"
#include "Init.h"
#include "DS1302.h"
#include "iic.h"


/*变量声明区*/
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量
unsigned char Key_Slow_Down;//按键减速专用变量
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char ucLed[8]={0,0,0,0,0,0,0,0};//Led显示数据存放数组

/*界面控制*/
unsigned char Seg_Disp_Mode = 0;//数码管显示模式 0-频率界面 1-参数界面 2-时间界面 3-回显界面
unsigned char Para_Disp_Mode = 0;//参数界面子界面： 0-超限参数界面 1-校准值参数界面
unsigned char Echo_Disp_Mode = 0;//回显界面子界面 0-回显界面（频率） 1-回显界面（时间）

/*相关变量*/
idata unsigned int Timer_1s;//1秒专用变量
idata unsigned int Freq;//频率值
idata unsigned char Freq_Para = 2;//将超限参数除以1000进行存储，1000~9000 = 1`9 每次加减1000 = 1
idata char Calibration_Value_Para = 0;//将校准值除以100进行存储，-900~900 = -9~9，每加减100 = 1
idata unsigned char Rtc_Time[3] = {13,03,05}; //存储从DS1302获取时分秒
idata unsigned int Max_Freq;//最大频率
idata unsigned char Max_Rtc_Time[3];//记录最大频率的时间
idata unsigned char i;
idata unsigned char Led1_Timer_200ms;//L1 以 0.2s 为间隔亮灭
idata unsigned char Led2_Timer_200ms;//L2 以 0.2s 为间隔亮灭
idata unsigned char Timer200ms;
bit Freq_Error_Flag;//频率测量错误标志位

/*键盘处理函数*/
void Key_Proc()
{
	if(Key_Slow_Down)return;
	Key_Slow_Down=1;//键盘减速程序
	
	Key_Val = Key_Read();//实时读取键码值
	Key_Down = Key_Val & (Key_Old ^ Key_Val);//判断按键是否按下
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);//捕捉按键上升沿
	Key_Old = Key_Val;//辅助扫描变量
	
	switch(Key_Down)
	{
		case 4://主界面切换
			if(++Seg_Disp_Mode == 4)Seg_Disp_Mode = 0;
			Para_Disp_Mode = Echo_Disp_Mode = 0;
			ucLed[0] = Led1_Timer_200ms = 0;
		break;
		case 5://子界面切换
			if(Seg_Disp_Mode == 1)//在参数界面下
				Para_Disp_Mode ^=1;
			else if(Seg_Disp_Mode == 3)//在回显子界面下
				Echo_Disp_Mode ^= 1;
		break;
		case 8: //参数加
			if(Seg_Disp_Mode == 1)
			{
				if(Para_Disp_Mode == 0 && ++Freq_Para == 10)Freq_Para = 9;
				else if(Para_Disp_Mode == 1 && ++Calibration_Value_Para == 10)Calibration_Value_Para = 9;
			}
		break;
		case 9: //参数减
			if(Seg_Disp_Mode == 1)
			{
				if(Para_Disp_Mode == 0 && --Freq_Para == 0)Freq_Para = 1;
				else if(Para_Disp_Mode == 1 && --Calibration_Value_Para == -10)Calibration_Value_Para = -9;
			}
		break;
	}
}

/*信息处理函数*/
void Seg_Proc()
{

	if(Seg_Slow_Down) return;
	Seg_Slow_Down = 1;//数码管减速程序
	
	Read_Rtc(Rtc_Time);	
	
	switch(Seg_Disp_Mode)
	{
		case 0://频率界面
			Seg_Buf[0] = 11;//F
			Seg_Buf[1] = 10;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = Freq_Error_Flag ? 10 : Freq / 10000 % 10;
			Seg_Buf[4] = Freq_Error_Flag ? 10 : Freq / 1000 % 10;
			Seg_Buf[5] = Freq_Error_Flag ? 10 : Freq / 100 % 10;
			Seg_Buf[6] = Freq_Error_Flag ? 17 : Freq / 10 % 10;
			Seg_Buf[7] = Freq_Error_Flag ? 17 : Freq % 10;		
			for(i=3;i<7 && Seg_Buf[i] == 0;i++)
				Seg_Buf[i] = 10;

		break;
		case 1://参数界面
			Seg_Buf[0] = 12;//P
			Seg_Buf[1] = Para_Disp_Mode + 1;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			switch(Para_Disp_Mode)//参数界面子界面
			{
				case 0://超限参数界面
					Seg_Buf[4] = Freq_Para;
					Seg_Buf[5] = 0;
					Seg_Buf[6] = 0;
					Seg_Buf[7] = 0;
				break;
				case 1://校准值参数界面 
					Seg_Buf[4] = Calibration_Value_Para < 0 ? 13 : 10;//参数小于0吗小于就显示符号 13 -
					Seg_Buf[5] = Calibration_Value_Para == 0 ? 10 :(Calibration_Value_Para < 0 ? -Calibration_Value_Para : Calibration_Value_Para);
					Seg_Buf[6] = Calibration_Value_Para == 0 ? 10 : 0;
					Seg_Buf[7] = 0;
				break;
			}
		break;
		case 2://时间界面
			Seg_Buf[2] = Seg_Buf[5] = 13;//-
			Seg_Buf[0] = Rtc_Time[0] / 10 % 10;
			Seg_Buf[1] = Rtc_Time[0] % 10;
			Seg_Buf[3] = Rtc_Time[1] / 10 % 10;
			Seg_Buf[4] = Rtc_Time[1] % 10;
			Seg_Buf[6] = Rtc_Time[2] / 10 % 10;
			Seg_Buf[7] = Rtc_Time[2] % 10;
		
		break;
		case 3://回显界面
			switch(Echo_Disp_Mode)
			{
				case 0://最大频率界面
					Seg_Buf[0] = 14;//H
					Seg_Buf[1] = 15;//F
					Seg_Buf[2] = 10; 
					Seg_Buf[3] = Max_Freq / 10000 % 10;
					Seg_Buf[4] = Max_Freq / 1000 % 10;
					Seg_Buf[5] = Max_Freq / 100 % 10;
					Seg_Buf[6] = Max_Freq / 10 % 10;
					Seg_Buf[7] = Max_Freq % 10;
					for(i = 3; Seg_Buf[i] == 0 && i < 7; i++) // 高位熄灭
					Seg_Buf[i] = 10;
				break;
				case 1://记录最大频率的时间
					Seg_Buf[0] = 14;//H
					Seg_Buf[1] = 16;//A
					Seg_Buf[2] = Max_Rtc_Time[0] / 10 % 10;
					Seg_Buf[3] = Max_Rtc_Time[0] % 10;
					Seg_Buf[4] = Max_Rtc_Time[1] / 10 % 10;
					Seg_Buf[5] = Max_Rtc_Time[1] % 10;
					Seg_Buf[6] = Max_Rtc_Time[2] / 10 % 10;
					Seg_Buf[7] = Max_Rtc_Time[2] % 10;	
				break;
			}
		break;
	}

}

/*其他显示函数*/
void Led_Proc()
{
	Freq_Error_Flag = (Freq > 64000);
	if(Max_Freq < Freq)
	{
		Max_Freq = Freq;
		for(i=0;i<3;i++)
			Max_Rtc_Time[i] = Rtc_Time[i];
	}
	
	if(Timer200ms) return;
	
	if(Freq_Error_Flag == 0)
	{
		if(Freq >= (Freq_Para * 1000))//如果测量频率大于超限参数
			Da_Write(5);
		else if(Freq <= 500)
			Da_Write(1);
		else
			Da_Write((4.0 / (Freq_Para * 1000 - 500)) * (Freq - 500) + 1);
	}
	else//频率状态错误
	{
		Da_Write(0);
		ucLed[1] = 1;
	}
}

void Timer0_Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0x7F;			//定时器时钟12T模式
	TMOD &= 0xF0;			//设置定时器模式
	TMOD |= 0x05;			//设置定时器模式  计数模式
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
	if(++Key_Slow_Down == 10)Key_Slow_Down=0;//键盘减速专用
	if(++Seg_Slow_Down == 100)Seg_Slow_Down = 0;//数码管减速专用

	if(++Led1_Timer_200ms == 200 && Seg_Disp_Mode == 0)//频率界面下
	{
		Led1_Timer_200ms = 0;
		ucLed[0] ^=1; 
	}
	
	if(++Timer_1s == 1000)
	{
		Timer_1s = 0;
		Freq = ((TH0 << 8) | TL0);
		Freq = Freq + (Calibration_Value_Para * 100);
		TH0 = TL0 = 0x00;
	}
	
	if(++Timer200ms == 200)
	{
		Timer200ms = 0;
	}
	
	if(Freq_Error_Flag == 0)
	{
		if(Freq > (Freq_Para * 1000))//当频率值大于超限值时
		{
			if(++Led2_Timer_200ms == 200)
			{
				Led2_Timer_200ms = 0;
				ucLed[1] ^=1; 		
			}
		}
		else
		{
			ucLed[1] = Led2_Timer_200ms = 0;
		}
	}

	if(++Seg_Pos == 8) Seg_Pos = 0;//数码管显示专用
	if(Seg_Buf[Seg_Pos] > 20)
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos]-'.',1);
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
	
	Led_Disp(ucLed,1);
}

/* Main */
void main()
{
	Sys_Init();
	Set_Rtc(Rtc_Time);
	Timer0_Init();
	Timer1_Init();
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
	}
}
