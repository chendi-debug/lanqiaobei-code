/*??????????*/
#include <STC15F2K60S2.H>
#include "Key.h"
#include "Seg.h"
#include "Led.h"
#include "Init.h"
#include "Uart.h"
#include "ultrasound.h"
#include "stdio.h"
#include "string.h"
#include "math.h"
#define abs(x) (x<0)?-(x):(x)
/*??????????*/
unsigned char Key_Val,Key_Down,Key_Old,Key_Up;//??????????
unsigned char Key_Slow_Down;//??????????????
unsigned char Seg_Buf[8]={10,10,10,10,10,10,10,10};//??????????????????
unsigned char Seg_Pos;//??????????????
unsigned int Seg_Slow_Down;//??????????????
unsigned char Distan_Slow_Down;
unsigned char ucLed[8]={0,0,0,0,0,0,0};//Led?????????????
unsigned char Seg_Disp_Mode = 0;//0-??????? 1-?????? 2-????????
unsigned char	Work_Mode = 0;//0-???? 1-??? 2-????
unsigned char Speed_Mode;//0-
pdata unsigned int Target_Vlaue[2] = {0,0};//????????
bit Target_Value_Flag = 0;
pdata unsigned int Current_Value[2]={0,0};//????õô????
unsigned char i;
pdata unsigned int Freq;
pdata unsigned int Timer_1s;//???1???
pdata signed int Speed_Value;
bit Para_Mode;//0-R 1-B
pdata float R_Value_10x = 10;
signed char B_Value = 0;
pdata unsigned int Diatance_Value;
/*????*/
pdata unsigned char Uart_Rx_Buf[9];     //???????
unsigned char Uart_Rx_Index = 0;        // ????????¡À?
unsigned char Uart_Rx_Tick = 0;         // ???????
bit Uart_Rx_Flag = 0;                   // ?????????
/*????*/
pdata unsigned int Run_Slow_Down;       // ?????????
pdata unsigned long Line_Distance;      // ??????(?????????)
/*???????????*/

void Key_Proc()
{
	if(Key_Slow_Down < 10)return;
	Key_Slow_Down=0;//??????????
	
	Key_Val =Key_Read();//??????????
	Key_Down = Key_Val & (Key_Old ^ Key_Val);//?§Ø?????????
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);//?????????????
	Key_Old = Key_Val;//??????????
	
	switch(Key_Down)
	{
		case 4:
			Para_Mode = 0;
			if(Work_Mode == 0)//????????
			{
				if(Target_Value_Flag==1)//??????????
				{
					Work_Mode = 2;
					Target_Value_Flag = 0;
				}
				else 
					Work_Mode = 0;
			}
			else if(Work_Mode == 1)//???????
			{
				if(Diatance_Value == 0)//?????
				{
					Work_Mode = 2;
				}
				else
				{
					Work_Mode = 1;
				}
			}
			else if(Work_Mode == 2)//????????
			{
				
				Work_Mode = 1;
			}
		break;
		case 5:
			if(Work_Mode == 0)//??????????????õô???¦Ë??????
			{
				Current_Value[0] = 0;
				Current_Value[1] = 0;		
			}

		break;
		case 8:
			if(++Seg_Disp_Mode == 3)Seg_Disp_Mode = 0;
		break;
		case 9:
			Para_Mode ^= 1;
		break;
		case 12:
			if(Para_Mode == 0)//R
			{
				R_Value_10x = (R_Value_10x == 20)?20:R_Value_10x+1;
			}
			else if(Para_Mode == 1)
			{
				B_Value = (B_Value == 90)?90:B_Value+5;
			}
		break;
		case 13:
			if(Para_Mode == 0)//R
			{
				R_Value_10x = (R_Value_10x == 10)?10:R_Value_10x-1;
			}
			else if(Para_Mode == 1)
			{
				B_Value = (B_Value == -90)?-90:B_Value-5;
			}
		break;
	}
}




