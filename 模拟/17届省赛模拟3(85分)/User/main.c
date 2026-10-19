#include <STC15F2K60S2.H>
#include "Seg.h"
#include "Init.h"
#include "Led.h"
#include "Key.h"
#include "iic.h"


/******************************************************************************/
idata unsigned char Led_Buf[] = {0,0,0,0,0,0,0,0};
idata unsigned char Seg_Buf[] = {10,10,10,10,10,10,10,10};
unsigned char Seg_Pos;
unsigned char Key_Slow_Down;
unsigned char Key_Val,Key_Down,Key_Up,Key_Old;
unsigned char Seg_Slow_Down;
unsigned int ad_Val_100x;
idata unsigned char Seg_Disp_Mode;//数码管显示界面 0-准备界面H 1-设置界面U 2-比赛界面A 3-结果界面E
idata unsigned char Time_Value = 5;//时间参数
idata unsigned char Time_Value_Ctrol;//实际时间参数
idata unsigned char Competition_results;//比赛结果参数
idata unsigned int Time_1000ms;//1000ms变量

/******************************************************************************/
void Key_Proc()
{
	if(Key_Slow_Down<10)return;
	Key_Slow_Down = 0;
	
	Key_Val = Key_Read();
	Key_Down = Key_Val&(Key_Val ^ Key_Old);
	Key_Up = ~Key_Val&(Key_Val ^ Key_Old);
	
	if(Seg_Disp_Mode >= 3)//在结果界面下
	{
		if(Key_Old == 0 && Key_Val)//如果所有选手都松手了，并且再次按下了按键
			Seg_Disp_Mode = 0;//跳转到准备界面
	}
	Key_Old = Key_Val;
	
	switch(Key_Down)
	{
		case 4:
			
		break;
		case 5:
			
		break;
		case 6:
			if(Seg_Disp_Mode == 1)//在设置界面
			{
				if(--Time_Value == 255)
					Time_Value = 0;
			}
		break;
		case 7:
			if(Seg_Disp_Mode == 1)//在设置界面
			{
				if(++Time_Value == 100)
					Time_Value = 99;
			}			
		break;

		
	}
	switch(Key_Val)
	{
		case 22:
			if(Seg_Disp_Mode == 0)
			{
				Seg_Disp_Mode = 2;
				Time_Value_Ctrol = Time_Value;
			}
		break;
	}
	
	if(Seg_Disp_Mode == 2)//在比赛界面
	{
		if(Key_Val !=22)//如果有人抬起按键
		{
			Seg_Disp_Mode = 3;//转到淘汰界面
			Competition_results = 22-Key_Val;
		}
	}
	
}
void Seg_Proc()
{
	if(Seg_Slow_Down<80)return;
	Seg_Slow_Down = 0;
	
	ad_Val_100x = Ad_Read(0x41)/51.0f*100;
	
	
	if((Seg_Disp_Mode == 1) && (ad_Val_100x > 120))//在设置界面下，检测到亮
		Seg_Disp_Mode = 0;
	if((Seg_Disp_Mode == 0) && (ad_Val_100x <= 120))//在准备界面下，检测到暗
		Seg_Disp_Mode = 1;
	
	switch(Seg_Disp_Mode)
	{
		case 0://准备界面
			Seg_Buf[0] = 11;//H
			Seg_Buf[1] = 12;//E
			Seg_Buf[2] = 13;//L
			Seg_Buf[3] = 13;//L
			Seg_Buf[4] = 0;//O
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
			Seg_Buf[6] = Time_Value / 10 % 10;
			Seg_Buf[7] = Time_Value % 10;
		
		break;
		case 2://比赛界面
			Seg_Buf[0] = 15;//A
			Seg_Buf[1] = 10;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			Seg_Buf[5] = 10;
			Seg_Buf[6] = Time_Value_Ctrol / 10 % 10;
			Seg_Buf[7] = Time_Value_Ctrol % 10;			
		break;
		case 3://结果淘汰界面
			Seg_Buf[0] = 12;//E
			Seg_Buf[1] = 10;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			Seg_Buf[5] = 10;
			Seg_Buf[6] = 10;
			Seg_Buf[7] = Competition_results % 10;				
		break;
		case 4://倒计时结束界面
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
/*LED处理函数*/
void Led_Proc()
{
	Led_Disp(Led_Buf);

	Led_Buf[0] = (Seg_Disp_Mode == 0);
	Led_Buf[1] = (Seg_Disp_Mode == 1);
	Led_Buf[2] = (Seg_Disp_Mode == 2);
	Led_Buf[3] = (Seg_Disp_Mode == 3);
	Led_Buf[4] = (Seg_Disp_Mode == 4);
}


/*中断函数*/

void Timer1_Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0xBF;			//定时器时钟12T模式
	TMOD &= 0x0F;			//设置定时器模式
	TL1 = 0x18;				//设置定时初始值
	TH1 = 0xFC;				//设置定时初始值
	TF1 = 0;				//清除TF1标志
	TR1 = 1;				//定时器1开始计时
	EA = 1;
	ET1 = 1;	
}


void Timer1_Isr(void) interrupt 3
{
	Key_Slow_Down++;
	Seg_Slow_Down++;
	
	if(++Seg_Pos == 8)Seg_Pos = 0;
	if(Seg_Buf[Seg_Pos]>='.')
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos] - '.',1);
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
	if(Seg_Disp_Mode == 2) //在比赛界面
	{
		if(++Time_1000ms == 1000)
		{
			Time_1000ms = 0;
			if(--Time_Value_Ctrol == 255)//当减到0时
			{
				Seg_Disp_Mode = 4;
			}
		}
	}
}

/******************************************************************************/

void main()
{
	System_Init();
	Timer1_Init();
	
	
	while(1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
	}
}
