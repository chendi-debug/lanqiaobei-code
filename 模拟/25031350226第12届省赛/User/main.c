/*头文件声明区*/
#include <STC15F2K60S2.H>
#include "Key.h"
#include "Seg.h"
#include "Led.h"
#include "Init.h"
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
unsigned char Seg_Disp_Mode; //0-温度显示界面 1-参数设置界面 2-DAC输出界面 
float Temperature; //温度变量
unsigned char Temp_Disp = 25; //温度参数变量（用于设置显示）
unsigned char Temp_Ctrol = 25; //温度参数控制变量（用于实际控制） 初始值为25
float Voltage_Output; //实时输出电压
bit Output_Mode;//DAC输出模式 0-DAC输出电压与温度相关 1-DAC给出的关系输出电压

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
		case 4://界面切换按键
			if(++Seg_Disp_Mode == 3) Seg_Disp_Mode = 0;
			if(Seg_Disp_Mode == 1) Temp_Disp = Temp_Ctrol;
		  if(Seg_Disp_Mode == 2) Temp_Ctrol = Temp_Disp;
		break;
		case 8://减
			if(Seg_Disp_Mode == 1 && --Temp_Disp == 255) Temp_Disp = 0;
		case 9://加
			if(Seg_Disp_Mode == 1 && ++Temp_Disp == 100) Temp_Disp = 99;
		break;
		case 5://DAC模式切换按键
			Output_Mode ^=1;
		break;
		
	}
}

/*信息处理函数*/
void Seg_Proc()
{
	if(Seg_Slow_Down) return;
	Seg_Slow_Down = 1;//数码管减速程序
	
	/*信息获取区域*/
	Temperature = rd_temperature();
	
	/*数据显示区域*/
	switch(Seg_Disp_Mode)
	{
		case 0://温度显示界面
			Seg_Buf[0] = 11;//C
			Seg_Buf[4] = (unsigned char)Temperature / 10 % 10;
			Seg_Buf[5] = (unsigned char)Temperature % 10;
			Seg_Buf[6] = (unsigned int)(Temperature * 100) / 10 % 10;
			Seg_Buf[7] = (unsigned int)(Temperature * 100) % 10;
			Seg_Point[5] = 1;
		break;
		case 1://参数设置界面
			Seg_Buf[0] = 12;//P
			Seg_Buf[1] = Seg_Buf[2] = Seg_Buf[3] = Seg_Buf[4] = Seg_Buf[5]= 10;
			Seg_Buf[6] = Temp_Disp / 10 % 10;
			Seg_Buf[7] = Temp_Disp % 10;
			Seg_Point[5] = 0;
		break;
		case 2://DAC输出界面 
			Seg_Buf[0] = 13;//A
			Seg_Buf[1] = Seg_Buf[2] = Seg_Buf[3] = Seg_Buf[4] = 10;
			Seg_Buf[5] = (unsigned char)Voltage_Output;
			Seg_Buf[6] = (unsigned int)(Voltage_Output * 100) / 10 % 10;
			Seg_Buf[7] = (unsigned int)(Voltage_Output * 100) % 10;		
			Seg_Point[5] = 1;
		break;
	}
}



/*其他显示函数*/
void Led_Proc()
{
	unsigned char i;
	/*DAC相关*/
	if(Output_Mode == 0)
	{
		if(Temperature < Temp_Ctrol)
			Voltage_Output = 0;
		else
			Voltage_Output = 5;
	}
		else
		{
			if(Temperature < 20)
				Voltage_Output = 1;
			else if(Temperature >40)
				Voltage_Output = 4;
			else
				Voltage_Output = 0.15*(Temperature - 20) + 1;
		}
	Da_Write(Voltage_Output * 51);
	
	/*LED相关*/
	ucLed[0] = ~Output_Mode;
	for(i=0;i<3;i++)
	{
		ucLed[1+i] = (i == Seg_Disp_Mode);
	}
	
}

/* 定时器0中断初始化函数 */
void Timer0Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0x7F;		//定时器时钟12T模式
	TMOD &= 0xF0;		//设置定时器模式
	TL0 = 0x18;		//设置定时初始值
	TH0 = 0xFC;		//设置定时初始值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
	ET0 = 1;    //定时器中断0打开
	EA = 1;     //总中断打开
}


/*定时器0中断服务函数*/
void Timer0_Init() interrupt 1
{
	if(++Key_Slow_Down == 10)Key_Slow_Down=0;//键盘减速专用
	if(++Seg_Slow_Down == 200)Seg_Slow_Down = 0;//数码管减速专用
	
	if(++Seg_Pos == 8) Seg_Pos = 0;//数码管显示专用
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
	Led_Disp(Seg_Pos,ucLed[Seg_Pos]);
}
/*延时函数*/
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
	rd_temperature();
	Delay750ms();
	Sys_Init();
	Timer0Init();
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();

	}
}