/*???????????*/
void Seg_Proc()
{
	if(Seg_Slow_Down < 100) return;
	Seg_Slow_Down = 0;//???????????
	switch(Seg_Disp_Mode)
	{
		case 0://???????
			Seg_Buf[0] = 11;//L
			if(Work_Mode == 0)//??????
			{
					Seg_Buf[1] = Current_Value[0] / 100 % 10;
					Seg_Buf[2] = Current_Value[0] / 10 % 10;
					Seg_Buf[3] = Current_Value[0] % 10;
					for(i=1;i<3&&Seg_Buf[i]==0;i++)
						Seg_Buf[i] = 10;
					Seg_Buf[4] = 12;//-
					Seg_Buf[5] = Current_Value[1] / 100 % 10;
					Seg_Buf[6] = Current_Value[1] / 10 % 10;
					Seg_Buf[7] = Current_Value[1] % 10;	
					for(i=5;i<7&&Seg_Buf[i]==0;i++)
						Seg_Buf[i] = 10;				
			}
			else 
			{
					Seg_Buf[1] = Target_Vlaue[0] / 100 % 10;
					Seg_Buf[2] = Target_Vlaue[0] / 10 % 10;
					Seg_Buf[3] = Target_Vlaue[0] % 10;
					for(i=1;i<3&&Seg_Buf[i]==0;i++)
						Seg_Buf[i] = 10;
					Seg_Buf[4] = 12;//-
					Seg_Buf[5] = (unsigned char)(Line_Distance / 100 % 10);
					Seg_Buf[6] = (unsigned char)(Line_Distance / 10 % 10);
					Seg_Buf[7] = (unsigned char)(Line_Distance % 10);	
					for(i=5;i<7&&Seg_Buf[i]==0;i++)
						Seg_Buf[i] = 10;				
			}
		break;
		case 1://??????
				Seg_Buf[0] = 13;//E 
			if(Work_Mode == 2)//??????
			{
				Seg_Buf[1] = 1;
				Seg_Buf[2] = 10;
				Seg_Buf[3] = Speed_Value / 10000 % 10;
				Seg_Buf[4] = Speed_Value / 1000 % 10;
				Seg_Buf[5] = Speed_Value / 100 % 10;
				Seg_Buf[6] = Speed_Value / 10 % 10+'.';
				Seg_Buf[7] = Speed_Value % 10;
				for(i=3;i<6&&Seg_Buf[i]==0;i++)
					Seg_Buf[i] = 10;
			}
			else if(Work_Mode == 0)//??????
			{
				Seg_Buf[1] = 2;
				Seg_Buf[2] = 10;
				Seg_Buf[3] = 12;
				Seg_Buf[4] = 12;
				Seg_Buf[5] = 12;
				Seg_Buf[6] = 12;
				Seg_Buf[7] = 12;			
			}
			else if(Work_Mode == 1)//?????
			{
				Seg_Buf[1] = 3;
				Seg_Buf[2] = 10;
				Seg_Buf[3] = Diatance_Value / 10000 % 10;
				Seg_Buf[4] = Diatance_Value / 1000 % 10;
				Seg_Buf[5] = Diatance_Value / 100 % 10;
				Seg_Buf[6] = Diatance_Value / 10 % 10;
				Seg_Buf[7] = Diatance_Value % 10;
				for(i=3;i<7&&Seg_Buf[i]==0;i++)
					Seg_Buf[i] = 10;
			}
		break;
		case 2://????????
			Seg_Buf[0] = 14;//P
			Seg_Buf[1] = 10;
			Seg_Buf[2] = (unsigned char)(R_Value_10x) / 10 % 10+'.';
			Seg_Buf[3] = (unsigned char)(R_Value_10x) % 10;
			Seg_Buf[4] = 10;
			if(B_Value > 0&&B_Value<10)
			{
				Seg_Buf[5] = 10;
				Seg_Buf[6] = 10;
				Seg_Buf[7] = abs(B_Value % 10);
			}
			else if(B_Value >= 10)
			{
				Seg_Buf[5] = 10;
				Seg_Buf[6] = abs(B_Value / 10 % 10);
				Seg_Buf[7] = abs(B_Value % 10);				
			}
			else if(B_Value < 0)
			{
				Seg_Buf[5] = (B_Value < 0 && B_Value > -10)?10:12;
				Seg_Buf[6] = (Seg_Buf[5]==10)?12:abs(B_Value / 10 % 10);
				Seg_Buf[7] = abs(B_Value % 10);				
			}
			else
			{
				Seg_Buf[5] = 10;
				Seg_Buf[6] = 10;
				Seg_Buf[7] = abs(B_Value % 10);				
			}
		break;
	}

}



/*???????????*/
void Led_Proc()
{
	Led_Disp(ucLed);
	
}

