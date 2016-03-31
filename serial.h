/**********************SERIAL.H*************************
Biblioteca para uso da interface serial do 8051
********************************************************/
#ifndef SERIAL_H
#define SERIAL_H

#include <at89s8252.h> //compatível com AT89S52

/**************funções de serial.h*******************/
void inicia_serial();
void envia_serial(unsigned char *);
/****************fim das funções*******************/

void inicia_serial(){
/**Inicia a interface serial do 8051**/
	SCON=0xD0;//inicia a comunicação serial no modo 3 (9 bits)
	TMOD=0x22;//timer 1 no modo 2 - auto recarregável
	TH1=0xfd;//valor para gerar a baud rate de 9600
	TR1=1;//liga o timer 1
	ES=1;//habilita a interrupção serial
}

void envia_serial(unsigned char *dados){
/**Envia pela serial 5 vezes o byte 0xff e depois envia a string dados, de 9 bytes**/
	unsigned char hobbit;
	for(hobbit=0;hobbit!=5;hobbit++){
		A=0xff;//carrega no acumulador
		TB8=P;//acerta o bit de paridade da transmissão
		SBUF=0xff;//envia caractere
		while(TI==0);//espera fim da transmissão
		TI=0;
	}
	for(hobbit=0;hobbit!=9;hobbit++){
		A=dados[hobbit];//carrega no acumulador
		TB8=P;//acerta o bit de paridade da transmissão
		SBUF=dados[hobbit];//envia caractere
		while(TI==0);//espera fim da transmissão
		TI=0;
	}
}

#endif
