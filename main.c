
#include <at89s8252.h>

/*********DEFINIÇÃO DOS PORTS E CONSTANTES**************/
// ports
# define	DQ			P3_1
# define	DQ1			P3_0
# define	ch0			P3_2
# define	ch1			P3_3
# define	LCD_dados		P1
# define	backlight		P3_4
# define	RdWr			P3_6
# define	RS			P3_5
# define	E			P3_7
# define	releh			P2_0
# define	valvula			P2_1
/********************************************************/

/*************** VARIÁVEIS EXTERNAS*********************/



char temp_msb,max_msb,min_msb,tcon_msb=0x01,rele_max_msb,rele_min_msb;//tdiff_msb=0x00;
unsigned char temp_lsb,max_lsb,min_lsb,tcon_lsb=0x40,tdiff_lsb=0x10,rele_max_lsb,rele_min_lsb;
__code char msg1[17]="Temp de Controle";
__code char msg2[14]="Variacao (dt)";
unsigned volatile char ctrl=0;

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

/**************funções de main.c*******************/
void DelayUs(int );
__bit rst_one_wire(void);
__bit ReadBit(void);
void WriteBit(char );
unsigned char ReadByte(void);
void WriteByte(unsigned char );
unsigned char calcula_crc(unsigned char [],__bit);
void bin2lcd(char ,unsigned char , char );
void le_temperatura() __critical;
void inicia_display();
void atualiza_temp() __critical;
void envia_lcd(char ) ;
void layout_lcd();
void primeira_impressao();
void atraso_250us(unsigned char ) ;
void atraso_long();
void teclado0() __critical __interrupt 0;
void teclado1() __critical __interrupt 2;
void select_mode() __critical;
void set_temp();
void set_temp_var();
void atualiza_limites();
void ctrl_releh();
void inicia_ds18b20();
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
        //unsigned char i=0;
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

