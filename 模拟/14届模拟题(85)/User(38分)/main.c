/*头文件声明区*/
#include <STC15F2K60S2.H>
#include "Key.h"
#include "Seg.h"
#include "Led.h"
#include "Init.h"
#include "Uart.h"
#include "ultrasound.h"
#include "stdio.h"

/*变量声明区*/
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量
unsigned char Key_Slow_Down;//按键减速专用变量
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char ucLed[8]={0,0,0,0,0,0,0};//Led显示数据存放数组
unsigned char Seg_Disp_Mode;//数码管显示模式  0-距离显示模式  1-参数显示模式
unsigned char i;
unsigned char Uart_Slow_Down;
unsigned int Distance;//距离参数
unsigned int Distamce_Para = 30;//距离参数
unsigned char Uart_send[10] = {0,0,0,0,0,0,0,0,0,0};//缓冲区

unsigned int Timer_200ms;//200毫秒计数
bit Light_Flag;//闪烁标志位





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
		case 4://界面切换按键
			Seg_Disp_Mode ^=1;
		break;
		case 8://参数按键
			if(Seg_Disp_Mode == 0)//在距离显示界面
			Distamce_Para = Distance;
		break;
		case 9://发送按键
			printf("Distance:%dcm\r\n",Distance);
		break;
		case 12://加
			if(Seg_Disp_Mode == 1)//在参数显示界面下
			{
				Distamce_Para = (Distamce_Para == 100) ? 100 : Distamce_Para + 10;
			}
		break;
		case 16://减
			if(Seg_Disp_Mode == 1)//在参数显示界面下
			{
				Distamce_Para = (Distamce_Para == 0) ? 0 : Distamce_Para - 10;
			}			
		break;
	}
	
}




/*信息处理函数*/
void Seg_Proc()
{
	if(Seg_Slow_Down < 100) return;
	Seg_Slow_Down = 0;//数码管减速程序

	Distance = Ut_Wave_Data();
	
	
	switch(Seg_Disp_Mode)
	{
		case 0://距离显示模式 U 1000 0011 1100 0001 0xc1
			Seg_Buf[0] = 11;//U
			Seg_Buf[1] = 1;//1
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;		
			Seg_Buf[5] = (unsigned char)Distance / 100 % 10;
			Seg_Buf[6] = (unsigned char)Distance / 10 % 10;
			Seg_Buf[7] = (unsigned char)Distance % 10;	
			for(i=5;i<7 && Seg_Buf[i] == 0;i++)
			{
					Seg_Buf[i] = 10;
			}
		break;
		case 1:
			Seg_Buf[0] = 11;//U
			Seg_Buf[1] = 2;//2
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;		
			Seg_Buf[5] = Distamce_Para / 100 % 10;
			Seg_Buf[6] = Distamce_Para / 10 % 10;
			Seg_Buf[7] = Distamce_Para % 10;	
			for(i=5;i<7 && Seg_Buf[i] == 0;i++)
			{
					Seg_Buf[i] = 10;
			}		
		break;
	}

}



/*其他显示函数*/
void Led_Proc()
{
	ucLed[0] = (Seg_Disp_Mode == 0);
	ucLed[1] = (Seg_Disp_Mode == 1);
	if(Distance > Distamce_Para)
	{
		ucLed[2] = Light_Flag;
	}
	Led_Disp(ucLed);

}

/* 串口处理函数 */
void Uart_Proc()
{
	if(Uart_Slow_Down < 200) return;
	Uart_Slow_Down = 1;//串口减速程序	

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
	Uart_Slow_Down++;
	
	if(++Seg_Pos == 8) Seg_Pos = 0;//数码管显示专用
	if(Seg_Buf[Seg_Pos]>='.')
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos] - '.',1);
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
	
	if(++Timer_200ms == 400)
	{
		Timer_200ms = 0;
		Light_Flag = 1;//点亮
	}
	else 
	{
		Light_Flag = 0;
	}
}

/* Main */
void main()
{
	Sys_Init();
	PCA_Init();
	Timer1_Init();
	Uart1_Init();

	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		Uart_Proc();
	}
}



