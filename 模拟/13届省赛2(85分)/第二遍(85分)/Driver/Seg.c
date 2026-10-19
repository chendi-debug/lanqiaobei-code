#include <Seg.h>
unsigned char Seg_Dula[] = {0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90,0xff
	,0xc1 //11 U
	,0x8c //12 P
	,0xc7 //13 L
	,0x88 //14 A
};//0-9 10-熄灭

void Seg_Disp(unsigned char wela,unsigned char dula,unsigned char point)
{
	//目的是清除上一次显示的残留段码，避免数码管串扰（鬼影）
	P0 = 0xff;          // 段码口输出全1（熄灭所有段）
	P2 = P2 & 0x1f | 0xe0; // 选通段码锁存器（通过P2口高3位控制，0xe0对应段码锁存器地址）
	P2 &= 0x1f;         // 关闭锁存器（恢复P2口高3位为0）
	
	P0 = 0x01 << wela;
	P2 = P2 & 0x1f | 0xc0;
  P2 &=	0x1f;
	
	P0 = Seg_Dula[dula];
	if(point)
		P0 &= 0x7f;
	P2 = P2 & 0x1f | 0xe0;
	P2 &= 0x1f;
}