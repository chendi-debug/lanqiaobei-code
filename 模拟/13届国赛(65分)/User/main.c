/*头文件声明区*/
#include <STC15F2K60S2.H>
#include "Key.h"
#include "Seg.h"
#include "Led.h"
#include "Init.h"
#include "iic.h"
#include "ultrasound.h"


/*变量声明区*/
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//按键专用变量
unsigned char Key_Slow_Down;//按键减速专用变量
pdata unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};//数码管显示数据存放数组
unsigned char Seg_Pos;//数码管扫描专用变量
unsigned int Seg_Slow_Down;//数码管减速专用变量
pdata unsigned char ucLed[8]={0,0,0,0,0,0,0};//Led显示数据存放数组
idata unsigned char Seg_Disp_Mode;//数码管显示界面 0-频率界面 1-湿度界面 2-测距界面 3-参数界面
idata unsigned char Freq_Disp_Mode;//频率显示模式 0-hz 1-Khz
idata unsigned char Distance_Disp_Mode;//距离显示模式 0-cm 1-m
idata unsigned char Parameter_Disp_Mode;//参数显示模式 0-频率参数 1-湿度参数 2-距离参数

idata unsigned char Humidity_Value = 0;//湿度变量
idata unsigned int Freq = 0;//频率变量
idata unsigned int Distance = 0;//距离变量
idata unsigned char Humidity_Para_Ctrol = 0;//湿度控制变量
idata unsigned int Freq_Ctrol = 0;//频率控制变量
idata unsigned int Distance_Ctrol = 0;//距离控制变量

idata unsigned int Timer_1s;//1秒计数频率变量

