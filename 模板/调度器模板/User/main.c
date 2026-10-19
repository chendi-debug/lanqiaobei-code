#include <STC15F2K60S2.H>
#include "ds1302.h"
#include "key.h"
#include "seg.h"
#include "onewire.h"
#include "led.h"
#include "init.h"
#include "intrins.h"
#include "iic.h"
/*变量声明*/
//计时器
idata unsigned int uwTick = 0;//调度器计时

//按键
idata unsigned char Key_Val,Key_Up,Key_Down,Key_Old;

//LED
pdata unsigned char ucLed[8] = {0,0,0,0,0,0,0,0};

//数码管
idata unsigned char Seg_Pos;
pdata unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10};

/*变量*/



/*键盘处理函数*/
void Key_Proc()
{
	Key_Val = Key_Read();
	Key_Down = Key_Val&(Key_Val^Key_Old);
	Key_Up = ~Key_Val&(Key_Val^Key_Old);
	Key_Old = Key_Val;
	
}

/*信息处理函数*/
void Seg_Proc()
{
	
}

/*其他显示函数*/
void Led_Proc()
{
	Led_Disp(ucLed);	
}

/*获取温度*/
void Get_Temperature()
{
	
}

/*获取时间*/
void Get_Time()
{

}


/*定时器1初始化*/
void Timer1_Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0xBF;			//定时器时钟12T模式
	TMOD &= 0x0F;			//设置定时器模式
	TL1 = 0x18;				//设置定时初始值
	TH1 = 0xFC;				//设置定时初始值
	TF1 = 0;				//清除TF1标志
	TR1 = 1;				//定时器1开始计时
	ET1 = 1;
	EA = 1;
}


/*定时器1中断*/
void Timer1_Isr(void) interrupt 3
{
	uwTick++;                 //系统计时变量增加
	if(++Seg_Pos == 8) Seg_Pos = 0; //更新数码管位置
	if(Seg_Buf[Seg_Pos]>20)
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos] - ',',1);
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
		
	
}

/*调度器*/
typedef struct
{
	void (*task_func)(void);//任务函数
	unsigned long int rate_ms;//任务执行的周期(ms)
	unsigned long int last_ms;//任务上一次执行的时间
	
}task_t;


//任务列表
idata task_t Scheduler_Task[] = {
				{Led_Proc,1,0},     // LED任务，每1ms
				{Key_Proc,10,0},		// 按键任务，每10ms
				{Get_Time,200,0},		// 获取时间任务，每100ms
				{Get_Temperature,300,0},// 获取温度任务，每300ms
				{Seg_Proc,20,0}				// 数码管任务，每20ms
};

idata unsigned char task_num;//任务数量

/**
 * @brief 调度器初始化
 * 计算任务列表中的任务数量。
 */
void Scheduler_Init()
{
	task_num = sizeof(Scheduler_Task)/sizeof(task_t);
}

/**
 * @brief 调度器运行函数
 * 循环检查每个任务是否到达执行时间。
 */
void Scheduler_Run()
{
	unsigned char i;
	for(i=0;i<task_num;i++)
	{
		unsigned long int now_time = uwTick;//获取当前时间
		//判断是否到达任务时间(当前时间>=上次执行的时间 +周期)
		if(now_time>=Scheduler_Task[i].last_ms + Scheduler_Task[i].rate_ms)
		{
			Scheduler_Task[i].last_ms = now_time;//时间更新
			Scheduler_Task[i].task_func();//任务运行
		}
			
	}
	
}
/*750延时函数*/
void Delay750ms(void)	//@12.000MHz
{
	unsigned char data i, j, k;

	_nop_();
	_nop_();
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

void main()
{
	System_Init();//系统底层初始化
	Scheduler_Init();//任务调度器初始化
	Timer1_Init();//定时器1(系统心跳)初始化
	
	//主循环
	while(1)
	{
		Scheduler_Run();  //循环执行任务调度器
	}
}