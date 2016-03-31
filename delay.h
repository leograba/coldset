#ifndef DELAY_H
#define DELAY_H

#include <at89s8252.h> //compatível com AT89S52

/**************funções de delay.h*******************/
void atraso(unsigned char);
void atraso_250us(unsigned char);
/****************fim das funções*******************/

void atraso(unsigned char tempo){
	//essa rotina não é recomendada para tempos de menos que 30us(22us na verdade, massss....)
	//as instruções a seguir corrigem o offset da função
	tempo-=15;	//1us
	tempo/=6;	//8us
	//cada laço while demora 6us
	while(tempo){	//enquanto o tempo em us não for zero
		--tempo;//decrementa a cada ciclo do laço while
	}
}

void atraso_250us(unsigned char vezes)	// 125us para 24MHz
{						// Inicio da Função atraso_display
		while(vezes){			// Espera o n* de vezes de Estouro zerar
			atraso(250);
			--vezes;		// Desconta do número de vezes
		}
}						// Fim da funçao atraso_20us

#endif