#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "main.h"
#include "eeprom.h"
#include "usart.h"
#include <util/delay.h>

#define SPEKTRUM_NORMAL 0 
#define SPEKTRUM_HIRES  1


// configuration 
#ifndef FALSE
#define FALSE   0
#endif
#ifndef TRUE
#define TRUE    1
#endif

#define SPEKTRUM_TYPE SPEKTRUM_NORMAL
#define SPEKTRUM_OUTPUT FALSE
#define PPM_OUTPUT TRUE
#define RSSI_OUTPUT TRUE
#define FORCE_FAILSAFE FALSE
#define ENABLE_LED TRUE
// 8 or 12
#define PPMCH 12


volatile unsigned int reset_pw = 0;
#if PPM_OUTPUT == TRUE
volatile unsigned char  isr_channel_number = 0;
volatile unsigned int   isr_channel_pw[(PPMCH +1)]; // +1 is for the reset pulse.
#endif
volatile uint8_t timeout = 30;
volatile uint16_t timeout2 = 1000;


#define TIMER1_PRESCALER   64
#define TIMER1_PRESCALER_BITS   ((0<<CS02)|(1<<CS01)|(1<<CS00))


#define RC_PPM_FRAME_TIMER_VAL          ((((F_CPU/1000) * 28000)/1000)/TIMER1_PRESCALER) 
#define RC_RESET_PW_TIMER_VAL           ((((F_CPU/1000) * 7500)/1000)/TIMER1_PRESCALER) 
#define RC_PPM_SYNC_PW_VAL              ((((F_CPU/1000) * 300)/1000)/TIMER1_PRESCALER)

#if ENABLE_LED == TRUE
volatile uint8_t ledcycle = 0;
volatile uint8_t ledmode = 5;
volatile uint16_t ledpause = 0x2FFF;
#endif

#if PPM_OUTPUT == TRUE
ISR(TIMER1_COMPB_vect)
{
	isr_channel_number++;
	if( isr_channel_number >= (PPMCH + 1) ) 
	{
		isr_channel_number = 0; 
		reset_pw = RC_PPM_FRAME_TIMER_VAL; 
	}
	
	if(isr_channel_number < PPMCH)
	{
		OCR1A = isr_channel_pw[isr_channel_number];
		reset_pw -= OCR1A;
	}else{
		OCR1A = reset_pw;
	} 


	
	return;

}

ISR(TIMER1_OVF_vect)
{   
	return;
}
#endif 
ISR(TIMER0_OVF_vect)
{  
#if ENABLE_LED == TRUE
	if(ledpause == 0)
	{
		if(ledcycle < ((ledmode*2)-1))
		{
			ledpause = 0x6FF;
			ledcycle++;
		}
		else
		{
			ledcycle = 0;
			ledpause = 0x2FFF;
		}
	
		if(PORTC & (1<<PORTC4))
		{
			PORTC &= ~(1<<PORTC4);
		}
		else
		{
			PORTC |= (1<<PORTC4);
		}

		if(PORTB & (1<<PORTB1))
		{
			PORTB &= ~(1<<PORTB1);
		}
		else
		{
			PORTB |= (1<<PORTB1);
		}
	};
	ledpause--;
#endif

	if(timeout > 0) timeout--;
	if(timeout2 > 0) timeout2--;
	return;
}  


#ifdef version1
ISR(PCINT2_vect)
{
	if(PIND & (1<<PIND2))
	{
		PORTC &= ~(1<<PORTC5);
	}
	else
	{
		PORTC |= (1<<PORTC5);
	}
}
#endif


