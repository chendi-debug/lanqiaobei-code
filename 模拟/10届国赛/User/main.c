#include <STC15F2K60S2.H>
#include "Init.h"
#include "Key.h"
#include "Led.h"
#include "Seg.h"
#include "Distance.h"
#include "iic.h"
#include "Uart.h"
#include "onewire.h"
#include "string.h"
#include "stdio.h"

unsigned char Key_Val,Key_Down,Key_Up,Key_Old;
unsigned char Key_Slow_Down;
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};
unsigned char Seg_Pos;
unsigned char Seg_Slow_Down;
unsigned char ucLed[8]={0,0,0,0,0,0,0,0};
unsigned char Seg_Disp_Mode;//0-数据界面 1-参数界面
unsigned char Data_Mode;//0-温度数据 1-距离数据 2-变更次数
unsigned char Para_Mode;//0-温度参数 1-距离参数
float Temperature;
unsigned char Temperature_Para=30;
unsigned int Temperature_Para_Ctrol;
static unsigned int Temperature_Para_Ctrol_old;
unsigned char Distance;
unsigned char Distance_Para=35;
unsigned char Distance_Para_Ctrol;
static unsigned char Distance_Para_Ctrol_old;
unsigned int Count;
unsigned char i;
bit Count_Flag=0;
static unsigned int old_State=0;
unsigned int Timer_Count=0;
unsigned int Timer_DAC=0;
bit Long_Press_s12=0;
bit Long_Press_s13=0;
bit DAC_Mode;//0-开启 1-停止

pdata unsigned char Uart_Buf[10]={0};//串口接收缓冲区
idata unsigned char Uart_Rx_Index=0;	//串口接收索引
idata unsigned char Uart_Recv_Tick=0;	//串口接收时间标志
idata unsigned char Uart_Rx_Flag=0;	//串口接收标志位

void Key_Proc()
{
	if(Key_Slow_Down<10)return;
	 Key_Slow_Down=0;
	Key_Val=Key_Read();
	Key_Down=Key_Val&(Key_Val^Key_Old);
	Key_Up=~Key_Val&(Key_Val^Key_Old);
	Key_Old=Key_Val;
	
	switch(Key_Down)
	{
		case 13:
			Long_Press_s13=1;

		break;
		case 12:
				Long_Press_s12=1;

		break;
		case 16:
			if(Para_Mode==0&&Seg_Disp_Mode==1)//温度参数
			{
				Temperature_Para_Ctrol=(Temperature_Para_Ctrol==0)?0:Temperature_Para_Ctrol-2;
			}
			if(Para_Mode==1&&Seg_Disp_Mode==1)//距离参数
			{
				Distance_Para_Ctrol=(Distance_Para_Ctrol==0)?0:Distance_Para_Ctrol-5;
			}
		break;
		case 17:
			if(Para_Mode==0&&Seg_Disp_Mode==1)//温度参数
			{
				Temperature_Para_Ctrol=(Temperature_Para_Ctrol==99)?99:Temperature_Para_Ctrol+2;
			}
			if(Para_Mode==1&&Seg_Disp_Mode==1)//距离参数
			{
				Distance_Para_Ctrol=(Distance_Para_Ctrol==99)?99:Distance_Para_Ctrol+5;
			}
		break;
	}
	switch(Key_Up)
	{
		case 12:
			if(Timer_Count>1000)
			{
				Timer_Count=0;
				Long_Press_s12=0;
				Count=0;
			}
			else
			{
				Timer_Count=0;
				Long_Press_s12=0;
				if(Seg_Disp_Mode==0)//数据界面
				{
					if(++Data_Mode==3)Data_Mode=0;
				}
				if(Seg_Disp_Mode==1)//参数界面
				{
					if(++Para_Mode==2)Para_Mode=0;
				}
			}			
		break;
		case 13:
			if(Timer_DAC>1000)
			{
				Timer_DAC=0;
				Long_Press_s13=0;
				DAC_Mode ^= 1;
			}
			else
			{
				Timer_DAC=0;
				Long_Press_s13=0;
				if(++Seg_Disp_Mode==2)Seg_Disp_Mode=0;
				Data_Mode=Para_Mode=0;
				if(Seg_Disp_Mode==1)//参数界面
				{
					Temperature_Para_Ctrol=Temperature_Para;
					Distance_Para_Ctrol=Distance_Para;
				}
				else if(Seg_Disp_Mode==0)//数据界面
				{
					Temperature_Para=Temperature_Para_Ctrol;
					Distance_Para=Distance_Para_Ctrol;
					if((Temperature_Para_Ctrol_old!=Temperature_Para_Ctrol)||(Distance_Para_Ctrol_old!=Distance_Para_Ctrol))
						Count_Flag=1;
					else
						Count_Flag=0;
				if(Count_Flag==1&&old_State==0)
				{
					Count++;
				}
					old_State=Count_Flag;
					Temperature_Para_Ctrol_old=Temperature_Para;
					Distance_Para_Ctrol_old=Distance_Para;
				}
			}
		break;
	}
	
}

