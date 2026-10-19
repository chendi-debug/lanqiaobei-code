#include "Init.h"

void Sys_Init(void)
{
	P0 = 0xff;//Led全灭
	P2 = P2 & 0x1f | 0x80;//选择Led通道
	P2 &= 0x1f;//关闭通道 
	
	P0 = 0x00;//关闭继电器蜂鸣器
	P2 = P2 & 0x1f | 0xa0;//选中外设通道
	P2 &= 0x1f;//关闭通道
}