/*???????????*/
void Uart_Proc()
{
	unsigned int Temp_x,Temp_y;
	if(Uart_Rx_Flag == 0)
		return;
	//???????????
	if(Uart_Rx_Tick >= 10)
	{
		Uart_Rx_Tick = 0;//???¦Í?????????
		Uart_Rx_Flag = 0;//??????¦Í?????????

		if(strcmp(Uart_Rx_Buf,"#") == 0)		//???¦Ë??
			printf("(%u,%u)",Current_Value[0],Current_Value[1]);
		else if(strcmp(Uart_Rx_Buf, "?")==0)		//?????
		{
			switch(Work_Mode)
			{
				case 0://??????
					printf("Idle");
				break;
				case 1://?????
					printf("Wait");
				break;
				case 2://??????
					printf("Busy");
				break;
			}
		}
		else if(Uart_Rx_Buf[0] == '(' && Uart_Rx_Buf[Uart_Rx_Index-1]==')')    	//????¦Ë??
		{
			if(sscanf(Uart_Rx_Buf,"(%u,%u)",&Temp_x,&Temp_y) == 2)//?????????????????????
			{
				if(Work_Mode == 0)//??????????????§¹
				{
					Target_Vlaue[0] = Temp_x;
					Target_Vlaue[1] = Temp_y;
					Target_Value_Flag = 1;
					printf("Got it");					
				}
				else
					printf("Busy");				
			}
			else
				printf("Error");
		}
		else
			printf("Error");
		memset(Uart_Rx_Buf, 0, 9);
		Uart_Rx_Index = 0;
	}
	
}
void Uart1_Isr() interrupt 4
{
	if (RI)
	{
		Uart_Rx_Flag = 1;
		Uart_Rx_Tick = 0; // ?????????
		Uart_Rx_Buf[Uart_Rx_Index] = SBUF;
		Uart_Rx_Index++;
		RI = 0;
		// ??????¦¶????????§¹
		if (Uart_Rx_Index > 9)
		{
			Uart_Rx_Flag = 0;
			Uart_Rx_Tick = 0;
			Uart_Rx_Index = 0;
			memset(Uart_Rx_Buf, 0, 9);
		}
	}
}
/*??????????????*/
void Distance_Proc()
{
	if(Distan_Slow_Down < 200)return;
		Distan_Slow_Down = 0;
	Diatance_Value = Ut_Wave_Data();
	Speed_Value = (R_Value_10x * 3.14 * Freq / 1000 + B_Value)*10;
}

/*????????????*/
void Run_Proc()
{
	signed long dx, dy;
	unsigned int step;

	if(Work_Mode != 2) return;

	// ?1s??????¦²??????Speed_Value/10 dm???ms???Speed_Value/10000 dm
	// ?100ms??????¦²????????Speed_Value/100 dm
	if(Run_Slow_Down < 1000) return;
	Run_Slow_Down = 0;

	// ?????????????
	dx = (signed long)Target_Vlaue[0] - (signed long)Current_Value[0];
	dy = (signed long)Target_Vlaue[1] - (signed long)Current_Value[1];
	Line_Distance = (unsigned long)sqrt((float)(dx*dx + dy*dy));

	// ????X??Y??????
	// ???????????:Speed_Value/10 dm/s
	// ???¦Ë: dm????§¹????
	if(dx != 0 || dy != 0)
	{
		// ???Speed_Value??§¹??????
		step = (Speed_Value > 0) ? (unsigned int)(Speed_Value / 10) : 1;
		if(step == 0) step = 1;

		if(Line_Distance <= step)
		{
			// ??????
			Current_Value[0] = Target_Vlaue[0];
			Current_Value[1] = Target_Vlaue[1];
			Line_Distance = 0;
			Work_Mode = 0; // ??????????????
		}
		else
		{
			// ???X??Y??????
			Current_Value[0] += (unsigned int)(dx * step / (signed long)Line_Distance);
			Current_Value[1] += (unsigned int)(dy * step / (signed long)Line_Distance);
		}
	}
	else
	{
		Line_Distance = 0;
		Work_Mode = 0;
	}
}

void Timer0_Init(void)		//1????@12.000MHz
{
	AUXR |= 0x80;			//????????1T??
	TMOD &= 0xF0;			//??????????
	TMOD |= 0x05;			//??????????
	TL0 = 0x00;				//??????????
	TH0 = 0x00;				//??????????
	TF0 = 0;				//???TF0???
	TR0 = 1;				//?????0??????
}
/* ?????1?§Ø????????? */
void Timer1_Init(void)		//1????@12.000MHz
{
	AUXR &= 0xBF;			//????????12T??
	TMOD &= 0x0F;			//??????????
	TL1 = 0x18;				//??????????
	TH1 = 0xFC;				//??????????
	TF1 = 0;				//???TF1???
	TR1 = 1;				//?????1??????
	ET1 = 1;				//???????1?§Ø?
	EA = 1;
}


/*?????1?§Ø??????*/
void Timer1_Isr(void) interrupt 3
{
	Key_Slow_Down++;
	Seg_Slow_Down++;
	Distan_Slow_Down++;
	if(Uart_Rx_Flag) Uart_Rx_Tick++;
	if(Work_Mode == 2) Run_Slow_Down++;
	
	if(++Seg_Pos == 8) Seg_Pos = 0;//???????????
	if(Seg_Buf[Seg_Pos]>='.')
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos] - '.',1);
	else
		Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);
	
	if(++Timer_1s == 1000)
	{
		Timer_1s = 0;
		Freq = (TH0 << 8) | TL0;
		TH0 = TL0 = 0;
		if(Freq>10000)
			Freq = 10000;
		else if(Freq<200)
			Freq = 200;
	}
}


/* Main */
void main()
{
	Sys_Init();
	PCA_Init();
	Timer1_Init();
	Timer0_Init();
	Uart1_Init();  //?????????
	while (1)
	{
		Key_Proc();
		Seg_Proc();
		Led_Proc();
		Distance_Proc();
		Uart_Proc();
		Run_Proc();
	}
}



