#include <Key.h>
unsigned char Key_Read()
{
	unsigned char temp = 0;
	if(P3_4 == 0) temp = 1;
	if(P3_5 == 0) temp = 2;
	if(P3_6 == 0) temp = 3;
	if(P3_7 == 0) temp = 4;
	
	return temp;
}