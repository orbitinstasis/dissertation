/*
   Haptic Feedback Case - Firmware
   Copyright (C) 2015: Ben Kazemi, ben.kazemi@gmail.com
   Copyright (C) 2009: Daniel Roggen, droggen@gmail.com

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#define F_CPU 7372800    // AVR clock frequency in Hz, used by util/delay.h
#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>
#include "serial.h"
volatile struct SerialData SerialData0;
#define ROW_COUNT 10
#define COLUMN_COUNT 16
#define PIN_XYZ_ADC_INPUT 0b00000111   // for row data 
#define TOUCH_THRESH 1
#define PACKET_END_BYTE           0xFF
#define MAX_SEND_VALUE            254  //reserve 255 (0xFF) to mark end of packet
#define COMPRESSED_ZERO_LIMIT     254
#define MIN_SEND_VALUE            1    //values below this threshold will be treated and sent as zeros

int compressed_zero_count = 0;
// Following holds data for *each* side strip in the following *ordered* structure:
//		0: FSLP#(silkscreen); 1: Terminal1; 2: Res; 3: Terminal2; 4: Terminal3; 5: Terminal1ADC;
uint8_t sideStrip[4][6] = {{1,0,1,2,3,0b00100000},
						   {2,4,5,6,7,0b00100100},
						   {3,0,1,2,3,0b00000000},
						   {4,4,5,5,6,0b00000100}};  //res#4 is on portL, rest is on PORTF 

/*
 *	ripple a 1 through all the columns in sequence 
 */
void shiftColSet(uint8_t shiftColVal) 
{	
	if (shiftColVal < 8) {  // left BIT SHIFT for input (0->7) starting from 0x01 on portA with 0x00 on portc
		if (shiftColVal < 1) { //if it's the starting number of iter, then set ports to initial and exit 
			PORTA = 0x01; 
			PORTC = 0x00;	
		} else 	   //if it's not initial but it's still low, then shift left 1
			PORTA <<= 1;	  // this will work since the function will always call initial first, then called in ascending order 
	} else { 	 // >7 (8->15) then mod input with 8 (with if at start) & right shift from 0x80 on portC with 0x00 on portA 
		shiftColVal %= 8;
		if (shiftColVal < 1) { //if at point of port change, then set ports to initial and exit 
			PORTA = 0x00; 
			PORTC = 0x80;
		} else 				   //if it's not initial but it's high, then shift right 1
			PORTC >>= 1;
	}
}

int uart0_fputchar(char ch,FILE* stream) //f rom dan
{
	while(!(UCSR0A & 0x20)); // wait for empty transmit buffer
	UDR0 = ch;     	 		 // write char
	return 0;
}

FILE uart_str = FDEV_SETUP_STREAM(uart0_fputchar, NULL, _FDEV_SETUP_RW);

void buffer_clear(volatile struct BUFFEREDIO *io) // from dan
{
	io->wrptr=io->rdptr=0;
}

int uart0_init(unsigned int ubrr, unsigned char u2x) //from dan 
{
	buffer_clear(&SerialData0.rx);
	buffer_clear(&SerialData0.tx);
	// Setup the serial port
	UCSR0B = 0x00; 				  //disable while setting baud rate
	UCSR0C = 0x06; 				  // Asyn,NoParity,1StopBit,8Bit,
	if(u2x)
	UCSR0A = 0x02;				  // U2X = 1
	else
	UCSR0A = 0x00;
	UBRR0H = (unsigned char)((ubrr>>8)&0x000F);
	UBRR0L = (unsigned char)(ubrr&0x00FF);
	UCSR0B = (1<<RXCIE0)|(1<<RXEN0)|(1<<TXEN0);			// RX/TX Enable, RX/TX Interrupt Enable
	return 1;
}

int readADC()
{ 
	ADCSRA |= 1<<ADEN;		//turn on ADC feature
	ADCSRA |= 1<<ADSC;	//start first conversion
	while(ADCSRA&0x40); //start and wait until conversion complete  
	return ADCH;
} 

