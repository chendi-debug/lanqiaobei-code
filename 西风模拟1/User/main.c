/*å¤´æ–‡ä»¶å£°æ˜åŒº*/
#include <REGX52.H>
#include <Key.h>
#include <Seg.h>

/*å˜é‡å£°æ˜åŒº*/
unsigned char Key_Slow_Down;//æŒ‰é”®å‡é€Ÿä¸“ç”¨å˜é‡
unsigned int Seg_Slow_Down;//æ•°ç ç®¡å‡é€Ÿä¸“ç”¨å˜é‡
unsigned char Key_Val,Key_Down,Key_Old;//æŒ‰é”®ä¸“ç”¨å˜é‡
unsigned char Seg_Pos;//æ•°ç ç®¡æ‰«æä¸“ç”¨å˜é‡
unsigned char Seg_Buf[6]={10,10,10,10,10,10};//æ•°ç ç®¡æ˜¾ç¤ºæ•°æ®å­˜æ”¾æ•°ç»„
unsigned char Seg_Mode;//æ•°ç ç®¡æ¨¡å¼
unsigned int Timer_1000ms;//1000MS
unsigned char Time_Count = 30;//ÏµÍ³¼ÆÊ±±äÁ¿
bit System_Flag;//ÏµÍ³±êÖ¾Î» 0-ÔİÍ£ 1-¿ªÊ¼
unsigned char Set_Dat[3]={15,30,60};//ÉèÖÃ²ÎÊı´æ´¢Êı×é
unsigned char Set_Dat_Index = 1;//Êı×éÖ¸Õë
unsigned int Timer_500ms;//500ms±êÖ¾Î»
bit Seg_Flag;//ÊıÂë¹ÜÉÁË¸±êÖ¾Î»

/*°´¼ü´¦Àíº¯Êı*/
void Key_Proc()
{
	if(Key_Slow_Down)return;
	Key_Slow_Down=1;//°´¼ü¼õËÙ³ÌĞò
	
	Key_Val=Key_Read();//¶ÁÈ¡°´ÏÂµÄ¼üÂëÖµ
	Key_Down=Key_Val & (Key_Val^Key_Old);//²¶×½ÏÂ½µÑØ
	Key_Old=Key_Val;//¸¨ÖúÉ¨Ãè
	
	switch(Key_Down)
	{
		case 1://Æô¶¯
			if(Seg_Mode ==0)
				System_Flag=1;
		break;
		case 3://ÇĞ»»
			if(Seg_Mode==1)
			Time_Count=Set_Dat[Set_Dat_Index];
			Seg_Mode ^=1;//È¡·´
		break;
		case 4://ÉèÖÃ
			if(Seg_Mode)
			{
				if(++Set_Dat_Index==3)
				Set_Dat_Index = 0;
			}
		break;
		case 2://¸´Î»
			if(Seg_Mode==0)
				Time_Count=Set_Dat[Set_Dat_Index];
		break;
	}

}


/*ĞÅÏ¢´¦Àíº¯Êı*/
void Seg_Proc()
{
	if(Seg_Slow_Down)return;
	Seg_Slow_Down=1;//ÊıÂë¹Ü¼õËÙ³ÌĞò
	
	Seg_Buf[0] = Seg_Mode+1;
	if(Seg_Mode == 0)//ÏµÍ³´¦ÓÚÏÔÊ¾½çÃæ
	{
		Seg_Buf[4]=Time_Count / 10 % 10;
		Seg_Buf[5]=Time_Count % 10;
	}
	else//ÏµÍ³´¦ÓÚÉèÖÃ½çÃæ
	{
		if(Seg_Flag==1)
		{
		Seg_Buf[4]=Set_Dat[Set_Dat_Index] / 10 % 10;
		Seg_Buf[5]=Set_Dat[Set_Dat_Index] % 10;
		}
		else
		{
		Seg_Buf[4]=10;
		Seg_Buf[5]=10;			
		}
	}
}


/*ÆäËûÏÔÊ¾º¯Êı*/
void Led_Proc()
{
	if(Time_Count==0)
	{
		P1=0x00;//LEDµÆµãÁÁ
		P2_3=0;//·äÃùÆ÷´ò¿ª
	}
	else
	{
		P1=0xff;
		P2_3=1;
	}
}


/*¶¨Ê±Æ÷0³õÊ¼»¯º¯Êı*/
void Timer0Init(void)
{
	TMOD &=0xF0;	//ÉèÖÃ¶¨Ê±Æ÷Ä£Ê½
	TMOD |=0x01;	//ÉèÖÃ¶¨Ê±Æ÷Ä£Ê½
	TL0 = 0x18;	//ÉèÖÃ¶¨Ê±³õÊ¼Öµ
	TH0 = 0xFC;//ÉèÖÃ¶¨Ê±³õÊ¼Öµ
	TF0 = 0;
	TR0 = 1;
	ET0 = 1;
	EA = 1;
}

/*¶¨Ê±Æ÷0ÖĞ¶Ï·şÎñº¯Êı*/
void Timer0Sever()interrupt 1
{
	TL0 = 0x18;	//ÉèÖÃ¶¨Ê±³õÊ¼Öµ
	TH0 = 0xFC; //ÉèÖÃ¶¨Ê±³õÊ¼Öµ
	if(++Key_Slow_Down== 10)Key_Slow_Down=0;
	if(++Seg_Slow_Down== 500)Seg_Slow_Down=0;
	if(++Seg_Pos == 6)Seg_Pos=0;
	Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos]);
	if(System_Flag==1)
	{
		if(++Timer_1000ms == 1000)
		{
			Timer_1000ms = 0;
			Time_Count--;//1ÃëÖÓ¼õÒ»´Î
			if(Time_Count==255)
				Time_Count=0;
		}
  }
	if(++Timer_500ms==500)
	{
		Timer_500ms = 0;
		Seg_Flag ^=1;
	}
}


/*Main*/
void main()
{
	Timer0Init();
	while(1)
	{
	 Key_Proc();
   Seg_Proc();
   Led_Proc();
	}

}

