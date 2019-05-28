/******************************************************************************/
/*   (C) Copyright HTL - HOLLABRUNN  2009-2009 All rights reserved. AUSTRIA   */ 
/*                                                                            */ 
/* File Name:   Lauflicht_Interrupt.c                                         */
/* Autor: 		Josef Reisinger                                               */
/* Version: 	V1.00                                                         */
/* Date: 		11/03/2012                                                    */
/* Description: Demoprogramm für Timer 1                                      */
/******************************************************************************/
/* History: 	V1.00  creation										          */
/******************************************************************************/
#include <armv10_std.h>						 

/*------------------------------ Function Prototypes -------------------------*/
static void TIM4_Config(void);
static void NVIC_init(char position, char priority);
static void set_clock_32MHz(void);
static void Uhr(void);

/*------------------------------ Static Variables-------------------------*/
volatile int zehntel;

static void set_clock_32MHz(void)
{
	FLASH->ACR = 0x12;
	
	RCC->CR |= RCC_CR_HSEON;
	while ((RCC->CR & RCC_CR_HSERDY) == 0);
	
	RCC->CFGR |= RCC_CFGR_PLLMULL4;
	RCC->CFGR |= RCC_CFGR_ADCPRE;
	RCC->CFGR |= RCC_CFGR_PPRE1;
	RCC->CFGR |= RCC_CFGR_PLLSRC;
	
	RCC->CR |= RCC_CR_PLLON;
	while ((RCC->CR & RCC_CR_PLLRDY) == 0);
	
	RCC->CFGR |= RCC_CFGR_SW_PLL;
	while ((RCC->CFGR & RCC_CFGR_SWS_PLL) == 0);
	
	while ((RCC->CFGR & RCC_CFGR_SWS) != ((RCC->CFGR<<2) & RCC_CFGR_SWS));
	
	RCC->BDCR |=RCC_BDCR_LSEON;
}
/******************************************************************************/
/*           Interrupt Service Routine  Timer4 (General Purpose Timer)        */
/******************************************************************************/
void TIM4_IRQHandler(void)	//Timer 4, löst alle 1000ms aus
{
	TIM4->SR &=~0x01;	 //Interrupt pending bit löschen (Verhinderung eines nochmaligen Interrupt-auslösens)
	zehntel++;
}

/******************************************************************************/
/*                     Initialization Timer1 (General Purpose Timer)          */
/******************************************************************************/
static void TIM4_Config(void)
{	
	/*---------------------- Timer 4 konfigurieren -----------------------*/
	RCC->APB1ENR |= 0x0004;	//Timer 4 Clock enable
	TIM4->SMCR = 0x0000;	//Timer 4 Clock Selection: CK_INT wird verwendet
	TIM4->CR1  = 0x0000;	// Auswahl des Timer Modus: Upcounter --> Zählt: von 0 bis zum Wert des Autoreload-Registers

	/* T_INT = 126,26ns, Annahme: Presc = 150 ---> Auto Reload Wert = 52801 (=0xCE41) --> 1s Update Event*/
	TIM4->PSC = 15;		//Wert des prescalers (Taktverminderung)
	TIM4->ARR = 0x21;		//Auto-Reload Wert = Maximaler Zaehlerstand des Upcounters
	//TIM4->RCR = 0;			//Repetition Counter deaktivieren

	/*--------------------- Interrupt Timer 4 konfigurieren ---------------------*/
	TIM4->DIER = 0x01;	   //enable Interrupt bei einem UEV (Überlauf / Unterlauf)
	NVIC_init(TIM4_IRQn,1);	   //Enable Timer 4 Update Interrupt, Priority 1

	/*-------------------------- Timer 4 Starten -------------------------------*/
    TIM4->CR1 |= 0x0001;   //Counter-Enable bit setzen

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

static void Uhr(void)
{
	int lcd_zehntel; 		// aktueller Zählerstand auf LCD
	int sekunden;
	int lcd_sekunden;
	int minuten;
	int lcd_minuten;
	int stunden;
	int lcd_stunden;
	char buffer[30];
	
	lcd_zehntel=zehntel%10;
	sekunden=zehntel/10;
	lcd_sekunden=sekunden%60;
	minuten=sekunden/60;
	lcd_minuten=minuten%60;
	stunden=minuten/60;
	lcd_stunden=stunden%60;
	
	lcd_set_cursor(0,0);                 // Cursor auf Ursprung
	sprintf(&buffer[0],"%d:%d:%d:%d", lcd_stunden, lcd_minuten, lcd_sekunden, lcd_zehntel); // zehntelzaehler auf LCD aktualisieren
  lcd_put_string(&buffer[0]);
	
	if(lcd_stunden == 23 & lcd_minuten == 59 & lcd_sekunden == 59 /*& lcd_zehntel == 9*/)	//nur als Testzwecke auskommentiert!!!!
	{
		zehntel=0;
	}
}

/******************************************************************************/
/*                                MAIN function                               */
/******************************************************************************/
int main (void) 
{
	int zehntel_pruf=0;
	
	set_clock_32MHz();
  uart_init(9600);    // 9600,8,n,1
  uart_clear();       // Send Clear String to VT 100 Terminal
  lcd_init();         // LCD initialisieren
	lcd_clear();        // Lösche LCD Anzeige		
  zehntel=0; 		// zehntelzaehler initialisieren
	TIM4_Config();	    // Timer 1 starten: Upcounter --> löst alle 1s einen Update Interrupt
	uart_put_string("Inkrementalgeber:\n");
	do
  {
		if (zehntel_pruf != zehntel)    // Hat sich Zählerstand verändert?
	 	{
			Uhr();
			zehntel_pruf=zehntel;	
		}
  	} while (1);

}
