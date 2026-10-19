/*头文件声明区*/
#include <STC15F2K60S2.H>
#include "Key.h"
#include "Seg.h"
#include "Led.h"
#include "Init.h"
#include "ultrasound.h"
#include "iic.h"

/*变量声明区*/
unsigned char Key_Val, Key_Down, Key_Old, Key_Up; // 按键专用变量
unsigned char Key_Slow_Down;                       // 按键减速计数器，10ms扫描一次
unsigned char Seg_Buf[8] = {10,10,10,10,10,10,10,10}; // 数码管显示缓冲，10=熄灭
unsigned char Seg_Pos;                             // 数码管扫描位索引 0~7
unsigned int  Seg_Slow_Down;                       // 数码管刷新减速计数器
unsigned char ucLed[8] = {0,0,0,0,0,0,0,0};       // LED显示缓冲
unsigned char i;
unsigned char AD_Slow_Down;                        // ADC采样减速计数器
unsigned char Distance_Slow_Down;                  // 测距减速计数器

unsigned char Seg_Disp_Mode; // 显示界面：0-频率 1-湿度 2-测距 3-参数
bit Freq_Mode = 0;           // 频率单位：0-Hz 1-KHz
bit Distance_Mode = 0;       // 距离单位：0-厘米 1-米
unsigned char Para_Mode;     // 参数界面子项：0-频率 1-湿度 2-距离
unsigned int  Freq;          // 频率测量值（Hz）
unsigned int  Time_1s;       // 1秒计时计数器
unsigned char Humidity;      // 湿度值（%RH）
unsigned int  Distance;      // 距离值（cm）
unsigned int Freq_Para = 9000;//频率参数
unsigned char Humidity_Para = 40;//湿度参数
unsigned char Distance_Para = 60;//距离参数
unsigned int Time_1s_Relay;//1秒计时
unsigned char Time_100ms;
static unsigned char Relay_Count = 0;//继电器吸合次数
bit Long_Press = 0;//长按标志


/*========== 按键处理函数 ==========*/
void Key_Proc()
{
    if(Key_Slow_Down < 10) return; // 10ms扫描一次，消抖
    Key_Slow_Down = 0;

    Key_Val  =  Key_Read();
    Key_Down =  Key_Val & (Key_Old ^ Key_Val);  // 下降沿：按下事件
    Key_Up   = ~Key_Val & (Key_Old ^ Key_Val);  // 上升沿：松开事件
    Key_Old  =  Key_Val;
	
		switch(Key_Up)
		{
			case 7:
				if(Time_1s_Relay > 1000)
					Relay_Count = 0;
			break;
		}
    switch(Key_Down)
    {
        case 4: // S4：循环切换显示界面
          if(++Seg_Disp_Mode == 4) Seg_Disp_Mode = 0;
				Freq_Mode = 0;
				Distance_Mode = 0;
				Para_Mode = 0;
        break;
				case 5:
					if(Seg_Disp_Mode == 3)
					{
						if(++Para_Mode == 3)Para_Mode = 0;
					}
				break;
				case 6:
					if(Seg_Disp_Mode == 2)//在测距界面下 
						Distance_Mode ^=1;
					if(Para_Mode == 0)//在频率参数状态下
						Freq_Para = (Freq_Para == 12000) ? 1000 :Freq_Para + 500;
					if(Para_Mode == 1)//在湿度参数状态下
						Humidity_Para = (Humidity_Para == 60) ? 10 :Humidity_Para + 10;
					if(Para_Mode == 2)//在距离参数状态下
						Distance_Para = (Distance_Para == 120) ? 10 : Distance_Para + 10;
					break;
				case 7:
					Long_Press = 1;
					if(Seg_Disp_Mode == 0)//在频率界面下 
						Freq_Mode ^=1;
					if(Para_Mode == 0)//在频率参数状态下
						Freq_Para = (Freq_Para == 1000) ? 12000 :Freq_Para - 500;
					if(Para_Mode == 1)//在湿度参数状态下
						Humidity_Para = (Humidity_Para == 10) ? 60 :Humidity_Para - 10;
					if(Para_Mode == 2)//在距离参数状态下
						Distance_Para = (Distance_Para == 10) ? 120 : Distance_Para - 10;
					break;
				break;
    }
}

