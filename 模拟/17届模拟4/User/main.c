/*头文件声明区*/
#include <STC15F2K60S2.H>
#include "Key.h"
#include "Seg.h"
#include "Led.h"
#include "Init.h"
#include "ds1302.h"
#include "string.h"

/*变量声明区*/
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量
unsigned char Key_Slow_Down;//按键减速专用变量
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
unsigned char ucLed[8]={0,0,0,0,0,0,0};//Led显示数据存放数组
unsigned char Seg_Disp_Mode;// 0-待机界面 1-开锁界面 2-记录界面
unsigned char RTC_Time[] = {23,45,55};
unsigned char Input_Buf[4] = {11,11,11,11};
unsigned char Input_Count = 0;//已输入位数
unsigned char i;
unsigned char Record_Time[] = {0,0,0};
unsigned char Record_Flag;
unsigned int Time_3s;
unsigned char Time_Start = 0;//计时开始
bit Lock_Verify = 0;//开锁验证
bit Relay_State = 1;//继电器状态 1-关闭 0-打开



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
		case 11://s4界面切换按键
			if(++Seg_Disp_Mode == 3)Seg_Disp_Mode = 0;
			Input_Count = 0;
			Lock_Verify = 0;//切换界面时清除验证状态
			Relay_State = 1;//切换界面时关闭继电器
              for(i = 0; i < 4; i++)
                  Input_Buf[i] = 11;
		break;
		case 12://s12确认按键
			if(Seg_Disp_Mode == 1 && Lock_Verify == 1)//在开锁界面且密码已验证
			{
				Relay_State = 0;//打开继电器
				Record_Flag = 1;//记录开锁时间
				Lock_Verify = 0;//重置验证状态
				Time_Start = 1;//启动3秒计时
				Time_3s = 0;//重置计时器
			}
		break;
		case 13://s13清除按键
			if(Seg_Disp_Mode == 1)//仅在开锁界面有效
			{
				Input_Count = 0;//重置输入计数
				Lock_Verify = 0;//清除验证状态
				for(i = 0; i < 4; i++)
					Input_Buf[i] = 11;//清空输入缓冲区
			}
		break;
	}
	// 仅在开锁界面处理数字键输入（排除s4、s12、s13功能键）
  if(Seg_Disp_Mode == 1 && Key_Down != 0 && Key_Down != 11 && Key_Down != 12 && Key_Down != 13)
	{
			unsigned char i;
			unsigned char key = Key_Down; // 根据你的键值映射调整

			unsigned char digit = (Key_Down == 10) ? 0 : Key_Down;

			// 4位左移
			for(i = 0; i < 3; i++)
					Input_Buf[i] = Input_Buf[i + 1];

			Input_Buf[3] = digit; // 新值填入最右位

			if(Input_Count < 4) Input_Count++;
	}	
}




/*信息处理函数*/
void Seg_Proc()
{
	if(Seg_Slow_Down < 100) return;
	Seg_Slow_Down = 0;//数码管减速程序
	
	Read_Rtc(RTC_Time);

	switch(Seg_Disp_Mode)
	{
		case 0://待机界面
			Seg_Buf[0] = RTC_Time[0] / 10 % 10;
			Seg_Buf[1] = RTC_Time[0] % 10;
			Seg_Buf[2] = 11;//- 
			Seg_Buf[3] = RTC_Time[1] / 10 % 10;
			Seg_Buf[4] = RTC_Time[1] % 10;
			Seg_Buf[5] = 11;//-
			Seg_Buf[6] = RTC_Time[2] / 10 % 10;
			Seg_Buf[7] = RTC_Time[2] % 10;		
		break;
		case 1://开锁界面
			Seg_Buf[0] = 12; // P
			Seg_Buf[1] = 10; // 空白
			Seg_Buf[2] = 10; // 空白
			Seg_Buf[3] = 10; // 空白
			Seg_Buf[4] = Input_Buf[0];
			Seg_Buf[5] = Input_Buf[1];
			Seg_Buf[6] = Input_Buf[2];
			Seg_Buf[7] = Input_Buf[3];
		break;
		case 2://记录界面
			Seg_Buf[0] = 13;//A
			Seg_Buf[1] = 10;
			Seg_Buf[2] = Record_Time[0] / 10 % 10;
			Seg_Buf[3] = Record_Time[0] % 10;
			Seg_Buf[4] = Record_Time[1] / 10 % 10;
			Seg_Buf[5] = Record_Time[1] % 10;
			Seg_Buf[6] = Record_Time[2] / 10 % 10;	
			Seg_Buf[7] = Record_Time[2] % 10;	
		break;
	}

}



/*其他显示函数*/
void Led_Proc()
{
	ucLed[0] = (Seg_Disp_Mode == 0);
	ucLed[1] = (Seg_Disp_Mode == 1);	
	ucLed[2] = (Seg_Disp_Mode == 2);
	ucLed[6] = (Relay_State == 0);
	
	Led_Disp(ucLed);

	Relay(Relay_State);//根据状态控制继电器

	if(Seg_Disp_Mode == 1 )
	{
		if(Seg_Buf[4] == 9 && Seg_Buf[5] == 5 && Seg_Buf[6] == 2 && Seg_Buf[7] == 7)
		{
			Lock_Verify = 1;//密码正确，设置验证标志，等待按下s12确认
		}
		else
		{
			Lock_Verify = 0;//密码错误，清除验证标志
		}
	}
	if(Record_Flag)
	{

		Record_Time[0] = RTC_Time[0];
		Record_Time[1] = RTC_Time[1];
		Record_Time[2] = RTC_Time[2];	
		
	}
	Record_Flag = 0;

	// 3秒后自动闭合继电器并清空密码
	if(Time_Start == 0 && Relay_State == 0)//定时结束且继电器处于断开状态
	{
		Relay_State = 1;//重新闭合继电器
		// 清空密码输入
		Input_Count = 0;
		for(i = 0; i < 4; i++)
			Input_Buf[i] = 11;
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

	// 继电器定时控制
	if(Time_Start)
	{
		if(++Time_3s >= 3000)//3秒
		{
			Time_3s = 0;
			Time_Start = 0;
		}
	}

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
	Timer1_Init();
	Set_Rtc(RTC_Time);

	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();

	}
}