unsigned char i;
pdata unsigned char Relay_Num = 0;//继电器开关次数
pdata unsigned char EEPROM_Lock = 13;//EEPROM加锁
pdata unsigned int Time_1s_Press = 0;//检测长按的1s变量
idata bit Long_Press_Flag;//长按标志位
idata bit Relay_Work = 0;//继电器工作模式
idata bit Relay_Work_Old = 0;//
pdata unsigned char PWM_Period = 0;//pwm调光周期 10就是1ms
pdata unsigned char PWM_Compare = 2;//PWM调光比较值
pdata unsigned char Timer_1000ms_Freq_Show = 0;//100ms闪烁 频率界面
pdata unsigned char Timer_1000ms_Humidity_Show = 0;//100ms闪烁 湿度界面
pdata unsigned char Timer_1000ms_Distance_Show = 0;//100ms闪烁 距离界面
pdata unsigned char Led_Freq_Show = 0;//频率界面闪烁标志
pdata unsigned char Led_Humidity_Light = 0;//湿度界面闪烁标志位
pdata unsigned char Led_Distance_Light = 0;//距离界面闪烁标志位

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
		case 0://频率界面
			if(Key_Down == 4)
			{
				Seg_Disp_Mode = 1;//切换到湿度界面

			}
			if(Key_Down == 7)//频率显示模式切换
				Freq_Disp_Mode ^=1;
		break;
		case 1://湿度界面
			if(Key_Down == 4)
			{
				Seg_Disp_Mode = 2;//切换到测距界面
				Distance_Disp_Mode = 0;//保证在CM界面
			}
			if(Key_Down == 7)
				Long_Press_Flag = 1;//触发长按按键检测
			if(Key_Up == 7)
			{
				if(Time_1s_Press >= 1000)
				{
					Relay_Num = 0;//继电器计数清零
					EEPROM_Write(&Relay_Num,0,1);   //？？？？？？
					EEPROM_Write(&EEPROM_Lock,8,1);//？？？？？？
				}
				Long_Press_Flag = 0;
			}
		break;
		case 2://测距界面
			if(Key_Down == 4)
			{
				Seg_Disp_Mode = 3;//切换到参数界面
				Humidity_Para_Ctrol = Humidity_Value;//真实值赋给控制值
				Freq_Ctrol = Freq;
				Distance_Ctrol = Distance;
				
			}
		break;
		case 3://参数界面
			if(Key_Down == 4)
			{
				Seg_Disp_Mode = 0;//切换到频率界面			
				Freq_Disp_Mode = 0;//HZ界面
				Humidity_Value = Humidity_Para_Ctrol;//控制值给真实值
				Freq = Freq_Ctrol;
				Distance = Distance_Ctrol;
			}
			if(Key_Down == 5)//按键5在参数界面下切换界面
				Parameter_Disp_Mode = (++Parameter_Disp_Mode)%3;
			switch(Parameter_Disp_Mode)
			{
				case 0://频率参数
					if(Key_Down == 6)//加
						Freq_Ctrol = (Freq_Ctrol == 120) ? 10 : Freq_Ctrol+5;
					if(Key_Down == 7)//减
						Freq_Ctrol = (Freq_Ctrol == 10) ? 120 : Freq_Ctrol-5;
				break;
				case 1://湿度参数
					if(Key_Down == 6)//加
						Humidity_Para_Ctrol = (Humidity_Para_Ctrol == 60) ? 10 : Humidity_Para_Ctrol+10;
					if(Key_Down == 7)//减
						Humidity_Para_Ctrol = (Humidity_Para_Ctrol == 10) ? 60 : Humidity_Para_Ctrol-10;					
				break;
				case 2://距离参数
					if(Key_Down == 6)//加
						Distance_Ctrol = (Distance_Ctrol == 12) ? 1 : Distance_Ctrol+1;
					if(Key_Down == 7)//减
						Distance_Ctrol = (Distance_Ctrol == 1) ? 12 : Distance_Ctrol-1;					
				break;			
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
		case 0://频率界面F 01110001 1000 1110 0x8e
			Seg_Buf[0] = 11;//F
			Seg_Buf[1] = 10;
			switch(Freq_Disp_Mode)
			{
				case 0://HZ
					Seg_Buf[2] = Freq / 100000 % 10;
					Seg_Buf[3] = Freq / 10000 % 10;
					Seg_Buf[4] = Freq / 1000 % 10;
					Seg_Buf[5] = Freq / 100 % 10;
					Seg_Buf[6] = Freq / 10 % 10;
					Seg_Buf[7] = Freq % 10;
					for(i=2;i<7 && Seg_Buf[i]==0;i++)
						Seg_Buf[i] = 10;					
				break;
				case 1://KHZ
					Seg_Buf[2] = 10;
					Seg_Buf[3] = 10;
					Seg_Buf[4] = 10;
					Seg_Buf[5] = Freq / 10000 % 10;
					Seg_Buf[6] = Freq / 1000 % 10 + '.';
					Seg_Buf[7] = Freq / 100 % 10;
					for(i=2;i<7 && Seg_Buf[i]==0;i++)
						Seg_Buf[i] = 10;					
				break;
			}

		break;
		case 1://湿度界面
			Seg_Buf[0] = 12;//H 10010001 1000 1001 0x89
			Seg_Buf[1] = 10;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			Seg_Buf[5] = 10;
			Seg_Buf[6] = Humidity_Value / 10 % 10;
			Seg_Buf[7] = Humidity_Value % 10;
			
		break;
		case 2://测距界面 00010001 1000 1000 0x88 A
			Seg_Buf[0] = 13;//A
			Seg_Buf[1] = 10;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;
			switch(Distance_Disp_Mode)//距离显示模式
			{
				case 0://厘米
					Seg_Buf[5] = Distance / 100 % 10;
					Seg_Buf[6] = Distance / 10 % 10;
					Seg_Buf[7] = Distance % 10;
					if(Seg_Buf[5]==0)
						Seg_Buf[5] = 10;
					if(Seg_Buf[5] && Seg_Buf[6] == 0)
						Seg_Buf[6] = 10;
				break;
				case 1://米
					Seg_Buf[5] = Distance / 100  + '.';
					Seg_Buf[6] = Distance / 10 % 10;
					Seg_Buf[7] = Distance % 10;
					if(Seg_Buf[5]==0)
						Seg_Buf[5] = 10;
					if(Seg_Buf[5] && Seg_Buf[6] == 0)
						Seg_Buf[6] = 10;					
				break;
			}
		break;
		case 3://参数界面
			Seg_Buf[0] = 14;//P
			Seg_Buf[1] = Parameter_Disp_Mode + 1;
			Seg_Buf[2] = 10;
			Seg_Buf[3] = 10;
			Seg_Buf[4] = 10;			
			switch(Parameter_Disp_Mode)//参数显示模式
			{
				case 0://频率参数
					Seg_Buf[5] = (Freq_Ctrol/100 == 0)?10:Freq_Ctrol / 100 % 10;
					Seg_Buf[6] = Freq_Ctrol / 10 % 10 + '.';
					Seg_Buf[7] = Freq_Ctrol % 10;
				
				break;
				case 1://湿度参数
					Seg_Buf[5] = 10;
					Seg_Buf[6] = Humidity_Para_Ctrol / 10 % 10 + '.';
					Seg_Buf[7] = Humidity_Para_Ctrol % 10;					
				break;
				case 2://距离参数
					Seg_Buf[5] = 10;
					Seg_Buf[6] = Distance_Ctrol / 10 % 10 + '.';
					Seg_Buf[7] = Distance_Ctrol % 10;							
				break;
			}
		break;
		
	
	}

}



