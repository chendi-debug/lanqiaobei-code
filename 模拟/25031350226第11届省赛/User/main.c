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
unsigned char Seg_Point[8]={0,0,0,0,0,0,0,0};//数码管小数点数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char ucLed[8]={0,0,0,0,0,0,0};//Led显示数据存放数组
unsigned char Seg_Disp_Mode;//0-数据界面  1-参数界面
unsigned char Temperature; //实时温度读取
unsigned char Temp_Disp[2] = {30,20};//温度变量显示数组
unsigned char Temp_Ctrol[2] = {30,20};//温度变量控制数组
bit Temp_Index = 1;//参数数组指针
bit Error_Flag;//错误标志位
/*键盘处理函数*/

void Key_Proc()
{
	if(Key_Slow_Down)return;
	Key_Slow_Down=1;//键盘减速程序
	
	Key_Val =Key_Read();//实时读取键码值
	Key_Down = Key_Val & (Key_Old ^ Key_Val);//判断按键是否按下
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);//捕捉按键上升沿
	Key_Old = Key_Val;//辅助扫描变量
	
	switch(Key_Down)
	{
		case 4://切换界面
			Seg_Disp_Mode ^=1;
			if(Seg_Disp_Mode == 1) //从数据界面切换到参数界面
			{
				Temp_Index = 1;
				Temp_Disp[0] = Temp_Ctrol[0];
				Temp_Disp[1] = Temp_Ctrol[1];
			}
			else                  //从参数界面切换到数据界面
			{
				if(Temp_Disp[0] >= Temp_Disp[1])//如果上限大于下限才保存
				{
					Error_Flag = 0;
					Temp_Ctrol[0] = Temp_Disp[0];
					Temp_Ctrol[1] = Temp_Disp[1];
				}
				else
					Error_Flag = 1;
			}
		break;
		case 5:
			if(Seg_Disp_Mode == 1)
				Temp_Index ^=1;
		break;
		case 6:
			if(Seg_Disp_Mode == 1)
			{
				if(++Temp_Disp[Temp_Index] == 100)
					Temp_Disp[Temp_Index] = 99;
			}
		break;
		case 7:
			if(Seg_Disp_Mode == 1)
			{
				if(--Temp_Disp[Temp_Index] == 255)
					Temp_Disp[Temp_Index] = 0;
			}
		break;
	}
}




/*信息处理函数*/
void Seg_Proc()
{
	if(Seg_Slow_Down) return;
	Seg_Slow_Down = 1;//数码管减速程序
	
	/*温度获取*/
  Temperature = rd_temperature();
	
	
	if(Seg_Disp_Mode == 0)//数据界面
	{
		Seg_Buf[0] = 11;//C
		Seg_Buf[3] = Seg_Buf[4] = 10;
		Seg_Buf[6] = (unsigned char)Temperature / 10 % 10; 
		Seg_Buf[7] = (unsigned char)Temperature % 10; 
	}
	else//参数界面
	{
		Seg_Buf[0] = 12;//P
		Seg_Buf[3] = Temp_Disp[0] / 10 % 10;
		Seg_Buf[4] = Temp_Disp[0] % 10;	
		Seg_Buf[6] = Temp_Disp[1] / 10 % 10;
		Seg_Buf[7] = Temp_Disp[1] % 10;			
	}
}



/*其他显示函数*/
void Led_Proc()
{
	ucLed[0] = (Temperature > Temp_Ctrol[0]);
	ucLed[1] = (Temperature >= Temp_Ctrol[1] && Temperature <=Temp_Ctrol[0]);	
	ucLed[2] = (Temperature < Temp_Ctrol[1]);
	ucLed[3] = Error_Flag;
	if(Temperature > Temp_Ctrol[0])
	{
		Da_Write(51*4);
	}
	else if(Temperature >= Temp_Ctrol[1] && Temperature <=Temp_Ctrol[0])
	{
		Da_Write(51*3);
	}
	else 
		Da_Write(51*2);
	
	/*
	unsigned char i;
	for(i=0;i<3;i++)
	{
		if(ucLed[i] == 1)
		{
			Da_Write(51*(4-i));
			break;
		}
	}		
	*/
}


/* 定时器0中断初始化函数 */
void Timer0Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0x7F;		//定时器时钟12T模式
	TMOD &= 0xF0;		//设置定时器模式
	TL0 = 0x18;		//设置定时初始值
	TH0 = 0xFC;		//设置定时初始值
	TF0 = 0;		//清除TF0标志
	TR0 = 1;		//定时器0开始计时
	ET0 = 1;    //定时器中断0打开
	EA = 1;     //总中断打开
}


/*定时器0中断服务函数*/
void Timer0_Init() interrupt 1
{
	if(++Key_Slow_Down == 10)Key_Slow_Down=0;//键盘减速专用
	if(++Seg_Slow_Down == 500)Seg_Slow_Down = 0;//数码管减速专用
	if(++Seg_Pos == 8) Seg_Pos = 0;//数码管显示专用
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
	Led_Disp(Seg_Pos,ucLed[Seg_Pos]);
}

/*延时函数*/
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
	rd_temperature();
	Delay750ms();
	Sys_Init();
	Timer0Init();
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();

	}
}



