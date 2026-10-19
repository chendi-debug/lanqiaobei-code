/*头文件声明区*/
#include <STC15F2K60S2.H>
#include "Key.h"
#include "Seg.h"
#include "Led.h"
#include "Init.h"
#include "ultrasound.h"
#include "iic.h"
#include "onewire.h"

/*变量声明区*/
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量
unsigned char Key_Slow_Down;//按键减速专用变量
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char ucLed[8]={0,0,0,0,0,0,0};//Led显示数据存放数组
unsigned char Distance_Slow_Down;


idata unsigned char Seg_Disp_Mode;//0-测距界面 1-参数界面 2-工厂模式
idata unsigned char Para_Mode;//0-距离参数 1-温度参数
idata unsigned char Factory_Mode;//0-校准值设置界面 1-介质设置界面 2-DAC输出设置界面 
bit Distance_Mode;//0-cm 1-m
idata unsigned int Temperature_10x;
idata signed int Distance;
idata unsigned char Distance_Para = 40;
idata unsigned char Temperature_Para = 30;
idata signed int Calibrate_Value = 0;//校准值
signed int Speed = 340;//传送速度
idata unsigned char i;
idata unsigned char DAC_Value = 10;
#define ABS(x) (x)<0?-(x):(x)
unsigned char L1_Timer;
bit L1_Flag;
bit Long_Press;
idata unsigned int Time_2s;


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
			  Para_Mode = 0;
			  Factory_Mode = 0;
		break;
		case 5:
			if(Seg_Disp_Mode == 0)//在测距界面下
			{
				Distance_Mode ^= 1;
			}
			else if(Seg_Disp_Mode == 1)//在参数界面下
			{
				Para_Mode ^= 1;
			}
			else if(Seg_Disp_Mode == 2)//在工厂模式下
			{
				if(++Factory_Mode == 3)Factory_Mode = 0;
			}
		break;
		case 8://加
			if(Seg_Disp_Mode == 0)
			{
			}
			if(Seg_Disp_Mode == 1)
			{
				if(Para_Mode == 0)//距离参数界面下
				{
					Distance_Para = (Distance_Para==90)?90:Distance_Para+10;
				}
				else if(Para_Mode == 1)//温度参数界面下
				{
					Temperature_Para = (Temperature_Para==80)?80:Temperature_Para+1;
				}
			}
			if(Seg_Disp_Mode == 2)
			{
				if(Factory_Mode == 0)//校准值界面下
				{
					Calibrate_Value = (Calibrate_Value == 90)?90:Calibrate_Value+5;
				}
				else if(Factory_Mode == 1)//介质设置界面下
				{
					Speed = (Speed == 9990)?9990:Speed+10;
				}
				else if(Factory_Mode == 2)//DAC输出界面下
				{
					DAC_Value = (DAC_Value == 20)?20:DAC_Value+1;
				}
			}
		break;
		case 9://减
			if(Seg_Disp_Mode == 1)
			{
				if(Para_Mode == 0)//距离参数界面下
				{
					Distance_Para = (Distance_Para==10)?10:Distance_Para-10;
				}
				else if(Para_Mode == 1)//温度参数界面下
				{
					Temperature_Para = (Temperature_Para==0)?0:Temperature_Para-1;
				}
			}
			if(Seg_Disp_Mode == 2)
			{
				if(Factory_Mode == 0)//校准值界面下
				{
					Calibrate_Value = (Calibrate_Value == -90)?-90:Calibrate_Value-5;
				}
				else if(Factory_Mode == 1)//介质设置界面下
				{
					Speed = (Speed == 10)?10:Speed-10;
				}
				else if(Factory_Mode == 2)//DAC输出界面下
				{
					DAC_Value = (DAC_Value == 1)?1:DAC_Value-1;
				}
			}
		break;
	}
	if(Key_Val == 17)
	{
		Long_Press = 1;	
	}
	if(Long_Press == 1 && Time_2s == 2000 && Key_Val == 17)
	{
		Long_Press = 0;
		Time_2s = 0;
		Seg_Disp_Mode = 0;
		Distance_Para = 40;
		Temperature_Para = 30;
		Calibrate_Value = 0;
		Speed = 340;
		DAC_Value = 10;
	}
}