/*========== 数码管内容刷新函数 ==========*/
void Seg_Proc()
{
    if(Seg_Slow_Down < 100) return; // 100ms刷新一次显示内容
    Seg_Slow_Down = 0;

	
		EEPROM_Write(&Relay_Count,0,1);
    switch(Seg_Disp_Mode)
    {
        case 0: // 频率界面，首位显示 F
					Seg_Buf[0] = 11; // F
					Seg_Buf[1] = 10;
					Seg_Buf[2] = 10;
					if(Freq_Mode == 0) // 单位：Hz，显示5位整数
					{
						Seg_Buf[3] = Freq / 10000 % 10;
						Seg_Buf[4] = Freq / 1000 % 10;
						Seg_Buf[5] = Freq / 100 % 10;
						Seg_Buf[6] = Freq / 10 % 10;
						Seg_Buf[7] = Freq % 10;
						// 消除高位无效零
						for(i = 3; i < 7 && Seg_Buf[i] == 0; i++)
								Seg_Buf[i] = 10;
					}
					else // 单位：KHz，显示3位（保留百位小数）
					{
						Seg_Buf[3] = 10;
						Seg_Buf[4] = 10;
						Seg_Buf[5] = Freq / 10000 % 10;
						Seg_Buf[6] = Freq / 1000  % 10 + '.';
						Seg_Buf[7] = Freq / 100   % 10;
						for(i = 5; i < 7 && Seg_Buf[i] == 0; i++)
								Seg_Buf[i] = 10;
					}
        break;
        case 1: // 湿度界面，首位显示 H
            Seg_Buf[0] = 12; // H
            Seg_Buf[1] = 10;
            Seg_Buf[2] = 10;
            Seg_Buf[3] = 10;
            Seg_Buf[4] = 10;
            Seg_Buf[5] = 10;
            Seg_Buf[6] = Humidity / 10 % 10;
            Seg_Buf[7] = Humidity % 10;
        break;
        case 2: // 测距界面，首位显示 A
					Seg_Buf[0] = 13; // A
					Seg_Buf[1] = 10;
					Seg_Buf[2] = 10;
					Seg_Buf[3] = 10;
					Seg_Buf[4] = 10;
					if(Distance_Mode == 0) // 单位：cm，显示3位整数
					{
						Seg_Buf[5] = Distance / 100 % 10;
						Seg_Buf[6] = Distance / 10 % 10;
						Seg_Buf[7] = Distance % 10;
						for(i = 5; i < 7 && Seg_Buf[i] == 0; i++)
								Seg_Buf[i] = 10;
					}
					else // 单位：m，格式 X.XX，小数点加在第5位
					{
						Seg_Buf[5] = Distance / 100 % 10 + '.'; // 整数位+小数点标记
						Seg_Buf[6] = Distance / 10 % 10;
						Seg_Buf[7] = Distance % 10;
					}
        break;

        case 3: // 参数界面，首位显示 P
          Seg_Buf[0] = 14; // P
					Seg_Buf[1] = Para_Mode + 1;
					switch(Para_Mode)
					{
						case 0://频率
							Seg_Buf[2] = 10;
							Seg_Buf[3] = 10;
							Seg_Buf[4] = 10;
							Seg_Buf[5] = Freq_Para / 10000 % 10;
							Seg_Buf[6] = Freq_Para / 1000 % 10 + '.';
							Seg_Buf[7] = Freq_Para / 100 % 10;
							if(Seg_Buf[5] == 0)Seg_Buf[5] = 10;
						break;
						case 1://湿度
							Seg_Buf[2] = 10;
							Seg_Buf[3] = 10;
							Seg_Buf[4] = 10;
							Seg_Buf[5] = 10;
							Seg_Buf[6] = Humidity_Para / 10 % 10;
							Seg_Buf[7] = Humidity_Para % 10;							
						break;
						case 2://距离
							Seg_Buf[2] = 10;
							Seg_Buf[3] = 10;
							Seg_Buf[4] = 10;
							Seg_Buf[5] = 10;
							Seg_Buf[6] = Distance_Para / 100 % 10 + '.';
							Seg_Buf[7] = Distance_Para / 10 % 10;								
						break;						
					}
        break;
    }
}

