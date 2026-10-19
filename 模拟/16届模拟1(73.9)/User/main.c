#include <STC15F2K60S2.H>
#include "Init.h"
#include "Key.h"
#include "Seg.h"
#include "Led.h"
#include "onewire.h"


#define abs(x)  (x) >= 0 ? (x) : -(x)
unsigned char Key_Val,Key_Down,Key_Up,Key_Old;
unsigned char Key_Slow_Down;//按键减速变量
unsigned char Seg_Pos;
unsigned char Seg_Slow_Down;//数码管减速变量
unsigned char Seg_Buf[] = {10,10,10,10,10,10,10,10};
unsigned char ucLed[] = {0,0,0,0,0,0,0,0};
unsigned char Seg_Disp_Mode;//数码管显示界面 0-温度界面 1-校准值界面 2-参数界面 

//温度变量
float Temperature;
char Calibration_Value;//校准值(-99~99)

signed char Para_Value = 26;//参数值(-99~99)
signed char Temp_Para_Value_Ctrol;//参数负数暂存值
bit Trigger_Mode;//触发模式 0-上触发 1-下触发


/*按键处理函数*/
void Key_Proc()
{
	if(Key_Slow_Down != 10)return;
	Key_Slow_Down = 0;
	
	Key_Val = Key_Read();
	Key_Down = Key_Val & (Key_Val ^ Key_Old);
	Key_Up = ~Key_Val & (Key_Val ^ Key_Old);
	Key_Old = Key_Val;
	
	switch(Key_Down)
	{
		case 4://界面切换按键
			Seg_Disp_Mode = (++Seg_Disp_Mode) % 3;
		break;
		case 5:
			Trigger_Mode ^= 1;
		break;
		case 8://减
			if(Seg_Disp_Mode == 1)//在校准值界面下
			{
				Calibration_Value = (Calibration_Value == -99) ? -99 : Calibration_Value-1;
			}
			if(Seg_Disp_Mode == 2)//在参数界面下
			{
				Para_Value =  (Para_Value == -99)?-99 : Para_Value-1;
			}
		break;
		case 9://加
			if(Seg_Disp_Mode == 1)//在校准值界面下
			{
				Calibration_Value = (Calibration_Value == 99) ? 99 : Calibration_Value+1;
			}
			if(Seg_Disp_Mode == 2)//在参数界面下
			{
				Para_Value =  (Para_Value == 99) ? 99 : Para_Value+1;
			}		
		break;
	}
	
}

/*数码管显示函数*/
void Seg_Proc()
{
	if(Seg_Slow_Down != 100)return;
	Seg_Slow_Down = 0;
	
	//温度值=DS18B20采集的环境温度+校准值
	Temperature = rd_temperature()  + Calibration_Value;
	
	switch(Seg_Disp_Mode)
	{
		case 0://温度界面
			Seg_Buf[0] = 11;//C 0110 0011 1100 0110 0xc6
			Seg_Buf[1] = 10;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			Seg_Buf[5] = Temperature >= 10 ? (unsigned char)Temperature / 10 % 10 : 10;
			Seg_Buf[6] = (unsigned char)Temperature % 10 + '.';
			Seg_Buf[7] = (signed int)(Temperature * 10) % 10;		
		break;
		case 1://校准值界面 - 1111 1101  1011 1111 0xbf
		
			Seg_Buf[0] = 12;//E 0110 0001 1000 0110 0x86
			Seg_Buf[1] = 10;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
      if(Calibration_Value < 0)
      {
        Seg_Buf[5] = Calibration_Value <= -10 ? 13 : 10;
        Seg_Buf[6] = (Seg_Buf[5] != 13 ? 13 : abs(Calibration_Value) / 10 % 10);
        Seg_Buf[7] = abs(Calibration_Value) % 10;
      }
      else
      {
        Seg_Buf[5] = 10;
        Seg_Buf[6] = Calibration_Value >= 10 ? Calibration_Value / 10 % 10 : 10;
        Seg_Buf[7] = Calibration_Value % 10;
      }
		break;
		case 2://参数界面 H 1001 0001 1000 1001 0x89
			Seg_Buf[0] = 14;//H
			Seg_Buf[1] = 10;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
      if(Para_Value < 0)
      {
        Seg_Buf[5] = Para_Value <= -10 ? 13 : 10;
        Seg_Buf[6] = (Seg_Buf[5] != 13 ? 13 : abs(Para_Value) / 10 % 10);
        Seg_Buf[7] = abs(Para_Value) % 10;
      }
      else
      {
        Seg_Buf[5] = 10;
        Seg_Buf[6] = Para_Value >= 10 ? Para_Value / 10 % 10 : 10;
        Seg_Buf[7] = Para_Value % 10;
      }

		break;	
	}
	
}
/*其他显示函数*/
void Led_Proc()
{


	ucLed[0] = (Seg_Disp_Mode == 0);
	ucLed[1] = (Seg_Disp_Mode == 1);
	ucLed[2] = (Seg_Disp_Mode == 2);
	ucLed[3] = (Trigger_Mode == 0);
	ucLed[4] = (Trigger_Mode == 1);


	if(Trigger_Mode == 0)//上触发模式
	{
		if(Temperature > Para_Value)
			ucLed[7] = 1;
	}
  else if(Trigger_Mode == 1)//下触发模式
	{
		if(Temperature < Para_Value)
			ucLed[7] = 1;
	}
	else
	{
		ucLed[7] = 0;
	}
}


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

void Timer1_Isr(void) interrupt 3
{
	Key_Slow_Down++;
	Seg_Slow_Down++;
	
	Seg_Pos = (++Seg_Pos) % 8;
	if(Seg_Buf[Seg_Pos] > 20)
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos]-'.',1);
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
		Led_Disp(ucLed);	
}
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

void main()
{
	System_Init();
	Timer1_Init();
	rd_temperature();
	Delay750ms();


	while(1)
	{
		Led_Proc();
		Key_Proc();
		Seg_Proc();

	}
}