/*其他显示函数*/
void Led_Proc()
{
	/*LED指示灯功能*/
	if(Seg_Disp_Mode == 3)//在参数界面下
	{
		ucLed[0] = Led_Freq_Show;	
		ucLed[1] = Led_Distance_Light;
		ucLed[2] = Led_Humidity_Light;	
	}
	else//不在参数界面
	{
		ucLed[0] = (Seg_Disp_Mode == 0);//频率界面下，指示灯L1点亮
		ucLed[1] = (Seg_Disp_Mode == 1);//湿度界面下，指示灯L2点亮
		ucLed[2] = (Seg_Disp_Mode == 2);//距离界面下，指示灯L3点亮
	}
//	ucLed[3] = (Freq_Value)
	Led_Disp(ucLed);
	/*继电器控制功能*/
	Relay_Work = (Distance * 100 > Distance_Ctrol);
	if(Relay_Work != Relay_Work_Old)
	{
			Relay(Relay_Work);
			Relay_Num++;
			EEPROM_Write(&Relay_Num,0,1);//传入继电器计数的地址, 0-EEPROM的写入起始地址 1-要写入的字节数量  
			EEPROM_Write(&EEPROM_Lock,8,1);//传入继电器加锁的地址 8-EEPROM的写入起始地址 	1-要写入的字节数量 	
			Relay_Work_Old = Relay_Work;
	}


}

/*数模转换*/
void AD_DA()
{
	unsigned char da_temp;
	Humidity_Value = Ad_Read(0x43) / 255.0 * 100.0; //转换成0-100
	if(Humidity_Value == 100)
		Humidity_Value = 99;//限幅
	
	if(Humidity_Para_Ctrol>Humidity_Value)
		Da_Write(1*51);
	else if(Humidity_Para_Ctrol<Humidity_Value)
		Da_Write(5*51);	
	else
	{
		da_temp = 4*(float)(80-Humidity_Value)/(float)(80-Humidity_Para_Ctrol);
		Da_Write(da_temp*51);
	}
}
/*超声波*/
void Get_Distance()
{
	unsigned char temp;
	temp = Ut_Wave_Data();


}


/* 定时器0中断初始化函数 */
void Timer0_Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0x7F;			//定时器时钟12T模式
	TMOD &= 0xF0;			//设置定时器模式
	TMOD |= 0x05;			//设置计数器模式
	TL0 = 0x00;				//设置定时初始值
	TH0 = 0x00;				//设置定时初始值
	TF0 = 0;				//清除TF0标志
	TR0 = 1;				//定时器0开始计时
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
	EA = 1;
}

void Timer2_Init(void)		//100微秒@12.000MHz
{
	AUXR &= 0xFB;			//定时器时钟12T模式
	T2L = 0x9C;				//设置定时初始值
	T2H = 0xFF;				//设置定时初始值
	AUXR |= 0x10;			//定时器2开始计时
	IE2 |= 0x04;
	EA = 1;
}

void Timer2_Isr(void) interrupt 12
{
	
}

/*定时器1中断服务函数*/
void Timer1_Isr(void) interrupt 3
{
	Key_Slow_Down++;
	Seg_Slow_Down++;
	if(++Seg_Pos == 8) Seg_Pos = 0;//数码管显示专用
	if(Seg_Buf[Seg_Pos]>='.')
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos] - '.',1);
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
	
	if(++Timer_1s == 1000)
	{
		Timer_1s = 0;
		Freq = (TH0 << 8) | TL0;
		TH0 = TL0 = 0;
	}
	if(Long_Press_Flag)
	{
		if(++Time_1s_Press >= 1000)
			Time_1s_Press = 1001;
	}
	else
		Long_Press_Flag = 0;
	if(Seg_Disp_Mode == 3)//在参数界面下
	{
		switch(Parameter_Disp_Mode)
		{
			case 0://参数频率界面
				Timer_1000ms_Humidity_Show = 0;
				Timer_1000ms_Distance_Show = 0;
				Led_Humidity_Light = 0;
				Led_Distance_Light = 0;
				if(++Timer_1000ms_Freq_Show == 100)
				{
					Led_Freq_Show ^=1;
					Timer_1000ms_Freq_Show = 0;
				}				
			break;
			case 1://参数湿度界面
				Timer_1000ms_Freq_Show = 0;
				Timer_1000ms_Distance_Show = 0;
				Led_Freq_Show = 0;
				Led_Distance_Light = 0;
				if(++Timer_1000ms_Humidity_Show == 100)
				{
					Led_Humidity_Light ^=1;
					Timer_1000ms_Humidity_Show = 0;
				}				
			break;
			case 2://参数距离界面
				Timer_1000ms_Freq_Show = 0;
				Timer_1000ms_Humidity_Show = 0;
				Led_Freq_Show = 0;
				Led_Humidity_Light = 0;
				if(++Timer_1000ms_Distance_Show == 100)
				{
					Led_Distance_Light ^=1;
					Timer_1000ms_Distance_Show = 0;
				}				
			break;
		}
	}
	else
	{
		Timer_1000ms_Freq_Show = 0;
		Timer_1000ms_Distance_Show = 0;
		Timer_1000ms_Humidity_Show = 0;
		Led_Freq_Show = 0;
		Led_Distance_Light = 0;
		Led_Humidity_Light = 0;
	}
}


/* Main */
void main()
{
	Sys_Init();
	PCA_Init();
	Timer2_Init();
	Timer1_Init();
	Timer0_Init();

	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		AD_DA();
		Get_Distance();
	}
}