void setADC(uint8_t mux) { //separate function to avoid setting ADC mux to the same channel for each iter in the pad loop
	ADMUX = (ADMUX & 0b11100000) | (mux & 0b00011111);    //MUX 0 to 4
	mux >>= 2;
	ADCSRB = (ADCSRB & 0b11110111) | (mux & 0b00001000);  //MUX 5    //0b00100000	 
}

void digitalWrite(volatile uint8_t *port, uint8_t pin, uint8_t logic) { //port, pin, logic
	if (logic == 1) { //set high!
		*port |= (1<<pin);
		} else {
		*port &= ~(1<<pin);
	}
}

void setDir(volatile uint8_t *port, uint8_t pin, uint8_t logic) {
	if (logic == 0) { // if intended output, write 1 
		*port |= (1<<pin);
		} else { // if intended input, write 0 
		*port &= ~(1<<pin); //output 
	}
}

/*
* setRowMuxSelectors() - set the selector pins on the mux dependant on which row we're reading
*/
void setRowMuxSelectors(uint8_t row_number)
{  
	PORTL = (PORTL & 0xF0) | (row_number & 0x0F);
}

/*
* sendCompressed() - If value is nonzero, send it via serial terminal as a single char. If value is zero,
* increment zero count. The current zero count is sent and cleared before the next nonzero value
*/
void sendCompressed(uint8_t value)
{
	if(value < MIN_SEND_VALUE) //if under the threshold
	{
		if(compressed_zero_count < (COMPRESSED_ZERO_LIMIT - 1))
		{
			compressed_zero_count ++;
		}
		else
		{
    		uart0_fputchar(0,&uart_str);
    		uart0_fputchar(COMPRESSED_ZERO_LIMIT,&uart_str);
    		compressed_zero_count = 0;
		}
	}
	else
	{
		if(compressed_zero_count > 0)
		{
    		uart0_fputchar(0,&uart_str);
    		uart0_fputchar(compressed_zero_count,&uart_str);
    		compressed_zero_count = 0;
		}
	}
    if(value > MAX_SEND_VALUE) // don't go over 254 - leave 255 for the matrix break/restart
    {
	    uart0_fputchar(MAX_SEND_VALUE,&uart_str);
    }
    else if (value >= MIN_SEND_VALUE)
    {
	    uart0_fputchar(value,&uart_str);
    }
}

void padInput() {
	compressed_zero_count = 0;
	setADC(PIN_XYZ_ADC_INPUT);
	for(int i = 0; i < ROW_COUNT; i++) {                         // for all rows
	  setRowMuxSelectors(i);                                        // set the read row high    
	  for(int j = 0; j < COLUMN_COUNT; j++) {
		  shiftColSet(j);  
		  sendCompressed(readADC());
	  }
	}
	if(compressed_zero_count > 0)
	{
		uart0_fputchar(0,&uart_str);
		uart0_fputchar(compressed_zero_count,&uart_str);
	}
}

void init() {
	setDir(&DDRL, 6, 0); //FOR DEBUGGING 
	digitalWrite(&PORTL, 6, 1); //turn debug LED on  
	_delay_ms(1000);  // these 3 lines delay the uart output when plugged in in the hope of not being recognised as legacy mouse hardware  
	digitalWrite(&PORTL, 6, 0);
	DDRL = 0xFF; //setup row 1 on the mux by select 0000, en = 1, 
	PORTL = 0x00; //set led low, and for test, selectors low and enable low 
	DDRF = 0x00; // all ADCs are inputs 
	DDRK = 0x00;  
	DDRA = 0xFF; //setting all columns to outputs 
	DDRC = 0xFF; //change bottom for side strips too ! 
	digitalWrite(&ADCSRA, ADPS2, 1);//adjust prescalar to division factor 64
	digitalWrite(&ADCSRA, ADPS1, 1);  // 115.16 kHz sample rate 
	digitalWrite(&ADCSRA, ADPS0, 0); 
	ADMUX |= 1<<ADLAR;		//8 bit or 10 bit result -> setting ADLAR =1 to allow an 8bit reading frmo ADCH
	ADMUX |= 1<<REFS0;		//reference selection pins -- setting to use AVCC with ext cap @ AREF	
	uart0_init(1,0);  // Asyn,NoParity,1StopBit,8Bit,  -- baud rate: 230.4k u2xn = 1 
}

