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
unsigned char Seg_Point[8]={0,0,0,0,0,0,0,0};//数码管小数点数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char ucLed[8]={0,0,0,0,0,0,0};//Led显示数据存放数组
unsigned int Led_Time_Disp[4] = {400,400,400,400};//流转时间数据显示数组
unsigned int Led_Time_Ctrol[4] = {400,400,400,400};//流转时间数据控制数组
unsigned char Led_Time_Index;//流转时间间隔指针 - 运行模式编号
unsigned int Timer_400Ms;//400毫秒计时变量
unsigned char Seg_Index;//数码管索引值 0-熄灭 1-模式编号闪 2-流转间隔闪
unsigned char EEPROM_Dat[4];
unsigned char LED_Mode;//LED的4种流转方式
unsigned int  Ms_Tick;//特定的流转时间变量
unsigned char Led_Pos;//
bit System_Flag;//LED工作标志位
bit Seg_Star_Flag;//数码管闪烁标志位
bit Seg_Mode_Disp;//数码管模式显示界面  0-参数界面  1-亮度等级界面



/*键盘处理函数*/

void Key_Proc()
{
	unsigned char i;
	if(Key_Slow_Down)return;
	Key_Slow_Down=1;//键盘减速程序
	
	Key_Val =Key_Read();//实时读取键码值
	Key_Down = Key_Val & (Key_Old ^ Key_Val);//判断按键是否按下
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);//捕捉按键上升沿
	Key_Old = Key_Val;//辅助扫描变量
	
	switch(Key_Down)
	{
		case 6:   //模式切换
			if(++Seg_Index == 3) Seg_Index = 0;   //模式切换                             
			if(Seg_Index == 0)  										//当数码管从流转间隔闪跳到熄灭时就要保存数据了
			{
				Led_Time_Index = 0;   //当在编号设置界面时，需要将运行编号手动恢复默认值
				for(i=0;i<4;i++)
				{
					Led_Time_Ctrol[i] = Led_Time_Disp[i];
					EEPROM_Dat[i] = Led_Time_Ctrol[i] / 100;
				}
				EEPROM_Write(EEPROM_Dat,0,4);
			}
		break;
		case 5:   //+
			if(Seg_Index == 1) //模式编号
			{
				if(++Led_Time_Index == 4) Led_Time_Index = 0;
			}
			else if(Seg_Index == 2) //流转时间间隔
			{
				Led_Time_Disp[Led_Time_Index] +=100;
				if(Led_Time_Disp[Led_Time_Index] > 1200)
					Led_Time_Disp[Led_Time_Index] = 1200;
			}
		break;
		case 4:   //-
			if(Seg_Index == 1) //模式编号
			{
				if(--Led_Time_Index == 255) 
					Led_Time_Index = 3;
			}
			else if(Seg_Index == 2) //流转时间间隔
			{
				Led_Time_Disp[Led_Time_Index] -=100;
				if(Led_Time_Disp[Led_Time_Index] < 400)
					Led_Time_Disp[Led_Time_Index] = 400;
			}
		break;
		case 7:
			System_Flag ^=1;
		break;
	}
}




/*信息处理函数*/
void Seg_Proc()
{
	if(Seg_Slow_Down) return;
	Seg_Slow_Down = 1;//数码管减速程序
	
	if(Seg_Mode_Disp == 0) //处于参数界面
	{
		Seg_Buf[0] = Seg_Buf[2] = 11;
		Seg_Buf[1] = Led_Time_Index + 1;  //Led_Time_Index从0开始
		Seg_Buf[4] = Led_Time_Disp[Led_Time_Index] / 1000 % 10;
		Seg_Buf[5] = Led_Time_Disp[Led_Time_Index] / 100 % 10;
		Seg_Buf[6] = Led_Time_Disp[Led_Time_Index] / 10 % 10;
		Seg_Buf[7] = Led_Time_Disp[Led_Time_Index] % 10;
		if(Seg_Index == 1)  //模式编号闪
		{
			Seg_Buf[0] = Seg_Buf[2] = Seg_Star_Flag?10:11;
			Seg_Buf[1] = Seg_Star_Flag?10:Led_Time_Index + 1; 			
		}
		else if(Seg_Index == 2) //流转时间闪
		{
		Seg_Buf[4] = Seg_Star_Flag?10:Led_Time_Disp[Led_Time_Index] / 1000 % 10;
		Seg_Buf[5] = Seg_Star_Flag?10:Led_Time_Disp[Led_Time_Index] / 100 % 10;
		Seg_Buf[6] = Seg_Star_Flag?10:Led_Time_Disp[Led_Time_Index] / 10 % 10;
		Seg_Buf[7] = Seg_Star_Flag?10:Led_Time_Disp[Led_Time_Index] % 10;			
		}
		if(Seg_Buf[4] == 0) Seg_Buf[4] = 10;
	}

}



/*其他显示函数*/
void Led_Proc()
{
	unsigned char i;
	if(System_Flag == 1)
	{
		if(Ms_Tick == Led_Time_Ctrol[LED_Mode])
		{
			Ms_Tick = 0;
			switch(LED_Mode)//4种模式
			{
				case 0:
					if(++Led_Pos == 8)
					{
						Led_Pos = 7;
						LED_Mode = 1;
					}
				break;
				case 1:
					if(--Led_Pos == 255)
					{
						LED_Mode = 2;
					}
				break;
			}
		}
	}
	for(i=0;i<8;i++)
	{
		ucLed[i] = (i == Led_Pos);
	}
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
	if(Seg_Index != 0)
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],Seg_Point[Seg_Pos]);
	else
		Seg_Disp(Seg_Pos,10,0);
	Led_Disp(Seg_Pos,ucLed[Seg_Pos]);
	
	if(++Timer_400Ms == 400)   //闪烁计时
	{
		Timer_400Ms = 0;
		Seg_Star_Flag ^= 1;
	}
	if(System_Flag == 1)   //系统处于运动状态时，流转时间计时
		Ms_Tick++;
}

/* Main */
void main()
{
	unsigned char i;
	Sys_Init();
	EEPROM_Read(EEPROM_Dat,0,4);
	for(i=0;i<4;i++)
		Led_Time_Ctrol[i] = Led_Time_Disp[i] = EEPROM_Dat[i] * 100;
	Timer0Init();
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();

	}
}
