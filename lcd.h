/**************************LCD.H*************************
Library for using the standard 16x2 LCD display
********************************************************/
#ifndef LCD_H
#define LCD_H

#include <at89s8252.h> //compatible with AT89S52
#include "delay.h"

/********* PORTS AND CONSTANTS DEFINITION****************/
# define	LCD			P2
# define	backlight		P2_3
# define	RdWr			P2_1
# define	RS			P2_0
# define	E			P2_2
/********************************************************/

/**************lcd.h functions declaration***************/
void bin2lcd(char, unsigned char, char );
void inicia_display();
void atualiza_temp() __critical;
void reset_lcd();
void envia_lcd(unsigned char ) ;
void layout_lcd();
/************end of the functions declaration************/

void inicia_display()
/**Initialize the display**/
{
	const unsigned char instr[]={0x06,0x0C,0x28,0x01};
	unsigned char brewdog,fullers;
	RS=RdWr=E=0;
	for(brewdog=0;brewdog!=120;brewdog++){		//generates a 15ms initialization delay
		atraso(250);
	}
	E=1;
	LCD=0x24;			//send 4 bit config command
	E=0;
	for(fullers=0;fullers!=40;fullers++){		//generates a 5ms initialization delay
		atraso(250);
	}
	for(brewdog=0;brewdog!=4;brewdog++){	//send the initialization instructions
		envia_lcd(instr[brewdog]);
		for(fullers=0;fullers!=40;fullers++){	//generates a 5ms initialization delay
			atraso(250);
		}
	}
}

void reset_lcd(){
/**Reset the display**/
	unsigned char bamberg;
	RS=RdWr=0;
	E=1;
	//LCD=0x04;
	LCD=(0x0f&LCD);//bit mask for the control bits
	E=0;
	E=1;
	//LCD=0x14;
	LCD=(0x0f&LCD)|(0x10);//bit mask for the control bits
	E=0;
	for(bamberg=0;bamberg!=8;bamberg++){	//generates a 2ms delay
		atraso(250);
	}
}

void envia_lcd(unsigned char letra)
/**Send byte to the LCD**/
{
	E=1;
	LCD=(0x0f&LCD)|(letra&0xf0);//bit mask for the control bits
	E=0;
	letra<<=4;//nibble swap
	E=1;
	LCD=(0x0f&LCD)|(letra&0xf0);//bit mask for the control bits
	E=0;
	atraso(250);//wait until the data is interpreted
}

void layout_lcd()
/**Reset the LCD and send the layout defined in "fixos" matrix**/
{
	__code char fixos[2][11]={"Temp:","L:     /H:"};//fixos[2][chars+1]
	char letra;
	reset_lcd();
	RS=0;
	envia_lcd(0x80);
	RS=1;
	for(letra=0;letra!=5;letra++){//send first line
		envia_lcd(fixos[0][letra]);
	}
	RS=0;
	envia_lcd(0xC0);
	RS=1;
	for(letra=0;letra!=10;letra++){//send second line
		envia_lcd(fixos[1][letra]);
	}
}

#endif
