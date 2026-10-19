/*头文件声明区*/
#include <STC15F2K60S2.H>
#include "Key.h"
#include "Seg.h"
#include "Led.h"
#include "Init.h"
#include "iic.h"


/*变量声明区*/
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量
unsigned char Key_Slow_Down;//按键减速专用变量
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Point[8]={0,0,0,0,0,0,0,0};//数码管小数点数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char ucLed[8]={0,0,0,0,0,0,0};//Led显示数据存放数组
unsigned char Seg_Disp_Mode;//数码管显示模式变量 0-频率显示界面 1-电压显示界面
unsigned char DAC_Output_Mode;//DAC输出模式变量 默认为RB2
unsigned int Freq;//频率变量
unsigned int Timer_1000Ms;//1s变量
unsigned char i;
unsigned int Voltage_100x;//电压100倍变量
unsigned char Led_Mode;//LED工作模式
unsigned char Seg_Mode;//Seg工作模式

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
		case 4://显示界面切换
			Seg_Disp_Mode ^=1;
		break;
		case 5://DAC输出模式按键
			DAC_Output_Mode ^=1;
		break;
		case 6://显示界面切换
			Led_Mode ^=1;
		break;
		case 7://数码管显示功能
			Seg_Mode ^=1;
		break;
		
	}
}




/*信息处理函数*/
void Seg_Proc()
{
	if(Seg_Slow_Down < 100) return;
	Seg_Slow_Down = 0;//数码管减速程序
	
	Voltage_100x = Ad_Read(0x43) / 51.0 * 100;
	
	if(Seg_Mode == 0)
	{
		switch(Seg_Disp_Mode)
		{
			case 1://频率显示界面 F 0111 0001 1000 1110
				Seg_Buf[0] = 11;//F
				Seg_Buf[1] = 10;
				Seg_Buf[2] = Freq / 100000 % 10;
				Seg_Buf[3] = Freq / 10000 % 10;
				Seg_Buf[4] = Freq / 1000 % 10;
				Seg_Buf[5] = Freq / 100 % 10;
				Seg_Buf[6] = Freq / 10 % 10;
				Seg_Buf[7] = Freq % 10;
				for(i=2;i<7 && Seg_Buf[i]==0;i++)
					Seg_Buf[i] = 10;	
			break;
			case 0://电压显示界面
				Seg_Buf[0] = 12;//U
				Seg_Buf[1] = 10;
				Seg_Buf[2] = 10;
				Seg_Buf[3] = 10;
				Seg_Buf[4] = 10;
				Seg_Buf[5] = (DAC_Output_Mode == 0)?2 + '.':Voltage_100x / 100 % 10 + '.';
				Seg_Buf[6] = (DAC_Output_Mode == 0)?0:Voltage_100x / 10 % 10;
				Seg_Buf[7] = (DAC_Output_Mode == 0)?0:Voltage_100x % 10;
			break;
		}
	}
	else
	{
		Seg_Buf[0] = 10;
		Seg_Buf[1] = 10;
		Seg_Buf[2] = 10;
		Seg_Buf[3] = 10;
		Seg_Buf[4] = 10;
		Seg_Buf[5] = 10;
		Seg_Buf[6] = 10;
		Seg_Buf[7] = 10;
		Seg_Buf[8] = 10;		
	}
}



/*其他显示函数*/
void Led_Proc()
{
	Led_Disp(ucLed);
	if(Led_Mode == 0)
	{
		ucLed[0] = (Seg_Disp_Mode == 0);//电压测量指示灯
		ucLed[1] = (Seg_Disp_Mode == 1);//频率测量功能
		ucLed[2] = ((150 <= Voltage_100x && Voltage_100x < 250) || (Voltage_100x >= 350));
		ucLed[3] = ((1000 <= Freq && Freq < 5000) || (Freq >= 10000));
		ucLed[4] = (DAC_Output_Mode == 0)?0:1;
	}
	else
	{
		ucLed[0] = 0;		
		ucLed[1] = 0;		
		ucLed[2] = 0; 		
		ucLed[3] = 0;		
		ucLed[4] = 0;	
	}
}

void Timer0_Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0x7F;			//定时器时钟12T模式
	TMOD &= 0xF0;			//设置定时器模式
	TMOD |= 0x05;			//设置计数器模式
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
	if(++Timer_1000Ms == 1000)
	{
		EA = 0;
		Timer_1000Ms = 0;
		Freq = (TH0 << 8) | TL0;
		TH0 = TL0 = 0;
		EA = 1;
	}
	
}

/* Main */
void main()
{
	Sys_Init();
	Timer1_Init();
	Timer0_Init();

	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();

	}
}

