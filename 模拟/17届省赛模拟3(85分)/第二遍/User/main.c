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
unsigned char Seg_Disp_Mode;// 0-准备界面 1-设置界面 2-比赛界面 3-结果界面
unsigned char Time_Para = 5;//时间参数
unsigned int ad_Val_100x;
unsigned int Timer_1000;
unsigned char Comperature;//比赛结果界面
/*键盘处理函数*/

void Key_Proc()
{
	if(Key_Slow_Down < 10)return;
	Key_Slow_Down=0;//键盘减速程序
	
	Key_Val = Key_Read();//实时读取键码值
	Key_Down = Key_Val & (Key_Old ^ Key_Val);//判断按键是否按下
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);//捕捉按键上升沿


	if(Seg_Disp_Mode >= 3)//在结果界面下
	{
		if(Key_Old == 0 && Key_Val)//如果所有选手都松手了，并且再次按下了按键
			Seg_Disp_Mode = 0;//跳转到准备界面
	}	
	Key_Old = Key_Val;	
	switch(Key_Down)
	{
		case 4://4号

		break;
		case 5://5号
			
		break;
		case 6:
			if(Seg_Disp_Mode == 1)//参数减
			{
				Time_Para = (Time_Para == 0) ? 0 : Time_Para-1;
			}
		break;
		case 7:
			if(Seg_Disp_Mode == 1)//参数加
			{
				Time_Para = (Time_Para == 99) ? 99 : Time_Para+1;
			}			
		break;
	}
	switch(Key_Val)
	{
		case 22:
			if(Seg_Disp_Mode == 0)
				Seg_Disp_Mode = 2;
		break;
			
	}
	if(Seg_Disp_Mode == 2)
	{
		if(Key_Val != 22)
		{
			Seg_Disp_Mode = 3;
			Comperature = 22-Key_Val;
		}
	}
//	if(Seg_Disp_Mode == 3 || Seg_Disp_Mode ==4)
//	{
//		if(Key_Down != 0)
//			Seg_Disp_Mode = 0;
//	}
}




/*信息处理函数*/
void Seg_Proc()
{
	if(Seg_Slow_Down < 100) return;
	Seg_Slow_Down = 0;//数码管减速程序
	
	ad_Val_100x = Ad_Read(0x41)/51.0f*100;
	
	if((Seg_Disp_Mode == 1) && (ad_Val_100x >= 100))
		Seg_Disp_Mode = 0;	
	if((Seg_Disp_Mode == 0) && (ad_Val_100x < 100))
		Seg_Disp_Mode = 1;	

	
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
		  Seg_Buf[6] = Time_Para / 10 % 10;
		  Seg_Buf[7] = Time_Para % 10;
		break;
		case 2://比赛界面
			Seg_Buf[0] = 15;//A
			Seg_Buf[1] = 10;
		  Seg_Buf[2] = 10;
		  Seg_Buf[3] = 10;
		  Seg_Buf[4] = 10;
		  Seg_Buf[5] = 10;
		  Seg_Buf[6] = Time_Para / 10 % 10;
		  Seg_Buf[7] = Time_Para % 10;		
		break;
		case 3://结果界面
			Seg_Buf[0] = 12;//E	
			Seg_Buf[1] = 10;
      Seg_Buf[2] = 10;
      Seg_Buf[3] = 10;
      Seg_Buf[4] = 10;
      Seg_Buf[5] = 10;
      Seg_Buf[6] = 10;
			Seg_Buf[7] = Comperature % 10;		
		break;
		case 4:
			Seg_Buf[0] = 12;//E	
			Seg_Buf[1] = 10;
      Seg_Buf[2] = 10;
      Seg_Buf[3] = 10;
      Seg_Buf[4] = 10;
      Seg_Buf[5] = 10;
      Seg_Buf[6] = 12;//E
			Seg_Buf[7] = 12;//E	
	}

}



/*其他显示函数*/
void Led_Proc()
{
	ucLed[0] = (Seg_Disp_Mode == 0);
	ucLed[1] = (Seg_Disp_Mode == 1);
	ucLed[2] = (Seg_Disp_Mode == 2);
	ucLed[3] = (Seg_Disp_Mode == 3);
	ucLed[3] = (Seg_Disp_Mode == 4);
	
	Led_Disp(ucLed);

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
	
	if(Seg_Disp_Mode == 2)//在比赛界面
	{
		if(++Timer_1000 == 1000)
		{
			Timer_1000 = 0;
			if(--Time_Para == 255)
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
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();

	}
}



