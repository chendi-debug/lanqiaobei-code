/*头文件声明区*/
#include <STC15F2K60S2.H>
#include "Key.h"
#include "Seg.h"
#include "Led.h"
#include "Init.h"
#include "ultrasound.h"
#include "onewire.h"
#include "iic.h"


/*变量声明区*/
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量
unsigned char Key_Slow_Down;//按键减速专用变量
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char ucLed[8]={0,0,0,0,0,0,0};//Led显示数据存放数组
unsigned char Ad_Slow_Down;
unsigned char Tempera_Slow_Down;
unsigned int Diatance_Slow_Down;
unsigned char i;

unsigned char Seg_Disp_Mode;//0-环境状态界面 1-运动检测界面  2-参数设置界面  3-统计数据界面
bit Para_Mode;//0-温度参数 1-距离参数
bit Lock_Flag = 0;//参数界面锁定标志
unsigned char Tempera;//温度变量
unsigned char Light_Level = 0;//光强等级
float AD_Value;//AD值
unsigned int Diatance;//距离变量
unsigned char Sport_Mode;//运动状态 1-静止 2-徘徊 3-跑动
unsigned char Tempera_Para = 30;//温度参数
unsigned char Distance_Para = 30;//距离参数
#define abs(x) (x<0)?-(x):(x)
bit Hot_Flag = 0;//高温判定 
bit Near_Flag = 0;//接近标志
unsigned int Relay_Count = 0;
static int Last_State = 0;
unsigned int Current_Condition;
bit Timer_Flag = 0;
unsigned int Timer_2s;
unsigned char L8_Blink_Count = 0;//L8闪烁计数器
unsigned int Sport_Lock_Count = 0;//运动状态锁定计数器


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
			if(++Seg_Disp_Mode == 4)
				Seg_Disp_Mode = 0;
			Para_Mode = 0;
			Lock_Flag = (Seg_Disp_Mode == 2) ? 1 : 0;
		break;
		case 5:
			if(Seg_Disp_Mode == 2)
			{
				Para_Mode ^= 1;
			}
		break;
		case 8:
			if(Para_Mode == 0)//温度参数界面
			{
				Tempera_Para = (Tempera_Para >= 80) ? 80 : Tempera_Para +1;
			}
			if(Para_Mode == 1)//距离参数界面
			{
				Distance_Para = (Distance_Para >= 80) ? 80 : Distance_Para + 5;
			}
		break;
		case 9:
			if(Para_Mode == 0)//温度参数界面
			{
				Tempera_Para = (Tempera_Para <= 20) ? 20 : Tempera_Para - 1;
			}
			if(Para_Mode == 1)//距离参数界面
			{
				Distance_Para = (Distance_Para <= 20) ? 20 : Distance_Para - 5;
			}
		break;
		case 17:
			if(Seg_Disp_Mode == 3)
				Timer_Flag = 1;
		break;			
	}
	switch(Key_Val)
	{

	}
}




/*信息处理函数*/
void Seg_Proc()
{
	if(Seg_Slow_Down < 100) return;
	Seg_Slow_Down = 0;//数码管减速程序

	
	switch(Seg_Disp_Mode)
	{
		case 0://环境状态界面
			Seg_Buf[0] = 11;//C
			Seg_Buf[1] = Tempera / 10 % 10;
			Seg_Buf[2] = Tempera % 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			Seg_Buf[5] = 10;
			Seg_Buf[6] = 12;//n
			Seg_Buf[7] = Light_Level;
		break;
		case 1://运动检测界面
			Seg_Buf[0] = 13;//L 
			Seg_Buf[1] = Sport_Mode;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			Seg_Buf[5] = Diatance / 100 % 10;
			Seg_Buf[6] = Diatance / 10 % 10;
			Seg_Buf[7] = Diatance % 10;
		break;
		case 2://参数设置界面
			Seg_Buf[0] = 14;//P
			if(Para_Mode == 0)				
				Seg_Buf[1] = 11;//C			//温度参数
			else												
				Seg_Buf[1] = 13;//L			//距离参数
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			Seg_Buf[5] = 10;
			if(Para_Mode == 0)//温度参数
			{
				Seg_Buf[6] = Tempera_Para / 10 % 10;
				Seg_Buf[7] = Tempera_Para % 10;	
			}
			else
			{
				Seg_Buf[6] = Distance_Para / 10 % 10;
				Seg_Buf[7] = Distance_Para % 10;					
			}				
		break;
		case 3://统计数据界面
			Seg_Buf[0] = 12;//n
			Seg_Buf[1] = 11;//C
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = Relay_Count / 1000 % 10;
			Seg_Buf[5] = Relay_Count / 100 % 10;
			Seg_Buf[6] = Relay_Count / 10 % 10;
			Seg_Buf[7] = Relay_Count % 10;
		for(i=4;i<7&&Seg_Buf[i]==0;i++)
			Seg_Buf[i] = 10;
		break;
	}
}

