#include "Distance.h"
#include "intrins.h"
sbit Tx = P1^0;
sbit Rx = P1^1;

void Delay12us(void)	//@12.000MHz
{
	unsigned char data i;

	_nop_();
	_nop_();
	i = 33;
	while (--i);
}

void PCA_Init()
{
	CMOD = 0x01;
	CCON = 0x00;
	CR = 0;
	
}

unsigned int Ut_Wave_Data()
{
	unsigned int time;
	Tx = 0;
	Delay12us();
	Tx = 1;
	Delay12us();	
	Tx = 0;

	while(Rx==0);
	CH=CL=0;
	CR=1;
	while(Rx==1);
	CR=0;
	time=(CH<<8)|CL;
	return time / 58;	
}