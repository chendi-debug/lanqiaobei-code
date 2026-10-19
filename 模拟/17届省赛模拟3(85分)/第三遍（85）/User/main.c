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
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char ucLed[8]={0,0,0,0,0,0,0};//Led显示数据存放数组
unsigned char Seg_Disp_Mode;//0-准备界面 1-设置界面 2-比赛界面 3-结果界面  
unsigned char Timer_Para = 5;//时间参数
unsigned char Timer_Para_Ctrol = 0;//剩余时间
unsigned int Timer_1s = 0;
unsigned char Ad_Slow_Down;
unsigned char Ad_Value;
unsigned char Comper;


/*键盘处理函数*/

void Key_Proc()
{
	if(Key_Slow_Down < 10)return;
	Key_Slow_Down=0;//键盘减速程序
	
	Key_Val =Key_Read();//实时读取键码值
	Key_Down = Key_Val & (Key_Old ^ Key_Val);//判断按键是否按下
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);//捕捉按键上升沿
	if(Seg_Disp_Mode >=3 )
	{
		if(Key_Down && Key_Old == 0)
		{
			Seg_Disp_Mode = 0;
		}
	}
	Key_Old = Key_Val;//辅助扫描变量

	switch(Key_Down)
	{
		case 6:
			if(Seg_Disp_Mode == 1)//在设置界面下
			{
				Timer_Para = (Timer_Para == 0)?0:Timer_Para-1;
			}
		break;
		case 7:
			if(Seg_Disp_Mode == 1)//在设置界面下
			{
				Timer_Para = (Timer_Para == 99)?99:Timer_Para+1;
			}
		break;
	}
	switch(Key_Val)
	{
		case 22:
			if(Seg_Disp_Mode == 0)//在准备界面下
			{
				Seg_Disp_Mode = 2;
			}
		break;	
	}
	if(Seg_Disp_Mode == 2)
	{
		if(Key_Val != 22)
			{
				Comper = 22-Key_Val;
				Seg_Disp_Mode = 3;
			}
	}
	
	if(Seg_Disp_Mode == 1)
	{
			Timer_Para_Ctrol = Timer_Para;
	}
			
}




/*信息处理函数*/
void Seg_Proc()
{
	if(Seg_Slow_Down < 100) return;
	Seg_Slow_Down = 0;//数码管减速程序

	
	switch(Seg_Disp_Mode)
	{
		case 0://准备界面
			Seg_Buf[0] = 11;//H
			Seg_Buf[1] = 12;//E
			Seg_Buf[2] = 13;//L
			Seg_Buf[3] = 13;//L
			Seg_Buf[4] = 0;//0
			Seg_Buf[5] = 10;
			Seg_Buf[6] = 10;
			Seg_Buf[7] = 10;
		break;
		case 1://设置界面
			Seg_Buf[0] = 14;//U
			Seg_Buf[1] = 10;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			Seg_Buf[5] = 10;
			Seg_Buf[6] = Timer_Para / 10 % 10;
			Seg_Buf[7] = Timer_Para % 10;			
		break;
		case 2://比赛界面
			Seg_Buf[0] = 15;//A
			Seg_Buf[1] = 10;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			Seg_Buf[5] = 10;
			Seg_Buf[6] = Timer_Para_Ctrol / 10 % 10;
			Seg_Buf[7] = Timer_Para_Ctrol % 10;							
		break;
		case 3://结果界面
			Seg_Buf[0] = 12;//E 
			Seg_Buf[1] = 10;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			Seg_Buf[5] = 10;
			Seg_Buf[6] = 10;
			Seg_Buf[7] = Comper % 10;				
		break;
		case 4://结果界面
			Seg_Buf[0] = 12;//E 
			Seg_Buf[1] = 10;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			Seg_Buf[5] = 10;
			Seg_Buf[6] = 12;//E
			Seg_Buf[7] = 12;//E		
		break;
	}
	
}



/*其他显示函数*/
void Led_Proc()
{
	ucLed[0] = (Seg_Disp_Mode == 0);	
	ucLed[1] = (Seg_Disp_Mode == 1);
	ucLed[2] = (Seg_Disp_Mode == 2);
	if(Seg_Disp_Mode >= 3)
	{
		ucLed[3] = 1;
	}
	else
	{
		ucLed[3] = 0;
		
	}
	Led_Disp(ucLed);
	
}
/*AD_DA*/
void AD_Proc()
{
	if(Ad_Slow_Down < 70)return;
		Ad_Slow_Down = 0;
	Ad_Value = Ad_Read(0x41) / 51.0 * 100;
	if(Seg_Disp_Mode == 0)//在准备界面
	{
		if(Ad_Value <=90)//暗
		{
			Seg_Disp_Mode = 1;
		}
	}
	else if(Seg_Disp_Mode == 1) 
	{
		if(Ad_Value > 90)//亮
		{
			Seg_Disp_Mode = 0;
		}
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
	Ad_Slow_Down++;
	
	
	if(++Seg_Pos == 8) Seg_Pos = 0;//数码管显示专用
	if(Seg_Buf[Seg_Pos]>='.')
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos] - '.',1);
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
	
	
	if(Seg_Disp_Mode == 2)
	{
		if(++Timer_1s == 1000)
		{
			Timer_1s = 0;
			if(--Timer_Para_Ctrol == 255)
			{
				Seg_Disp_Mode = 4;					
			}		
		}
	}
}

/* Main */
void main()
{
	Sys_Init();
	Timer1_Init();
	Timer_Para_Ctrol = Timer_Para;
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		AD_Proc();
		
		
	}
}



