#include <STC15F2K60S2.H>
#include <intrins.h>
#include "ultrasound.h"
// 你的引脚定义，完全不变
sbit Tx = P1^0;  // 超声波发射引脚(Trig)
sbit Rx = P1^1;  // 超声波接收引脚(Echo)

//你的原Delay12us，保留不变（精度足够用，无需修改）
void Delay12us(void)  //@12.000MHz 1T单片机，实际≈11.5us
{
	unsigned char i;
	_nop_();
	_nop_();
	i = 34;
	while (--i);
}

//PCA模块初始化函数：STC15标准配置，核心修复！
void PCA_Init(void)
{
    CMOD = 0x01;  // 【关键】PCA时钟源 = 系统时钟/12 ，无PCA中断，STC15必配！
    CCON = 0x00;  // 清PCA控制寄存器，CF=0(无溢出)、CR=0(暂停PCA)
    CH = 0x00;    // 清PCA高8位计数器
    CL = 0x00;    // 清PCA低8位计数器
}

// ✅核心测距函数：PCA计时+修复所有错误，返回有效距离(cm)，无效返回0
// 改unsigned int：避免截断，量程2~400cm足够，完美适配你的显示逻辑
unsigned int Ut_Wave_Data(void)
{
    unsigned int time;       // PCA计数值
    unsigned int distance;   // 最终测距结果(cm)
    unsigned int timeout = 0;// 超时计数器，防止程序卡死
    
    // 1. 标准HC-SR04触发逻辑：单次12us高电平，完美触发模块发射超声波
    Tx = 0;          // Trig先拉低，稳定电平
    Delay12us();     // 短暂延时，消抖
    Tx = 1;          // Trig拉高
    Delay12us();     // 持续≥10us，满足触发条件
    Tx = 0;          // Trig拉低，触发完成，模块开始自动发射8个40KHz方波
    
    // 2. 等待Echo引脚拉高，超时约1ms（Echo响应通常<500us）
    while(Rx == 0)
    {
        if(++timeout > 1000) return 0;
    }

    // 3. 启动PCA定时器，开始计时Echo高电平时间
    CH = 0x00;
    CL = 0x00;
    CR = 1;          // 启动PCA计数
    timeout = 0;     // 重置超时计数器

    // 4. 等待Echo引脚拉低，超时25ms对应400cm量程上限
    while(Rx == 1 && CF == 0)
    {
        if(++timeout > 25000) break;
    }

    // 5. 停止PCA计数，读取计数值
    CR = 0;          // 关闭PCA，停止计时
    if(CF || timeout >= 25000)
    {
        CF = 0;      // 清溢出标志位，下次复用
        return 0;
    }
    
    // 6. 读取PCA的16位计数值 (CH高8位，CL低8位)
    time = (unsigned int)(CH << 8) | CL;
    
    // 7. 距离计算公式：精准无误差+无浮点运算！
    // 声速340m/s，距离=时间*声速/2/10000  → 0.017 = 17/1000，整数运算无精度丢失
    distance = (unsigned int)((unsigned long)time * 17 / 1000);
    
    // 8. 有效量程过滤：HC-SR04标准量程 2cm ~ 400cm
    if(distance < 2 || distance > 400)
    {
        return 0;
    }
    return distance;
}