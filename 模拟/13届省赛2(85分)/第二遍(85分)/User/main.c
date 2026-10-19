/*头文件声明区*/
#include <STC15F2K60S2.H>
#include "Key.h"
#include "Seg.h"
#include "Led.h"
#include "Init.h"
#include "ultrasound.h"
#include "iic.h"

/*变量声明区*/
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量
unsigned char Key_Slow_Down;//按键减速专用变量
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char ucLed[8]={0,0,0,0,0,0,0};//Led显示数据存放数组
unsigned char Seg_Disp_Mode;// 0-电压界面 1-参数界面 2-测距界面 
unsigned int AD_Value;
unsigned char AD_Slow_Down;
unsigned char Para_Value[2] = {0,0};
unsigned char Para_Value_Ctrl[2] = {45,5};
bit Para_Index;
unsigned char Distance_Slow_Down;
unsigned char Distance;
unsigned char i;
bit Distance_Start;
unsigned char L7_Timer;


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
			{
				Para_Value[0]	= Para_Value_Ctrl[0];
				Para_Value[1]	= Para_Value_Ctrl[1];
			}

		break;
		case 5:
			Para_Index ^= 1;
		break;
		case 6:
			if(Seg_Disp_Mode == 2)
			{
				Para_Value_Ctrl[Para_Index] = (Para_Value_Ctrl[Para_Index] == 50)?5 :Para_Value_Ctrl[Para_Index]+5;
			}
		break;
		case 7:
			if(Seg_Disp_Mode == 2)
			{
				Para_Value_Ctrl[Para_Index] = (Para_Value_Ctrl[Para_Index] == 5)?50 :Para_Value_Ctrl[Para_Index]-5;
			}
		break;
	}
}




/*信息处理函数*/
void Seg_Proc()
{
	if(Seg_Slow_Down < 100) return;
	Seg_Slow_Down = 0;//数码管减速程序
	
	
	switch(Seg_Disp_Mode)
	{
		case 0://电压界面
			Seg_Buf[0] = 11;//U 
			Seg_Buf[1] = 10;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			Seg_Buf[5] = AD_Value / 100 % 10 + '.';
			Seg_Buf[6] = AD_Value / 10 % 10;
			Seg_Buf[7] = AD_Value % 10;	
		break;
		case 2://参数界面
			Seg_Buf[0] = 12;//P 
			Seg_Buf[1] = 10;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = Para_Value_Ctrl[0] / 10 % 10+'.';
			Seg_Buf[4] = Para_Value_Ctrl[0] % 10;
			Seg_Buf[5] = 10;
			Seg_Buf[6] = Para_Value_Ctrl[1] / 10 % 10+'.';
			Seg_Buf[7] = Para_Value_Ctrl[1] % 10;
		break;
		case 1://测距界面
			Seg_Buf[0] = 13;//L  
			Seg_Buf[1] = 10;
			Seg_Buf[2] = 10;		
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			Seg_Buf[5] = (Distance_Start == 1) ? Distance / 100 % 10 : 14;//A 		
			Seg_Buf[6] = (Distance_Start == 1) ? Distance / 10 % 10 : 14;
			Seg_Buf[7] = (Distance_Start == 1) ? Distance % 10 : 14;		
		for(i=5;i<7&&Seg_Buf[i]==0;i++)
		{
			Seg_Buf[i] = 10;
		}
		break;	
	}
}

/*测距函数*/
void Distance_Proc()
{
	if(Distance_Slow_Down < 80) return;
		Distance_Slow_Down = 0;
	Distance = Ut_Wave_Data();
	
	
}

/*其他显示函数*/
void Led_Proc()
{
	ucLed[0] = (Seg_Disp_Mode == 0);
	ucLed[1] = (Seg_Disp_Mode == 1);
	ucLed[2] = (Seg_Disp_Mode == 2);
	if(Distance_Start == 1)
	{
		ucLed[7] = (L7_Timer < 100)?1:0;
	}
	else
		ucLed[7] = 0;
	Led_Disp(ucLed);
	
}

/*AD函数*/
void AD_Proc()
{
	if(AD_Slow_Down < 60)return;
		AD_Slow_Down = 0;
	AD_Value = Ad_Read(0x43) / 51.0 * 100;
	
	if(Para_Value_Ctrl[1] < AD_Value/10 && AD_Value/10 < Para_Value_Ctrl[0])
	{
		Distance_Start = 1;
	}
	else
		Distance_Start = 0;
	
	if(Distance_Start == 1)
	{
		if(Distance < 20)
		{
			Da_Write(1*51.0);
		}
		else if(Distance >= 80)
		{
			Da_Write(5*51.0);
		}
		else 
		{
			Da_Write((0.0667*Distance - 0.35)*51);
		}
	}
	else
	{
			Da_Write(0*51.0);		
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
	AD_Slow_Down++;				
	Distance_Slow_Down++;

		if(++L7_Timer == 200)L7_Timer = 0;

	
	if(++Seg_Pos == 8) Seg_Pos = 0;//数码管显示专用
	if(Seg_Buf[Seg_Pos]>='.')
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos] - '.',1);
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
	
	
}

/* Main */
void main()
{
	Sys_Init();
	PCA_Init();
	Timer1_Init();
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		AD_Proc();
		Distance_Proc();
	}
}



