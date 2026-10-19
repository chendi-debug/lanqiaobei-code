#include "Distance.h"
#include "intrins.h"
sbit Tx=P1^0;
sbit Rx=P1^1;


extern unsigned int Speed;

void Delay12us(void)	//@12.000MHz
{
	unsigned char data i;

	_nop_();
	_nop_();
	i = 33;
	while (--i);
}

void Ut_Wave_Init()
{
	unsigned char i;
	EA=0;
	for(i=0;i<8;i++)
	{
		Tx=1;
		Delay12us();
		Tx=0;
		Delay12us();
	}
	EA=1;
}

unsigned int Ut_Wave_Data()
{
	unsigned int time;
	CMOD=0;
	CH=CL=0;
	Ut_Wave_Init();
	CR=1;
	while((Rx==1)&&(CF==0));
	CR=0;
	if(CF==0)
	{
		time=((CH<<8)|CL);
		return (unsigned char)((unsigned long)time*Speed/20000);
	}
	else
	{
		CF=0;
		return 0;
	}
		
}