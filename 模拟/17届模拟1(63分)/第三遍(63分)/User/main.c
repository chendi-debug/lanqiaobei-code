/*头文件声明区*/
#include <STC15F2K60S2.H>
#include "Key.h"
#include "Seg.h"
#include "Led.h"
#include "Init.h"
#include "ultrasound.h"
#include "ds1302.h"

/*变量声明区*/
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量
unsigned char Key_Slow_Down;//按键减速专用变量
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char ucLed[8]={0,0,0,0,0,0,0};//Led显示数据存放数组
unsigned char Seg_Disp_Mode;//0-数据界面 1-时间界面 2-记录界面 
unsigned int Distance;
unsigned char i;
unsigned char RTC_Timer[] = {23,59,50};
unsigned char Distance_Slow_Down;
unsigned char Record_Index = 0;
unsigned int Recout_Current;
static unsigned int Recount_Old;
unsigned char Record_Timer[3][3] = {0,0,0};
bit Alarm_Flag;


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
			Record_Index = 0;
		break;
		case 5:
			if(++Record_Index == 3)
				Record_Index = 0;
		break;
		case 8:
			if(Seg_Disp_Mode == 2)//记录界面
			{
				Record_Timer[Record_Index][0]=0;
				Record_Timer[Record_Index][0]=0;
				Record_Timer[Record_Index][1]=0;
				Record_Timer[Record_Index][1]=0;
				Record_Timer[Record_Index][2]=0;
				Record_Timer[Record_Index][2]=0;
			}
		break;
		case 9:
			if(Alarm_Flag == 1)
			{
				Alarm_Flag = 0;
				Beep(0);
			}
		break;
	}
}




/*信息处理函数*/
void Seg_Proc()
{
	if(Seg_Slow_Down < 100) return;
	Seg_Slow_Down = 0;//数码管减速程序

	Read_Rtc(RTC_Timer);
	
	switch(Seg_Disp_Mode)
	{
		case 0://数据界面
			Seg_Buf[0] = 11;//L
			Seg_Buf[1] = 10;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			Seg_Buf[5] = Distance / 100 % 10;
			Seg_Buf[6] = Distance / 10 % 10;
			Seg_Buf[7] = Distance % 10;
			for(i=5;i<7&&Seg_Buf[i]==0;i++)
				Seg_Buf[i] = 10;
		break;
		case 1://时间界面
			Seg_Buf[0] = RTC_Timer[0] / 10 % 10;
			Seg_Buf[1] = RTC_Timer[0] % 10;
			Seg_Buf[2] = 12;//-
			Seg_Buf[3] = RTC_Timer[1] / 10 % 10;
			Seg_Buf[4] = RTC_Timer[1] % 10;
			Seg_Buf[5] = 12;//-
			Seg_Buf[6] = RTC_Timer[2] / 10 % 10;
			Seg_Buf[7] = RTC_Timer[2] % 10;
		
		break;
		case 2://记录界面
			Seg_Buf[0] = 13;//A
			Seg_Buf[1] = Record_Index + 1;
			Seg_Buf[2] = (Record_Timer[Record_Index][0]==0)?12:Record_Timer[Record_Index][0] / 10 % 10;
			Seg_Buf[3] = (Record_Timer[Record_Index][0]==0)?12:Record_Timer[Record_Index][0] % 10;
			Seg_Buf[4] = (Record_Timer[Record_Index][1]==0)?12:Record_Timer[Record_Index][1] / 10 % 10;
			Seg_Buf[5] = (Record_Timer[Record_Index][1]==0)?12:Record_Timer[Record_Index][1] % 10;
			Seg_Buf[6] = (Record_Timer[Record_Index][2]==0)?12:Record_Timer[Record_Index][2] / 10 % 10;
			Seg_Buf[7] = (Record_Timer[Record_Index][2]==0)?12:Record_Timer[Record_Index][2] % 10;
		break;
	}
}



/*其他显示函数*/
void Led_Proc()
{
	ucLed[0] = (Seg_Disp_Mode == 0);
	ucLed[1] = (Seg_Disp_Mode == 1);
	ucLed[2] = (Seg_Disp_Mode == 2);

	Led_Disp(ucLed);
	
	if(Recout_Current == 1 && Alarm_Flag == 1)
	{
		Beep(1);
	}
	else
	{
		Beep(0);
		Alarm_Flag = 0;
	}
}

/*距离显示函数*/
void Distance_Proc()
{
	if(Distance_Slow_Down < 80)return;
		Distance_Slow_Down = 0;
	Distance = Ut_Wave_Data();
	
	Recout_Current = (Distance < 30) ? 1 : 0;
	if(Recout_Current == 1 && Recount_Old == 0)
	{

		Record_Timer[1][0] = Record_Timer[2][0];
		Record_Timer[1][1] = Record_Timer[2][1];
		Record_Timer[1][2] = Record_Timer[2][2];


		Record_Timer[0][0] = Record_Timer[1][0];
		Record_Timer[0][1] = Record_Timer[1][1];
		Record_Timer[0][2] = Record_Timer[1][2];
		
		Record_Timer[0][0] = RTC_Timer[0];
		Record_Timer[0][1] = RTC_Timer[1];
		Record_Timer[0][2] = RTC_Timer[2];
		Alarm_Flag = 1;	
	}
	Recount_Old = Recout_Current;
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
	Distance_Slow_Down++;
	
	
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
	Set_Rtc(RTC_Timer);
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		Distance_Proc();
	}
}



