#include <at89s8252.h> //compatível com AT89S52

/*********DEFINIÇÃO DOS PORTS E CONSTANTES**************/
// ports
# define	DQ			P3_1
//# define	DQ1			P3_0
/********************************************************/

/*************** VARIÁVEIS EXTERNAS*********************/
volatile char temp_msb,max_msb,min_msb;
volatile unsigned char temp_lsb,max_lsb,min_lsb;
/********************************************************/

/***********Tabela para cálculo do CRC*******************/
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

/**************funções de ds18b20.h*******************/
void DelayUs(int );
__bit rst_one_wire(void);
__bit ReadBit(void);
void WriteBit(char );
unsigned char ReadByte(void);
void WriteByte(unsigned char );
unsigned char calcula_crc(unsigned char *,__bit);
void inicia_ds18b20();
void le_temperatura() __critical;
void atraso_250us(unsigned char ) ;
/****************fim das funções*******************/

void DelayUs(int us)
{
        int amon_ra;
        for (amon_ra=0; amon_ra<us; amon_ra++);
}

__bit rst_one_wire(void)
{
        __bit presence;
        DQ = 0;                 //pull DQ line low
        DelayUs(40);    	// leave it low for about 490us
        DQ = 1;                 // allow line to return high
        DelayUs(20);    	// wait for presence 55 uS
        presence = DQ;  	// get presence signal
        DelayUs(25);    	// wait for end of timeslot 316 uS
        return(presence);
}// 0=presence, 1 = no part

__bit ReadBit(void)
{
        unsigned char zeus=0;
        DQ = 0;        		// pull DQ low to start timeslot
        DQ=1;
        for (zeus=0; zeus<1; zeus++); 	// delay 17 us from start of timeslot
        return(DQ); 		// return value of DQ line
}

void WriteBit(char Dbit)
{
    	DQ=0;
        DQ = Dbit ? 1:0;
        DelayUs(5);         	// delay about 39 uS
        DQ = 1;
}

unsigned char ReadByte(void)
{
        unsigned char horus;
        unsigned char Din = 0;
        for (horus=0;horus!=8;horus++)
        {
                Din|=ReadBit()? 0x01<<horus:Din;
                DelayUs(8);
        }
        return(Din);
}

void WriteByte(unsigned char Dout)
{
        unsigned char isis;
        for (isis=0; isis!=8; isis++) 		// writes byte, one bit at a time
        {
                WriteBit(Dout&0x1);	// write bit in temp into
                Dout = Dout >> 1;
        }
        DelayUs(6);
}

unsigned char calcula_crc(unsigned char *dado,__bit tipo){
	unsigned char joe,tamanho;
	unsigned char crc=0,crc_index;
	if(tipo){	//se tipo diferente de zero, faz crc do scratchpad
		tamanho=9;
	}		//se tipo igual a zero, faz crc da ROM
	else{
		tamanho=8;
	}
	for(joe=0;joe!=tamanho;joe++){//realiza oito iterações
		crc_index=crc^dado[joe];//o índice do crc é uma XOR com o dado
		crc=crc_lut[crc_index];//novo valor do crc atribuído
	}
	return crc;//se tudo deu certo, o valor do crc é zero
}

void inicia_ds18b20(){
	while(1){	//lê até receber valor válido
		le_temperatura();
		if(temp_msb!=5){
			if(temp_lsb!=0x50){
				//poderia ser return, mas fiz com break pensando em outras funcionalidades
				min_msb=max_msb=temp_msb;//inicializa valores com a primeira leitura
				min_lsb=max_lsb=temp_lsb;
				break;//quando para de receber +85 graus sai da rotina
			}
		}
	}
}

void le_temperatura() __critical{
	unsigned char scratchpad[9];//guarda os 9 bytes do scrathpad(memória de rascunho)
	unsigned char pipoca;

	rst_one_wire();
	//WriteByte(0xCC);//SKIP ROM, só serve pra um único termômetro
	//WriteByte(0x4E);//WRITE SCRATCHPAD
	//WriteByte(0x00);//ESCREVE NO TH
	//WriteByte(0x00);//ESCREVE NO TL
	//WriteByte(0x1F);//CONFIGURA RESOLUÇÃO 9 bits
	//com essa seção comentada, a resolução fica em 12bits
	//rst_one_wire();
	WriteByte(0xCC);//SKIP ROM, só serve pra um único termômetro
	WriteByte(0x44);//start conversion
	atraso_250us (0x70);//0x3C espera converter temperatura
	//while(!DQ);//espera converter temperatura - só serve para alimentação não parasita
	do{		//lê até achar um valor válido
		rst_one_wire();
		WriteByte(0xCC);//SKIP ROM, só serve pra um único termômetro
		WriteByte(0xBE);//READ SCRATCHPAD
		for(pipoca=0;pipoca!=9;pipoca++){
			scratchpad[pipoca]=ReadByte();
		}
	} while(calcula_crc(scratchpad,1));
	temp_msb=scratchpad[1];//bit mais significativo(de sinal)
	//P0=temp_msb;
	temp_lsb=scratchpad[0];//temperatura
	//P0=temp_lsb;
	envia_serial(scratchpad);//manda os dados do sensor pela serial(onde está o transmissor RF)
}

void atraso_250us(unsigned char vezes)	// 125us para 24MHz
{						// Inicio da Função atraso_display
		while(vezes){			// Espera o n* de vezes de Estouro zerar
			atraso(0xff);
			--vezes;		// Desconta do número de vezes
		}
}						// Fim da funçao atraso_display
