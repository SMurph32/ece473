//Zachary Murphy
//10.22.14


//  HARDWARE SETUP:
//  PORTA is connected to the segments of the LED display. and to the pushbuttons.
//  PORTA.0 corresponds to segment a, PORTA.1 corresponds to segement b, etc.
//  PORTB bits 4-6 go to a,b,c inputs of the 74HC138.
//  PORTB bit 7 goes to the PWM transistor base.

#define F_CPU 16000000 // cpu speed in hertz 
#define TRUE 1
#define FALSE 0
#define MAX_CHECKS 12
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>



#define Y7 0b01110000
#define DC 0b10001111

uint8_t debounced_state = 0; // Debounced state of the switches
uint8_t state[MAX_CHECKS]; // Array that maintains bounce status
uint8_t check_index = 0; // Pointer into State
int count=0, block = 0, press = 0, button=0;

void DebounceSwitch();


uint16_t bcd;
uint8_t digit_data[5] = {
	0b00000000,//0
	0b00010000,//1
	0b00100000,//:
	0b00110000,//2
	0b01000000 //3
};


//holds data to be sent to the segments. logic zero turns segment on
uint8_t segment_data[5] = { 
	0b11111111,
	0b11111111,
	0b11111111,
	0b11111111,
	0b11111111
};

//decimal to 7-segment LED display encodings, logic "0" turns on segment
uint8_t dec_to_7seg[12] = {
	0b11000000, //0
	0b11111001, //1
	0b10100100, //2
	0b10110000, //3
	0b10011001, //4
	0b10010010, //5
	0b10000010, //6
	0b11111000, //7
	0b10000000, //8
	0b10011000, //9
	0b11111111, //1
	0b10011000 //9
}; 
void spi_init(void){
	DDRB  |=   0xff;          //Turn on SS, MOSI, SCLK
	SPCR  |=   (1 << SPE) | (1 << MSTR);     //set up SPI mode
	SPSR  |=   (1 << SPI2X);           // double speed operation
}//spi_init

/***********************************************************************/
//                              tcnt0_init                             
//Initalizes timer/counter0 (TCNT0). TCNT0 is running in async mode
//with external 32khz crystal.  Runs in normal mode with no prescaling.
//Interrupt occurs at overflow 0xFF.
//

/*************************************************************************/
//                           timer/counter0 ISR                          
//When the TCNT0 overflow interrupt occurs, the count_7ms variable is    
//incremented.  Every 7680 interrupts the minutes counter is incremented.
//tcnt0 interrupts come at 7.8125ms internals.
// 1/32768         = 30.517578uS
//(1/32768)*256    = 7.8125ms
//(1/32768)*256*64 = 500mS
/*************************************************************************/
ISR(TIMER0_OVF_vect){
	static uint8_t count_7ms = 0;        //holds 7ms tick count in binary
	count_7ms++;

	PORTA = 0xff;//make PORTA an input port with pullups 
	DDRA = 0x00;
	PORTB = 0b01110000;//enable tristate buffer for pushbutton switches
	DebounceSwitch();
	if(debounced_state && button==-1)
		button=debounced_state;


	if ((count_7ms % 64)==0) //?? interrupts equals one half second 
		count = (++count)%1023;//bound the count to 0 - 1023
	return 0;
}

void int0_init(){
	TIMSK |= (1 << TCNT0);			//enable interrupts
	TCCR0 |= (1 << CS02) | (1 << CS00); 	//normal mode, prescale by 128


}

//debouncing switch checks for 12 consecutive signals from same button before returning 1
void DebounceSwitch(){
	uint8_t i,j;
	state[check_index++%MAX_CHECKS]=0xff - PINA;
	j=0xff;
	for(i=0; i<MAX_CHECKS-1;i++)j=j & state[i];
	debounced_state = j;
}
//returns the display to segmentn_data in decimal
void segsum(uint16_t sum) {
	int num_d=0, i;
	int temp, temp2;
	temp = sum;
	while(sum >= (pow(10, num_d))) {num_d++;}//record number of digits



	//seg segment_data array correctly
	temp = sum/1000;
	temp2=temp*1000;
	segment_data[4] = dec_to_7seg[temp];
	temp = sum - temp2;
	temp = temp/100;
	temp2+=temp*100;
	segment_data[3] = dec_to_7seg[temp];
	temp = sum - temp2;
	temp = temp/10;
	temp2+=temp*10;
	segment_data[1] = dec_to_7seg[temp];
	temp = sum - temp2;
	segment_data[0] = dec_to_7seg[temp];

	//remove leading 0's
	if(num_d>2) {num_d++;}
	for(i=5;i>num_d;i--) {segment_data[i-1] = 0b11111111;}	

}



uint8_t main()
{
	int i;
	static uint8_t bar_display = 0x08; //holds count for display 

	int0_init();
	//set port bits 4-7 B as outputs
	spi_init();    //initalize SPI port
	sei();         //enable interrupts before entering loop

	DDRB = 0xff;
	DDRC = 0xff;
	PORTC = 0x00;
	DDRA = 0xff;
	PORTA = 0x00;
	SPDR = 0x01;

	while(1)
	{


		if(button>=0){
			switch(button){
				case 1:
					SPDR = 0x08;
					break;
				case 2:
					SPDR = 0x18;
					break;

			}
			button = -1;
		}

		while(bit_is_clear(SPSR, SPIF));	
		PORTB |=  0x01;          //strobe output data reg in HC595 - rising edge
		PORTB &=  0xfe;        //falling edge
		segsum(count);
		//			}
		//		}else block = 0;

		for(i=4;i>=0;i--){//bound a counter (0-4) to keep track of digit to display 
			DDRA = 0xff;//make PORTA an output
			PORTA = segment_data[i];//segment_data[i];
			PORTB = digit_data[i];//update digit to display
			_delay_us(1000);
		}
}
return 0;
}//main
/*
   ISR(TIMER0_OVF_vect){
   static uint8_t count_7ms = 0;        //holds 7ms tick count in binary
   static uint8_t display_count = 0x01; //holds count for display 

   count_7ms++;                //increment count every 7.8125 ms 
   if ((count_7ms % 64)==0){ //?? interrupts equals one half second 
   SPDR = display_count;               //wait till data is sent out (while spin loop)
   while(bit_is_clear(SPSR, SPIF));	
   PORTB |=  0x01;          //strobe output data reg in HC595 - rising edge
   PORTB &=  0xfe;        //falling edge
   display_count = (display_count << 1); //shift display bit for next time 
   }
   if (display_count == 0x00){display_count=0x01;} //display back to 1st positon
   }*/


