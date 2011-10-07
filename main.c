#include <msp430g2211.h>

#define LED0 (BIT0)
#define LED1 (BIT1)
#define LED2 (BIT2)
#define LED3 (BIT4)
#define LED4 (BIT5)
#define BUTTON (BIT3)

#define ALL_LEDS (LED0|LED1|LED2|LED3|LED4)
#define MAX_LEDS (5-1)

#define MODE_ALL_FLASH 		(0)
#define MODE_SERIES_FLASH	(1)

#define RATE_NORMAL	(33)
#define RATE_FASTER	(13)

#define CLEAR_ALL_LEDS (P1OUT &= ~ALL_LEDS)

typedef unsigned char uint8_t;

enum {
	MODE_ALL_FLASH_NORMAL = 0,
	MODE_ALL_FLASH_FASTER,
	MODE_SERIES_NORMAL,
	MODE_SERIES_FASTER
};

struct mode_t {
	unsigned char mode_type;
	unsigned char rate;
};

struct mode_t modes[] = {
	{MODE_ALL_FLASH_NORMAL, RATE_NORMAL},
	{MODE_ALL_FLASH_FASTER, RATE_FASTER},
	{MODE_SERIES_NORMAL, RATE_NORMAL},
	{MODE_SERIES_FASTER, RATE_FASTER}
};


volatile struct mode_t *mode = &modes[0];
volatile uint8_t mode_index = 0;

volatile uint8_t current_mode_index = 0;

volatile uint8_t count = 0;

volatile uint8_t series_index = 0;

const uint8_t series_lookup[] = {
	LED0,
	LED1,
	LED2,
	LED3,
	LED4
};

void main(void) {
	WDTCTL = WDT_MDLY_32; 	// Watchdog timer interrupt every 30ms
	IE1 |= WDTIE;			// Enable Watchdog timer interrupt
	
	P1DIR |= ALL_LEDS;
	P1OUT &= ~ALL_LEDS;
	
	P1IE  |= BUTTON; // P1.3 interrupt enabled
	P1IES |= BUTTON; // P1.3 Hi/lo edge
	P1IFG &= ~BUTTON; // P1.3 IFG cleared
		
	_BIS_SR(LPM0_bits + GIE);
}

#pragma vector=PORT1_VECTOR
__interrupt void push_button(void) {
	if (current_mode_index >= 3) {
		mode = &modes[0];
		current_mode_index = 0;
	} else {
		mode++;
		current_mode_index++;
	}
	
	series_index = 0;
	count = 0;
	
	CLEAR_ALL_LEDS;
		
	P1IFG &= ~BUTTON; // P1.3 IFG cleared. Acknowledge the transition
}

#pragma vector=WDT_VECTOR
__interrupt void watchdog_timer(void) {
	volatile struct mode_t *m = mode;
	
	switch (m->mode_type) {
		case MODE_ALL_FLASH_NORMAL:
		case MODE_ALL_FLASH_FASTER:
			if (count > m->rate) {
				P1OUT ^= ALL_LEDS;
				count = 0;				
			} else {
				count++;
			}
			break;
			
		case MODE_SERIES_NORMAL:
		case MODE_SERIES_FASTER:
			if (count > m->rate) {
				CLEAR_ALL_LEDS;
				P1OUT |= series_lookup[series_index];
				series_index++;
				if (series_index > MAX_LEDS) series_index = 0;
				count = 0;
			} else {
				count++;
			}
			break;
		
	}
}
