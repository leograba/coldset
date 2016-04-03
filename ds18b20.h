/************************DS18B20.H***********************
Library for reading the DS18B20 temperature sensor
Only works for ONE sensor attached to an 8051 pin
To use more sensors, they must be attached to different
pins of the 8051, and the library must be modified
********************************************************/
#ifndef DS18B20_H
#define DS18B20_H

#include <at89s8252.h> //compatible with AT89S52
#include "delay.h"

/********* PORTS AND CONSTANTS DEFINITION****************/
// ports
# define	DQ			P3_1
//# define	DQ1			P3_0
/********************************************************/

/************* EXTERNAL VARIABLES (GLOBAL)***************/
volatile char temp_msb,max_msb,min_msb;
volatile unsigned char temp_lsb,max_lsb,min_lsb;
/********************************************************/

/***********Lookup table for the CRC check*******************/
__code unsigned char crc_lut[256]={
0,94, 188, 226, 97, 63, 221, 131, 194, 156, 126, 32, 163, 253, 31, 65,
157, 195, 33, 127, 252, 162, 64, 30, 95, 1, 227, 189, 62, 96, 130, 220,
35, 125, 159, 193, 66, 28, 254, 160, 225, 191, 93, 3, 128, 222, 60, 98,
190, 224, 2, 92, 223, 129, 99, 61, 124, 34, 192, 158, 29, 67, 161, 255,
70, 24, 250, 164, 39, 121, 155, 197, 132, 218, 56, 102, 229, 187, 89, 7,
219, 133, 103, 57, 186, 228, 6, 88, 25, 71, 165, 251, 120, 38, 196, 154,
101, 59, 217, 135, 4, 90, 184, 230, 167, 249, 27, 69, 198, 152, 122, 36,
248, 166, 68, 26, 153, 199, 37, 123, 58, 100, 134, 216, 91, 5, 231, 185,
140, 210, 48, 110, 237, 179, 81, 15, 78, 16, 242, 172, 47, 113, 147, 205,
17, 79, 173, 243, 112, 46, 204, 146, 211, 141, 111, 49, 178, 236, 14, 80,
175, 241, 19, 77, 206, 144, 114, 44, 109, 51, 209, 143, 12, 82, 176, 238,
50, 108, 142, 208, 83, 13, 239, 177, 240, 174, 76, 18, 145, 207, 45, 115,
202, 148, 118, 40, 171, 245, 23, 73, 8, 86, 180, 234, 105, 55, 213, 139,
87, 9, 235, 181, 54, 104, 138, 212, 149, 203, 41, 119, 244, 170, 72, 22,
233, 183, 85, 11, 136, 214, 52, 106, 43, 117, 151, 201, 74, 20, 246, 168,
116, 42, 200, 150, 21, 75, 169, 247, 182, 232, 10, 84, 215, 137, 107, 53};
/********************************************************/

/**************ds18b20.h functions declaration*******************/
__bit rst_one_wire(void);
__bit ReadBit(void);
void WriteBit(char );
unsigned char ReadByte(void);
void WriteByte(unsigned char );
unsigned char calcula_crc(unsigned char *,__bit);
void inicia_ds18b20();
void le_temperatura() __critical;
/***************end of the functions declaration*****************/

__bit rst_one_wire(void)
/**Reset the DS18B20 sensor (and possibly other one-wire devices attached to the line)**/
{
        __bit presence;
	DQ = 0;   			//pull DQ line low
	atraso(250);		//leave it low for about 490us
	atraso(250);		//using 600us instead
	atraso(100);
	DQ = 1;				// allow line to return high
	atraso(55);			// wait for presence 55 uS
	presence = DQ;  	// get presence signal
	atraso(200);		// wait for end of timeslot 316 uS
	atraso(116);
	return(presence);
}// 0=presence, 1 = no part

__bit ReadBit(void)
/** Read bit from the DS18B20**/
{
	unsigned char zeus=0;
	DQ = 0;        					// pull DQ low to start timeslot
	DQ=1;
	for (zeus=0; zeus<1; zeus++); 	// delay 17 us from start of timeslot
	return(DQ); 					// return value of DQ line
}

void WriteBit(char Dbit)
/**Send bit to the DS18B20**/
{
	DQ=0;
	DQ = Dbit ? 1:0;
	atraso(39);	// delay about 39 uS
	DQ = 1;
}

unsigned char ReadByte(void)
/**DS18B20 byte read**/
{
	unsigned char horus;
	unsigned char Din = 0;
	for (horus=0;horus!=8;horus++){
		Din|=ReadBit()? 0x01<<horus:Din;
		atraso(144);
	}
	return(Din);
}

void WriteByte(unsigned char Dout)
/**DS18B20 byte write**/
{
	unsigned char isis;
	for (isis=0; isis!=8; isis++){	// writes byte, one bit at a time
		WriteBit(Dout&0x1);	// write bit in temp into
			Dout = Dout >> 1;
	}
	atraso(114);
}

unsigned char calcula_crc(unsigned char *dado,__bit tipo){
/** Calculate the DS18B20 received data CRC and return the value
if zero is returned, data is consistent**/
	unsigned char joe,tamanho;
	unsigned char crc=0,crc_index;
	if(tipo){	//if "tipo" different from zero, do the scratchpad CRC
		tamanho=9;
	}		//if "tipo" equals zero, do the ROM CRC
	else{
		tamanho=8;
	}
	for(joe=0;joe!=tamanho;joe++){//do 8 iterations
		crc_index=crc^dado[joe];//the CRC index is a XOR with data
		crc=crc_lut[crc_index];//new CRC value
	}
	return crc;//if everything is ok, return 0
}

void inicia_ds18b20(){
/**Read the DS18B20 until it stop sending +85 degree celsius, indicating ready for operation**/
	while(1){	//Read until a valid value comes
		le_temperatura();
		if(temp_msb!=5){
			if(temp_lsb!=0x50){
				min_msb=max_msb=temp_msb;//initialize first reading values
				min_lsb=max_lsb=temp_lsb;
				break;//break when different from +85
			}
		}
	}
}

void le_temperatura() __critical{
/**Read DS18B20 until a good reading is done**/
	unsigned char scratchpad[9];//stores the scratchpad 9 bytes
	unsigned char pipoca;

	rst_one_wire();
	//WriteByte(0xCC);//SKIP ROM, only works with single probe connected
	//WriteByte(0x4E);//WRITE SCRATCHPAD
	//WriteByte(0x00);//write to the TH
	//WriteByte(0x00);//write to the TL
	//WriteByte(0x1F);//resolution set to 9 bits
	//otherwise it is configured as 12bits
	//rst_one_wire();
	WriteByte(0xCC);//SKIP ROM, only works with single probe connected
	WriteByte(0x44);//start conversion
	while(!DQ);//wait for conversion - only for non parasite power supply
	do{		//read until a good reading is done
		rst_one_wire();
		WriteByte(0xCC);//SKIP ROM, only works with single probe connected
		WriteByte(0xBE);//READ SCRATCHPAD
		for(pipoca=0;pipoca!=9;pipoca++){
			scratchpad[pipoca]=ReadByte();
		}
	} while(calcula_crc(scratchpad,1));
	temp_msb=scratchpad[1];//MSBit (sign bit)
	temp_lsb=scratchpad[0];//temperature
	envia_serial(scratchpad);//send data through the UART interface
}

#endif