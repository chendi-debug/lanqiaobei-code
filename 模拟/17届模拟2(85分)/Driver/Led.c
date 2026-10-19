#include "Led.h"

idata unsigned char temp_1 = 0x00;
idata unsigned char temp_1_old = 0xff;

void Led_Disp(unsigned char *ucLed)
{

	temp_1 = 0x00;
	temp_1 = ((ucLed[0] << 0) | (ucLed[1] << 1) | (ucLed[2] << 2) | (ucLed[3] << 3) |
						( ucLed[4] << 4) | (ucLed[5] << 5) | (ucLed[6] << 6) | (ucLed[7] << 7) ); 
	
	if(temp_1 != temp_1_old)
	{
		P0 = ~temp_1;
		P2 = (P2 & 0x1f) | 0x80;
		P2 &= 0x1f;
		temp_1_old = temp_1;

	}
	
}