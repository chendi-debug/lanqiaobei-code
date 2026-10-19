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
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char ucLed[8]={0,0,0,0,0,0,0};//Led显示数据存放数组
unsigned char Seg_Disp_Mode ;//数码管显示模式 0-信号界面 1-温度界面 2-参数界面
unsigned int Freq;//频率测量值
unsigned int Timer_1000ms;//1s计时变量
unsigned char i;
float Temperature;//温度变量
unsigned char Temperature_Para;//温度参数变量
unsigned char Temperature_Para_Ctrol = 25;//温度参数变量
unsigned char Timer_100ms;//0.1s计时
bit Flash_Flag;//闪烁标志位


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
			if(Seg_Disp_Mode == 2)
				Temperature_Para_Ctrol = Temperature_Para;
			if(Seg_Disp_Mode == 1)
				Temperature_Para = Temperature_Para_Ctrol;
		break;
		case 9:
			if(Seg_Disp_Mode == 2)
				Temperature_Para_Ctrol = (Temperature_Para_Ctrol == 100) ? 100 :Temperature_Para_Ctrol+1;
		break;
		case 8:
				Temperature_Para_Ctrol = (Temperature_Para_Ctrol == 0) ? 0 :Temperature_Para_Ctrol-1;
			
		break;
	}
}




/*信息处理函数*/
void Seg_Proc()
{
	if(Seg_Slow_Down < 100) return;
	Seg_Slow_Down = 0;//数码管减速程序

	Temperature = rd_temperature();
	
	switch(Seg_Disp_Mode)
	{
		case 0://信号界面
			Seg_Buf[0] = 11;//P 0011 0001 1000 1100 0x8c
			Seg_Buf[1] = 10;
			Seg_Buf[2] = Freq / 100000 % 10;
			Seg_Buf[3] = Freq / 10000 % 10;
			Seg_Buf[4] = Freq / 1000 % 10;
			Seg_Buf[5] = Freq / 100 % 10;
			Seg_Buf[6] = Freq / 10 % 10;
			Seg_Buf[7] = Freq % 10;
		for(i=2; i<7 && Seg_Buf[i] == 0;i++)
			Seg_Buf[i] = 10; 
		break;
		case 1://温度界面 C 0110 0011 1100 0110 0xc6
			Seg_Buf[0] = 12;//C
			Seg_Buf[1] = 10;
		  Seg_Buf[2] = 10;
		  Seg_Buf[3] = 10;
		  Seg_Buf[4] = 10;
		  Seg_Buf[5] = (unsigned char)Temperature / 10 % 10;
		  Seg_Buf[6] = (unsigned char)Temperature % 10 + '.';
		  Seg_Buf[7] = (unsigned int)(Temperature * 10) % 10;
		break;
		case 2://温度参数
			Seg_Buf[0] = 13;//U 1000 0011 1100 0001 0xc1
			Seg_Buf[1] = 10;
			Seg_Buf[2] = 10;
		  Seg_Buf[3] = 10;
		  Seg_Buf[4] = 10;
		  Seg_Buf[5] = 10;
		  Seg_Buf[6] = Temperature_Para_Ctrol / 10 % 10;
		  Seg_Buf[7] = Temperature_Para_Ctrol % 10;
		break;
		
	}

}



/*其他显示函数*/
void Led_Proc()
{
	ucLed[0] = (Seg_Disp_Mode == 0);
	ucLed[1] = (Seg_Disp_Mode == 1);	
	ucLed[7] = (Flash_Flag == 1);	
	Led_Disp(ucLed);
	
	/*DA*/
	if(Freq < 200)
		Da_Write(0.5 * 51.0);
	else if(Freq > 2000)
		Da_Write(4.5 * 51.0);
	else
	{
		Da_Write((0.002 * Freq + 0.1) * 51.0);
	}
	
	if(Temperature > Temperature_Para)
		Relay(1);
	else
		Relay(0);
		
	
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
	
	if(++Timer_1000ms == 1000)
	{
		Timer_1000ms = 0;
		Freq = (TH0 << 8) | TL0;
		TH0 = TL0 = 0;
	}
	
	if(++Timer_100ms == 100)
	{
		Timer_100ms = 0;
		Flash_Flag = 1;
	}
	else
	{
		Flash_Flag = 0;
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



