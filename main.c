#include <at89s8252.h>

/*********DEFINIÇÃO DOS PORTS E CONSTANTES**************/
// ports
# define    termometro      P2_7
# define    LCD_dados       P1
# define    backlight       P3_4
# define    RdWr            P3_6
# define    RS              P3_5
# define    E               P3_7
/********************************************************/

/*************** VARIÁVEIS EXTERNAS*********************/


char temp_atual[4]={'+', '5','5','5'};
char temp_min[4]={'-','6','6','6'};
char temp_max[4]={'+','7','7','7'};
unsigned char cont='0';


/********************************************************/

/**************funções de main.c*******************/
void inicia_display();
void atualiza_display();
void atraso_250us(char );
void envia_lcd(char );
unsigned char reset(void);
unsigned char le_bit(void);
void escreve_bit(char);
unsigned char le_byte(void);
void escreve_byte(unsigned char);
void le_temperatura();
/****************fim das funções*******************/


unsigned char reset(void){
    unsigned char presenca;
    unsigned char delay;

    termometro=0;//derruba a linha para nível baixo
//gera atraso >480us
    for (delay=0;delay<72;delay++);//gera delay de 250us
    for (delay=0;delay<73;delay++);//gera delay de 250us
    termometro=1;//libera a linha para o termometro
    for (delay=0;delay<13;delay++);//espera a resposta
    presenca=termometro;
    for (delay=0;delay<72;delay++);//espera a resposta
    return presenca;//retorna reset mal sucedido
}

unsigned char le_bit(void){
    unsigned char i;
    termometro=0;//coloca em 0 para começar timeslot
    termometro=1;//coloca em 1
    for (i=0;i<2;i++);//gera delay de 15us
    return (termometro);//lê o valor do bit na linha
}

void escreve_bit(char bitval){
    unsigned char delay;
    if(bitval==1){//se deseja escrever 1, libera a linha
        termometro=0;//coloca em 0 pra começar timeslot
        termometro=1;
        for (delay=0;delay<10;delay++);//espera o resto do timeslot
    }
    if(bitval==0){
        termometro=0;
        for (delay=0;delay<10;delay++);//espera timeslot 60us<Tx<120us
        termometro=1;//se quer enviar 0, seta a linha só agora
    }
}

unsigned char le_byte(void){
    unsigned char valor=0;
    unsigned char i,delay;

    for (i=0;i<8;i++){//faz para 8 bits = 1 byte
        if (le_bit())//se lê bit '1'
            valor=valor|0x01;//adiciona ele ao msb
        valor<<1;//desloca 1 bit à esquerda
        for (delay=0;delay<3;delay++);//espera pelo resto do timeslot

    }
    return valor;
}

void escreve_byte(unsigned char byteval){
    unsigned char i,temp,delay;
    for(i=0;i<8;i++){
        temp=byteval&0x01;//temp recebe o valor do lsb
        byteval>>1;//desloca p/ direita, descartando o lsd
        escreve_bit(temp);//escreve efetivamente o bit
    }
    for (delay=0;delay<10;delay++);//gera um atraso que tinha no exemplo, sei lá pra quê
}

void le_temperatura(){
    unsigned char scratchpad[10];//guarda os 9 bytes do scrathpad(memória de rascunho)
    unsigned char k;
    unsigned char temp_msb,temp_lsb;

    reset();
    escreve_byte(0xCC);//SKIP ROM, só serve pra um único termômetro
    escreve_byte(0x4E);//WRITE SCRATCHPAD
    escreve_byte(0x00);//ESCREVE NO TH
    escreve_byte(0x00);//ESCREVE NO TL
    escreve_byte(0x1F);//CONFIGURA RESOLUÇÃO 9 bits

    reset();
    escreve_byte(0xCC);//SKIP ROM, só serve pra um único termômetro
    escreve_byte(0x44);//start conversion
    atraso_250us (0x3C);

    reset();
    escreve_byte(0xCC);//SKIP ROM, só serve pra um único termômetro
    escreve_byte(0xBE);//READ SCRATCHPAD
    for(k=0;k<9;k++){
        scratchpad[k]=le_byte();
    }
    temp_msb=scratchpad[1];//bit mais significativo(de sinal)
    temp_lsb=scratchpad[0];//temperatura
    if(temp_msb==0){//se é uma temperatura positiva
        temp_atual[0]='+';
        //define o dígito decimal
        if(temp_lsb&0x01)   temp_atual[3]='5';
        else            temp_atual[3]='0';
        //define o resto
        temp_lsb=temp_lsb>>1;//exclui o dígito decimal
        if(temp_lsb<10) {
            temp_atual[2]=('0'+temp_lsb);
            temp_atual[1]='0';
          }
        else if(temp_lsb>=10){
            temp_atual[1]=('0'+temp_lsb/10);
            temp_atual[2]=('0'+temp_lsb-(temp_atual[2]-'0')*10);
        }
    }
    else if(temp_msb!=0){//se é uma temperatura negativa
        temp_atual[0]='-';
        //define o dígito decimal
        if(temp_lsb&0x01)   temp_atual[3]='5';
        else            temp_atual[3]='0';
        //define o resto
        temp_lsb=temp_lsb>>1;//exclui o dígito decimal
        temp_msb=temp_msb&0x80;//máscara para o bit de sinal
        temp_lsb=temp_msb|temp_lsb;//junta o bit de sinal aos outros
        temp_lsb=~(temp_lsb)+1;//complemento de 2
        if(temp_lsb<10)  {
            temp_atual[2]=('0'+temp_lsb);
            temp_atual[1]='0';
         }
        else if(temp_lsb>=10){
            temp_atual[1]=('0'+temp_lsb/10);
            temp_atual[2]=('0'+temp_lsb-(temp_atual[2]-'0')*10);
        }
    }
}

