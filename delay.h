/************************DELAY.H************************
Library that generates precision delay for the 8051,
as long as it is compiled with the SDCC 3.3.0.
Other situations must be reviewed if precision is intended
********************************************************/
#ifndef DELAY_H
#define DELAY_H

#include <at89s8252.h> //compatible with AT89S52

/**************delay.h functions declaration*******************/
void atraso(unsigned char);
void atraso_250us(unsigned char);
/***************end of the functions declaration***************/

void atraso(unsigned char tempo){
/**Generates delay of 255us maximum**/
	//This routine isn't recommended for delay of less than 30us (22us truly, but...)
	//The following instruction compensate the function offset
	tempo-=15;	//1us
	tempo/=6;	//8us
	//every while loop takes 6us
	while(tempo){	//while time in us is different from zero
		--tempo;//decrements every iteration
	}
}

void atraso_250us(unsigned char vezes)	// 125us for a 24MHz clock
/**Generates delay of approximately (250us x vezes)**/
{
		while(vezes){		// Wait until the number of times the 250us is called is enough
			atraso(250);
			--vezes;		//decrements every iteration
		}
}

#endif