/*========== LED刷新函数 ==========*/
void Led_Proc()
{
	ucLed[0] = (Seg_Disp_Mode == 0);

    Led_Disp(ucLed);
}

/*========== ADC采样函数（湿度）==========*/
void AD_Proc(void)
{
	if(AD_Slow_Down < 60) return; // 60ms采样一次
	AD_Slow_Down = 0;

	Humidity = 20 * (Ad_Read(0x43) / 51.0);
	
	if(Humidity < Humidity_Para)
		Da_Write(1*51.0f);
	else if(Humidity > 80)
		Da_Write(5*51.0f);
	else	
		Da_Write((4/(80-Humidity_Para)*Humidity + (5-320/(80-Humidity_Para)))*51.0f);		
}

/*========== 超声波测距函数 ==========*/
void Distance_Proc(void)
{
    if(Distance_Slow_Down < 200) return; // 200ms测距一次，避免频繁阻塞
    Distance_Slow_Down = 0;
    Distance = Ut_Wave_Data(); // 调用测距，内部已限制最大阻塞约26ms
		if(Distance > Distance_Para)
		{
			Relay(1);
			Relay_Count++;
		}
		else
			Relay(0);
}

/*========== 定时器0初始化（计数模式，用于频率测量）==========*/
void Timer0_Init(void) // 外部计数@T0引脚
{
    AUXR &= 0x7F; // 定时器0时钟12T模式
    TMOD &= 0xF0;
    TMOD |= 0x05; // 定时器0设为计数模式（T0引脚输入）
    TL0 = 0x00;
    TH0 = 0x00;
    TF0 = 0;
    TR0 = 1;      // 启动计数
}

/*========== 定时器1初始化（1ms中断，系统节拍）==========*/
void Timer1_Init(void) // 1毫秒@12.000MHz
{
    AUXR &= 0xBF; // 定时器1时钟12T模式
    TMOD &= 0x0F;
    TL1 = 0x18;
    TH1 = 0xFC;
    TF1 = 0;
    TR1 = 1;
    ET1 = 1;      // 使能定时器1中断
    EA  = 1;      // 开总中断
}

/*========== 定时器1中断服务函数（1ms节拍）==========*/
void Timer1_Isr(void) interrupt 3
{
    // 各模块减速计数器累加
    Key_Slow_Down++;
    Seg_Slow_Down++;
    AD_Slow_Down++;
    Distance_Slow_Down++;

    // 数码管动态扫描：每1ms切换一位，8位轮流点亮
    if(++Seg_Pos == 8) Seg_Pos = 0;
    if(Seg_Buf[Seg_Pos] >= '.') // >= 46，说明该位需要显示小数点
        Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos] - '.', 1); // 减去'.'还原数字索引，带小数点
    else
        Seg_Disp(Seg_Pos, Seg_Buf[Seg_Pos], 0);

    if(++Time_1s == 1000)
    {
        Time_1s = 0;
        Freq = (TH0 << 8) | TL0; 
        TH0 = TL0 = 0;           
    }
		if(Long_Press)
		{
			if(++Time_1s_Relay == 1000)
			{
				Time_1s_Relay = 1001;
				/*继电器清零*/
			}
			else
			{
				Time_1s_Relay = 0;
				Long_Press = 0;
			}
			Long_Press = 0;
		}
		if(Para_Mode == 0)
		{
			if(++Time_100ms == 200)
			{
				Time_100ms = 0;
				ucLed[0] = 1;
			}
			else
				ucLed[0] = 0;
		}
		else
		{
			ucLed[0] = 0;
		}
}

/*========== 主函数 ==========*/
void main()
{
    Sys_Init();    // 系统初始化（关闭外设、初始化IO）
    PCA_Init();    // PCA初始化（超声波计时用）
    Timer1_Init(); // 定时器1：1ms系统节拍
    Timer0_Init(); // 定时器0：频率计数
		EEPROM_Read(&Relay_Count,0,1);

    while(1)
    {
        Key_Proc();      // 按键扫描与处理
        Seg_Proc();      // 数码管显示内容更新
        Led_Proc();      // LED状态更新（主循环调用已无实际作用，中断里已处理）
        AD_Proc();       // ADC采样（湿度）
        Distance_Proc(); // 超声波测距
    }
}