void inicia_display()
{
    char nvetor;                                                // Variavel que ira indicar o valor do vetor
    char start[8]={0x38,0x38,0x38,0x38,0x0A,0x01,0x06,0x0C};    // Carrega os Vetores com os valores de inicialização
    atraso_250us (0x3C);                                        // Gera um atraso de 15ms
    RS=0; RdWr=0;                                               // Prepara para escrita de instrução
    for(nvetor=0;nvetor<8;nvetor++){                            // Laço que controla o número do vetor que será lido
        E=1;                                                    // Seta o Enable para a gravação ser possível
        atraso_250us(0x08);                                     // Espera 5ms
        LCD_dados=start[nvetor];                                // Envia ao barramento de dados os valores de Inicialização
        atraso_250us(0x08);
        E=0;                                    // Reseta o Enable para confirmar o recebimento
        atraso_250us(0x08);                     // Espera 5ms

    }                                           // Fim do For
    RS=1;                                       // Prepara para escrita de dados no display
}                                               // Fim de inicia_display

void atualiza_display(){
//atualiza temperatura atual
    RS=0;
    envia_lcd(0x85);//endereço da dezena
    RS=1;
    envia_lcd(temp_atual[0]);
    envia_lcd(temp_atual[1]);
    envia_lcd(',');
    envia_lcd(temp_atual[2]);
    envia_lcd(temp_atual[3]);
    envia_lcd(cont);
//atualiza temperatura minima
    RS=0;
    envia_lcd(0xC2);//endereço da dezena
    RS=1;
    envia_lcd(temp_min[0]);
    envia_lcd(temp_min[1]);
    envia_lcd(',');
    envia_lcd(temp_min[2]);
    envia_lcd(temp_min[3]);
//atualiza temperatura máxima
    RS=0;
    envia_lcd(0xCA);//endereço da dezena
    RS=1;
    envia_lcd(temp_max[0]);
    envia_lcd(temp_max[1]);
    envia_lcd(',');
    envia_lcd(temp_max[2]);
    envia_lcd(temp_max[3]);
}

void envia_lcd(char dado)
{                               // Inicia função escreve_lcd
        E=1;                    // Seta E do display
        atraso_250us(0x08);     // Aguarda para confirmar o Set de E
        LCD_dados=dado;         // Envia o dado para o display
        atraso_250us(0x08);     // Aguarda para confirmar o recebimento do dado
        E=0;                    // Reseta o E para imprimir caracter
}                               // Fim da Função escreve_lcd

void primeira_impressao()
{
    char fixos[2][17]={"Temp:+00,0","L:-11,1/H:+22,2"};//fixos[2][caracteres+1]
    char letra;
    RS=0;
    envia_lcd(0x80);
    RS=1;
    for(letra=0;letra<10;letra++){//envia linha superior
        envia_lcd(fixos[0][letra]);
    }
    RS=0;
    envia_lcd(0xC0);
    RS=1;
    for(letra=0;letra<15;letra++){//envia linha inferior
        envia_lcd(fixos[1][letra]);
    }
}// fim primeira impressao

void atraso_250us(char vezes)   // 125us para 24MHz
{                               // Inicio da Função atraso_display
        TMOD=0x02;              // Habilita o timer 0 no modo 2
        TH0=0x05;               // Carrega o TH0 para recarga com 5
        TL0=0x05;               // Carrega o TL0 com 5, onde irá contar 250 vezes
        while(vezes>0){         // Espera o n* de vezes de Estouro zerar
            TCON=0x10;          // Inicia o Timer 0
            while(!TF0);        // Aguarda haver o OverFlow do Contador
            vezes--;            // Desconta do número de vezes
            TF0=0;              // Reseta o bit de Overflow
        }
}                               // Fim da funçao atraso_display


void main(){

    inicia_display();
    primeira_impressao();
    for(;;){//loop infinito
        atualiza_display();
        le_temperatura();
        if(cont=='9') cont='0';
        else cont++;
    }
}