/*距离获取函数*/
void Get_Distance_Proc()
{
	static float Old1 = 0;
	unsigned char D_Diatance = 0;
	unsigned char New_Sport_Mode;

	if(Diatance_Slow_Down < 1000)return;//每隔一秒采集
		Diatance_Slow_Down = 0;

	/*下面三句代码必须要在前面*/
	Diatance = Ut_Wave_Data();
	D_Diatance = abs(Diatance - Old1);
	Old1 = Diatance;

	if(Sport_Lock_Count == 0)//未锁定时才判定
	{
		if(D_Diatance < 5)
			New_Sport_Mode = 1;//静止
		else if(D_Diatance >= 5 && D_Diatance < 10)
			New_Sport_Mode = 2;//徘徊
		else
			New_Sport_Mode = 3;//跑动

		if(New_Sport_Mode != Sport_Mode)
		{
			Sport_Mode = New_Sport_Mode;
			Sport_Lock_Count = 3000;//锁定3秒
		}
	}


	if(Diatance < Distance_Para)
		Near_Flag = 1;
	else
		Near_Flag = 0;

}

/*其他显示函数*/
void Led_Proc()
{
	if(Lock_Flag) return;//参数界面锁定

	if(Near_Flag)
	{
		if(Light_Level == 1)
		{
			ucLed[0] = 1;ucLed[1] = ucLed[2] = ucLed[3] = 0;
		}
		else if(Light_Level == 2)
		{
			ucLed[0] = ucLed[1] = 1;ucLed[2] = ucLed[3] = 0;
		}
		else if(Light_Level == 3)
		{
			ucLed[0] = ucLed[1] = ucLed[2] = 1; ucLed[3] = 0;			
		}
		else if(Light_Level == 4)
		{
			ucLed[0] = ucLed[1] = ucLed[2] = ucLed[3] = 1;						
		}
	}
	else
	{
			ucLed[0] = ucLed[1] = ucLed[2] = ucLed[3] = 0;
	}
	if(Sport_Mode == 1)//静止 熄灭
	{
		ucLed[7] = 0;
	}
	else if(Sport_Mode == 2)//徘徊 点亮
	{
		ucLed[7] = 1;
	}
	else if(Sport_Mode == 3)//跑动 闪烁
	{
		ucLed[7] = (L8_Blink_Count < 100) ? 1 : 0;
	}
	Led_Disp(ucLed);
	 
	Current_Condition = (Near_Flag == 1 && Hot_Flag == 1) ? 1 : 0;
	
	if(Current_Condition == 1)
	{
		Relay(1);
	}
	else
	{
		Relay(0);
	}
	
	if(Current_Condition == 1 && Last_State == 0)
	{
		Relay_Count = Relay_Count+1;
	}
	Last_State = Current_Condition;
}
/*AD转换函数*/
void AD_Proc()
{
	if(Ad_Slow_Down < 60)return;
	Ad_Slow_Down = 0;
	AD_Value = Ad_Read(0x41) / 51.0;
	if(AD_Value >= 3)
		Light_Level = 1;
	else if(AD_Value >= 2 && AD_Value < 3.0)
		Light_Level = 2;
	else if(AD_Value >= 0.5 && AD_Value < 2)
		Light_Level = 3;
  else if(AD_Value < 0.5)
		Light_Level = 4;
}


/*温度获取函数*/
void Tempera_Proc()
{
	if(Tempera_Slow_Down < 70)return;
	Tempera_Slow_Down = 0;
	
	Tempera = rd_temperature();
	
	if(Tempera > Tempera_Para)
		Hot_Flag = 1;
	else
		Hot_Flag = 0;
	
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
	Tempera_Slow_Down++;
	Diatance_Slow_Down++;
	if(++L8_Blink_Count == 200) L8_Blink_Count = 0;//0.2秒周期
	if(Sport_Lock_Count > 0) Sport_Lock_Count--;//运动状态锁定递减

	
	if(++Seg_Pos == 8) Seg_Pos = 0;//数码管显示专用
	if(Seg_Buf[Seg_Pos]>='.')
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos] - '.',1);
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
	
	if(Timer_Flag)
	{
		if(++Timer_2s == 2000)
		{
		
			if(Key_Val == 17)
			{
				Timer_2s = 0;
				Relay_Count = 0;
				Timer_Flag = 0;
			}
		}
	}
	
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

/* Main */
void main()
{
	Sys_Init();
	PCA_Init();
	Timer1_Init();
	rd_temperature();
	Delay750ms();
	
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		Tempera_Proc();
		AD_Proc();
		Get_Distance_Proc();
	}
}



