//********************************************************
//
// Parametros para el compilador: +FM +t
//
#include <y:\picc\examples\ctype.h>
#include <y:\picc\examples\16f84A.h>
#include <y:\picc\examples\string.h>

#byte port_b=6 
#byte port_a=5

#define NUM_RELES		6
#define EEPROM			0
#define ST_COLGADO		0
#define ST_COMUNICANDO  1
#define ALL_OUTPUT		0
#define ALL_INPUT		0xff

#define CODE_LEN		1
#define INTS_PER_SECOND	23
#define RELES_TIMEOUT (INTS_PER_SECOND)*2

#fuses XT,NOWDT,PUT,PROTECT
#use delay (clock=3579545)

#use rs232 (RESTART_WDT,baud=9600, BITS=8,xmit=PIN_B7,rcv=PIN_B6)

byte const tabla[12]={'1','2','3','4','5','6','7','8','9','0','*','#'};
byte codigo[10];

struct
{
	byte ModeProg;
	byte cnt;
	byte TimeOut;		
}timers;

struct
{          
	byte TimeOut;
	byte address;
}TimersRel[NUM_RELES];		//Un timer para cada relé

void LeeIndicativo (void)
{
	int n;
	
	memset (codigo,0,10);
	
	for (n=0;n<CODE_LEN ;n++ )
	{
		codigo[n]=read_eeprom (n);	
		if (codigo[n]>'9')
			codigo[n]='0';
	
		putchar (codigo[n]);
	}
	puts("\n\r");
}

signed GetTone(byte bTimeOut)
{
	byte tone=0;
	//Esperamos a que entre un tono
	while (!(port_a&0x10))
	{
		 if (bTimeOut && !timers.TimeOut)
		 	return (-1);
	}
	tone = tabla[(port_a&0xf)-1];
	putchar (tone);
	
	//Esperamos a que el tono desaparezca
	while ((port_a&0x10));
	return (tone);
}

signed password (void)
{
	byte tone=0,n;
	byte buff[10];    
	memset (buff,0,10);
		
	LeeIndicativo();
	
	for (n=0;n<CODE_LEN;n++ )
	{
		tone=GetTone(FALSE);
		if (tone<0)
			return -1;
			
		buff[n]=tone;
	}
	return (strncmp (buff,codigo,CODE_LEN));
}

void main (void)
{       
	signed tone=0;
	byte cnt,n;

	set_tris_a (ALL_INPUT);
	set_tris_b (ALL_OUTPUT);

	set_rtcc(0);	
	setup_counters (RTCC_INTERNAL,RTCC_DIV_256);

	memset (&timers,0,sizeof (timers));
	timers.cnt=3;

	for (n=0;n<NUM_RELES;n++)
		TimersRel[n].TimeOut=0;
			                              
	puts ("Programa iniciado\n\r");
	enable_interrupts (INT_RTCC);
	enable_interrupts (GLOBAL);

	for (; ; )
	{		
		puts ("Esperando indicativo\n\r");
		if (password()==0)
		{
			puts ("Indicativo reconocido\n\r");
			port_b|=0x40;		//Encendemos el LED
			
			timers.TimeOut=(INTS_PER_SECOND)*5;	//5 SEGUNDOS
			tone = GetTone(TRUE);
			
			if (tone>-1)
			{
				switch (tone)
				{
					case '0':
						port_b=0;
					break;
	
					case '1':
						port_b^=1;
						TimersRel[0].TimeOut=RELES_TIMEOUT;
						TimersRel[0].address=1;
					break;
	
					case '2':
						port_b^=2;
						TimersRel[1].TimeOut=RELES_TIMEOUT;
						TimersRel[1].address=2;
					break;
	
					case '3':
						port_b^=4;
						TimersRel[2].TimeOut=RELES_TIMEOUT;
						TimersRel[2].address=4;
					break;
	
					case '4':
						port_b^=8;
						TimersRel[3].TimeOut=RELES_TIMEOUT;
						TimersRel[3].address=8;
					break;
	
					case '5':
						port_b^=0x10;
						TimersRel[4].TimeOut=RELES_TIMEOUT;
						TimersRel[4].address=10;
					break;	
					
					case '6':
						port_b^=0x20;
						TimersRel[5].TimeOut=RELES_TIMEOUT;
						TimersRel[5].address=20;
					break;	
					
					case '#':		//PROGRAMACIÓN 
					
						timers.ModeProg=TRUE;
						puts ("Modo programación\n\r");	
						for (cnt=0;cnt<CODE_LEN;cnt++)
						{
							tone=GetTone(TRUE);
							if (tone>-1)
								write_eeprom (cnt,tone);
						}
						timers.ModeProg=FALSE;
					break;
				}
			}
			puts ("... Fin\n\r");
			port_b&=0xb;	   //Apagamos el LED.
		}
	}
}


#INT_RTCC
void timer (void)
{
	static int flag,n; 

	if (timers.ModeProg)
	{	
		if (--timers.cnt<1)
		{
			if (flag)
			{
				port_b|=0x40;		//Encendemos el LED	
			}
			else
				port_b&=0x0b;		//Apagamos el LED	
				
			flag=!flag;	
			timers.cnt=3;
		}
				
		if (timers.TimeOut)
			timers.TimeOut--;		
	}

	for (n=0;n<NUM_RELES;n++)
	{
		if (TimersRel[n].TimeOut>0)
		{
			TimersRel[n].TimeOut--;
			
			if (TimersRel[n].TimeOut==0)
				port_b&=~(TimersRel[n].address);		//Desactivamos el relé
		}
	}		

}

