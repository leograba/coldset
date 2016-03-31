/**********Parâmetros do Sistema*************************/
/**
Microcontrolador: AT89S52
Frequência ideal de operação: f=12Mhz
Ainda assim, sensor DS18B20 e LCD funcionando com f=20Mhz na prática.
Compilador usado: SDCC 3.3.0 #8604 (may/11/2013)
IDE usada: MCU 8051 IDE v1.4.7
Diretivas do SDCC:sdcc -mmcs51 --iram-size 256 --xram-size 0 --code-size 8192  --nooverlay --noinduction --verbose --debug --opt-code-speed -V --std-sdcc89 
--model-small   "main.c"
Comprimento de Tabulação (Tab Width):1 tab = 8 espaços (1tab = 8 spaces)
**/
/***********Descrição do Sistema***********************/
/**
Este sistema lê periodicamente a temperatura a partir de um sensor DS18B20 e
mostra esta temperatura em um display de LCD 16x2 HD44780 compatível. O display também mostra
a máxima e a mínima temperatura registradas desde que o circuito foi ligado ou o último reset.
Com base na temperatura lida, um relé de 5V é ligado ou desligado, de modo a controlar a temperatura
de uma geladeira.
Duas chaves pushbutton mostram/ajustam as configurações do controle de temperatura.
Os dados do sensor de temperatura são enviados através de um módulo emissor de RF conectado
à porta serial para outro microcontrolador, para análise posterior e monitoramento remoto.
**/

#include <at89s8252.h> //compatível com AT89S52
#include "lcd.h"//biblioteca do display LCD16x2 4 bits
#include "serial.h"//biblioteca da serial
#include "ds18b20.h"

/*********DEFINIÇÃO DOS PORTS E CONSTANTES**************/
// ports
# define	DQ			P3_1
//# define	DQ1			P3_0
# define	ch0			P3_2
# define	ch1			P3_3
# define	LCD			P2
# define	backlight		P2_3
# define	RdWr			P2_1
# define	RS			P2_0
# define	E			P2_2
# define	releh			P1_0
/********************************************************/

/*************** VARIÁVEIS EXTERNAS*********************/
volatile char tcon_msb=0x01,rele_max_msb,rele_min_msb;
volatile unsigned char tcon_lsb=0x40,tdiff_lsb=0x10,rele_max_lsb,rele_min_lsb;
__code char msg1[17]="Temp de Controle";
__code char msg2[14]="Variacao (dt)";
volatile __bit flag_backlight=0;
/********************************************************/

/**************funções de main.c*******************/
void bin2lcd(char ,unsigned char , char );
void atualiza_temp() __critical;
void atraso_long();
void teclado0() __critical __interrupt 0;
void teclado1() __critical __interrupt 2;
void select_mode(__bit) __critical;
void set_temp();
void set_temp_var();
void atualiza_limites();
void ctrl_releh();
/****************fim das funções*******************/



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

void atraso_long(){
	unsigned char joe;
		if(!flag_backlight){
			return;//se não recebeu interrupção, não gera atraso
		}
		flag_backlight=0;//zera a flag de tratamento de interrupção
		for(joe=0;joe!=32;joe++){//gera um atraso de aprox. 2s
			if(flag_backlight){//se recebe interrupção durante atraso, sai do atraso
			//mas mantem a flag setada para iniciar novo atraso
				return;
			}
			 atraso_250us(250);
		}
		backlight=0;//desliga backlight após atraso
}

void teclado0() __critical __interrupt 0 {//caso chave 0 seja acionada
	flag_backlight=1;
	select_mode(0);
}

void teclado1() __critical __interrupt 2 {//caso chave 1 seja acionada
	flag_backlight=1;
	select_mode(1);
}

