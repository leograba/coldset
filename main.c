
#include <at89s8252.h>

/*********DEFINIÇÃO DOS PORTS E CONSTANTES**************/
// ports
# define    DQ          P3_1
# define    LCD_dados       P1
# define    backlight       P3_4
# define    RdWr            P2_6
# define    RS          P2_5
# define    E           P2_7
/********************************************************/

/*************** VARIÁVEIS EXTERNAS*********************/


char temp_atual[4]={'+', '0','0','0'};
//char temp_min[4]={'+','0','0','0'};
//char temp_max[4]={'+','0','0','0'};
unsigned char cont='0';
//unsigned char temperatura,minima,maxima;
char temp_msb,max_msb,min_msb;
unsigned char temp_lsb,max_lsb,min_lsb;


/********************************************************/

/**************funções de main.c*******************/
void inicia_display();
void atualiza_temp();
void atraso_250us(char );
void envia_lcd(char );
unsigned char le_bit(void);
void escreve_bit(char);
void le_temperatura();
/****************fim das funções*******************/


void DelayUs(int us)//rotina para 4MHz
{
        int i;
        for (i=0; i<us; i++);
}

//----------------------------------------
// Reset DS1820
//----------------------------------------
bit ResetDS1820(void)
{
        bit presence;
        DQ = 0;                 //pull DQ line low
        DelayUs(40);    // leave it low for about 490us
        DQ = 1;                 // allow line to return high
        DelayUs(20);     // wait for presence 55 uS
        presence = DQ;  // get presence signal
        DelayUs(25);    // wait for end of timeslot 316 uS
        return(presence);
}       // 0=presence, 1 = no part

//-----------------------------------------
// Read one bit from DS1820
//-----------------------------------------
bit ReadBit(void)
{
        unsigned char i=0;
        DQ = 0;         // pull DQ low to start timeslot
        DQ=1;
        for (i=0; i<1; i++); // delay 17 us from start of timeslot
        return(DQ); // return value of DQ line
}

//-----------------------------------------
// Write one bit to DS1820
//-----------------------------------------
void WriteBit(bit Dbit)
{
        unsigned char i=0;
    DQ=0;
        DQ = Dbit ? 1:0;
        DelayUs(5);                     // delay about 39 uS
        DQ = 1;
}

//-----------------------------------------
// Read 1 byte from DS1820
//-----------------------------------------
unsigned char ReadByte(void)
{
        unsigned char i;
        unsigned char Din = 0;
        for (i=0;i<8;i++)
        {
                Din|=ReadBit()? 0x01<<i:Din;
                DelayUs(6);
        }
        return(Din);
}

//-----------------------------------------
// Write 1 byte
//-----------------------------------------
void WriteByte(unsigned char Dout)
{
        unsigned char i;
        for (i=0; i<8; i++) // writes byte, one bit at a time
        {
                WriteBit((bit)(Dout & 0x1));            // write bit in temp into
                Dout = Dout >> 1;
        }
        DelayUs(5);
}



