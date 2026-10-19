#include "Key.h"

unsigned char Key_Read()
{
	unsigned char temp = 0x00;
	if(P30 == 0) if(temp == 0)temp = 7;
	if(P31 == 0) if(temp == 0)temp = 6;else temp+=6;
	if(P32 == 0) if(temp == 0)temp = 5;else temp+=5;
	if(P33 == 0) if(temp == 0)temp = 4;else temp+=4;
  
	return temp;
	
}