unsigned char calcula_crc(unsigned char dado[9],__bit tipo){
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

void bin2lcd(char msb,unsigned char lsb, char lcd_add)
{
	__code unsigned char tab[16]={'0','1','1','2','3','3','4','4','5','6','6','7','8','8','9','9'};
	unsigned char temperatura;
	unsigned char atual[4];
	if(!(msb&0xF0)){//se é uma temperatura positiva
		atual[3]=tab[(lsb&0x0f)];
		//define o resto
		lsb=(lsb>>4)&0x0F;//exclui os bits decimais
		msb=(msb<<4)&0x70;//exclui os bits de sinal
		temperatura=lsb|msb;//junta temperatura inteira num unico byte
		if(temperatura>=100){
			atual[0]='1';
			temperatura-=100;
		}
		else{
			atual[0]=' ';//se temperatura menor que 100 fica em branco
		}
		if(temperatura<10)	{
			atual[2]=('0'+temperatura);
			atual[1]='0';
		}
		else if(temperatura>=10){
			atual[1]=('0'+temperatura/10);
			atual[2]=('0'+temperatura%10);
		}
	}
	else if((msb&0xF0)){//se é uma temperatura negativa
		atual[0]='-';
		if(lsb){//complemento de 2
			msb=~msb;
			lsb=~(lsb)+1;//complemento de 2
		}
		else if(!lsb){//exceção à regra quando lsb=0
			msb=(~msb+1);
		}//fim do complemento de 2
		//define o dígito decimal
		atual[3]=tab[(lsb&0x0f)];
		//define o resto
		lsb=lsb>>4;//exclui os bits decimais
		msb=(msb<<4)&0x70;//exclui os bits de sinal
		temperatura=lsb|msb;//junta temperatura inteira num unico byte
		if(temperatura<10)	{
			atual[2]=('0'+temperatura);
			atual[1]='0';
		}
		else if(temperatura>=10){
			atual[1]=('0'+temperatura/10);
			atual[2]=('0'+temperatura%10);
		}
	}
	//Envia os caracteres para o endereço selecionado
	RS=0;
	envia_lcd(lcd_add);
	RS=1;
	envia_lcd(atual[0]);
	envia_lcd(atual[1]);
	envia_lcd(atual[2]);
	envia_lcd(',');
	envia_lcd(atual[3]);
}

void le_temperatura() __critical{
	unsigned char scratchpad[10];//guarda os 9 bytes do scrathpad(memória de rascunho)
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
}

void inicia_display()
{
	char nvetor;						// Variavel que ira indicar o valor do vetor
	__code char start[5]={0x38,0x0A,0x01,0x06,0x0C};	// Carrega os Vetores com os valores de inicialização
	atraso_250us (0x3C);					// Gera um atraso de 15ms
	RS=0; RdWr=0;						// Prepara para escrita de instrução
	for(nvetor=0;nvetor!=8;nvetor++){			// Laço que controla o número do vetor que será lido
		E=1;						// Seta o Enable para a gravação ser possível
		LCD_dados=start[nvetor];			// Envia ao barramento de dados os valores de Inicialização
		E=0;						// Reseta o Enable para confirmar o recebimento
		atraso_250us(0x08);				// Espera 5ms
	}							// Fim do For
	RS=1;							// Prepara para escrita de dados no display
}								// Fim de inicia_display

void atualiza_temp() __critical{
//atualiza temperatura atual
	bin2lcd(temp_msb,temp_lsb,0x85);

	//verifica se precisa atualizar Max e Min
	if(temp_msb>max_msb){//se for maior, atualiza a máxima
		//atualiza temperatura máxima
		max_msb=temp_msb;
		max_lsb=temp_lsb;
		bin2lcd(max_msb,max_lsb,0xCA);
	}
	else if((temp_msb==max_msb)&&(temp_lsb>max_lsb)){//se for maior, atualiza a máxima
		//atualiza temperatura máxima
		max_msb=temp_msb;
		max_lsb=temp_lsb;
		bin2lcd(max_msb,max_lsb,0xCA);
	}
	 if(temp_msb<min_msb){//se for menor, atualiza a mínima
		//atualiza temperatura minima
		min_msb=temp_msb;
		min_lsb=temp_lsb;	
		bin2lcd(min_msb,min_lsb,0xC2);
	}
	else if((temp_msb==min_msb)&&(temp_lsb<min_lsb)){//se for menor, atualiza a mínima
		//atualiza temperatura minima	
		min_msb=temp_msb;
		min_lsb=temp_lsb;	
		bin2lcd(min_msb,min_lsb,0xC2);
	}
}

void reset_lcd(){
	RS=0;
	E=1;				// Seta E do display
	LCD_dados=0x01;			// Envia o dado para o display
	E=0;				// Reseta o E para imprimir caracter
	atraso_250us(8);		// 2ms para resetar
}

void envia_lcd(char dado)
{						// Inicia função escreve_lcd
		E=1;				// Seta E do display
		LCD_dados=dado;			// Envia o dado para o display
		E=0;				// Reseta o E para imprimir caracter
		atraso_250us(0x01);		// Ao menos 40us para envio do dado
}						// Fim da Função escreve_lcd

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


void atraso_250us(unsigned char vezes)	// 125us para 24MHz
{								// Inicio da Função atraso_display
		TMOD=(TMOD&0xf0)|0x02;				// Habilita o timer 0 no modo 2
		TH0=0x05;				// Carrega o TH0 para recarga com 5
		TL0=0x05;				// Carrega o TL0 com 5, onde irá contar 250 vezes
		while(vezes!=0){			// Espera o n* de vezes de Estouro zerar
			TR0=1;			// Inicia o Timer 0
			while(!TF0);		// Aguarda haver o OverFlow do Contador
			--vezes;			// Desconta do número de vezes
			TF0=0;				// Reseta o bit de Overflow
		}
		TR0=0;					//Desliga o timer 0
}							// Fim da funçao atraso_display

void atraso_long(){
	unsigned char joe;
		for(joe=0;joe!=32;joe++){//gera um atraso de aprox. 2s
			atraso_250us(250);
		}
}

void teclado0() __critical __interrupt 0 {//caso chave 0 seja acionada
	ctrl=0x0f;
}

void teclado1() __critical __interrupt 2 {//caso chave 1 seja acionada
	ctrl=0xff;
}

void select_mode() __critical{//verifica em qual ajuste entrar, ou se deve imprimir os parametros
	unsigned char kaes;
	unsigned char letra;
	__code char status1[6]="Tcon:";
	__code char status2[7]="Tdiff:";
	backlight=1;	//liga o backlight do lcd
	TMOD=(TMOD&0x0f)|0x10;//timer 1 no modo 16 bits
	for(kaes=0;kaes!=30;kaes++){//pra entrar no modo configuração de temp, tem que segurar por 2 segundos
		TH1=0x00;//conta o máximo possível
		TL1=0x00;
		TR1=1;//inicia contagem do timer 0
		if(ctrl==0x0f){//se int veio da tecla 0
			while(!TF1){
				if(ch0){//se soltar o push button já era muleke!
					reset_lcd();//reseta display
					RS=1;
					for(letra=0;letra!=5;letra++){
						envia_lcd(status1[letra]);
					}
					bin2lcd(tcon_msb,tcon_lsb,0x85);
					RS=0;
					envia_lcd(0xC0);//reseta display
					RS=1;
					for(letra=0;letra!=6;letra++){
						envia_lcd(status2[letra]);
					}
					bin2lcd(0x00,tdiff_lsb,0xC6);
					atraso_long();
					layout_lcd();
					bin2lcd(min_msb,min_lsb,0xC2);
					bin2lcd(max_msb,max_lsb,0xCA);
					return;//não entra no modo configuração
				}
			}
		}
		else if(ctrl==0xff){//se int veio da tecla 1
			while(!TF1){
				if(ch1){//se soltar o push button já era muleke!
					reset_lcd();//reseta display
					RS=1;
					for(letra=0;letra!=5;letra++){
						envia_lcd(status1[letra]);
					}
					bin2lcd(tcon_msb,tcon_lsb,0x85);
					RS=0;
					envia_lcd(0xC0);//reseta display
					RS=1;
					for(letra=0;letra!=6;letra++){
						envia_lcd(status2[letra]);
					}
					bin2lcd(0x00,tdiff_lsb,0xC6);
					atraso_long();
					layout_lcd();
					bin2lcd(min_msb,min_lsb,0xC2);
					bin2lcd(max_msb,max_lsb,0xCA);
					return;//não entra no modo configuração
				}
			}
		}
		TF1=0;
		TR1=0;
	}//se passou daqui, entrou no modo configura temperatura
	reset_lcd();//reseta display
	RS=1;
	for(letra=0;letra!=16;letra++){//envia "Temp de Controle"
		envia_lcd(msg1[letra]);
	}
	for(kaes=0;kaes!=90;kaes++){//pra entrar no modo configuração tem que segurar por 8 segundos
		TH1=0x00;//conta o máximo possível
		TL1=0x00;
		TR1=1;//inicia contagem do timer 0
		if(ctrl==0x0f){//se int veio da tecla 0
			while(!TF1){
				if(ch0){//se soltar o push button configura set_temp
					set_temp();
					return;
				}
			}
		}
		else if(ctrl==0xff){//se int veio da tecla 1
			while(!TF1){
				if(ch1){//se soltar o push button configura set_temp
					set_temp();
					return;
				}
			}
		}
		TF1=0;
		TR1=0;
	}//se passou daqui, entrou no modo configura variação de temp
	reset_lcd();//reseta display
	RS=1;
	for(letra=0;letra!=13;letra++){//envia "Var de Temp"
		envia_lcd(msg2[letra]);
	}
	if(ctrl==0x0f){
		while(!ch0);
	}
	else if(ctrl==0xff){
		while(!ch1);
	}
	set_temp_var();//config. da variação da temperatura de controle
}

void set_temp(){//seta temperatura de controle
	//unsigned char letra;
	unsigned char iguana,kavalo,lhama=20,macaco=4;
	__bit test=0;
	//TMOD=(TMOD&0x0f)|0x10;//timer 1 no modo 16 bits
	TH1=0x00;//conta o máximo possível
	TL1=0x00;
	TF1=0;
	TR1=1;
	while(1){
		for(iguana=0;iguana!=40;iguana++){//espera pelo menos um tempo pra apertar alguma tecla
		/*para 250 repetições:
		está fazendo uns 8ms pra cada laço for, medido no simulador do MCU
		o que dá uma espera de 2,25s
		porém segundo meus cálculos, deveria ser 65535us por laço
		portanto a espera é de 16,4s - resultado compatível com a
		simulação no Proteus*/
			TF1=0;
			while(!TF1){
				//atualiza temperatura de controle no LCD
				bin2lcd(tcon_msb,tcon_lsb,0xC5);//bem no meio da segunda linha
				if(!ch0||!ch1){
					test=1;	//seta a flag de tecla apertada
					if(!ch0){//se apertar
						//test=1;	//seta a flag de tecla apertada
						if(tcon_lsb!=0){
							tcon_lsb=tcon_lsb-0x08;//subtrai meio grau
						}
						else{
							tcon_lsb=0xf8;//se é 0 vai pro máximo
							tcon_msb--;//e subtrai um desse
						}
						if(lhama!=1)
							lhama-=macaco;//aumenta velocidade de decremento
							if(macaco!=1)
								macaco--;
						if((tcon_msb==-4)&&(tcon_lsb==0xD8)){
							tcon_lsb+=8;//trava em -50 graus
						}
						atualiza_limites();
						if((rele_min_msb==-4)&&(rele_min_lsb==0xA8)){
							tcon_lsb+=8;//trava em tcon-tdiff=-53 graus
						}
						iguana=0;//reseta o contador de espera
						//atualiza temperatura de controle no LCD
						bin2lcd(tcon_msb,tcon_lsb,0xC5);//bem no meio da segunda linha
					}
					if(!ch1){//se apertar
						//test=1;	//seta a flag de tecla apertada
						if(tcon_lsb!=0xf8){
							tcon_lsb=tcon_lsb+0x08;//soma meio grau
						}
						else{
							tcon_lsb=0x00;//se é máximo vai pro zero
							tcon_msb++;//e soma um desse
						}
						if(lhama!=1)
							lhama-=macaco;//aumenta velocidade de incremento
							if(macaco!=1)
								macaco--;
						if((tcon_msb==0x07)&&(tcon_lsb==0x88)){
							tcon_lsb-=8;//trava em +120 graus
						}
						atualiza_limites();
						if((rele_max_msb==0x07)&&(rele_max_lsb==0xB8)){
							tcon_lsb-=8;//trava em tcon+tdiff=+123 graus
						}
						iguana=0;//reseta o contador de espera
						//atualiza temperatura de controle no LCD
						bin2lcd(tcon_msb,tcon_lsb,0xC5);//bem no meio da segunda linha
					}
					for(kavalo=0;kavalo!=lhama;kavalo++){//atraso do debounce
						atraso_250us(0xff);
					}
					break;
				}
				else{
					lhama=20;
					macaco=4;
				}
			}
		}
		if(!test){//se nenhuma tecla pressionada em 2 segundos, sai
			RS=0;//volta o lcd para a condição normal e imprime Th e Tl atuais
			layout_lcd();
			bin2lcd(min_msb,min_lsb,0xC2);
			bin2lcd(max_msb,max_lsb,0xCA);
			TR1=0;//desliga o timer
			return;
		}
		test=0;//se foi pressionada, zera para fazer o teste novamente
	}
}

void set_temp_var(){//ajusta a variação da temperatura de controle
	//unsigned char letra;
	unsigned char indio,oca,cocar=20,tribo=4;
	unsigned char anterior;
	__bit test=0;
	//TMOD=(TMOD&0x0f)|0x10;//timer 1 no modo 16 bits
	TH1=0x00;//conta o máximo possível
	TL1=0x00;
	TR1=1;
	while(1){
		for(indio=0;indio!=40;indio++){//espera pelo menos um tempo pra apertar alguma tecla
			TF1=0;
			while(!TF1){
				if(!ch0||!ch1){
					anterior=tdiff_lsb;
					test=1;	//seta flag de tecla apertada
					if(!ch0){//se apertar
						if(tdiff_lsb>0x00){//trava em 0,0 graus
							
if(((tdiff_lsb&0x0f)==0x02)||((tdiff_lsb&0x0f)==0x05)||((tdiff_lsb&0x0f)==0x07)||((tdiff_lsb&0x0f)==0x0A)||((tdiff_lsb&0x0f)==0x0D)||((tdiff_lsb&0x0f)==0x0F)){
								tdiff_lsb-=2;//subtrai
							}
							else	tdiff_lsb--;
						}
						if(cocar!=1){
							cocar-=tribo;//aumenta velocidade de decremento
							if(tribo!=1)
								tribo--;
						}
						indio=0;//reseta o contador de espera
						//break;
					}
					if(!ch1){//se apertar
						if(tdiff_lsb<0xf0){//trava em +15,0
							
if(((tdiff_lsb&0x0f)==0x00)||((tdiff_lsb&0x0f)==0x03)||((tdiff_lsb&0x0f)==0x05)||((tdiff_lsb&0x0f)==0x08)||((tdiff_lsb&0x0f)==0x0B)||((tdiff_lsb&0x0f)==0x0D)){
									tdiff_lsb+=2;//soma
							}
								else	tdiff_lsb++;
						}
						/*else{
							tdiff_lsb=0x00;//se é máximo vai pro zero
							tdiff_msb++;//e soma um desse
						}*/
						if(cocar!=1){
							cocar-=tribo;//aumenta velocidade de incremento
							if(tribo!=1)
								tribo--;
						}
						indio=0;//reseta o contador de espera
						//break;
					}
					atualiza_limites();
					//É importante que o IF abaixo leve em conta valores maiores que +123 e menores que -53,
					//já que alguns valores decimais de tdiff são ignorados
					if(((rele_min_msb==-4)&&(rele_min_lsb<0xB0))||((rele_max_msb==0x07)&&(rele_max_lsb>0xB0))){
						tdiff_lsb=anterior;
						//atualiza temperatura de controle no LC
						bin2lcd(0x00,tdiff_lsb,0xC5);//bem no meio da segunda linha
					}
					for(oca=0;oca!=cocar;oca++){//atraso do debounce
						atraso_250us(0xff);
					}
					break;
				}
				else{
					cocar=20;
					tribo=4;
				}
			}
			//atualiza temperatura de controle no LC
			bin2lcd(0x00,tdiff_lsb,0xC5);//bem no meio da segunda linha
		}
		if(!test){//se nenhuma tecla pressionada em 2 segundos, sai
			RS=0;//volta o lcd para a condição normal e imprime Th e Tl atuais
			layout_lcd();
			bin2lcd(min_msb,min_lsb,0xC2);
			bin2lcd(max_msb,max_lsb,0xCA);
			TR1=0;//desliga o contador
			return;
		}
		test=0;//se foi pressionada, zera para fazer o teste novamente
	}
}

void atualiza_limites(){
	rele_min_msb=rele_max_msb=tcon_msb;//pois tdiff_msb é sempre igual a zero - nem existe mais
	CY=0;//flag de carry zerada
	rele_min_lsb=tcon_lsb-tdiff_lsb;//faz a subtração para tmin
	if(CY){//se setou carry, resultado negativo
		rele_min_msb--;
	}
	CY=0;
	rele_max_lsb=tcon_lsb+tdiff_lsb;//faz a soma para tmax
	if(CY){//se setou carry, estouro de resultado
		rele_max_msb++;
	}
}

void ctrl_releh(){//ativa e desativa o relé do compressor
//A histerese está entre tcon-tdiff e tcon+tdiff, ou seja, H=(tmin,tmax)=2*tdiff
	atualiza_limites();
//Agora faz a comparação para decidir se aciona o relé
	if(temp_msb>rele_max_msb){//se ultrapassa máxima liga o relé
		releh=1;
		return;
	}
	else if(temp_msb==rele_max_msb){
		if(temp_lsb>rele_max_lsb){
			releh=1;
			return;
		}
	}
	if(temp_msb<rele_min_msb){//se ultrapassa minima desliga o relé
		releh=0;
	}
	else if(temp_msb==rele_min_msb){
		if(temp_lsb<rele_min_lsb){
			releh=0;
		}
	}
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




void main(){
	releh=0; valvula=0;//começa com as saídas de controle desligadas
	inicia_display();
	layout_lcd();
	inicia_ds18b20();
	IE=0x85;//ativa as interrupções externas \int0 e \int1
	backlight=0;
	for(;;){//loop infinito
		le_temperatura();
		atualiza_temp();
		ctrl_releh();
		if(ctrl){//se recebeu interrupção, entra na select_mode
		//dessa maneira que estou fazendo, parece mais uma varredura elegante do que interrupção
			select_mode();
			ctrl=0;
			atraso_long();//deixa o backlight ligado no layout padrão por um tempo
			backlight=0;
		}
	}
}

