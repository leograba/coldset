/**********System Parameters*************************/
/**
Microcontroller: AT89S52
Frequency of operation: f=12Mhz
Still, the DS18B20 and the LCD are working with 20MHz crystal on the bench.
Compiler: SDCC 3.3.0 #8604 (may/11/2013)
IDE: MCU 8051 IDE v1.4.7
Compiler directives: sdcc -mmcs51 --iram-size 256 --xram-size 0 --code-size 8192
--nooverlay --noinduction --verbose --debug --opt-code-speed -V --std-sdcc89 
--model-small   "main.c"
Tab Width: 1tab = 8 spaces
**/
/***********System Description***********************/
/**
This system periodically reads the temperature from a DS18B20 sensor and display
the reading in a HD44780 compatible 16x2 LCD. The display also shows the highest
and the lowest temperature readings since the circuit was powered on or reset.
Based on the temperature reading, a 5V relay is activated/deactivated, so the
temperature controlling can be interfaced to a power system, such as a 
refrigerator.
Two pushbutton let the user get/adjust the system configurations. The probe data
are sent through an RF module connected to the UART, so remote analysis or
control can be implemented. Of course the RF can be ommited and wires used.
**/

#include <at89s8252.h> //compatible with AT89S52
#include "lcd.h"
#include "serial.h"
#include "ds18b20.h"

/********* PORTS AND CONSTANTS DEFINITION****************/
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

/************* EXTERNAL VARIABLES (GLOBAL)***************/
volatile char tcon_msb=0x01,rele_max_msb,rele_min_msb;
volatile unsigned char tcon_lsb=0x40,tdiff_lsb=0x10,rele_max_lsb,rele_min_lsb;
__code char msg1[17]="Temp de Controle";
__code char msg2[14]="Variacao (dt)";
volatile __bit flag_backlight=0;
/********************************************************/

/************main.c functions declaration****************/
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
/***************end of the functions declaration*****************/


