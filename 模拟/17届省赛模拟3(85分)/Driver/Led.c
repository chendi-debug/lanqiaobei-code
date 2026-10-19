#include "Led.h"
idata unsigned char temp_1 = 0x00;
idata unsigned char temp_1_old = 0xff;


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
