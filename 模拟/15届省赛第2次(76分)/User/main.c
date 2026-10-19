/*头文件声明区*/
#include <STC15F2K60S2.H>
#include "Key.h"
#include "Seg.h"
#include "Led.h"
#include "Init.h"
#include "onewire.h"
#include "iic.h"
#include "filtering.h"

/*变量声明区*/
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量
unsigned char Key_Slow_Down;//按键减速专用变量
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Point[8]={0,0,0,0,0,0,0,0};//数码管小数点数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char ucLed[8]={0,0,0,0,0,0,0};//Led显示数据存放数组
unsigned char AD_Slow_Down;//AD减速专用变量

/*显示变量*/
unsigned char Seg_Disp_Mode;//数码管显示模式 0-温度界面 1-DAC输出控制界面 2-校准值界面
unsigned char DAC_Mode = 0;//DAC模式 0-温度控制 1-手动控制

/*温度相关*/
signed char Temperature;//温度变量+校准值

/*校准值相关*/
signed char Calibration_value = 0;//校准值变量 
signed char Calibration_value_Ctrl;//校准值控制
#define abs(x) (x<0)? -(x) : (x)

unsigned char DA_Digital = 100;//DAC手动模式下数字量
unsigned char DA_Temperature;//DA温度模式下数字量

/*继电器相关*/
unsigned int Timer_1500ms;//1.5秒定时
bit Long_Press_Flag = 0;//长按检测标志
bit Relay_Lock;//解锁标志  0-解锁 1-锁定
bit Relay_Work_Mode;//继电器状态 0-未工作 1-工作

/*温度报警相关*/
idata bit Temperature_Up_Flag = 0;	 // 温度上升
idata bit Temperature_Down_Flag = 0; // 温度下降
bit Temperature_Up_Wring = 0;// 温度上升超过1度警告
bit Temperature_Down_Wring = 0;// 温度下降超过1度警告
bit Temperature_Wring = 0;	 // 用于报警
idata unsigned int Time_2s_Up = 0;	 // 温度上升2s计时
idata unsigned int Time_2s_Down = 0; // 温度下降2s计时

idata bit Led_Light_Wring = 0;			  // Led闪烁标志位
idata unsigned char Time_200ms_Light = 0; // 200ms闪烁计时
idata unsigned int Time_3s_Wring = 0;	  // 3s闪烁总计时


/*键盘处理函数*/

void Key_Proc()
{
	if(Key_Slow_Down < 10)return;
	Key_Slow_Down=0;//键盘减速程序
	
	Key_Val =Key_Read();//实时读取键码值
	Key_Down = Key_Val & (Key_Old ^ Key_Val);//判断按键是否按下
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);//捕捉按键上升沿
	Key_Old = Key_Val;//辅助扫描变量
	
	switch(Seg_Disp_Mode)
	{
		case 0://温度界面
		if(Key_Down == 12)
		{
			Seg_Disp_Mode = 1;
			DAC_Mode = 0;
		}
		if(Relay_Lock == 1)//在解锁模式的前提下
		{
			if(Key_Down == 16)//按下s16打开继电器
			{
				Relay_Work_Mode = 1;
			}
			if(Key_Down == 17)//按下s17关闭继电器
			{
				Relay_Work_Mode = 0;
			}
		}
		break;
		case 1://DAC输出控制界面
			if(Key_Down == 12)
			{
				Seg_Disp_Mode = 2;
				Calibration_value_Ctrl = Calibration_value;
			}
				if(Key_Down == 16)//在DAC界面下 按下s16 DAC数字量加5
				{
					DA_Digital = (DA_Digital >= 255)?255:DA_Digital+5;
					
				}
				if(Key_Down == 17)//在DAC界面下 按下s17 DAC数字量减5
				{
					DA_Digital = (DA_Digital <= 0)?0:DA_Digital-5;
				}		
		break;
		case 2://校准值界面
			if(Key_Down == 12)
			{
				Seg_Disp_Mode = 0;
				Calibration_value = Calibration_value_Ctrl;
			}
			if(Key_Down == 16)
			{
				Calibration_value_Ctrl = (Calibration_value_Ctrl == 9)?9:Calibration_value_Ctrl+1;
				
			}
			if(Key_Down == 17)
			{
				Calibration_value_Ctrl = (Calibration_value_Ctrl == -9)?-9:Calibration_value_Ctrl-1;		
			}
		break;
	}
	if(Key_Down == 13)
		Long_Press_Flag = 1;//按下s13长按检测
	
	if(Key_Up == 13)
	{
		if(Timer_1500ms == 1500)//长按
		{
			Relay_Lock ^=1;
			Long_Press_Flag = 0;
			Timer_1500ms = 0;
		}
		else										//短按
		{
			DAC_Mode ^=1;
		Long_Press_Flag = 0;
		Timer_1500ms = 0;
		}
	}
}