void bin2lcd(char msb,unsigned char lsb, char lcd_add)
/**Transcodes the temperature read to ASCII, rounding to 1 decimal, then send
the converted data to the LCD**/
{
	__code unsigned char tab[16]={'0','1','1','2','3','3','4','4','5','6','6','7','8','8','9','9'};
	unsigned char temperatura;
	unsigned char atual[4];
	if(!(msb&0xF0)){//if it is a positive temperature
		atual[3]=tab[(lsb&0x0f)];
		lsb=(lsb>>4)&0x0F;//excludes the decimal bits
		msb=(msb<<4)&0x70;//excludes the sign bits
		temperatura=lsb|msb;//get temperature into a single byte
		if(temperatura>=100){
			atual[0]='1';
			temperatura-=100;
		}
		else{
			atual[0]=' ';//if temperature is less than 100, space is used
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
	else if((msb&0xF0)){//if it is a negative temperature
		atual[0]='-';
		if(lsb){//two's complement
			msb=~msb;
			lsb=~(lsb)+1;//two's complement
		}
		else if(!lsb){//singular case when lsb=0
			msb=(~msb+1);
		}//end of two's complement
		//defines the decimal digit
		atual[3]=tab[(lsb&0x0f)];
		lsb=lsb>>4;//excludes the decimal bits
		msb=(msb<<4)&0x70;//excludes the sign bits
		temperatura=lsb|msb;//get temperature into a single byte
		if(temperatura<10)	{
			atual[2]=('0'+temperatura);
			atual[1]='0';
		}
		else if(temperatura>=10){
			atual[1]=('0'+temperatura/10);
			atual[2]=('0'+temperatura%10);
		}
	}
	//Send chars to the LCD specified address
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
/**Refreshes the LCD data**/
	//refreshes current temperature
	bin2lcd(temp_msb,temp_lsb,0x85);

	//check if max and min need refreshing
	if(temp_msb>max_msb){//if higher, refresh max
		max_msb=temp_msb;
		max_lsb=temp_lsb;
		bin2lcd(max_msb,max_lsb,0xCA);
	}
	else if((temp_msb==max_msb)&&(temp_lsb>max_lsb)){//if lower, refresh min
		max_msb=temp_msb;
		max_lsb=temp_lsb;
		bin2lcd(max_msb,max_lsb,0xCA);
	}
	 if(temp_msb<min_msb){//if lower, refresh min
		min_msb=temp_msb;
		min_lsb=temp_lsb;	
		bin2lcd(min_msb,min_lsb,0xC2);
	}
	else if((temp_msb==min_msb)&&(temp_lsb<min_lsb)){//if lower, refresh min
		min_msb=temp_msb;
		min_lsb=temp_lsb;
		bin2lcd(min_msb,min_lsb,0xC2);
	}
}

void atraso_long(){
/**Generates a visible delay and turn backlight off after it**/
	unsigned char joe;
		if(!flag_backlight){
			return;//only delay if it was requested by user pushbutton press
		}
		flag_backlight=0;//prepare for another interrupt
		for(joe=0;joe!=32;joe++){//generate approximately 2s delay
			if(flag_backlight){//if another interrupt, stop the current delay
			//but keep the flag set so another delay will start
				return;
			}
			 atraso_250us(250);
		}
		backlight=0;//turn backlight off after delay
}

void teclado0() __critical __interrupt 0 {
/**Interrupt activated when pushbutton 0 is pressed indicates that backlight
must be lit**/
	flag_backlight=1;
	select_mode(0);
}

void teclado1() __critical __interrupt 2 {
/**Interrupt activated when pushbutton 1 is pressed indicates that backlight
must be lit**/
	flag_backlight=1;
	select_mode(1);
}

void select_mode(__bit ctrl) __critical{
/**Check, based on the time that the user holds the pushbutton pressed, if must:
1) Print the current system configuration
2) Enter the setpoint (tcon) adjust mode 
3) Enter the hysteresis (tdiff) adjust mode**/
	unsigned char kaes;
	unsigned char letra;
	__code char status1[6]="Tcon:";
	__code char status2[7]="Tdiff:";
	backlight=1;	//light up the backlight
	TMOD=(TMOD&0xf0)|0x01;//timer 0 in the 16 bit mode
	for(kaes=0;kaes!=30;kaes++){//at least 2s hold until enter the setpoint adjust
		TH0=0x00;//count as high as possible
		TL0=0x00;
		TR0=1;//start timer 0 counting
		if(!ctrl){//if interrupt came from pushbutton 0
			while(!TF0){
				if(ch0){//if user release the pushbutton
					reset_lcd();//reset display
					RS=1;
					for(letra=0;letra!=5;letra++){
						envia_lcd(status1[letra]);
					}
					bin2lcd(tcon_msb,tcon_lsb,0x85);
					RS=0;
					envia_lcd(0xC0);//reset display
					RS=1;
					for(letra=0;letra!=6;letra++){
						envia_lcd(status2[letra]);
					}
					bin2lcd(0x00,tdiff_lsb,0xC6);
					atraso_long();
					backlight=1;//atraso_long turn backlight off
					flag_backlight=1;//need to set, otherwise backlight won't turn off
					layout_lcd();
					bin2lcd(min_msb,min_lsb,0xC2);
					bin2lcd(max_msb,max_lsb,0xCA);
					return;//don't enter system configuration
				}
			}
		}
		else if(ctrl){//if interrupt came from pushbutton 1
			while(!TF0){
				if(ch1){//if user release the pushbutton
					reset_lcd();//reset display
					RS=1;
					for(letra=0;letra!=5;letra++){
						envia_lcd(status1[letra]);
					}
					bin2lcd(tcon_msb,tcon_lsb,0x85);
					RS=0;
					envia_lcd(0xC0);//reset display
					RS=1;
					for(letra=0;letra!=6;letra++){
						envia_lcd(status2[letra]);
					}
					bin2lcd(0x00,tdiff_lsb,0xC6);
					atraso_long();
					backlight=1;//atraso_long turn backlight off
					flag_backlight=1;//need to set, otherwise backlight won't turn off
					layout_lcd();
					bin2lcd(min_msb,min_lsb,0xC2);
					bin2lcd(max_msb,max_lsb,0xCA);
					return;//don't enter system configuration
				}
			}
		}
		TF0=0;
		TR0=0;
	}//if got here, setpoint adjust mode
	reset_lcd();//reset display
	RS=1;
	for(letra=0;letra!=16;letra++){//send "Temp de Controle"
		envia_lcd(msg1[letra]);
	}
	for(kaes=0;kaes!=90;kaes++){//to configure hysteresis, must hold for 8s
		TH0=0x00;//count as high as possible
		TL0=0x00;
		TR0=1;//start timer 0 counting
		if(!ctrl){//if int came from pushbutton 0
			while(!TF0){
				if(ch0){//if user release the pushbutton
					set_temp();
					return;
				}
			}
		}
		else if(ctrl){//if int came from pushbutton 0
			while(!TF0){
				if(ch1){//if user release the pushbutton
					set_temp();
					return;
				}
			}
		}
		TF0=0;
		TR0=0;
	}//if got here, hysteresis adjust mode
	reset_lcd();//reset display
	RS=1;
	for(letra=0;letra!=13;letra++){//send "Var de Temp"
		envia_lcd(msg2[letra]);
	}
	if(!ctrl){//wait for user release of the pushbutton
		while(!ch0);
	}
	else if(ctrl){
		while(!ch1);
	}
	set_temp_var();
}

void set_temp(){
/**Setpoint (tcon) adjust mode**/
	//unsigned char letra;
	unsigned char iguana,kavalo,lhama=20,macaco=4;
	__bit test=0;
	//TMOD=(TMOD&0x0f)|0x10;//timer 1 in the 16 bit mode
	TH0=0x00;//count as far as possible
	TL0=0x00;
	TF0=0;
	TR0=1;
	while(1){
		for(iguana=0;iguana!=40;iguana++){//wait for the user to press a pushbutton
		/*for 250 repetitions (not sure this info is still true):
		Around 8ms to every iteration, measured in the IDE simulator, which gives 2.25s
		But by my calculations, should be 65535us every iteration, which gives 16.4s
		and is compatible with the Proteus simulation*/
			TF0=0;
			while(!TF0){
				//refresh LCD setpoint value
				bin2lcd(tcon_msb,tcon_lsb,0xC5);//right in the middle of the second line
				if(!ch0||!ch1){
					test=1;	//set the pressed button flag
					if(!ch0){//if push 0 is pressed
						//test=1;	//set the pressed button flag
						if(tcon_lsb!=0){
							tcon_lsb=tcon_lsb-0x08;//subtract 0.5 degree
						}
						else{
							tcon_lsb=0xf8;//if it is zero goes to 
							tcon_msb--;//and decrement the other
						}
						if(lhama!=1)
							lhama-=macaco;//grow the decrement speed
							if(macaco!=1)
								macaco--;
						if((tcon_msb==-4)&&(tcon_lsb==0xD8)){
							tcon_lsb+=8;//lock in -50 degree
						}
						atualiza_limites();
						if((rele_min_msb==-4)&&(rele_min_lsb==0xA8)){
							tcon_lsb+=8;//lock in tcon-tdiff=-53 degree
						}
						iguana=0;//reset the waiting counter
						//refresh LCD setpoint value
						bin2lcd(tcon_msb,tcon_lsb,0xC5);//right in the middle of the second line
					}
					if(!ch1){//if push 1 is pressed
						//test=1;	//set the pressed button flag
						if(tcon_lsb!=0xf8){
							tcon_lsb=tcon_lsb+0x08;//add half degree
						}
						else{
							tcon_lsb=0x00;//if it is max go to zero
							tcon_msb++;//and increment the other
						}
						if(lhama!=1)
							lhama-=macaco;//grow increment speed
							if(macaco!=1)
								macaco--;
						if((tcon_msb==0x07)&&(tcon_lsb==0x88)){
							tcon_lsb-=8;//lock in +120 degree
						}
						atualiza_limites();
						if((rele_max_msb==0x07)&&(rele_max_lsb==0xB8)){
							tcon_lsb-=8;//lock in tcon+tdiff=+123 degree
						}
						iguana=0;//reset the waiting counter
						//refresh LCD setpoint value
						bin2lcd(tcon_msb,tcon_lsb,0xC5);//right in the middle of the second line
					}
					for(kavalo=0;kavalo!=lhama;kavalo++){//debouncing
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
		if(!test){//if no pushbutton is pressed within 2s, exit
			RS=0;//print lcd default screen
			layout_lcd();
			bin2lcd(min_msb,min_lsb,0xC2);
			bin2lcd(max_msb,max_lsb,0xCA);
			TR0=0;//shutdown the timer
			return;
		}
		test=0;//if pushbutton was pressed, prepare for another testing
	}
}

void set_temp_var(){
/**Hysteresis (tdiff) adjust mode**/
	//unsigned char letra;
	unsigned char indio,oca,cocar=20,tribo=4;
	unsigned char anterior;
	__bit test=0;
	//TMOD=(TMOD&0x0f)|0x10;//timer 1 in the 16 bit mode
	TH0=0x00;//count as far as possible
	TL0=0x00;
	TR0=1;
	while(1){
		for(indio=0;indio!=40;indio++){//wait for the user to press a pushbutton
			TF0=0;
			while(!TF0){
				//refresh LCD hysteresis value
				bin2lcd(0x00,tdiff_lsb,0xC5);//right in the middle of the second line
				if(!ch0||!ch1){
					anterior=tdiff_lsb;
					test=1;	//set the pressed button flag
					if(!ch0){//se apertar
						if(tdiff_lsb>0x00){//lock in 0.0 degree
							if(((tdiff_lsb&0x0f)==0x02)||((tdiff_lsb&0x0f)==0x05)||((tdiff_lsb&0x0f)==0x07)||((tdiff_lsb&0x0f)==0x0A)||((tdiff_lsb&0x0f)==0x0D)||((tdiff_lsb&0x0f)==0x0F)){
								tdiff_lsb-=2;//subtracts
							}
							else	tdiff_lsb--;
						}
						if(cocar!=1){
							cocar-=tribo;//grow decrement speed
							if(tribo!=1)
								tribo--;
						}
						indio=0;//reset the waiting counter
						//break;
						//refresh LCD hysteresis value
						bin2lcd(0x00,tdiff_lsb,0xC5);//right in the middle of the second line
					}
					if(!ch1){//if push 1 is pressed
						if(tdiff_lsb<0xf0){//lock in +15.0 degree
							if(((tdiff_lsb&0x0f)==0x00)||((tdiff_lsb&0x0f)==0x03)||((tdiff_lsb&0x0f)==0x05)||((tdiff_lsb&0x0f)==0x08)||((tdiff_lsb&0x0f)==0x0B)||((tdiff_lsb&0x0f)==0x0D)){
									tdiff_lsb+=2;//add
							}
								else	tdiff_lsb++;
						}
						/*else{
							tdiff_lsb=0x00;//if it is max go to zero
							tdiff_msb++;//and add one here
						}*/
						if(cocar!=1){
							cocar-=tribo;//grow increment speed
							if(tribo!=1)
								tribo--;
						}
						indio=0;//reset the waiting counter
						//break;
						//efresh LCD hysteresis value
						bin2lcd(0x00,tdiff_lsb,0xC5);//right in the middle of the second line
					}
					atualiza_limites();
					//It is important that the following if consider values above +123 and below -53,
					//once some decimal values of tdiff are previously ignored
					if(((rele_min_msb==-4)&&(rele_min_lsb<0xB0))||((rele_max_msb==0x07)&&(rele_max_lsb>0xB0))){
						tdiff_lsb=anterior;
					}
					for(oca=0;oca!=cocar;oca++){//debouncing
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
		if(!test){//if no pushbutton is pressed within 2s, exit
			RS=0;//print lcd default screen
			layout_lcd();
			bin2lcd(min_msb,min_lsb,0xC2);
			bin2lcd(max_msb,max_lsb,0xCA);
			TR0=0;//shutdown the timer
			return;
		}
		test=0;//if pushbutton was pressed, prepare for another testing
	}
}

void atualiza_limites(){
/**Refreshes the superior and inferior relay activation limits**/
	rele_min_msb=rele_max_msb=tcon_msb;//tdiff_msb always equals to zero - was deleted
	CY=0;//carry flag
	rele_min_lsb=tcon_lsb-tdiff_lsb;//subtracts for tmin
	if(CY){//negative result
		rele_min_msb--;
	}
	CY=0;
	rele_max_lsb=tcon_lsb+tdiff_lsb;//adds for tmax
	if(CY){//overflow
		rele_max_msb++;
	}
}

void ctrl_releh(){
/**Activate/deactivate the relay based on the current temperature value**/
//Hysteresis is between tcon-tdiff and tcon+tdiff, so H=(tmin,tmax)=2*tdiff
	atualiza_limites();
//now compares to decide what to do
	if(temp_msb>rele_max_msb){//too hot, relay on
		releh=1;
		return;
	}
	else if(temp_msb==rele_max_msb){
		if(temp_lsb>rele_max_lsb){
			releh=1;
			return;
		}
	}
	if(temp_msb<rele_min_msb){//too cold, relay off
		releh=0;
	}
	else if(temp_msb==rele_min_msb){
		if(temp_lsb<rele_min_lsb){
			releh=0;
		}
	}
}


void main(){
	releh=0;// valvula=0;//start with the outputs off
	inicia_serial();
	inicia_display();
	layout_lcd();
	inicia_ds18b20();
	IE=0x85;//activate external interrupts \int0 e \int1
	backlight=0;
	for(;;){//infinite loop
		le_temperatura();
		atualiza_temp();
		ctrl_releh();
		atraso_long();
	}
}

