
#include <at89s8252.h>

/*********DEFINIÇÃO DOS PORTS E CONSTANTES**************/
// ports
# define    DQ          P3_1
# define    ch0         P3_2
# define    ch1         P3_3
# define    LCD_dados       P1
# define    backlight       P2_3
# define    RdWr            P2_6
# define    RS          P2_5
# define    E           P2_7
/********************************************************/

/*************** VARIÁVEIS EXTERNAS*********************/


static volatile char temp_atual[4]={'+', '0','0','0'};
//char temp_min[4]={'+','0','0','0'};
//char temp_max[4]={'+','0','0','0'};
//unsigned char cont='0';
//unsigned char temperatura,minima,maxima;
char temp_msb,max_msb,min_msb,tcon_msb,tdiff_msb;
unsigned char temp_lsb,max_lsb,min_lsb,tcon_lsb,tdiff_lsb;
volatile char int_ctrl=0;


/********************************************************/

/**************funções de main.c*******************/
void DelayUs(int );
char ResetDS1820(void);
char ReadBit(void);
void WriteBit(char );
unsigned char ReadByte(void);
void WriteByte(unsigned char );
void bin2lcd(char ,unsigned char , char )
void le_temperatura() __critical;
void inicia_display();
void atualiza_temp() __critical;
void envia_lcd(char );
void primeira_impressao();
void atraso_250us(char );
void teclado0() __critical __interrupt 0;
void teclado1() __critical __interrupt 2;
void set_temp() __critical;
void set_temp_var() __critical;
/****************fim das funções*******************/


void DelayUs(int us)
{
        int i;
        for (i=0; i<us; i++);
}

char ResetDS1820(void)
{
        char presence;
        DQ = 0;                 //pull DQ line low
        DelayUs(40);    // leave it low for about 490us
        DQ = 1;                 // allow line to return high
        DelayUs(20);     // wait for presence 55 uS
        presence = DQ;  // get presence signal
        DelayUs(25);    // wait for end of timeslot 316 uS
        return(presence);
}       // 0=presence, 1 = no part

char ReadBit(void)
{
        unsigned char i=0;
        DQ = 0;         // pull DQ low to start timeslot
        DQ=1;
        for (i=0; i<1; i++); // delay 17 us from start of timeslot
        return(DQ); // return value of DQ line
}

void WriteBit(char Dbit)
{
        unsigned char i=0;
        DQ=0;
        DQ = Dbit ? 1:0;
        DelayUs(5);                     // delay about 39 uS
        DQ = 1;
}

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

void WriteByte(unsigned char Dout)
{
        unsigned char i;
        for (i=0; i<8; i++) // writes byte, one bit at a time
        {
                WriteBit(Dout&0x1);            // write bit in temp into
                Dout = Dout >> 1;
        }
        DelayUs(5);
}

