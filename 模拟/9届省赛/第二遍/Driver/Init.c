#include "Init.h"

void Sys_Init() //上电先执行
{
	/*关闭LED*/
	P0 = 0xff;
	P2 = P2 & 0x1f | 0x80;
	P2 &= 0x1f;
	
	
	/*关闭继电器蜂鸣器*/
	P0 = 0x00;//默认关闭
	P2 = P2 & 0x1f | 0xA0;
	P2 &= 0x1f;	
}