void le_temperatura(){
    unsigned char scratchpad[10];//guarda os 9 bytes do scrathpad(memória de rascunho)
    unsigned char k,temperatura;
    unsigned char t_msb,t_lsb;
    unsigned char tab[16]={'0','1','1','2','3','3','4','4','5','6','6','7','8','8','9','9'};

    ResetDS1820();
    //WriteByte(0xCC);//SKIP ROM, só serve pra um único termômetro
    //WriteByte(0x4E);//WRITE SCRATCHPAD
    //WriteByte(0x00);//ESCREVE NO TH
    //WriteByte(0x00);//ESCREVE NO TL
    //WriteByte(0x1F);//CONFIGURA RESOLUÇÃO 9 bits
    //com essa seção comentada, a resolução fica em 12bits
    //ResetDS1820();
    WriteByte(0xCC);//SKIP ROM, só serve pra um único termômetro
    WriteByte(0x44);//start conversion
    atraso_250us (0x3C);

    ResetDS1820();
    WriteByte(0xCC);//SKIP ROM, só serve pra um único termômetro
    WriteByte(0xBE);//READ SCRATCHPAD
    for(k=0;k<9;k++){
        scratchpad[k]=ReadByte();
    }
    temp_msb=t_msb=scratchpad[1];//bit mais significativo(de sinal)
    //P0=temp_msb;
    temp_lsb=t_lsb=scratchpad[0];//temperatura
    //P0=temp_lsb;
    //A comunicação com o sensor está funcionando no simulador PROTEUS ISIS
    if(!(t_msb&0xF0)){//se é uma temperatura positiva
        temp_atual[0]='+';
        //define o dígito decimal
        temp_atual[3]=tab[(t_lsb&0x0f)];
        //define o resto
        t_lsb=(t_lsb>>4)&0x0F;//exclui os bits decimais
        t_msb=(t_msb<<4)&0x70;//exclui os bits de sinal
        temperatura=t_lsb|t_msb;//junta temperatura inteira num unico byte
        if(temperatura<10)  {
            temp_atual[2]=('0'+temperatura);
            temp_atual[1]='0';
          }
        else if(temperatura>=10){
            temp_atual[1]=('0'+temperatura/10);
            temp_atual[2]=('0'+temperatura%10);
        }
    }
    else if((t_msb&0xF0)){//se é uma temperatura negativa
        temp_atual[0]='-';
        if(t_lsb){//complemento de 2
            t_msb=~t_msb;
            t_lsb=~(t_lsb)+1;//complemento de 2
        }
        else if(!t_lsb){//exceção à regra quando lsb=0
            t_msb=(~t_msb+1);
        }//fim do complemento de 2
        //define o dígito decimal
        temp_atual[3]=tab[(t_lsb&0x0f)];
        //define o resto
        t_lsb=t_lsb>>4;//exclui os bits decimais
        t_msb=(t_msb<<4)&0x70;//exclui os bits de sinal
        temperatura=t_lsb|t_msb;//junta temperatura inteira num unico byte
        if(temperatura<10)  {
            temp_atual[2]=('0'+temperatura);
            temp_atual[1]='0';
        }
        else if(temperatura>=10){
            temp_atual[1]=('0'+temperatura/10);
            temp_atual[2]=('0'+temperatura%10);
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
        LCD_dados=start[nvetor];                                        // Envia ao barramento de dados os valores de Inicialização
        atraso_250us(0x08);
        E=0;                                    // Reseta o Enable para confirmar o recebimento
        atraso_250us(0x08);                     // Espera 5ms

    }                                           // Fim do For
    RS=1;                                       // Prepara para escrita de dados no display
}                                               // Fim de inicia_display

void atualiza_temp(){
//atualiza temperatura atual
    RS=0;
    envia_lcd(0x85);//endereço da dezena
    RS=1;
    envia_lcd(temp_atual[0]);
    envia_lcd(temp_atual[1]);
    envia_lcd(temp_atual[2]);
    envia_lcd(',');
    envia_lcd(temp_atual[3]);

    //verifica se precisa atualizar Max e Min
    if(temp_msb>max_msb){//se for maior, atualiza a máxima
        //atualiza temperatura máxima
        max_msb=temp_msb;
        max_lsb=temp_lsb;
        RS=0;
        envia_lcd(0xCA);//endereço da dezena
        RS=1;
        envia_lcd(temp_atual[0]);
        envia_lcd(temp_atual[1]);
        envia_lcd(temp_atual[2]);
        envia_lcd(',');
        envia_lcd(temp_atual[3]);
    }
    else if((temp_msb==max_msb)&&(temp_lsb>max_lsb)){//se for maior, atualiza a máxima
        //atualiza temperatura máxima
        max_msb=temp_msb;
        max_lsb=temp_lsb;
        RS=0;
        envia_lcd(0xCA);//endereço da dezena
        RS=1;
        envia_lcd(temp_atual[0]);
        envia_lcd(temp_atual[1]);
        envia_lcd(temp_atual[2]);
        envia_lcd(',');
        envia_lcd(temp_atual[3]);
    }
     if(temp_msb<min_msb){//se for menor, atualiza a mínima
        //atualiza temperatura minima
        min_msb=temp_msb;
        min_lsb=temp_lsb;   
        RS=0;
        envia_lcd(0xC2);//endereço da dezena
        RS=1;
        envia_lcd(temp_atual[0]);
        envia_lcd(temp_atual[1]);
        envia_lcd(temp_atual[2]);
        envia_lcd(',');
        envia_lcd(temp_atual[3]);
    }
    else if((temp_msb==min_msb)&&(temp_lsb<min_lsb)){//se for menor, atualiza a mínima
        //atualiza temperatura minima   
        min_msb=temp_msb;
        min_lsb=temp_lsb;   
        RS=0;
        envia_lcd(0xC2);//endereço da dezena
        RS=1;
        envia_lcd(temp_atual[0]);
        envia_lcd(temp_atual[1]);
        envia_lcd(temp_atual[2]);
        envia_lcd(',');
        envia_lcd(temp_atual[3]);
    } 
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
    char fixos[2][17]={"Temp:+00,0","L:     /H:     "};//fixos[2][caracteres+1]
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
    unsigned char i,zero=0;
    inicia_display();
    primeira_impressao();
    for(i=0;i<0x25;i++){
    le_temperatura();//espera estabilizar
    }
    if(temp_msb<1) zero++;
    //min_msb=max_msb=temp_msb;//inicializa valores com a primeira leitura
    //min_lsb=max_lsb=temp_lsb;
    for(;;){//loop infinito
        if(temp_msb<1) zero++;
        le_temperatura();
        atualiza_temp();
        RS=0;
        envia_lcd(0x8f);
        RS=1;
        envia_lcd(('0'+zero));
    }
}

