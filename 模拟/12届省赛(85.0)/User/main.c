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
unsigned char Key_Slow_Down;//按键减速专用变量0    
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Point[8]={0,0,0,0,0,0,0,0};//数码管小数点数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char ucLed[8]={0,0,0,0,0,0,0};//Led显示数据存放数组
unsigned char Seg_Disp_Mode;//0-温度显示界面 1-参数设置界面 2-DAC输出界面
float Temperature;//实时温度变量
unsigned char Temp_Disp = 25;//参数显示界面
unsigned char Temp_Ctrol = 25;//参数控制界面
float Voltage_Output;//电压输出实时变量
bit Output_Mode;//DAC输出模式 0-模式1 1-模式2

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
		case 4://界面切换按键
			if(++Seg_Disp_Mode == 3)
				Seg_Disp_Mode = 0;
			if(Seg_Disp_Mode == 1) Temp_Ctrol = Temp_Disp;//在参数界面下控制值要给显示值
			if(Seg_Disp_Mode == 2) Temp_Disp = Temp_Ctrol;//如果从参数界面下跳出来就要把显示值给控制值 保存下来
		break;
		case 8://减
			if(Seg_Disp_Mode == 1)//在参数界面下
			{
				if(--Temp_Ctrol == 255)
					Temp_Ctrol = 0;
			}
		break;
		case 9://加
			if(Seg_Disp_Mode == 1)//在参数界面下
			{
				{
					if(++Temp_Ctrol == 100)
						Temp_Ctrol = 99;
				}
			}
		break;
		case 5://DAC模式切换按键
			Output_Mode ^= 1;
		break;
	}
}




/*信息处理函数*/
void Seg_Proc()
{
	if(Seg_Slow_Down) return;
	Seg_Slow_Down = 1;//数码管减速程序
	
	Temperature = rd_temperature();
	switch(Seg_Disp_Mode)
	{
		case 0://温度显示
			Seg_Buf[0] = 11;//C
			Seg_Buf[4] = (unsigned char)Temperature / 10 % 10;
			Seg_Buf[5] = (unsigned char)Temperature % 10;
			Seg_Buf[6] = (unsigned int)(Temperature * 100) / 10 % 10;
			Seg_Buf[7] = (unsigned int)(Temperature * 100) % 10;			
			Seg_Point[5] = 1;
		break;
		case 1://参数设置界面
			Seg_Buf[0] = 12;//P
			Seg_Buf[4] = Seg_Buf[5] = 10;
			Seg_Point[5] = 0;
			Seg_Buf[6] = Temp_Ctrol / 10 % 10;
			Seg_Buf[7] = Temp_Ctrol % 10;
		break;
		case 2://DAC
			Seg_Buf[0] = 13;//A
			Seg_Buf[5] = (unsigned int)(Voltage_Output * 100) / 100 % 10;
			Seg_Buf[6] = (unsigned int)(Voltage_Output * 100) / 10 % 10;
			Seg_Buf[7] = (unsigned int)(Voltage_Output * 100) % 10;
			Seg_Point[5] = 1;
		break;
	}

}

/*其他显示函数*/
void Led_Proc()
{
	unsigned char i;
	/*DAC相关*/
	if(Output_Mode == 0)//模式1
	{
		if(Temperature < Temp_Disp)
			Voltage_Output = 0;
		else
			Voltage_Output = 5;
	}
	else       //模式2
	{
		if(Temperature <= 20)
			Voltage_Output = 1;
		else if(Temperature >= 40)
			Voltage_Output = 4;
		else
			Voltage_Output = 0.15 * Temperature - 2 ;
	}
	Da_Write(Voltage_Output * 51);
	
	/*Led相关*/
	ucLed[0] = !Output_Mode;
	for(i=0;i<3;i++)
		ucLed[1+i] = (i == Seg_Disp_Mode);
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
	if(++Seg_Slow_Down == 100)Seg_Slow_Down = 0;//数码管减速专用
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
