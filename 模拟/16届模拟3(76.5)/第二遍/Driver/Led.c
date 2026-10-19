#include "Led.h"

idata unsigned char temp1 = 0x00;
idata unsigned char temp1_old = 0xff;

void Led_Disp(unsigned char *ucLed)
{
	temp1 = 0;
	temp1 = (ucLed[0]<<0)|(ucLed[1]<<1)|(ucLed[2]<<2)|(ucLed[3]<<3)|
					(ucLed[4]<<4)|(ucLed[5]<<5)|(ucLed[6]<<6)|(ucLed[7]<<7);
	
	if(temp1 != temp1_old)
	{
		P0 = ~temp1;
		P2 = P2 & 0x1f | 0x80;
		P2 &= 0x1f;
		temp1_old = temp1;
	}
}

idata unsigned char temp2 = 0x00;
idata unsigned char temp2_old = 0xff;

void Relay(bit enable)
{
	if(enable)
		temp2 |= 0x10;
	else
		temp2 &= ~(0x10);
	
	if(temp2!=temp2_old)
	{
		P0 = temp2;
		P2 = P2 & 0x1f | 0xa0;
		P2 &= 0x1f;
		temp2_old = temp2;
	}
}
