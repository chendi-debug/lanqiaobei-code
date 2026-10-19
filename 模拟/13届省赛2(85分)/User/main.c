#include <STC15F2K60S2.H>
#include "Key.h"
#include "Seg.h"
#include "Led.h"
#include "Init.h"
#include "iic.h"
#include "Ultrasound.h"

/*变量声明区*/
unsigned char Key_Val,Key_Down,Key_Up,Key_Old;
unsigned char Key_Slow_Down;//按键减速变量
pdata unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};
idata unsigned int Seg_Slow_Down;//数码管减速变量
unsigned char Seg_Pos;
pdata unsigned char ucLed[8] = {0,0,0,0,0,0,0,0};//Led显示数据存放数组
unsigned char Seg_Disp_Mode;//数码管显示模式 0-电压界面 1-测距界面 2-参数界面

idata unsigned int Voltage_Value;//电压变量
idata unsigned int Voltage_Buf[] = {45,5};//电压参数实际值(上下限)
idata unsigned char Voltage_Buf_Ctrol[] = {45,5};//电压参数控制值(上下限)
bit Voltage_Flag = 0;//数组索引值
idata unsigned char Distance_Value;//测距变量
idata bit Start_Ultrasound;//是否启动测距 0-不启动 1-启动
idata unsigned char i;
unsigned char Time_100ms;
bit Flash_Flag;
unsigned char AD_Slow_Down;
/*按键处理函数*/
void Key_Proc()
{
	if(Key_Slow_Down<100)return;
		Key_Slow_Down = 0;
	
	Key_Val = Key_Read();
	Key_Down = Key_Val & (Key_Old ^ Key_Val);
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);
	Key_Old = Key_Val;
	
	switch(Key_Down)
	{
		case 4://界面切换按键
			//当前处于测距界面，真实值给控制值
			if(Seg_Disp_Mode == 1)
			{
					Voltage_Buf_Ctrol[0] = Voltage_Buf[0];
					Voltage_Buf_Ctrol[1] = Voltage_Buf[1];
			}
			//当前处于参数界面，控制值给真实值
			if(Seg_Disp_Mode == 2)
			{
				if(Voltage_Buf_Ctrol[0]>Voltage_Buf_Ctrol[1])//判断合理性
				{
						Voltage_Buf[0] = Voltage_Buf_Ctrol[0];	
						Voltage_Buf[1] = Voltage_Buf_Ctrol[1];					
				}
			}
			if(++Seg_Disp_Mode == 3)
				Seg_Disp_Mode = 0;
		break;
		case 5:
			if(Seg_Disp_Mode == 2)
				Voltage_Flag ^= 1;
		break;
		case 6://参数加
			if(Seg_Disp_Mode == 2)
			{
				Voltage_Buf_Ctrol[Voltage_Flag] = (Voltage_Buf_Ctrol[Voltage_Flag]==50)?5:Voltage_Buf_Ctrol[Voltage_Flag]+5;
			}
		break;
		case 7://参数减
			if(Seg_Disp_Mode == 2)
			{
				Voltage_Buf_Ctrol[Voltage_Flag] = (Voltage_Buf_Ctrol[Voltage_Flag]==5)?50:Voltage_Buf_Ctrol[Voltage_Flag]-5;
			}			
		break;
	}
	
	
}


/*数码管处理函数*/
void Seg_Proc()
{
	if(Seg_Slow_Down<80)return;
		Seg_Slow_Down = 0;
	
	Voltage_Value = Ad_Read(0x43) / 51.0 * 100;
	Distance_Value = Ut_Wave_Data();
	
	if(Voltage_Buf[1] * 10 < Voltage_Value && Voltage_Buf[0] * 10 > Voltage_Value)
	{
		Start_Ultrasound = 1;//开启连续测距模式
	}	
	else 
		Start_Ultrasound = 0;//关闭
		
	
	switch(Seg_Disp_Mode)
	{
		case 0://电压界面 1000 0011  1100 0001
			Seg_Buf[0] = 11;//U
			Seg_Buf[1] = 10;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			Seg_Buf[5] = Voltage_Value / 100 % 10 + '.';
			Seg_Buf[6] = Voltage_Value / 10 % 10;
			Seg_Buf[7] = Voltage_Value % 10;		
		break;
		case 1://测距界面 
			Seg_Buf[0] = 13;//L
			Seg_Buf[1] = 10;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			if(Start_Ultrasound)//如果测距启动
			{
				Seg_Buf[5] = Distance_Value / 100 % 10 ;
				Seg_Buf[6] = Distance_Value / 10 % 10 ;
				Seg_Buf[7] = Distance_Value % 10;	
				for(i=5;i<7 && Seg_Buf[i] == 0;i++)
					Seg_Buf[i] = 10;
			}
			else//不启动
			{
				Seg_Buf[5] = 14;//A
				Seg_Buf[6] = 14;//A
				Seg_Buf[7] = 14;//A
			}
		break;
		case 2://参数界面 
			Seg_Buf[0] = 12;//P
			Seg_Buf[1] = 10;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = Voltage_Buf[0] / 10 % 10 + '.';
			Seg_Buf[4] = Voltage_Buf[0] % 10;
			Seg_Buf[5] = 10;
			Seg_Buf[6] = Voltage_Buf[1] / 10 % 10 + '.';
			Seg_Buf[7] = Voltage_Buf[1] % 10;	
		break;
	}
	
}

/*Led处理函数*/
void Led_Proc()
{
	Led_Disp(ucLed);
	
	ucLed[0] = (Seg_Disp_Mode == 0);
	ucLed[1] = (Seg_Disp_Mode == 1);
	ucLed[2] = (Seg_Disp_Mode == 2);
//	if(Start_Ultrasound == 1)
//		ucLed[7] = (Flash_Flag == 1);
//	else if(Start_Ultrasound == 0)
//		ucLed[7] = 0;


}

void AD_Proc(void)
{
	if(++AD_Slow_Down<60)return;
		AD_Slow_Down = 0;
	/*DA*/
	if(Start_Ultrasound == 1)
	{
		if(Distance_Value < 20)
		{
			Da_Write(1*51.0f);//1V
		}
		else if(Distance_Value > 80)
		{
			Da_Write(5*51.0f);//5V			
		}
		else
		{
			Da_Write((0.067 *Distance_Value - 0.34)*51);
		}
	}
	else
	{
		Da_Write(0.0f);
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
	AD_Slow_Down++;
	
	if(++Seg_Pos == 8)Seg_Pos = 0; //动态扫描
	if(Seg_Buf[Seg_Pos]>='.')
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos] - '.',1);//显示小数点
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);//不显示小数点
	
	if(++Time_100ms == 200)
	{
		Time_100ms = 0;
//		Flash_Flag = 1;
		if(Start_Ultrasound == 1)
			ucLed[7] = 1;
	}
	else
//		Flash_Flag = 0; 
		ucLed[7] = 0;
	if(Start_Ultrasound == 0)
		ucLed[7] = 0;
	
}
void main()
{
	
	System_Init();
	Timer1_Init();
	PCA_Init();
	while(1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		AD_Proc();
	}
}