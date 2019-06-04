/******************************************************************************/
/*   (C) Copyright HTL - HOLLABRUNN  2009-2009 All rights reserved. AUSTRIA   */ 
/*                                                                            */ 
/* File Name:   Inkrementalgeber.c                                         */
/* Autor: 		Matthias Breitseher / Patrick Steiner                                               */
/* Version: 	V1.00                                                         */
/* Date: 		02/06/2019                                                    */
/* Description: Uhr und Positionsanzeige für Inkrementalgeber                                      */
/******************************************************************************/
/* History: 	V1.00  creation										          */
/******************************************************************************/
#include <armv10_std.h>						 

/*------------------------------ Function Prototypes -------------------------*/
static void NVIC_init(char position, char priority);
static void GPIOA_init(void);
static void TIM4_Config(void);
static void EXTI0_Config(void);
static void EXTI1_Config(void);
static void set_clock_32MHz(void);
static void Uhr(void);
static void position_up(void);
void uart_initialisierung(unsigned long);

/*------------------------------ Static Variables-------------------------*/
volatile int links=0;
volatile int rechts=0;
volatile int ackr=0;
volatile int ackl=0;
volatile int zehntel;
volatile int position;

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
/*           External Interrupt Service Routine  Schalter0                    */
/******************************************************************************/
void EXTI0_IRQHandler(void)//ISR
{
	EXTI->PR |= (0x01 << 0); //Pending bit EXT0 rücksetzen (Sonst wird die ISR immer wiederholt)
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
/*                     Initialization Timer4 (General Purpose Timer)          */
/******************************************************************************/
static void TIM4_Config(void)
{	
	/*---------------------- Timer 4 konfigurieren -----------------------*/
	RCC->APB1ENR |= 0x0004;	//Timer 4 Clock enable
	TIM4->SMCR = 0x0000;	//Timer 4 Clock Selection: CK_INT wird verwendet
	TIM4->CR1  = 0x0000;	// Auswahl des Timer Modus: Upcounter --> Zählt: von 0 bis zum Wert des Autoreload-Registers

	/* T_INT = 126,26ns, Annahme: Presc = 150 ---> Auto Reload Wert = 52801 (=0xCE41) --> 1s Update Event*/
	TIM4->PSC = 0xC8;		//Wert des prescalers (Taktverminderung)
	TIM4->ARR = 0x7D0;		//Auto-Reload Wert = Maximaler Zaehlerstand des Upcounters
	TIM4->RCR = 0;			//Repetition Counter deaktivieren

	/*--------------------- Interrupt Timer 4 konfigurieren ---------------------*/
	TIM4->DIER = 0x01;	   //enable Interrupt bei einem UEV (Überlauf / Unterlauf)
	NVIC_init(TIM4_IRQn,1);	   //Enable Timer 4 Update Interrupt, Priority 1

	/*-------------------------- Timer 4 Starten -------------------------------*/
    TIM4->CR1 |= 0x0001;   //Counter-Enable bit setzen
}

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
  EXTI->RTSR &=~(0x01 << 0);	   //Rising Edge Trigger für EXTI1 Deaktivieren  

	EXTI->PR |= (0x01 << 0);	//EXTI_clear_pending: Das Auslösen auf vergangene Vorgänge nach	dem enablen verhindern 
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

static void GPIOA_init(void)
{
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;     	// enable clock for GPIOA (APB2 Peripheral clock enable register)
	GPIOA->CRL &= 0xFFFFFF00;                  // set Port Pins PA1 to Pull Up/Down Input mode (50MHz) = Mode 8
	GPIOA->CRL |= 0x00000088;                  
	GPIOA->ODR |= 0x0002;   					//  Output Register für PA1 auf "1" initialisieren
	GPIOA->ODR |= 0x0001;
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
	
	zehntel--;
	lcd_zehntel=zehntel%10;
	sekunden=zehntel/10;
	lcd_sekunden=sekunden%60;
	minuten=sekunden/60;
	lcd_minuten=minuten%60;
	stunden=minuten/60;
	lcd_stunden=stunden%60;
	
	lcd_set_cursor(0,0);                 // Cursor auf Ursprung
	sprintf(&buffer[0],"%02d:%02d:%02d:%d", lcd_stunden, lcd_minuten, lcd_sekunden, lcd_zehntel); // zehntelzaehler auf LCD aktualisieren
  lcd_put_string(&buffer[0]);
	
	if(lcd_stunden == 23 & lcd_minuten == 59 & lcd_sekunden == 59 & lcd_zehntel == 9)
	{
		zehntel=0;
	}
}

static void position_up(void)
{
	char buffer[30];	
	lcd_set_cursor(1,10);
	lcd_put_string("       ");
	lcd_set_cursor(1,10);
	
	sprintf(&buffer[0],"%d", position); // zaehler auf LCD aktualisieren
  lcd_put_string(&buffer[0]);
	sprintf(&buffer[0],"Position: %d", position);
	uart_put_string(&buffer[0]);

	
}
/*************************************
//uart init
*/
void uart_initialisierung(unsigned long baudrate)
{
  RCC->APB2ENR |= RCC_APB2ENR_IOPAEN; //GPIOA mit einem Takt versorgen
  
  GPIOA->CRH &= ~(0x00F0); //loesche PA.9 configuration-bits 
  GPIOA->CRH   |= (0x0BUL  << 4); //Tx (PA9) - alt. out push-pull

  GPIOA->CRH &= ~(0x0F00); //loesche PA.10 configuration-bits 
  GPIOA->CRH   |= (0x04UL  << 8); //Rx (PA10) - floating
  RCC->APB2ENR |= RCC_APB2ENR_USART1EN; //USART1 mit einem Takt versrogen

  USART1->BRR  = 32000000L/baudrate; //Baudrate festlegen statt 8000000 wie normal zu 32000000 ändern
  
  USART1->CR1 |= (USART_CR1_RE | USART_CR1_TE);  //aktiviere RX, TX
  USART1->CR1 |= USART_CR1_UE;                    //aktiviere USART

}




/******************************************************************************/
/*                                MAIN function                               */
/******************************************************************************/
int main (void) 
{
	int zehntel_pruf=0;
	int position_pruf=0;
	
	set_clock_32MHz();
	GPIOA_init();
  uart_initialisierung(9600);    // 9600,8,n,1
	USART1->CR1|=0x0020;	    //USART1 RxNE - Interrupt Enable
  //uart_clear();       // Send Clear String to VT 100 Terminal
  lcd_init();         // LCD initialisieren
	lcd_clear();        // Lösche LCD Anzeige		
  zehntel=0; 		// zehntelzaehler initialisieren
	TIM4_Config();	    // Timer 1 starten: Upcounter --> löst alle 1s einen Update Interrupt
	EXTI1_Config();		 // Konfigurieren des Externen Interrupts für Leitung PA1 (Falling Edges)
	EXTI0_Config();
	uart_put_string("Willkommen zum Inkrementalgeber:");
	lcd_set_cursor(1,0);
	lcd_put_string("Position: 0");
	do
  {
		if (zehntel_pruf != zehntel)    // Hat sich Zählerstand verändert?
	 	{
			Uhr();
			zehntel_pruf=zehntel;	
		}
		if (position_pruf != position)
		{
		position_up();
			//uart_put_char(position);
			position_pruf=position;
		}
		if((links==1)&&(ackl==1))
		{
			position++;
			
			links=0;
			rechts=0;
			ackl=0;
			ackr=0;
			
		}
		else if((rechts==1)&&(ackr==1))
		{
			position--;
			
			links=0;
			rechts=0;
			ackl=0;
			ackr=0;
		}
	//	links=0;
	//	rechts=0;
	//	ackl=0;
	//	ackr=0;
		
  	} while (1);

}
