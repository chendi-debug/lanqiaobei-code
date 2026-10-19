#include "Led.h"
idata unsigned char temp_1 = 0x00;
idata unsigned char temp_1_old = 0xff;

/*LED*/
void Led_Disp(unsigned char* ucled)
{
	temp_1 = 0x00;
	temp_1 = (ucled[0]<<0) | (ucled[1]<<1) | (ucled[2]<<2) | (ucled[3]<<3) | 
					 (ucled[4]<<4) | (ucled[5]<<5) | (ucled[6]<<6) | (ucled[7]<<7);
	
	if(temp_1 != temp_1_old)
	{
		P0 = ~temp_1;
		P2 = P2 & 0x1f | 0x80;
		P2&= 0x1f;
		temp_1_old = temp_1;	
		
	}
}

idata unsigned char temp_2 = 0x00;
idata unsigned char temp_2_old = 0xff;


void Beep(bit enable)
{
	
	
	//设置蜂鸣器状态(P0.6)
	if(enable) 
		temp_2 |= 0x40;//将第6位置1，开启蜂鸣器
	else 
		temp_2 &= ~(0x40);//将第6位置0关闭蜂鸣器
	
	if(temp_2!=temp_2_old)
	{
		P0 = temp_2;
		P2 = (P2 & 0x1f) | 0xa0;
		P2 &= 0x1f;
		temp_2_old = temp_2;
	}
}
