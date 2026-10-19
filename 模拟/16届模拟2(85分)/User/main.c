/*头文件声明区*/
#include <STC15F2K60S2.H>
#include "Key.h"
#include "Seg.h"
#include "Led.h"
#include "Init.h"
#include "ds1302.h"
#include "iic.h"


/*变量声明区*/
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量
unsigned char Key_Slow_Down;//按键减速专用变量
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char ucLed[8]={0,0,0,0,0,0,0};//Led显示数据存放数组
unsigned char RTC_Timer[] = {23,59,50};


float Light_Value;//光敏电阻电压值
float RB2_Value;//RB2电压值
unsigned char Seg_Disp_Mode;//数码管显示界面 0-时间界面 1-数据界面 2-历史查询界面 3-触发界面
unsigned char Time_Disp_Index;//查询模式下的时间索引
unsigned char Old_Seg_Disp_Mode,Old_Time_Disp_index;//触发前的状态备份

unsigned char History_Time[3][3];//存储3次触发时间3条记录*3个时间单位
unsigned char History_Time_index;//触发索引 1~3
unsigned int Timer_3000ms;//触发后3S返回
bit Trigger_Flag;//触发标志位


/*键盘处理函数*/
void Key_Proc()
{
	if(Key_Slow_Down != 10)return;
	Key_Slow_Down=0;//键盘减速程序
	
	Key_Val =Key_Read();//实时读取键码值
	Key_Down = Key_Val & (Key_Old ^ Key_Val);//判断按键是否按下
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);//捕捉按键上升沿
	Key_Old = Key_Val;//辅助扫描变量
	
	//界面切换按键
	switch(Key_Down)
	{
		case 4://界面切换按键
			if(++Seg_Disp_Mode == 3)Seg_Disp_Mode = 0;
			Time_Disp_Index = 0;
		break;
		case 5://历史查询按键
			if(Seg_Disp_Mode == 2 && ++Time_Disp_Index == 3)Time_Disp_Index = 0;
		break;
		case 8://历史清零按键
			History_Time_index = 0;
		break;
	}
}

/*信息处理函数*/
void Seg_Proc()
{
	Read_Rtc(RTC_Timer);
	
	Light_Value = Ad_Read(0x43) / 51.0 * 100;
	RB2_Value = Ad_Read(0x41) / 51.0 * 100;
	
	if(Seg_Slow_Down != 100) return;
	Seg_Slow_Down = 0;//数码管减速程序
	
	switch(Seg_Disp_Mode)
	{
		case 0://时间界面
			Seg_Buf[0] = RTC_Timer[0] / 10 % 10;
			Seg_Buf[1] = RTC_Timer[0] % 10;
			Seg_Buf[2] = 11;//- 
			Seg_Buf[3] = RTC_Timer[1] / 10 % 10;
			Seg_Buf[4] = RTC_Timer[1] % 10;
			Seg_Buf[5] = 11;//- 
			Seg_Buf[6] = RTC_Timer[2] / 10 % 10;
			Seg_Buf[7] = RTC_Timer[2] % 10;
		break;
		case 1://数据界面
			Seg_Buf[0] = 12;//P 
			Seg_Buf[1] = (unsigned int)Light_Value / 100 % 10+ '.';
			Seg_Buf[2] = (unsigned int)Light_Value / 10 % 10 ;
			Seg_Buf[3] = (unsigned int)Light_Value % 10;
			Seg_Buf[4] = 13;//U 1000 0011 1100 0001 0xc1
			Seg_Buf[5] = (unsigned int)RB2_Value / 100 % 10 + '.';
			Seg_Buf[6] = (unsigned int)RB2_Value / 10 % 10;
			Seg_Buf[7] = (unsigned int)RB2_Value % 10;
		
 		break;
		case 2://历史查询界面
			Seg_Buf[0] = 15;//A
			Seg_Buf[1] = Time_Disp_Index + 1;
			Seg_Buf[2] = Time_Disp_Index < History_Time_index ? History_Time[Time_Disp_Index][0] / 10 % 10 : 11;
			Seg_Buf[3] = Time_Disp_Index < History_Time_index ? History_Time[Time_Disp_Index][0] % 10 : 11;
			Seg_Buf[4] = Time_Disp_Index < History_Time_index ? History_Time[Time_Disp_Index][1] / 10 % 10 : 11;
			Seg_Buf[5] = Time_Disp_Index < History_Time_index ? History_Time[Time_Disp_Index][1] % 10 : 11;
			Seg_Buf[6] = Time_Disp_Index < History_Time_index ? History_Time[Time_Disp_Index][2] / 10 % 10 : 11;
			Seg_Buf[7] = Time_Disp_Index < History_Time_index ? History_Time[Time_Disp_Index][2] % 10 : 11;

		break;
		case 3://触发界面C 0110 0011 1100 0110 0xc6
			Seg_Buf[0] = 14;//C
			Seg_Buf[1] = 14;//C
			Seg_Buf[2] = History_Time[0][0] / 10 % 10;
			Seg_Buf[3] = History_Time[0][0] % 10;
			Seg_Buf[4] = History_Time[0][1] / 10 % 10;
			Seg_Buf[5] = History_Time[0][1] % 10;
			Seg_Buf[6] = History_Time[0][2] / 10 % 10;
			Seg_Buf[7] = History_Time[0][2] % 10;
		break;		
	}

}



/*其他显示函数*/
void Led_Proc()
{
	
	ucLed[0] = (Seg_Disp_Mode == 0);
	ucLed[1] = (Seg_Disp_Mode == 1);	
	ucLed[2] = (Seg_Disp_Mode == 2);
	ucLed[7] = (Seg_Disp_Mode == 3);
	Led_Disp(ucLed);
	
	
	if(RB2_Value > Light_Value)
	{
		Trigger_Flag = 1;
	}
	//触发后且非报警模式时，执行报警+记录时间
	else if(Seg_Disp_Mode != 3 && Trigger_Flag == 1)
	{
		Trigger_Flag = 0;
		Old_Seg_Disp_Mode = Seg_Disp_Mode;
		Old_Time_Disp_index = Time_Disp_Index;
		Timer_3000ms = 0;//重置3秒定时器
		Seg_Disp_Mode = 3;
		
		if(History_Time_index != 0)
		{
			History_Time[2][0] = History_Time[1][0];
			History_Time[2][1] = History_Time[1][1];
			History_Time[2][2] = History_Time[1][2];
			
			History_Time[1][0] = History_Time[0][0];
			History_Time[1][1] = History_Time[0][1];
			History_Time[1][2] = History_Time[0][2];			
		}
		History_Time[0][0] = RTC_Timer[0];
		History_Time[0][1] = RTC_Timer[1];
		History_Time[0][2] = RTC_Timer[2];

		//重难点，历史值如何加以及数值如何赋的值
		if(++History_Time_index >= 3)
			History_Time_index = 3;
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

	if(++Seg_Pos == 8) Seg_Pos = 0;//数码管显示专用
	if(Seg_Buf[Seg_Pos]>='.')
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos] - '.',1);
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
	
	if(Seg_Disp_Mode == 3 && ++Timer_3000ms == 3000)
	{
		Seg_Disp_Mode = Old_Seg_Disp_Mode;
		Time_Disp_Index = Old_Time_Disp_index;
	}

}

/* Main */
void main()
{
	Sys_Init();
	Set_Rtc(RTC_Timer);
	Timer1_Init();
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();

	}
}
