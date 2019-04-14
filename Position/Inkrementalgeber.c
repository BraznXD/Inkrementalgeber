/******************************************************************************/
/*   (C) Copyright HTL - HOLLABRUNN  2009-2009 All rights reserved. AUSTRIA   */ 
/*                                                                            */ 
/* File Name:   External_Interrupt.c                                          */
/* Autor: 		Josef Reisinger                                               */
/* Version: 	V1.00                                                         */
/* Date: 		11/03/2012                                                    */
/* Description: Demoprogramm für External Interrupt                           */
/******************************************************************************/
/* History: 	V1.00  creation										          */
/******************************************************************************/
#include <armv10_std.h>						 

/*------------------------------ Function Prototypes -------------------------*/
static void NVIC_init(char position, char priority); 
static void EXTI0_Config(void);
static void EXTI1_Config(void);

/*------------------------------ Static Variables-------------------------*/
int aufenthalt;
volatile int links;
volatile int rechts;
volatile int ackr;
volatile int ackl;

/******************************************************************************/
/*           Timer4 Interrupt Service Routine  							                  */
/******************************************************************************/
void TIM4_IRQHandler(void)
{
	
}
/******************************************************************************/
/*           External Interrupt Service Routine  Schalter1                    */
/******************************************************************************/
void EXTI1_IRQHandler(void)//ISR
{
	EXTI->PR |= (0x01 << 1); //Pending bit EXT0 rücksetzen (Sonst wird die ISR immer wiederholt)
	//aufenthalt++;    // Fallende Flanke an PA1 erkannt
	if(rechts==1)
	{
		ackr=1;
		return;
	}
	else
	{
		links=1;
		return;
	}
}

/******************************************************************************/
/*           External Interrupt Service Routine  Schalter1                    */
/******************************************************************************/
void EXTI0_IRQHandler(void)//ISR
{
	EXTI->PR |= (0x01 << 1); //Pending bit EXT0 rücksetzen (Sonst wird die ISR immer wiederholt)
	//aufenthalt--;    // Fallende Flanke an PA0 erkannt
	if(links==1)
	{
		ackl=1;
		return;
	}
	else
	{
		rechts=1;
		return;
	}
}

/******************************************************************************/
/*           Initialization Timer4 (General Purpose Timer)                    */
/******************************************************************************/
//static void TIM4_config(void) {/Skriptum S 43

/******************************************************************************/
/*                                   EXTI1_config                             */ 
/* Leitung PA1 wird mit EXTI1 verbunden, Interupt bei falling edge,Priorität 3*/ 
/******************************************************************************/
static void EXTI1_Config()	
{

	NVIC_init(EXTI1_IRQn,3);		//NVIC fuer initialisieren EXTI Line1 (Position 7, Priority 3) 

  RCC->APB2ENR |= 0x0001;		   //AFIOEN  - Clock enable	
	AFIO->EXTICR[0] &= 0xFFFFFF0F; //Interrupt-Line EXTI1 mit Portpin PA1 verbinden 
	EXTI->FTSR |= (0x01 << 1);	   //Falling Edge Trigger für EXIT1 Aktivieren
  EXTI->RTSR &=~(0x01 << 1);	   //Rising Edge Trigger für EXTI1 Deaktivieren

	EXTI->PR |= (0x01 << 1);	//EXTI_clear_pending: Das Auslösen auf vergangene Vorgänge nach	dem enablen verhindern
	EXTI->IMR |= (0x01 << 1);   // Enable Interrupt EXTI1-Line. Kann durch den NVIC jedoch noch maskiert werden
}

/******************************************************************************/
/*                                   EXTI0_config                             */ 
/* Leitung PA0 wird mit EXTI0 verbunden, Interupt bei falling edge,Priorität 3*/ 
/******************************************************************************/
static void EXTI0_Config()	
{

	NVIC_init(EXTI0_IRQn,3);		//NVIC fuer initialisieren EXTI Line1 (Position 7, Priority 3) 

  RCC->APB2ENR |= 0x0001;		   //AFIOEN  - Clock enable	
	AFIO->EXTICR[0] &= 0xFFFFFFF0; //Interrupt-Line EXTI1 mit Portpin PA0 verbinden 
	EXTI->FTSR |= (0x01 << 0);	   //Falling Edge Trigger für EXIT1 Aktivieren
  EXTI->RTSR &=~(0x01 << 1);	   //Rising Edge Trigger für EXTI1 Deaktivieren

	EXTI->PR |= (0x01 << 1);	//EXTI_clear_pending: Das Auslösen auf vergangene Vorgänge nach	dem enablen verhindern
	EXTI->IMR |= (0x01 << 0);   // Enable Interrupt EXTI1-Line. Kann durch den NVIC jedoch noch maskiert werden
}

/******************************************************************************/
/*                   NVIC_init(char position, char priority)    			  */ 
/* Funktion:                                                                  */    
/*   Übernimmt die vollständige initialisierung eines Interrupts  im Nested   */
/*   vectored Interrupt controller (Priorität setzen, Auslösen verhindern,    */
/*   Interrupt enablen)                                                       */
/* Übergabeparameter: "position" = 0-67 (Nummer des Interrupts)               */
/*                    "priority": 0-15 (Priorität des Interrupts)		      */
/******************************************************************************/
static void NVIC_init(char position, char priority) 
{	
	NVIC->IP[position]=(priority<<4);	//Interrupt priority register: Setzen der Interrupt Priorität
	NVIC->ICPR[position >> 0x05] |= (0x01 << (position & 0x1F));//Interrupt Clear Pendig Register: Verhindert, dass der Interrupt auslöst sobald er enabled wird 
	NVIC->ISER[position >> 0x05] |= (0x01 << (position & 0x1F));//Interrupt Set Enable Register: Enable interrupt
} 

/******************************************************************************/
/*                                MAIN function                               */
/******************************************************************************/
int main (void) 
{
int lcd_aufenthalt;
char buffer[30];

	/* GPIO Init: Schalter auf PA1 Leitung als Eingang initialisieren:	*/		
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;     	// enable clock for GPIOA (APB2 Peripheral clock enable register)
	GPIOA->CRL &= 0xFFFFFF00;                  // set Port Pins PA1 to Pull Up/Down Input mode (50MHz) = Mode 8
	GPIOA->CRL |= 0x00000088;                  
	GPIOA->ODR |= 0x0002;   					//  Output Register für PA1 auf "1" initialisieren
	GPIOA->ODR |= 0x0001;

	aufenthalt=0;     // Zaehler für falling edges initialisieren
	lcd_aufenthalt=0; 

    lcd_init ();	     // Initialisieren der LCD Anzeige
	lcd_clear();		 // LCD Anzeige löschen
	sprintf(&buffer[0],"Position: %d", lcd_aufenthalt); // zaehler auf LCD aktualisieren
    lcd_put_string(&buffer[0]);

	EXTI1_Config();		 // Konfigurieren des Externen Interrupts für Leitung PA1 (Falling Edges)
	EXTI0_Config();
	do
  {
		if (lcd_aufenthalt != aufenthalt)    // Anzeigewert geaendert?
		{
			lcd_aufenthalt = aufenthalt;
			lcd_clear();
			sprintf(&buffer[0],"Position: %d", lcd_aufenthalt); // zaehler auf LCD aktualisieren
    	lcd_put_string(&buffer[0]);
		}
		if(links==1)
		{
			aufenthalt++;
		}
		else if(rechts==1)
		{
			aufenthalt--;
		}
		links=0;
		rechts=0;
		ackl=0;
		ackr=0;
  	} while (1);

}
