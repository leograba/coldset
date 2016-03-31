#ifndef LCD_H
#define LCD_H

#include <at89s8252.h> //compatível com AT89S52
#include "delay.h"

/*********DEFINIÇÃO DOS PORTS E CONSTANTES**************/
// ports
# define	LCD			P2
# define	backlight		P2_3
# define	RdWr			P2_1
# define	RS			P2_0
# define	E			P2_2
/********************************************************/

/**************funções de lcd.h*******************/
void bin2lcd(char ,unsigned char , char );
void inicia_display();
void atualiza_temp() __critical;
void reset_lcd();
void envia_lcd(unsigned char ) ;
void layout_lcd();
//void atraso(unsigned char);
/****************fim das funções*******************/

void inicia_display()
{
	const unsigned char instr[]={0x06,0x0C,0x28,0x01};
	unsigned char i,j;
	RS=RdWr=E=0;
	for(i=0;i!=120;i++){		//gera um atraso de 15ms de inicialização
		atraso(250);
	}
	E=1;
	LCD=0x24;			//envia comando para operação em 4 bits
	E=0;
	for(j=0;j!=40;j++){		//gera um atraso de 5ms de inicialização
		atraso(250);
	}
	for(i=0;i!=4;i++){	//envia as instruções de inicialização
		envia_lcd(instr[i]);
		for(j=0;j!=40;j++){	//gera um atraso de 5ms de inicialização
			atraso(250);
		}
	}
}								// Fim de inicia_display

void reset_lcd(){
	unsigned char j;
	RS=RdWr=0;
	E=1;
	//LCD=0x04;
	LCD=(0x0f&LCD);//máscara para os bits de controle
	E=0;
	E=1;
	//LCD=0x14;
	LCD=(0x0f&LCD)|(0x10);//máscara para os bits de controle
	E=0;
	for(j=0;j!=8;j++){	//gera um atraso de 2ms de inicialização
		atraso(250);
	}
}

void envia_lcd(unsigned char letra)
{						// Inicia função escreve_lcd
	E=1;
	LCD=(0x0f&LCD)|(letra&0xf0);//máscara para os bits de controle
	E=0;
	letra<<=4;//faz o swap dos nibbles
	E=1;
	LCD=(0x0f&LCD)|(letra&0xf0);//máscara para os bits de controle
	E=0;
	atraso(250);//espera enviar o dado
}

void layout_lcd()
{
	__code char fixos[2][11]={"Temp:","L:     /H:"};//fixos[2][caracteres+1]
	char letra;
	reset_lcd();
	RS=0;
	envia_lcd(0x80);
	RS=1;
	for(letra=0;letra!=5;letra++){//envia linha superior
		envia_lcd(fixos[0][letra]);
	}
	RS=0;
	envia_lcd(0xC0);
	RS=1;
	for(letra=0;letra!=10;letra++){//envia linha inferior
		envia_lcd(fixos[1][letra]);
	}
}// fim primeira impressao

#endif
