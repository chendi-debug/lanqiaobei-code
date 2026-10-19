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
unsigned char Seg_Disp_Mode;// 0-温度界面 1-DAC输出控制界面 2-校准值界面
unsigned char Temperature;//温度变量
unsigned char Temp_Slow_Down;
unsigned char AD_Slow_Down;
bit DAC_Output_Mode;//DAC输出模式 0-温度控制 1-手动控制
unsigned char Hand_DAC = 100;//手动DAC变量
unsigned char i;
signed char Calibration_Value = 0;//校准值变量
#define abs(x) (x<0) ? -(x) : (x)
unsigned int Timer_1500ms;//1.5秒计时
bit Long_Press;//长按标志位
bit Key_Press;//按下标志位
bit Relay_Flag;//继电器状态 0-解锁 1-锁定
unsigned int L2_Timer, L3_Timer, L4_Timer;  // 单位ms
bit Temperature_Warning;
/*键盘处理函数*/

void Key_Proc()
{
	if(Key_Slow_Down < 10)return;
	Key_Slow_Down=0;//键盘减速程序
	
	Key_Val =Key_Read();//实时读取键码值
	Key_Down = Key_Val & (Key_Old ^ Key_Val);//判断按键是否按下
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);//捕捉按键上升沿
	Key_Old = Key_Val;//辅助扫描变量

	switch(Key_Up)
	{
		case 13:
			if(Long_Press)
			{
				Relay_Flag ^= 1;
			}
			else
				DAC_Output_Mode ^= 1;
			Key_Press = 0;
			Long_Press = 0;
		break;
	}
	switch(Key_Down)
	{
		case 12://界面切换按键
			if(++Seg_Disp_Mode == 3)Seg_Disp_Mode = 0;
		break;
		case 13:
			Key_Press = 1;
			Timer_1500ms = 0;
			Long_Press = 0;
		break;
		case 16://加
			if(Seg_Disp_Mode == 1)//DAC输出控制界面下
			{
				Hand_DAC = (Hand_DAC == 255) ? 255 : Hand_DAC+5;
			}
			if(Seg_Disp_Mode == 2)//在校准值界面下
			{
				Calibration_Value = (Calibration_Value == 9) ? 9 :Calibration_Value+1;			
			}
			if((Seg_Disp_Mode == 0) && (Relay_Flag == 0))
			{
				Relay(1);
			}
		break;
		case 17://减
			if(Seg_Disp_Mode == 1)//DAC输出控制界面下
			{
				Hand_DAC = (Hand_DAC == 0) ? 0 : Hand_DAC-5;
			}
			if(Seg_Disp_Mode == 2)//在校准值界面下
			{
				Calibration_Value = (Calibration_Value == -9) ? -9 :Calibration_Value-1;			
			}
			if((Seg_Disp_Mode == 0) && (Relay_Flag == 0))
			{
				Relay(0);
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
			for(i=5;i<7 && Seg_Buf[i] == 0;i++)
				Seg_Buf[i] = 10;
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
			Seg_Buf[6] = (Calibration_Value < 10) ? 10 : Calibration_Value / 10 % 10;
			Seg_Buf[7] = Calibration_Value % 10;	
		}
		else 
		{
			Seg_Buf[6] = 14;//14 - 
			Seg_Buf[7] = abs(Calibration_Value) % 10;			
		}
		break;
		
	}

}



/*其他显示函数*/
void Led_Proc()
{
		ucLed[0] = (Seg_Disp_Mode == 0);
		if(DAC_Output_Mode == 1)
			ucLed[0] = 1;
		if(L2_Timer) ucLed[1] = 1; else ucLed[1] = 0;
		if(L3_Timer) ucLed[2] = 1; else ucLed[2] = 0;
		if(L4_Timer) ucLed[3] = (L4_Timer / 200) % 2;  // 200ms切换
		else         ucLed[3] = 0;
	
		ucLed[7] = (Relay_Flag == 0);
		Led_Disp(ucLed);
}


/*温度相关*/
void Get_Temp_Proc(void)
{
	static signed char Tempera_Old1,Tempera_Old2;//前两次温度
	signed char D1,D2;
	
	if(Temp_Slow_Down < 100)return;
		Temp_Slow_Down = 0;
	
	Temperature = rd_temperature();
	D1 = (signed char)(Temperature - Tempera_Old1);
	D2 = (signed char)(Tempera_Old1 - Tempera_Old2);

	if(D1 > 0) L2_Timer = 2000;   // 上升
	if(D1 < 0) L3_Timer = 2000;   // 下降
	if(abs(D1) > 1 && abs(D2) > 1) L4_Timer = 3000;  // 突变
	
	
	Tempera_Old2 = Tempera_Old1;
	Tempera_Old1 = Temperature;		

}

/*AD相关*/
void AD_Proc(void)
{
	if(AD_Slow_Down < 40)return;
		AD_Slow_Down = 0;
	
	if(DAC_Output_Mode == 0)//温度控制
	{
		if((Temperature+Calibration_Value) <= 10)
			Da_Write(2 * 51.0f);
		else if((Temperature+Calibration_Value) >= 40)
		Da_Write(5 * 51.0f);	
		else 
			Da_Write(51.0f * (0.1 * (Temperature+Calibration_Value) + 1));		
	}
	else if(DAC_Output_Mode == 1)//手动控制
	{
		Da_Write(Hand_DAC);
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

/*定时器1中断服务函数*/
void Timer1_Isr(void) interrupt 3
{
	Key_Slow_Down++;
	Seg_Slow_Down++;
	Temp_Slow_Down++;
	AD_Slow_Down++;
	
	if(++Seg_Pos == 8) Seg_Pos = 0;//数码管显示专用
	if(Seg_Buf[Seg_Pos]>='.')
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos] - '.',1);
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
	
	if(Key_Press == 1)
	{
		if(++Timer_1500ms >= 1500)
		{
			Timer_1500ms = 0;
			Long_Press = 1;
			Key_Press = 0;
		}
	}
	
	if(L2_Timer) L2_Timer--;
  if(L3_Timer) L3_Timer--;
  if(L4_Timer) L4_Timer--;
}

/* Main */
void main()
{
	Sys_Init();
	rd_temperature();
	Delay750ms();
	Timer1_Init();
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		Get_Temp_Proc();
		AD_Proc();
	}
}
