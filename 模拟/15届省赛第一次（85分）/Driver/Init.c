#include <Init.h>

void Sys_Init()//关闭外设子函数
{
	P0 = 0xff;//Led全灭
	P2 = P2 & 0x1f | 0x80;//选中Led通道
	P2 &= 0x1f;//关闭通道
	
	P0 = 0x00;//关闭继电器和蜂鸣器
	P2 = P2 & 0x1f | 0xA0;//选中外设通道
	P2 &= 0x1f;//关闭通道
}


unsigned char temp = 0x00;
unsigned char temp_old = 0xFF;


void Buzz(unsigned char enable) // 蜂鸣器
{
	if(enable)
		temp |= 0x40;
	else
		temp &= ~0x40;
	
	if(temp != temp_old)
	{
		P0 = temp;
		P2 = P2 & 0x1F | 0xA0;
		P2 &= 0x1F;
		temp_old = temp;
	}
}

void Relay(unsigned char enable) // 继电器
{
	if(enable)
		temp |= 0x10;
	else
		temp &= ~0x10;
	
	if(temp != temp_old)
	{
		P0 = temp;
		P2 = P2 & 0x1F | 0xA0;
		P2 &= 0x1F;
		temp_old = temp;
	}
}