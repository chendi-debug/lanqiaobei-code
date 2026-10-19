#include "Init.h"

void System_Init(void)
{
	P0 = 0xff;//Led灯全灭
	P2 = P2 & 0x1f | 0x80;//选中Led通道
	P2 &= 0x1f;//关闭通道
	
	P0 = 0x00;//关闭继电器和蜂鸣器
	P2 = P2 & 0x1f | 0xA0;//选中Led通道	
	P2 &=0x1f;
}