void bin2lcd(char msb,unsigned char lsb, char lcd_add)
{
    unsigned char tab[16]={'0','1','1','2','3','3','4','4','5','6','6','7','8','8','9','9'};
    unsigned char temperatura;
    unsigned char atual[4];
    if(!(msb&0xF0)){//se é uma temperatura positiva
            atual[0]='+';
            //define o dígito decimal
            atual[3]=tab[(lsb&0x0f)];
            //define o resto
            lsb=(lsb>>4)&0x0F;//exclui os bits decimais
            msb=(msb<<4)&0x70;//exclui os bits de sinal
            temperatura=lsb|msb;//junta temperatura inteira num unico byte
            if(temperatura<10)  {
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
        if(temperatura<10)  {
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
    unsigned char k;

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
    atraso_250us (0x70);//0x3C
    //while(!DQ);//espera converter temperatura - só serve para alimentação não parasita

    ResetDS1820();
    WriteByte(0xCC);//SKIP ROM, só serve pra um único termômetro
    WriteByte(0xBE);//READ SCRATCHPAD
    for(k=0;k<9;k++){
        scratchpad[k]=ReadByte();
    }
    temp_msb=scratchpad[1];//bit mais significativo(de sinal)
    //P0=temp_msb;
    temp_lsb=scratchpad[0];//temperatura
    //P0=temp_lsb;
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

void envia_lcd(char dado)
{                       // Inicia função escreve_lcd
        E=1;                // Seta E do display
        atraso_250us(0x08);     // Aguarda para confirmar o Set de E
        LCD_dados=dado;         // Envia o dado para o display
        atraso_250us(0x08);     // Aguarda para confirmar o recebimento do dado
        E=0;                // Reseta o E para imprimir caracter
}                       // Fim da Função escreve_lcd

void primeira_impressao()
{
    char fixos[2][16]={"Temp:+00,0","L:     /H:     "};//fixos[2][caracteres+1]
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

void teclado0() __critical __interrupt 0 {//caso chave 0 seja acionada
    unsigned char i;
    backlight=1;
    TMOD=0x01;//timer 0 no modo 16 bits
    for(i=0;i<31;i++){//pra entrar no modo configuração de temp, tem que segurar por 2 segundos
        TH0=0x00;//conta o máximo possível
        TL0=0x00;
        TR0=1;//inicia contagem do timer 0
        while(!TF0){
            if(ch0){//se soltar o push button já era muleke!
                return;//não entra no modo configuração
            }
        }
        TF0=0;
        TR0=0;
    }//se passou daqui, entrou no modo configura temperatura
    int_ctrl=1;//indica ajuste de temperatura de controle
    for(i=0;i<92;i++){//pra entrar no modo configuração, tem que segurar por 8 segundos
        TH0=0x00;//conta o máximo possível
        TL0=0x00;
        TR0=1;//inicia contagem do timer 0
        while(!TF0){
            if(ch0){//se soltar o push button já era muleke!
                return;//não entra no modo configuração
            }
        }
        TF0=0;
        TR0=0;
    }//se passou daqui, entrou no modo configura temperatura
    int_ctrl=2;//indica ajuste de variação de temperatura de controle
    while(!ch0);//só retorna quando soltar o botão
}

void teclado1() __critical __interrupt 2 {//caso chave 1 seja acionada
    unsigned char i;
    backlight=1;
    TMOD=0x01;//timer 0 no modo 16 bits
    for(i=0;i<31;i++){//pra entrar no modo configuração de temp, tem que segurar por 2 segundos
        TH0=0x00;//conta o máximo possível
        TL0=0x00;
        TR0=1;//inicia contagem do timer 0
        while(!TF0){
            if(ch1){//se soltar o push button já era muleke!
                return;//não entra no modo configuração
            }
        }
        TF0=0;
        TR0=0;
    }//se passou daqui, entrou no modo configura temperatura
    int_ctrl=1;//indica ajuste de temperatura de controle
    for(i=0;i<92;i++){//pra entrar no modo configuração, tem que segurar por 8 segundos
        TH0=0x00;//conta o máximo possível
        TL0=0x00;
        TR0=1;//inicia contagem do timer 0
        while(!TF0){
            if(ch1){//se soltar o push button já era muleke!
                return;//não entra no modo configuração
            }
        }
        TF0=0;
        TR0=0;
    }//se passou daqui, entrou no modo configura temperatura
    int_ctrl=2;//indica ajuste de variação de temperatura de controle
    while(!ch1);
}

void set_temp() __critical{//seta temperatura de controle
    char msg[17]="Temp de Controle";
    unsigned char letra;
    unsigned char test=0,i;
    RS=0;
    envia_lcd(0x01);//reseta display
    RS=1;
    for(letra=0;letra<17;letra++){//envia "Temp de Controle"
        envia_lcd(msg[letra]);
    }
    TMOD=0x01;//timer 0 no modo 16 bits
    TH0=0x00;//conta o máximo possível
    TL0=0x00;
    while(1){
        for(i=0,test=0;i<31;i++){//2 segundos pra apertar uma tecla
            while(!TF0){
                if(ch0){
                    test++;
                    if(tcon_lsb>0){
                        tcon_lsb-=8;//subtrai meio grau
                    }
                    else{
                        tcon_lsb=0xf8;//se é 0 vai pro máximo
                        tcon_msb--;//e subtrai um desse
                    }
                }
                if(ch1){
                    test++;
                    if(tcon_lsb<0xff){
                        tcon_lsb+=8;//soma meio grau
                    }
                    else{
                        tcon_lsb=0x00;//se é máximo vai pro zero
                        tcon_msb++;//e soma um desse
                    }
                }
                //atualiza temperatura de controle no LCD
                
            }
            TF0=0;
            TR0=0;
        }
        if(!test){//se nenhuma tecla pressionada em 2 segundos, sai
            return;
        }
        test=0;//se foi pressionada, zera para fazer o teste novamente
    }
}

void set_temp_var() __critical{
    char msg[12]="Var de Temp";
    unsigned char letra;
    RS=0;
    envia_lcd(0x01);//reseta display
    RS=1;
    for(letra=0;letra<12;letra++){//envia "Var de Temp"
        envia_lcd(msg[letra]);
    }
    //laço infinito de teste
    while(1);
}



void main(){
    unsigned char i;
    inicia_display();
    primeira_impressao();
    for(i=0;i<0x10;i++){
        le_temperatura();//espera estabilizar
    }
    min_msb=max_msb=temp_msb;//inicializa valores com a primeira leitura
    min_lsb=max_lsb=temp_lsb;
    IE=0x85;//ativa as interrupções externas \int0 e \int1
    for(;;){//loop infinito
        le_temperatura();
        atualiza_temp();
        if(int_ctrl==1){//config. da temperatura de controle
            set_temp();
            int_ctrl=0;
        }
        else if(int_ctrl==2){//config. da variação da temperatura de controle
            set_temp_var();
            int_ctrl=0;
        }
    }
}