void Seg_Proc()
{
	if(Seg_Slow_Down<90)return;
		Seg_Slow_Down=0;
	Temperature=rd_Temperature();
	Distance=Ut_Wave_Data();
	if(Distance>=99)
		Distance=99;
	switch(Seg_Disp_Mode)
	{
		case 0://数据界面
			switch(Data_Mode)
			{
				case 0://温度数据
					Seg_Buf[0]=11;//C
					Seg_Buf[1]=10;
					Seg_Buf[2]=10;
					Seg_Buf[3]=10;
					Seg_Buf[4]=(unsigned char)Temperature/10%10;
					Seg_Buf[5]=(unsigned char)Temperature%10+'.';
					Seg_Buf[6]=(unsigned int)(Temperature*100)/10%10;
					Seg_Buf[7]=(unsigned int)(Temperature*100)%10;
				break;
				case 1://距离数据
					Seg_Buf[0]=12;//L
					Seg_Buf[1]=10;
					Seg_Buf[2]=10;
					Seg_Buf[3]=10;
					Seg_Buf[4]=10;
					Seg_Buf[5]=10;
					Seg_Buf[6]=Distance/10%10;
					Seg_Buf[7]=Distance%10;					
				break;
				case 2://变更次数
					Seg_Buf[0]=13;//n 
					Seg_Buf[1]=10;
					Seg_Buf[2]=10;
					Seg_Buf[3]=Count/10000%10;
					Seg_Buf[4]=Count/1000%10;
					Seg_Buf[5]=Count/100%10;
					Seg_Buf[6]=Count/10%10;
					Seg_Buf[7]=Count%10;	
					for(i=3;i<7&&Seg_Buf[i]==0;i++)
					 Seg_Buf[i]=10;
				break;
			}
		break;
		case 1://参数界面
			switch(Para_Mode)
			{
				case 0://温度参数
					Seg_Buf[0]=14;//P
					Seg_Buf[1]=10;
					Seg_Buf[2]=10;
					Seg_Buf[3]=1;
					Seg_Buf[4]=10;
					Seg_Buf[5]=10;
					Seg_Buf[6]=Temperature_Para_Ctrol/10%10;
					Seg_Buf[7]=Temperature_Para_Ctrol%10;				
				break;
				case 1://距离参数
					Seg_Buf[0]=14;//P
					Seg_Buf[1]=10;
					Seg_Buf[2]=10;
					Seg_Buf[3]=2;
					Seg_Buf[4]=10;
					Seg_Buf[5]=10;
					Seg_Buf[6]=Distance_Para_Ctrol/10%10;
					Seg_Buf[7]=Distance_Para_Ctrol%10;						
				break;
			}			
		break;
		
	}
	
}
void Led_Proc()
{
	
	if(Temperature>Temperature_Para)
		ucLed[0]=1;
	else
		ucLed[0]=0;
	if(Distance_Para>Distance)
		ucLed[1]=1;
	else
		ucLed[1]=0;
	Led_Disp(ucLed);
	
	/*DAC*/
	if(DAC_Mode==0)
	{
	if(Distance_Para>=Distance)
		Da_Write(2*51.0f);
	else if(Distance>Distance_Para)
		Da_Write(4*51.0f);
	}
	else
		Da_Write(0);

}


void Uart_Proc()
{
	if(Uart_Rx_Index==0)
		return;
	if(Uart_Recv_Tick>=10)
	{
		Uart_Recv_Tick=0;
		Uart_Rx_Flag=0;
			//处理函数？？？
		if(strcmp(Uart_Buf,"ST\r\n")==0)
			printf("$%bu,%.2f\r\n",Distance,Temperature);
		else if(strcmp(Uart_Buf,"PARA\r\n")==0)
			printf("#%bu,%bu\r\n",Distance_Para,Temperature_Para);
		else
			printf("ERROR\r\n");
			
		memset(Uart_Buf,0,Uart_Rx_Index);
		Uart_Rx_Index=0;
		
	}
}
void Timer1_Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0xBF;			//定时器时钟12T模式
	TMOD &= 0x0F;			//设置定时器模式
	TL1 = 0x18;				//设置定时初始值
	TH1 = 0xFC;				//设置定时初始值
	TF1 = 0;				//清除TF1标志
	TR1 = 1;				//定时器1开始计时
	ET1 = 1;				//使能定时器1中断
	EA=1;
}


void Uart1_Isr(void) interrupt 4
{

	if (RI)				//检测串口1接收中断
	{
		Uart_Rx_Flag=1;
		Uart_Recv_Tick=0;
		Uart_Buf[Uart_Rx_Index++]=SBUF;
		RI = 0;			//清除串口1接收中断请求位
		
		if(Uart_Recv_Tick>10)//超过10ms
		{
			Uart_Rx_Flag=0;
			Uart_Recv_Tick=0;
			Uart_Rx_Index=0;
			memset(Uart_Buf,0,10);
		}
	}
}

void Timer1_Isr(void) interrupt 3
{
	Key_Slow_Down++;
	Seg_Slow_Down++;
	if(++Seg_Pos==8)Seg_Pos=0;
	
	if(Uart_Rx_Flag)
		Uart_Recv_Tick++;
	
	if(Seg_Buf[Seg_Pos]>'.')
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos]-'.',1);
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
		
	if(Long_Press_s12==1)
	{
		if(++Timer_Count==1000)
		{
			Timer_Count=1000;
		}
	}
	if(Long_Press_s13==1)
	{
		if(++Timer_DAC==1000)
		{
			Timer_DAC=1000;
		}
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

void main()
{
	Sys_Init();
	Timer1_Init();
	Uart1_Init();
	rd_Temperature();
	Delay750ms();

	while(1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		Uart_Proc();
	}
}