/*信息处理函数*/
void Seg_Proc()
{
		Relay(Relay_Work_Mode);
	if(Seg_Slow_Down < 100) return;
	Seg_Slow_Down = 0;//数码管减速程序
  Temperature = rd_temperature() + Calibration_value;


	switch(Seg_Disp_Mode)
	{
		case 0://温度界面
			Seg_Buf[0] = 11;//C 0110 0011 1100 0110 0xc6
			Seg_Buf[1] = 10;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			Seg_Buf[5] = 10;
			Seg_Buf[6] = ((Temperature) < 0) ? 14: (Temperature) / 10 % 10;//14- 1111 1101 1011 1111 0xbf
		  Seg_Buf[7] = (Temperature) % 10;
		break;
		case 1://DAC输出界面 A 0001 0001 10001000 0x88
			Seg_Buf[0] = 12;//A
			Seg_Buf[1] = 10;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			if(DAC_Mode == 0)//DAC处于温度控制
			{
				Seg_Buf[5] = ((DA_Temperature/100%10)==0)? 10:(DA_Temperature / 100 % 10);
				Seg_Buf[6] =  ((Seg_Buf[5]==10) && (DA_Temperature/10%10) == 0)?10:(DA_Temperature / 10 % 10);
				Seg_Buf[7] = (DA_Temperature % 10);	
			}
			else if(DAC_Mode == 1)       		 //DAC手动控制模式
			{
				Seg_Buf[5] = ((DA_Digital/100%10)==0)? 10:(DA_Digital / 100 % 10);
				Seg_Buf[6] =  ((Seg_Buf[5]==10) && (DA_Digital/10%10) == 0)?10:(DA_Digital / 10 % 10);
				Seg_Buf[7] = (DA_Digital % 10);				
			}

		break;
		case 2://校准值界面
			Seg_Buf[0] = 13;//P 00110001 1000 1100 0x8c
			Seg_Buf[1] = 10;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			Seg_Buf[5] = 10;
			Seg_Buf[6] = (abs(Calibration_value_Ctrl) < 0) ? 14 : 10;
			Seg_Buf[7] = abs(Calibration_value_Ctrl);
		break;
	}
}



/*其他显示函数*/
void Led_Proc()
{

	ucLed[0] = ((Seg_Disp_Mode == 1) && (DAC_Mode == 0));//温度控制模式下
	ucLed[1] = Temperature_Up_Flag;
	ucLed[2] = Temperature_Down_Flag;
	ucLed[7] = (Relay_Lock == 0);//解锁状态下LED点亮
		Led_Disp(ucLed);


}
void AD_DA()
{
	

	if(DAC_Mode == 0)//温度控制
	{
	if (Temperature <= 10)
		DA_Temperature = 102;
	else if (Temperature >= 40)
		DA_Temperature = 255;
	else
		DA_Temperature = (Temperature / 10.0f + 1) * 51;
		
	}
}
void Get_Temperature()
{
//	float temp = rd_temperature();
//	temp = Median_Filter(temp) + Calibration_value;

//	if(Temperature > temp)//温度下降
//	{
//		Temperature_Down_Flag = 1;
//		Temperature_Up_Wring = 0;
//		Time_2s_Down = 0;
//		if(Temperature - temp > 1)//下降超过1度
//		{
//			if(Temperature_Down_Wring == 0)//第一次超过1度
//				Temperature_Down_Wring = 1;
//			else//第二次超过1度
//			{
//				Temperature_Wring = 1;
//				Time_200ms_Light = 0;
//				Time_3s_Wring = 0;//清零计时
//			}
//		}
//		else
//		{
//			Temperature_Down_Wring = 0;
//		}
//	}
//	else if(Temperature < temp)//温度上升
//	{
//		Temperature_Up_Flag = 1;
//		Time_2s_Up = 0;
//		Temperature_Down_Wring = 0;
//		if(temp-Temperature > 1)
//		{
//			if(Temperature_Up_Wring == 0)//第一次超过1度
//				Temperature_Up_Wring = 1;
//			else
//			{
//				Temperature_Wring = 1;
//				Time_200ms_Light = 0;
//				Time_3s_Wring = 0;//清零计时
//			}
//		}
//		else
//		{
//			Temperature_Up_Wring = 0;
//		}
//	}
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
	
	if(Long_Press_Flag)//长按检测
	{
		if(++Timer_1500ms > 1500)
			Timer_1500ms = 1500;//先锁死,到什么时候解除？按键抬起
	}
	else
		Long_Press_Flag = 0;
	
//	if(Temperature_Up_Flag)
//	{
//		if(++Time_2s_Up == 2000)
//		{
//			Time_2s_Up = 0;
//			Temperature_Up_Flag = 0;
//		}
//	}
//	else
//		Time_2s_Up = 0;
//	if(Temperature_Down_Flag)//温度下降
//	{
//		if(++Time_2s_Down == 2000)
//		{
//			Time_2s_Down = 0;
//			Temperature_Down_Flag = 0;
//		}
//	}
//	else
//		Time_2s_Down = 0;

//	if (Temperature_Wring)// 温度突变
//	{
//		if (++Time_200ms_Light == 200)
//		{
//			Time_200ms_Light = 0;
//			Led_Light_Wring ^= 1;
//		}
//		if (++Time_3s_Wring == 3000)
//		{
//			Time_3s_Wring = 0;
//			Temperature_Wring = 0;
//		}
//	}
//	else
//	{
//		Time_200ms_Light = 0;
//		Time_3s_Wring = 0;
//		Led_Light_Wring = 0;
//	}
}	

/*温度上电延时*/
void Delay12ms(void)	//@12.000MHz
{
	unsigned char data i, j;

	i = 141;
	j = 16;
	do
	{
		while (--j);
	} while (--i);
}


/* Main */
void main()
{

	Sys_Init();
	rd_temperature();
	Delay12ms();
	Timer1_Init();
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		AD_DA();
		Get_Temperature();
	}
}

