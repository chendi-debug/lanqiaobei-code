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
bit Seg_Buf_Mode;//数码管显示界面 0-频率显示界面 1-电压显示界面
unsigned int Freq;//实时频率值
unsigned int Timer_1000Ms;// 1s标志位
float Voltage;
bit Output_Mode;
float Voltage_Output;
bit Seg_Flag = 1;
bit Led_Flag = 1;
/*键盘处理函数*/

void Key_Proc()
{
	if(Key_Slow_Down)return;
	Key_Slow_Down=1;//键盘减速程序
	
	Key_Val =Key_Read();//实时读取键码值
	Key_Down = Key_Val & (Key_Old ^ Key_Val);//判断按键是否按下
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);//捕捉按键上升沿
	Key_Old = Key_Val;//辅助扫描变量
	
	switch(Key_Down)
	{
		case 4:  //界面切换按键
			Seg_Buf_Mode ^=1;
		break;
		case 5:  //电压输出模式切换按键
			Output_Mode ^=1;
		break;
		case 6:
			Led_Flag ^=1;
		break;
		case 7:
			Seg_Flag ^=1;
		break;
	}
}

/*信息处理函数*/
void Seg_Proc()
{
	unsigned char i = 3;
	if(Seg_Slow_Down) return;
	Seg_Slow_Down = 1;//数码管减速程序
	
	Voltage = Ad_Read(0x43) / 51.0;       //0x43是RB2，0x41是光敏电阻
	if(Output_Mode == 0)
	{
		Voltage_Output = 2;
	}
	else
	{
		Voltage_Output = Voltage;
	}
	if(Seg_Buf_Mode == 0) //频率显示界面
	{
		Seg_Buf[0] = 11;// F
		Seg_Buf[3] = Freq / 10000 % 10;
		Seg_Buf[4] = Freq / 1000 % 10;
		Seg_Buf[5] = Freq / 100 % 10;
		Seg_Buf[6] = Freq / 10 % 10;
		Seg_Buf[7] = Freq % 10;
		Seg_Point[5] = 0;
		while(Seg_Buf[i] == 0)
		{
			Seg_Buf[i] = 10;
			if(++i == 7) break;     //上电的瞬间是为0如果不加限制条件程序会卡死
		}
	}
	else
	{
		Seg_Buf[0] = 12;
		Seg_Buf[3] = Seg_Buf[4] =10;
		Seg_Point[5] = 1;
		Seg_Buf[5] = (unsigned int)(Voltage * 100) / 100 %10;
		Seg_Buf[6] = (unsigned int)(Voltage * 100) / 10 %10;
		Seg_Buf[7] = (unsigned int)(Voltage * 100) %10;

	} 
}



/*其他显示函数*/
void Led_Proc()
{
	unsigned char i;
	Da_Write(Voltage_Output * 51.0);
	for(i=0;i<2;i++)
	{
		ucLed[i] = (i == Seg_Buf_Mode);
	}
	ucLed[2] = ((Voltage >= 1.5 && Voltage < 2.5) || Voltage >= 3.5);
	ucLed[3] = ((Freq >= 1000 && Freq <5000) || Freq >= 10000);
	ucLed[4] = Output_Mode;
}



/* 定时器0中断初始化函数 */
void Timer0_Init(void)		//1毫秒@12.000MHz    //没办法生成1毫秒可以生成65536微妙
{
	AUXR &= 0x7F;			//定时器时钟12T模式
	TMOD &= 0xF0;			//设置定时器模式
	TMOD |= 0x05;                     // GATE-0  C/T# - 1   M1 -0 M2 - 1  0x50;
	TL0 = 0x00;				//设置定时初始值
	TH0 = 0x00;				//设置定时初始值
	TF0 = 0;				//清除TF0标志
	TR0 = 1;				//定时器0开始计时
}


/*定时器1中断初始化*/
void Timer1Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0xBF;			//定时器时钟12T模式
	TMOD &= 0x0F;			//设置定时器模式
	TL1 = 0x18;				//设置定时初始值
	TH1 = 0xFC;				//设置定时初始值
	TF1 = 0;				//清除TF1标志
	TR1 = 1;				//定时器1开始计时
	ET1 = 1;
	EA = 1;
}

/*定时器1中断服务函数*/
void Timer1_Init() interrupt 3
{
	if(++Key_Slow_Down == 10)Key_Slow_Down=0;//键盘减速专用
	if(++Seg_Slow_Down == 500)Seg_Slow_Down = 0;//数码管减速专用
	if(++Seg_Pos == 8) Seg_Pos = 0;//数码管显示专用
	if(Seg_Flag == 1)
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
	else
		Seg_Disp(Seg_Pos,10,0);
	if(Led_Flag == 1)
		Led_Disp(Seg_Pos,ucLed[Seg_Pos]);
	else
		Led_Disp(Seg_Pos,0);
	
	
	if(++Timer_1000Ms == 1000)
	{
		Timer_1000Ms = 0;
		Freq = TH0 << 8 | TL0;         //在1s的时间内定时器0累加的数值
		TH0 = TL0 = 0;   //因为计数定时器不自动重装载，要软件归位
	}
}

/* Main */
void main()
{
	Sys_Init();
	Timer1Init();
	Timer0_Init();
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();

	}
}



