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
unsigned char Seg_Disp_Mode;//0-湿度界面 1-参数界面 2-时间界面
float Voltage;//电压变量
unsigned char Humidity;//湿度变量
unsigned char Humidity_Para;//湿度变量
unsigned char Humidity_Para_Ctrol = 50;//湿度变量
unsigned char Timer = 3;//时间间隔变量
bit Relay_Flag;//0-停止 1-启动
unsigned int Timer_1000x;


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
				Humidity_Para = Humidity_Para_Ctrol;		
		break;
		case 5:
			Relay_Flag ^= 1;
		break;
		case 8:
			if(Seg_Disp_Mode == 1)
				Humidity_Para_Ctrol = (Humidity_Para_Ctrol == 30) ? 90 :Humidity_Para_Ctrol - 5;
			if(Seg_Disp_Mode == 2)
				Timer = (Timer == 1) ? 10 : Timer - 1;
		break;
		case 9:
			if(Seg_Disp_Mode == 1)
				Humidity_Para_Ctrol = (Humidity_Para_Ctrol == 90) ? 30 :Humidity_Para_Ctrol + 5;
			if(Seg_Disp_Mode == 2)
				Timer = (Timer == 1) ? 10 : Timer + 1;		
			break;
	}
}




/*信息处理函数*/
void Seg_Proc()
{
	if(Seg_Slow_Down < 100) return;
	Seg_Slow_Down = 0;//数码管减速程序

	Timer_1000x = Timer * 1000;
	
		/*AD*/
	Voltage = Ad_Read(0x43) / 51.0;
	
	if(Voltage > 4.0)
		Humidity = 90;
	else if(Voltage < 1.0)
		Humidity = 10;
	else 
	{

		Humidity = 26.7 * Voltage - 17;
	}
	
	switch(Seg_Disp_Mode)
	{
		case 0://湿度界面C 0110 0011 1100 0110 0xc6
			Seg_Buf[0] = 11;//C
			Seg_Buf[6] = Humidity / 10 % 10;
			Seg_Buf[7] = Humidity % 10;
		break;
		case 1://参数界面
			Seg_Buf[0] = 12;//E 0110 0001 1000 0110 0x86
			Seg_Buf[6] = Humidity_Para_Ctrol / 10 % 10;
			Seg_Buf[7] = Humidity_Para_Ctrol % 10;	
			if(Seg_Buf[6] == 0)
				Seg_Buf[6] = 10;
		break;
		case 2://时间界面
			Seg_Buf[0] = 13;//H 1001 0001 1000 1001 0x89
			Seg_Buf[6] = Timer / 10 % 10;
			Seg_Buf[7] = Timer % 10;	
			if(Seg_Buf[6] == 0)
				Seg_Buf[6] = 10;			
		break;
	}

}



/*其他显示函数*/
void Led_Proc()
{
	ucLed[0] = (Seg_Disp_Mode == 0);
	ucLed[1] = (Seg_Disp_Mode == 1);
	ucLed[2] = (Seg_Disp_Mode == 2);
	ucLed[7] = (Relay_Flag);
	
	Led_Disp(ucLed);
	
	if(Relay_Flag == 1)//在启动状态下
	{
		if(Humidity < Humidity_Para)
		{
			Relay(1);
		}
		else
		{
			Relay(0);
			Timer_1000x = Timer * 1000;
		}
	}
	else
	{
		Relay(0);
		Timer_1000x = Timer * 1000;		
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
	if((Relay_Flag == 1) && (Humidity < Humidity_Para))
	{
		if(Timer_1000x > 0)
		{
		 Timer_1000x--;
		}
		else
		{
			Timer_1000x = Timer * 1000;
		}
	}
	if(Timer_1000x == 0)
	{
		Relay(0);
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