void clear(uint8_t sensor) //clears existing chage
{
	uint8_t tempPort;
	uint8_t tempDir;
	if (sensor+1 <= 2) {
		tempPort = PORTK;
		tempDir = DDRK;  
	} else {
		tempPort = PORTF;
		tempDir = DDRF;		
	}
	setDir(&tempDir, sideStrip[sensor][1], 0);
	setDir(&tempDir, sideStrip[sensor][2], 0);
	setDir(&tempDir, sideStrip[sensor][3], 0);
	setDir(&tempDir, sideStrip[sensor][4], 0);
	digitalWrite(&tempPort, sideStrip[sensor][1], 0);
	digitalWrite(&tempPort, sideStrip[sensor][2], 0);
	digitalWrite(&tempPort, sideStrip[sensor][3], 0);
	digitalWrite(&tempPort, sideStrip[sensor][4], 0);
}

void getStrip_1_2(uint8_t sensor) {
	// force Reading
	setDir(&DDRK, sideStrip[sensor][4], 0); 
	digitalWrite(&PORTK, sideStrip[sensor][4], 1); //SEND t3 high
	setDir(&DDRK, sideStrip[sensor][2], 0);	
	digitalWrite(&PORTK, sideStrip[sensor][2], 0);  //gnd ref divider resistor
	setDir(&DDRK, sideStrip[sensor][3], 0);
	digitalWrite(&PORTK, sideStrip[sensor][3], 1); //sending 5V at both ends of the pot (T2)
	setADC(sideStrip[sensor][5]); //Set ADC at T1 as input 
	_delay_ms(1);
	uint8_t force = readADC();
    if(force > MAX_SEND_VALUE) // don't go over 254 - leave 255 for the matrix break/restart
    {
	    uart0_fputchar(MAX_SEND_VALUE,&uart_str);
    }
    else
    {
	    uart0_fputchar(force,&uart_str);
    }
    //position reading 
	clear(sensor);
	setDir(&DDRK, sideStrip[sensor][3], 0); // send T2 high
	digitalWrite(&PORTK, sideStrip[sensor][3], 1);
	setDir(&DDRK, sideStrip[sensor][4], 0);
	digitalWrite(&PORTK, sideStrip[sensor][4], 0); // send T3 Low
	setDir(&DDRK, sideStrip[sensor][2], 0);
	digitalWrite(&PORTK, sideStrip[sensor][2], 0);  //draining any charge that may have capacitively coupled to sense line during transition of sensor terminals
	setDir(&DDRK, sideStrip[sensor][2], 1);		//hiZ line 1/2; have pin on input with port 0.
	digitalWrite(&PORTK, sideStrip[sensor][2], 0); //hiZ line 2/2 ON THE RES 
	setDir(&DDRK, sideStrip[sensor][1], 1); //SET T1 TO ADC INPUT 
	_delay_ms(1); //aid reading sensitivity
	uint8_t position = readADC();  
    if(position > MAX_SEND_VALUE) // don't go over 254 - leave 255 for the matrix break/restart
    {
	    uart0_fputchar(MAX_SEND_VALUE,&uart_str);
    }
    else 
    {
	    uart0_fputchar(position,&uart_str);
    }
}

