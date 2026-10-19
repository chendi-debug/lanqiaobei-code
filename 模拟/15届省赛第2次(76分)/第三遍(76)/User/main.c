/*头文件声明区*/
#include <STC15F2K60S2.H>
#include "Key.h"
#include "Seg.h"
#include "Led.h"
#include "Init.h"
#include "onewire.h"
#include "iic.h"

/*变量声明区*/
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量
unsigned char Key_Slow_Down;//按键减速专用变量
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char ucLed[8]={0,0,0,0,0,0,0};//Led显示数据存放数组
unsigned char Temp_Slow_Down;
unsigned char DAC_Slow_Down;
unsigned char Seg_Disp_Mode;//0-温度界面 1-DAC输出控制界面 2-校准值界面
unsigned char Temperature;
signed char Calibration_Value = 0;//校准值
unsigned char DAC_Mode;//DAC输出模式 0-温度控制 1-手动控制
unsigned char Hand_DAC = 100;
#define abs(x) (x<0)?-(x):(x)
bit Press_Long=0;//长按标志位
unsigned int Timer_1500ms;
bit Relay_Staic = 0;//继电器状态 0-解锁 1-锁定
unsigned int L2_Timer,L3_Timer;


/*键盘处理函数*/

void Key_Proc()
{
	if(Key_Slow_Down < 10)return;
	Key_Slow_Down=0;//键盘减速程序
	
	Key_Val =Key_Read();//实时读取键码值
	Key_Down = Key_Val & (Key_Old ^ Key_Val);//判断按键是否按下
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);//捕捉按键上升沿
	Key_Old = Key_Val;//辅助扫描变量
	
	if(Key_Down == 13)
	{
		Press_Long = 1;
	}
	if(Key_Up == 13)
	{
		if(Timer_1500ms < 1500)
		{
			Press_Long = 0;
			Timer_1500ms = 0;
			DAC_Mode ^= 1;
		}	
		else
		{
			Press_Long = 0;
			Timer_1500ms = 0;
			Relay_Staic ^= 1;
		}
	}
	if(Seg_Disp_Mode == 0)
	{
		if(Relay_Staic == 0)//解锁
		{
			if(Key_Down == 16)
				Relay(1);
			if(Key_Down == 17)
				Relay(0);
		}
	}
	
	switch(Key_Down)
	{
		case 12:
			if(++Seg_Disp_Mode == 3)
				Seg_Disp_Mode = 0;
		break;
		case 16://加
			if(Seg_Disp_Mode == 1)//DAC输出控制界面
			{
				Hand_DAC = (Hand_DAC == 255)?255:Hand_DAC+5;
			}
			else if(Seg_Disp_Mode == 2)//校准值界面
			{
				Calibration_Value = (Calibration_Value == 9)?9:Calibration_Value+1;
			}
		break;
		case 17://减
			if(Seg_Disp_Mode == 1)//DAC输出控制界面
			{
				Hand_DAC = (Hand_DAC == 0)?0:Hand_DAC-5;
			}
			else if(Seg_Disp_Mode == 2)//校准值界面
			{
				Calibration_Value = (Calibration_Value == -9)?-9:Calibration_Value-1;
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
		case 0://温度界面
			Seg_Buf[0] = 11;//C 
			Seg_Buf[1] = 10; 		
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			Seg_Buf[5] = 10;
			Seg_Buf[6] = (Temperature + Calibration_Value) / 10 % 10;
			Seg_Buf[7] = (Temperature + Calibration_Value) % 10;		
		break;
		case 1://DAC输出控制界面
			Seg_Buf[0] = 12;//A
			Seg_Buf[1] = 10; 		
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			Seg_Buf[5] = Hand_DAC / 100 % 10;
			Seg_Buf[6] = Hand_DAC / 10 % 10; 
			Seg_Buf[7] = Hand_DAC % 10;	
			if(Seg_Buf[5] == 0)Seg_Buf[5] = 10;
		break;
		case 2://校准值界面
			Seg_Buf[0] = 13;//P
			Seg_Buf[1] = 10; 		
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			Seg_Buf[5] = 10;
			if(Calibration_Value >= 0)
			{
				Seg_Buf[6] = Calibration_Value / 10 % 10; 
				Seg_Buf[7] = Calibration_Value % 10;	
				if(Seg_Buf[6] == 0)Seg_Buf[6] = 10; 
			}	
			else if(Calibration_Value < 0)		
			{
				Seg_Buf[6] = 14;//-
				Seg_Buf[7] = abs(Calibration_Value) % 10;
			}				
		break;		
	}

}

/*DAC函数*/
void DAC_Proc()
{
	if(++DAC_Slow_Down < 60)return;
	DAC_Slow_Down = 0;
	
	if(DAC_Mode == 0)//处于温度控制
	{
		if((Temperature + Calibration_Value) < 10)
			Da_Write(2 * 51.0);
		else if((Temperature + Calibration_Value) > 40)
		{
			Da_Write(5 * 51.0);			
		}
		else if((Temperature + Calibration_Value) < 40 && (Temperature + Calibration_Value) > 10)
		{
			Da_Write(((Temperature + Calibration_Value) * 0.1 + 1) * 51.0);				
		}
	}
	else if(DAC_Mode == 1)//处于手动控制
	{
		Da_Write(Hand_DAC);
	}
}

/*其他显示函数*/
void Led_Proc()
{
	ucLed[0] = (Seg_Disp_Mode == 0);
	if(L2_Timer) ucLed[1] = 1;else ucLed[1] = 0;
	if(L3_Timer) ucLed[2] = 1;else ucLed[2] = 0;	
	if(Relay_Staic == 0)ucLed[7] = 1;else ucLed[7] = 0;
	Led_Disp(ucLed);

}
/*温度获取函数*/
void Get_Temperature_Proc(void)
{
	static unsigned char Tempera_Old1,Tempera_Old2;
	unsigned char D1,D2;
	if(++Temp_Slow_Down < 100) return;
		Temp_Slow_Down = 0;
		Temperature = rd_temperature();
	D1 = (unsigned char)(Temperature - Tempera_Old1);
	D2 = (unsigned char)(Tempera_Old2 - Tempera_Old1);
	
	if(abs(D1)>0) L2_Timer = 2000;
	if(abs(D1)<0) L3_Timer = 2000;
	
	Tempera_Old1 = Temperature;
	Tempera_Old2 = Tempera_Old1;
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
	Temp_Slow_Down++;
	DAC_Slow_Down++;
	
	
	if(++Seg_Pos == 8) Seg_Pos = 0;//数码管显示专用
	if(Seg_Buf[Seg_Pos]>='.')
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos] - '.',1);
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
	
	if(L2_Timer)L2_Timer--;
	if(L3_Timer)L3_Timer--;
	
	if(Press_Long)
	{
		Timer_1500ms++;
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
	Timer1_Init();
	rd_temperature();
	Delay750ms();
	
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		Get_Temperature_Proc();
		DAC_Proc();
	}
}