/*信息处理函数*/
void Seg_Proc()
{
	if(Seg_Slow_Down < 100) return;
	Seg_Slow_Down = 0;//数码管减速程序

	Temperature_10x = rd_temperature() * 10;
	
	switch(Seg_Disp_Mode)
	{
		case 0://测距界面
			Seg_Buf[0] = Temperature_10x / 100 % 10;
			Seg_Buf[1] = Temperature_10x / 10 % 10+'.';
			Seg_Buf[2] = Temperature_10x % 10;
			Seg_Buf[3] = 11;//-
			if(Distance_Mode == 0)
			{
				if(Distance<0)
				{
					Seg_Buf[4] = (Distance<-999)?11:10;
					Seg_Buf[5] = (Seg_Buf[4]!=11)?11:ABS(Distance / 100 % 10);
					Seg_Buf[6] = (Seg_Buf[5]!=11)?11:ABS(Distance / 10 % 10);
					Seg_Buf[7] = ABS(Distance % 10);
					for(i=4;i<7&&Seg_Buf[i]==0;i++)
						Seg_Buf[i] = 10;
				}
				else
				{
					Seg_Buf[4] = ABS(Distance / 1000 % 10);
					Seg_Buf[5] = ABS(Distance / 100 % 10);
					Seg_Buf[6] = ABS(Distance / 10 % 10);
					Seg_Buf[7] = ABS(Distance % 10);
					for(i=4;i<7&&Seg_Buf[i]==0;i++)
						Seg_Buf[i] = 10;
				}
			}
			else if(Distance_Mode == 1)
			{
				Seg_Buf[4] = 10;
				Seg_Buf[5] = Distance / 100 % 10+'.';
				Seg_Buf[6] = Distance / 10 % 10;
				Seg_Buf[7] = Distance % 10;				
			}
		break;
		case 1://参数界面
			Seg_Buf[0] = 12;//P
			Seg_Buf[1] = Para_Mode+1;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			Seg_Buf[5] = 10;
			if(Para_Mode == 0)//距离参数
			{
				Seg_Buf[6] = Distance_Para / 10 % 10;
				Seg_Buf[7] = Distance_Para % 10;		
		  }	
			else if(Para_Mode == 1)
			{
				Seg_Buf[6] = Temperature_Para / 10 % 10;
				Seg_Buf[7] = Temperature_Para % 10;					
			}
		break;
		case 2://工厂模式
					Seg_Buf[0] = 13;//F 
					Seg_Buf[1] = Factory_Mode+1;
				switch(Factory_Mode)
				{

					case 0://校准值设置界面
						Seg_Buf[2] = 10;
						Seg_Buf[3] = 10;
						Seg_Buf[4] = 10;
						if(Calibrate_Value < 0)
						{
							Seg_Buf[5] = Calibrate_Value <= -10 ? 11 : 10;
							Seg_Buf[6] = Seg_Buf[5] != 11 ? 11 : ABS(Calibrate_Value / 10 % 10);
							Seg_Buf[7] = ABS(Calibrate_Value % 10);
						}
						else 
						{
							Seg_Buf[5] = 10;
							Seg_Buf[6] = Calibrate_Value >= 10 ? Calibrate_Value / 10 % 10 : 10;
							Seg_Buf[7] = Calibrate_Value % 10;							
						}
					break;
					case 1://介质设置界面
						Seg_Buf[2] = 10;
						Seg_Buf[3] = 10;
						Seg_Buf[4] = Speed / 1000 % 10;
						Seg_Buf[5] = Speed / 100 % 10;
						Seg_Buf[6] = Speed / 10 % 10;
						Seg_Buf[7] = Speed % 10;	
						for(i=4;i<7&&Seg_Buf[i]==0;i++)
							Seg_Buf[i] = 10;					
					break;
					case 2://DAC输出设置界面
						Seg_Buf[2] = 10;
						Seg_Buf[3] = 10;
						Seg_Buf[4] = 10;
						Seg_Buf[5] = 10;
						Seg_Buf[6] = DAC_Value / 10 % 10 + '.';
						Seg_Buf[7] = DAC_Value % 10;							
					break;
				}
		break;
	}
}


/*其他显示函数*/
void Led_Proc()
{
	if(Seg_Disp_Mode == 0)//测距界面下
	{
		if(Distance > 255)
		{
			for(i=0;i<8;i++)
			{
				ucLed[i] = 1;
			}
		}
		else if(Distance)
		{
			for(i=0;i<8;i++)
			{
				ucLed[i] = (Distance >> i) & 0x01;	
			}				
		}
		else
		{
			ucLed[0] = ucLed[1] = ucLed[2] = ucLed[3] = ucLed[4] = ucLed[5] = ucLed[6] = ucLed[7] = 0;		
		}
	}
	else if(Seg_Disp_Mode == 1)
	{
			ucLed[0] = ucLed[1] = ucLed[2] = ucLed[3] = ucLed[4] = ucLed[5] = ucLed[6] = 0 ;ucLed[7] = 1;	
	}
	else if(Seg_Disp_Mode == 2)
	{
		L1_Flag = 1;
		ucLed[7] = 0;
		ucLed[0] = (L1_Timer<100)?1:0;
	}
	Led_Disp(ucLed);
	
	/*继电器控制 */
	if(Distance_Para - 5 <= Distance && Distance <= Distance_Para + 5 && (Temperature_10x / 10) <= Temperature_Para)
	{
		Relay(1);
	}
	else
		Relay(0);		
}
/*距离显示函数*/
void Distance_Proc()
{
	if(Distance_Slow_Down<200)return;
		Distance_Slow_Down = 0;
	Distance = Ut_Wave_Data() + Calibrate_Value;
	
	if(Distance<10)
	{
		Da_Write((DAC_Value/10)*51.0);
	}		
	else if(Distance > 90)
	{
		Da_Write(5*51.0);
	}
	else
	{
		Da_Write(((Distance*(5-(DAC_Value/10))/80)+1.125*(DAC_Value/10)-0.625)*51.0);
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
	
	if(L1_Flag)
	{
		if(++L1_Timer == 200)
		{
			L1_Timer = 0;
		}
	}
	if(Long_Press == 1)
	{
		if(++Time_2s == 2000)
		{
			Time_2s = 2000;
		}
	}
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
		Distance_Proc();
	}
}