void getStrip_3(uint8_t sensor) {
	// force Reading
	setDir(&DDRF, sideStrip[sensor][4], 0); 
	digitalWrite(&PORTF, sideStrip[sensor][4], 1);
	setDir(&DDRF, sideStrip[sensor][2], 0);	
	digitalWrite(&PORTF, sideStrip[sensor][2], 0);  //gnd divider resistor
	setDir(&DDRF, sideStrip[sensor][3], 0);
	digitalWrite(&PORTF, sideStrip[sensor][3], 1); //sending 5V at both ends of the pot 
	setADC(sideStrip[sensor][5]);
	_delay_ms(1);
	int force = readADC();
	if(force > MAX_SEND_VALUE) // don't go over 254 - leave 255 for the matrix break/restart
    {
	    uart0_fputchar(MAX_SEND_VALUE,&uart_str);
    }
    else
    {
	    uart0_fputchar(force,&uart_str);
    }
	//position reading
	clear(sensor);
	setDir(&DDRF, sideStrip[sensor][3], 0); 
	digitalWrite(&PORTF, sideStrip[sensor][3], 1);
	setDir(&DDRF, sideStrip[sensor][4], 0);
	digitalWrite(&PORTF, sideStrip[sensor][4], 0);
	setDir(&DDRF, sideStrip[sensor][2], 0);
	digitalWrite(&PORTF, sideStrip[sensor][2], 0);  //draining any charge that may have capacitively coupled to sense line during transition of sensor terminals
	setDir(&DDRF, sideStrip[sensor][2], 1);		//hiZ line 1/2; have pin on input with port 0.
	digitalWrite(&PORTF, sideStrip[sensor][2], 0); //hiZ line 2/2
	setDir(&DDRF, sideStrip[sensor][1], 1);
	_delay_ms(1); //aid reading sensitivity
	int position = readADC();
    if(position > MAX_SEND_VALUE) // don't go over 254 - leave 255 for the matrix break/restart
    {
	    uart0_fputchar(MAX_SEND_VALUE,&uart_str);
    }
    else 
    {
	    uart0_fputchar(position,&uart_str);
    }
}

void getStrip_4(uint8_t sensor) {
	// force Reading
	setDir(&DDRF, 6, 0);
	digitalWrite(&PORTF, 6, 1);
	setDir(&DDRL, 5, 0);
	digitalWrite(&PORTL, 5, 0);  //gnd divider resistor
	
	setDir(&DDRF, 5, 0);
	digitalWrite(&PORTF, 5, 1); //sending 5V at both ends of the pot
	setADC(0b00000100);
	_delay_ms(1);
	int force = readADC();
	if(force > MAX_SEND_VALUE) // don't go over 254 - leave 255 for the matrix break/restart
    {
	    uart0_fputchar(MAX_SEND_VALUE,&uart_str);
    }
    else
    {
	    uart0_fputchar(force,&uart_str);
    }
	setDir(&DDRF, 4, 0); //clear
	setDir(&DDRL, 5, 0); //done here since wiring is different from convention
	setDir(&DDRF, 5, 0);
	setDir(&DDRF, 6, 0);
	digitalWrite(&PORTF, 4, 0);
	digitalWrite(&PORTL, 5, 0);
	digitalWrite(&PORTF, 5, 0);
	digitalWrite(&PORTF, 6, 0);
	
	//position reading
		setDir(&DDRF, PF5, 0); //terminal 2 (right) 
		digitalWrite(&PORTF, PF5, 1);

		setDir(&DDRF, PF6, 0);  //Left end of pot is at +5V, Right end is at GND
		digitalWrite(&PORTF, PF6, 0);
		
		setDir(&DDRL, PL5, 0); // disconnect reference divider resistor
		digitalWrite(&PORTL, PL5, 0);
	
	setDir(&DDRL, PL5, 1); // disconnect reference divider resistor
	digitalWrite(&PORTL, PL5, 0);
	
	
	setDir(&DDRF, PF4, 1);
	setADC(0b00000100);
	_delay_ms(1); //aid reading sensitivity
	int position = readADC();
    if(position > MAX_SEND_VALUE) // don't go over 254 - leave 255 for the matrix break/restart
    {
	    uart0_fputchar(MAX_SEND_VALUE,&uart_str);
    }
    else 
    {
	    uart0_fputchar(position,&uart_str);
    }
}

int main() {
	init();
	while (1) {	
		uart0_fputchar(PACKET_END_BYTE,&uart_str);
		for (uint8_t i = 0; i<4; i++) {  
			switch (i)
			{
				case 0:
				case 1: getStrip_1_2(i); break;
				case 2: getStrip_3(i); break;
				case 3: getStrip_4(i);
			}
		}
	   padInput();	
	}
	return 0;
}