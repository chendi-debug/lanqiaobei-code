#ifndef _IIC_H
#define _IIC_H


void Da_Write(unsigned char dat);
unsigned char Ad_Read(unsigned char addr);
void IIC_Start(void); 
void IIC_Stop(void);  
bit IIC_WaitAck(void);  
void IIC_SendAck(bit ackbit); 
void IIC_SendByte(unsigned char byt); 
unsigned char IIC_RecByte(void); 
void EEPROM_Write(unsigned char* EEPROM_String,unsigned char addr,unsigned char num);
void EEPROM_Read(unsigned char* EEPROM_String,unsigned char addr,unsigned char num);

#endif
