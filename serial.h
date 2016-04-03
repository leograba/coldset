/**********************SERIAL.H*************************
Library to use the 8051 UART interface
********************************************************/
#ifndef SERIAL_H
#define SERIAL_H

#include <at89s8252.h> //compatible with the AT89S52

/**************serial.h functions declaration*******************/
void inicia_serial();
void envia_serial(unsigned char *);
/**************end of the functions declaration*****************/

void inicia_serial(){
/**Starts the UART**/
	SCON=0xD0;//configures modo 3 (9 bits)
	TMOD=0x22;//timer 1 in modo 2 - auto reload
	TH1=0xfd;//value to generate a baud rate of 9600
	TR1=1;//start the timer 1
	ES=1;//enable serial interruption
}

void envia_serial(unsigned char *dados){
/**Send 5 times the byte 0xff and then send the "dados" string, that is 9 bytes long**/
	unsigned char hobbit;
	for(hobbit=0;hobbit!=5;hobbit++){
		A=0xff;//load data into the the Acc
		TB8=P;//configure the transmission parity bit
		SBUF=0xff;//send char
		while(TI==0);//wait for the end of the transmission
		TI=0;
	}
	for(hobbit=0;hobbit!=9;hobbit++){
		A=dados[hobbit];//load data into the Acc
		TB8=P;//configure the transmission parity bit
		SBUF=dados[hobbit];//send char
		while(TI==0);//wait for the end of the transmission
		TI=0;
	}
}

#endif