void select_mode(__bit ctrl) __critical{//verifica em qual ajuste entrar, ou se deve imprimir os parametros
	unsigned char kaes;
	unsigned char letra;
	__code char status1[6]="Tcon:";
	__code char status2[7]="Tdiff:";
	backlight=1;	//liga o backlight do lcd
	TMOD=(TMOD&0xf0)|0x01;//timer 0 no modo 16 bits
	for(kaes=0;kaes!=30;kaes++){//pra entrar no modo configuração de temp, tem que segurar por 2 segundos
		TH0=0x00;//conta o máximo possível
		TL0=0x00;
		TR0=1;//inicia contagem do timer 0
		if(!ctrl){//se int veio da tecla 0
			while(!TF0){
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
					backlight=1;//a rotina atraso_long desliga o backlight
					flag_backlight=1;//precisa setar senão o display não apaga depois
					layout_lcd();
					bin2lcd(min_msb,min_lsb,0xC2);
					bin2lcd(max_msb,max_lsb,0xCA);
					return;//não entra no modo configuração
				}
			}
		}
		else if(ctrl){//se int veio da tecla 1
			while(!TF0){
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
					backlight=1;//a rotina atraso_long desliga o backlight
					flag_backlight=1;//precisa setar senão o display não apaga depois
					layout_lcd();
					bin2lcd(min_msb,min_lsb,0xC2);
					bin2lcd(max_msb,max_lsb,0xCA);
					return;//não entra no modo configuração
				}
			}
		}
		TF0=0;
		TR0=0;
	}//se passou daqui, entrou no modo configura temperatura
	reset_lcd();//reseta display
	RS=1;
	for(letra=0;letra!=16;letra++){//envia "Temp de Controle"
		envia_lcd(msg1[letra]);
	}
	for(kaes=0;kaes!=90;kaes++){//pra entrar no modo configuração tem que segurar por 8 segundos
		TH0=0x00;//conta o máximo possível
		TL0=0x00;
		TR0=1;//inicia contagem do timer 0
		if(!ctrl){//se int veio da tecla 0
			while(!TF0){
				if(ch0){//se soltar o push button configura set_temp
					set_temp();
					return;
				}
			}
		}
		else if(ctrl){//se int veio da tecla 1
			while(!TF0){
				if(ch1){//se soltar o push button configura set_temp
					set_temp();
					return;
				}
			}
		}
		TF0=0;
		TR0=0;
	}//se passou daqui, entrou no modo configura variação de temp
	reset_lcd();//reseta display
	RS=1;
	for(letra=0;letra!=13;letra++){//envia "Var de Temp"
		envia_lcd(msg2[letra]);
	}
	if(!ctrl){
		while(!ch0);
	}
	else if(ctrl){
		while(!ch1);
	}
	set_temp_var();//config. da variação da temperatura de controle
}

void set_temp(){//seta temperatura de controle
	//unsigned char letra;
	unsigned char iguana,kavalo,lhama=20,macaco=4;
	__bit test=0;
	//TMOD=(TMOD&0x0f)|0x10;//timer 1 no modo 16 bits
	TH0=0x00;//conta o máximo possível
	TL0=0x00;
	TF0=0;
	TR0=1;
	while(1){
		for(iguana=0;iguana!=40;iguana++){//espera pelo menos um tempo pra apertar alguma tecla
		/*para 250 repetições:
		está fazendo uns 8ms pra cada laço for, medido no simulador do MCU
		o que dá uma espera de 2,25s
		porém segundo meus cálculos, deveria ser 65535us por laço
		portanto a espera é de 16,4s - resultado compatível com a
		simulação no Proteus*/
			TF0=0;
			while(!TF0){
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
			TR0=0;//desliga o timer
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
	TH0=0x00;//conta o máximo possível
	TL0=0x00;
	TR0=1;
	while(1){
		for(indio=0;indio!=40;indio++){//espera pelo menos um tempo pra apertar alguma tecla
			TF0=0;
			while(!TF0){
				//atualiza temperatura de controle no LC
				bin2lcd(0x00,tdiff_lsb,0xC5);//bem no meio da segunda linha
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
						//atualiza temperatura de controle no LC
						bin2lcd(0x00,tdiff_lsb,0xC5);//bem no meio da segunda linha
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
						//atualiza temperatura de controle no LC
						bin2lcd(0x00,tdiff_lsb,0xC5);//bem no meio da segunda linha
					}
					atualiza_limites();
					//É importante que o IF abaixo leve em conta valores maiores que +123 e menores que -53,
					//já que alguns valores decimais de tdiff são ignorados
					if(((rele_min_msb==-4)&&(rele_min_lsb<0xB0))||((rele_max_msb==0x07)&&(rele_max_lsb>0xB0))){
						tdiff_lsb=anterior;
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
		}
		if(!test){//se nenhuma tecla pressionada em 2 segundos, sai
			RS=0;//volta o lcd para a condição normal e imprime Th e Tl atuais
			layout_lcd();
			bin2lcd(min_msb,min_lsb,0xC2);
			bin2lcd(max_msb,max_lsb,0xCA);
			TR0=0;//desliga o contador
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


void main(){
	releh=0;// valvula=0;//começa com as saídas de controle desligadas
	inicia_serial();
	inicia_display();
	layout_lcd();
	inicia_ds18b20();
	IE=0x85;//ativa as interrupções externas \int0 e \int1
	backlight=0;
	for(;;){//loop infinito
		le_temperatura();
		atualiza_temp();
		ctrl_releh();
		atraso_long();
	}
}