int main(void)
{
#if PPM_OUTPUT == TRUE
	isr_channel_pw[0] = ((((F_CPU/1000) * 1500)/1000)/TIMER1_PRESCALER);
	isr_channel_pw[1] = ((((F_CPU/1000) * 1500)/1000)/TIMER1_PRESCALER);
	isr_channel_pw[2] = ((((F_CPU/1000) * 1500)/1000)/TIMER1_PRESCALER);
	isr_channel_pw[3] = ((((F_CPU/1000) * 1500)/1000)/TIMER1_PRESCALER);
	isr_channel_pw[4] = ((((F_CPU/1000) * 1500)/1000)/TIMER1_PRESCALER);
	isr_channel_pw[5] = ((((F_CPU/1000) * 1500)/1000)/TIMER1_PRESCALER);
	isr_channel_pw[6] = ((((F_CPU/1000) * 1500)/1000)/TIMER1_PRESCALER);
	isr_channel_pw[7] = ((((F_CPU/1000) * 1500)/1000)/TIMER1_PRESCALER);
#if PPMCH == 12
	isr_channel_pw[8] = ((((F_CPU/1000) * 1500)/1000)/TIMER1_PRESCALER);
	isr_channel_pw[9] = ((((F_CPU/1000) * 1500)/1000)/TIMER1_PRESCALER);
	isr_channel_pw[10] = ((((F_CPU/1000) * 1500)/1000)/TIMER1_PRESCALER);
	isr_channel_pw[11] = ((((F_CPU/1000) * 1500)/1000)/TIMER1_PRESCALER);
#endif

								
	TCCR1A = (1<<COM1B1) | (0<<COM1B0) | (1<<WGM11) | (1<<WGM10);
	TCCR1B = (1<<WGM13)|(1<<WGM12);
	OCR1A = RC_RESET_PW_TIMER_VAL;
	OCR1B  = RC_PPM_SYNC_PW_VAL;
	TIMSK1 |= (1<<OCIE1B)|(1<<TOIE1);
	TCNT1 = 0; 

	isr_channel_number = 1;		
#endif

	// timer0 with prescaler 8, and interrupt, Int freq: 9433khz , ~0.0001s 
	TCCR0B |= (1<<CS01);
	TIMSK0 |= (1<<TOIE0);


	DDRB |= (1<<PORTB2); //ppm out

#ifdef version1
	DDRC |= (1<<PORTC5); //inv uart out

	PCMSK2 |= (1<<PCINT18);
	PCICR |= (1<<PCIE2);
#endif

	// led
#if ENABLE_LED == TRUE
	DDRC |= (1<<PORTC4); //led out
	PORTC |= (1<<PORTC4);
	DDRB |= (1<<PORTB1); //led out
	PORTB |= (1<<PORTB1);
#endif

#if RSSI_OUTPUT == TRUE
	//timer2 fuer RSSI
	DDRB |= (1<<PORTB3);
	TCCR2A |= (1<<COM2A1) | (1<<WGM20)| (1<<WGM21);
	TCCR2B |= (1<<CS21);
	OCR2A = 0;
#endif	

	USART_Init();
	sei();

	uint8_t data = 0;
		
	uint8_t current_bit_in_ch = 0;
	uint8_t current_bit_in_byte = 0;
	uint8_t current_byte = 0;
	uint8_t current_ch = 0;
	uint8_t i = 0;
	uint8_t sbus_bytes[25];
	int16_t channels[12];
	int16_t channels_old[12];
	int16_t channels_vold[12];


	uint8_t curr_byte = 0;
	uint8_t ready = 0;
#if SPEKTRUM_OUTPUT == TRUE
	uint8_t spektrum_frame = 1;
	uint8_t send_sp = 0;
#endif

	

#if FORCE_FAILSAFE == FALSE
	uint8_t first = 0;  
	ReadParameter();
	if((MODE != 1)&&(MODE != 2)) MODE = 1;
#endif
#if FORCE_FAILSAFE == TRUE
	MODE = 1;
#endif
	
					
	while(1)
	{
		if(timeout == 0)
		{
			curr_byte = 0;
			timeout = 60; // 6ms
		}
			
		if(uart_getc_nb(&data))
		{
			timeout = 60; //6ms
			if(curr_byte == 0)
			{
				if(data != 0x0F)
				{
					curr_byte = 30;
				}
			}
			
			if( curr_byte == 24)
			{
				if(data != 0)
				{
					curr_byte = 30;
				}
			}
			if(curr_byte < 25)
			{
				sbus_bytes[curr_byte]=data;
				curr_byte++;
			}
			
			if(curr_byte == 25)
			{
				curr_byte = 0;
				current_bit_in_byte = 0;
				current_bit_in_ch = 0;
				current_ch = 0;
				current_byte = 1;

				for(i=0;i<12;i++)
				{
					channels[i] = 0;
				}
				
				for(i=0;i<132;i++)
				{
					if(sbus_bytes[current_byte] & (1<<current_bit_in_byte))
					{
						channels[current_ch] |= (1<<current_bit_in_ch);
					}
					
					current_bit_in_byte++;
					current_bit_in_ch++;
					
					if(current_bit_in_byte == 8)
					{
						current_bit_in_byte =0;
						current_byte++;
					}
					if(current_bit_in_ch == 11)
					{
						current_bit_in_ch =0;
						current_ch++;
					}
					
					
				}

#if FORCE_FAILSAFE == FALSE
				if(first < 110)
				{
					first++;
				}


				if(first < 100)
				{
					if(channels[0] < 800)
					{
						MODE = 1;
						WriteParameter();
					}
					if(channels[0] > 1250)
					{
						MODE = 2;
						WriteParameter();
					}
				}
#endif				

				for(i=0;i<12;i++)
				{
					if(channels_vold[i] == channels_old[i])
					{
						if(channels_old[i] != channels[i])
						{
							channels_old[i] = channels[i];
							if(channels_vold[i] != 0)
							{
								channels[i] = channels_vold[i];
							}
						}
					};
				}

#if PPM_OUTPUT == TRUE						
				cli();
				uint8_t i=0;
				for(i=0;i<PPMCH;i++)
				{
					/*
					 * From linear regression of 7 PWM samples from
					 * 1100..1940us. R^2 = 0.999999.
					 * See https://gist.github.com/prattmic/8857047
					 */
					uint16_t pw = 0.624731*channels[i] + 880.561511;
					isr_channel_pw[i] = ((((F_CPU/1000) * pw)/1000)/TIMER1_PRESCALER);
				}
				sei();
#endif					

				//wenn kein failsafe dann output senden
				if((sbus_bytes[23] & 8) == 0)
				{
					if(ready == 0)
					{
						ready = 1;
#if ENABLE_LED == TRUE
						ledpause = 0x2FFF;
						ledcycle = 0;
						PORTC |= (1<<PORTC4);
						PORTB |= (1<<PORTB1);
						if(MODE == 1) ledmode =1;
						if(MODE == 2) ledmode =2;
#endif
#if PPM_OUTPUT == TRUE						
						TCCR1B |= TIMER1_PRESCALER_BITS;
#endif						
#if SPEKTRUM_OUTPUT == TRUE
						send_sp = 1;
#endif
					}
				}
				else
				{
#if RSSI_OUTPUT == TRUE				
					OCR2A=0;
#endif					
					if(ready == 1)
					{
						ready = 0;
#if ENABLE_LED == TRUE
						ledpause = 0x2FFF;
						ledcycle = 0;
						PORTC |= (1<<PORTC4);
						PORTB |= (1<<PORTB1);
						if(MODE == 1) ledmode =3;
#endif						
						if(MODE == 2)
						{ 
#if SPEKTRUM_OUTPUT == TRUE
							send_sp = 0;
#endif
#if ENABLE_LED == TRUE
							ledmode =4;
#endif
#if PPM_OUTPUT == TRUE						
							TCCR1B &= (~(1<<CS12)) & (~(1<<CS11)) & (~(1<<CS10));
#endif						
						}
					}
				}
				
#if SPEKTRUM_OUTPUT == TRUE
				
				if(send_sp == 1)
				{
					//115200
					uint16_t ubrr = (uint16_t) ((uint32_t) F_CPU/(8 * 115200) - 1);
					UBRRH = (uint8_t)(ubrr>>8);
					UBRRL = (uint8_t)ubrr;
					UCSRA |= (1 << U2X);
					//8N1
					UCSRC &= ~(1 << UPM1);
					UCSRC &= ~(1 << UPM0);
					UCSRC &= ~(1 << USBS);
					UCSRB &= ~(1 << UCSZ2);
					UCSRC |=  (1 << UCSZ1);
					UCSRC |=  (1 << UCSZ0);

					_delay_ms(2);


					if(spektrum_frame == 0)
					{
						spektrum_frame=1;

						USART_putc(0);
						USART_putc(0);
#if SPEKTRUM_TYPE == SPEKTRUM_NORMAL
						USART_putc( ((uint8_t)((channels[0] >> 9)&0x03)) + (0 << 2)  );
						USART_putc(  ((uint8_t)(channels[0] >> 1)) );
						USART_putc( ((uint8_t)((channels[1] >> 9)&0x03)) + (1 << 2)  );
						USART_putc(  ((uint8_t)(channels[1] >> 1)) );
						USART_putc( ((uint8_t)((channels[2] >> 9)&0x03)) + (2 << 2)  );
						USART_putc(  ((uint8_t)(channels[2] >> 1)) );
						USART_putc( ((uint8_t)((channels[3] >> 9)&0x03)) + (3 << 2)  );
						USART_putc(  ((uint8_t)(channels[3] >> 1)) );
						USART_putc( ((uint8_t)((channels[4] >> 9)&0x03)) + (4 << 2)  );
						USART_putc(  ((uint8_t)(channels[4] >> 1)) );
						USART_putc( ((uint8_t)((channels[5] >> 9)&0x03)) + (5 << 2)  );
						USART_putc(  ((uint8_t)(channels[5] >> 1)) );
						USART_putc( ((uint8_t)((channels[6] >> 9)&0x03)) + (6 << 2)  );
						USART_putc(  ((uint8_t)(channels[6] >> 1)) );
#endif    	                
#if SPEKTRUM_TYPE == SPEKTRUM_HIRES
						USART_putc( ((uint8_t)((channels[0] >> 8)&0x07)) + (0 << 3)  );
						USART_putc(  ((uint8_t)(channels[0])) );
						USART_putc( ((uint8_t)((channels[1] >> 8)&0x07)) + (1 << 3)  );
						USART_putc(  ((uint8_t)(channels[1])) );
						USART_putc( ((uint8_t)((channels[2] >> 8)&0x07)) + (2 << 3)  );
						USART_putc(  ((uint8_t)(channels[2])) );
						USART_putc( ((uint8_t)((channels[3] >> 8)&0x07)) + (3 << 3)  );
						USART_putc(  ((uint8_t)(channels[3])) );
						USART_putc( ((uint8_t)((channels[4] >> 8)&0x07)) + (4 << 3)  );
						USART_putc(  ((uint8_t)(channels[4])) );
						USART_putc( ((uint8_t)((channels[5] >> 8)&0x07)) + (5 << 3)  );
						USART_putc(  ((uint8_t)(channels[5])) );
						USART_putc( ((uint8_t)((channels[6] >> 8)&0x07)) + (6 << 3)  );
						USART_putc(  ((uint8_t)(channels[6])) );
#endif    	                
					}
					else
					{
						spektrum_frame=0;
						USART_putc(0);
						USART_putc(0);
#if SPEKTRUM_TYPE == SPEKTRUM_NORMAL                 		
						USART_putc( ((uint8_t)((channels[7] >> 9)&0x03)) + (7 << 2) + 0x80 );
						USART_putc( ((uint8_t) (channels[7] >> 1)) );
						USART_putc( ((uint8_t)((channels[8] >> 9)&0x03)) + (8 << 2)  );
						USART_putc(  ((uint8_t)(channels[8] >> 1)) );
						USART_putc( ((uint8_t)((channels[9] >> 9)&0x03)) + (9 << 2)  );
						USART_putc(  ((uint8_t)(channels[9] >> 1)) );
						USART_putc( ((uint8_t)((channels[10] >> 9)&0x03)) + (10 << 2) );
						USART_putc(  ((uint8_t)(channels[10] >> 1)) );
						USART_putc( ((uint8_t)((channels[11] >> 9)&0x03)) + (11 << 2) );
						USART_putc(  ((uint8_t)(channels[11] >> 1)) );

						USART_putc( ((uint8_t)((1024 >> 9)&0x03)) + (12 << 2) );
						USART_putc(  ((uint8_t)(1024 >> 1)) );
						USART_putc( ((uint8_t)((1024 >> 9)&0x03)) + (13 << 2) );
						USART_putc(  ((uint8_t)(1024 >> 1)) );
#endif                 		
#if SPEKTRUM_TYPE == SPEKTRUM_HIRES                 		
						USART_putc( ((uint8_t)((channels[7] >> 8)&0x07)) + (7 << 3) );
						USART_putc(  ((uint8_t)(channels[7])) );
						USART_putc( ((uint8_t)((channels[8] >> 8)&0x07)) + (8 << 3) );
						USART_putc(  ((uint8_t)(channels[8])) );
						USART_putc( ((uint8_t)((channels[9] >> 8)&0x07)) + (9 << 3) );
						USART_putc(  ((uint8_t)(channels[9])) );
						USART_putc( ((uint8_t)((channels[10] >> 8)&0x07)) + (10 << 3) );
						USART_putc(  ((uint8_t)(channels[10])) );
						USART_putc( ((uint8_t)((channels[11] >> 8)&0x07)) + (11 << 3) );
						USART_putc(  ((uint8_t)(channels[11])) );
						USART_putc( ((uint8_t)((1024 >> 8)&0x07)) + (12 << 3) );
						USART_putc(  ((uint8_t)(1024)) );
						USART_putc( ((uint8_t)((1024 >> 8)&0x07)) + (13 << 3) );
						USART_putc(  ((uint8_t)(1024)) );
#endif                 		
					}

					_delay_ms(2);

					//100000
					ubrr = (uint16_t) ((uint32_t) F_CPU/(8 * 100000) - 1);
					UBRRH = (uint8_t)(ubrr>>8);
					UBRRL = (uint8_t)ubrr;
					UCSRA |= (1 << U2X);
					// 8E2
					UCSRC |=  (1 << UPM1);
					UCSRC &= ~(1 << UPM0);
					UCSRC |=  (1 << USBS);
					UCSRB &= ~(1 << UCSZ2);
					UCSRC |=  (1 << UCSZ1);
					UCSRC |=  (1 << UCSZ0);
				}
#endif // SPEKTRUM_OUTPUT	


				//RSSI
#if RSSI_OUTPUT == TRUE				
				if( (sbus_bytes[23] & 4) == 0)
				{
					if(OCR2A < 240) OCR2A+=10;
				}
				else
				{
					if(OCR2A > 50) OCR2A-=10;
				}
#endif				

				for(i=0;i<12;i++)
				{
					channels_vold[i] = channels_old[i];
					channels_old[i] = channels[i];
				}

				timeout2 = 20000; // 2s
			}
		}
		else
		{
			if((timeout2 == 0)&&(ready == 1))
			{
				ready = 0;
#if ENABLE_LED == TRUE
				ledpause = 0x2FFF;
				ledcycle = 0;
				PORTC |= (1<<PORTC4);
				PORTB |= (1<<PORTB1);
				ledmode =5;
#endif				
#if SPEKTRUM_OUTPUT == TRUE
				send_sp = 0;
#endif
#if PPM_OUTPUT == TRUE
				TCCR1B &= (~(1<<CS12)) & (~(1<<CS11)) & (~(1<<CS10));
#endif				
			}
		}
	}
}

