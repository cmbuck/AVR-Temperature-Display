#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define DELAY_AMOUNT	4
#define setBit(a, b)	(a) |= (b)
#define clrBit(a, b)	(a) &= ~(b)
#define BIT(a)			(1 << a)

//This specifies the number of "resolution points" to adjust the
//temperature sensor readings by
#define CALIBRATION	(-10)

#define USART_BAUDRATE 2400
#define BAUD_PRESCALE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)
#define DIGIT1_PIN		3
#define DIGIT2_PIN		4

static volatile short displayBits;
enum {DIGIT1, DIGIT2, OFF} ISR_State;
static volatile int isrStateCount = 0;
static volatile int isrStateCountSetPoint = 0;

ISR(TIMER1_COMPA_vect)
{
	switch (ISR_State)
	{
	case DIGIT1 :
		PORTB &= ((~displayBits >> ((0)*8)) & 0x7F);	//clear PORTB bit if corresponding bits bit is 1
		setBit(DDRD, 1 << DIGIT1_PIN);
		setBit(PORTD, 1 << DIGIT1_PIN);
		ISR_State = DIGIT2;
		break;

	case DIGIT2 :
		clrBit(PORTD, 1 << DIGIT1_PIN);
		clrBit(DDRD, 1 << DIGIT1_PIN);
		PORTB |= 0x7F;
		PORTB &= ((~displayBits >> ((1)*8)) & 0x7F);	//clear PORTB bit if corresponding bits bit is 1
		setBit(DDRD, 1 << DIGIT2_PIN);
		setBit(PORTD, 1 << DIGIT2_PIN);
		ISR_State = OFF;
		break;

	case OFF :
		clrBit(PORTD, 1 << DIGIT2_PIN);
		clrBit(DDRD, 1 << DIGIT2_PIN);
		PORTB |= 0x7F;
		isrStateCount++;
		if (isrStateCount >= isrStateCountSetPoint)
		{
			ISR_State = DIGIT1;
			isrStateCount = 0;
		}
		break;
	}
}

/*
 * This lookup matrix is the pattern of bits to illuminate
 * The higher byte is the left digit, lower byte is the right digit
 */
const short lookup_matrix[64] = 
{	
	0x3333, 0x335B, 0x335F, 0x3370,
	0x337F, 0x3373, 0x5B7E, 0x5B30,
	0x5B6D, 0x5B79, 0x5B33, 0x5B5B,
	0x5B5F, 0x5B70, 0x5B7F, 0x5B73,
	0x5F7E, 0x5F30, 0x5F6D, 0x5F79,
	0x5F33, 0x5F5B, 0x5F5F, 0x5F70,
	0x5F7F, 0x5F73, 0x707E, 0x7030,
	0x706D, 0x7079, 0x7033, 0x705B,
	0x705F, 0x7070, 0x707F, 0x7073,
	0x7F7E, 0x7F30, 0x7F6D, 0x7F79,
	0x7F33, 0x7F5B, 0x7F5F, 0x7F70,
	0x7F7F, 0x7F73, 0x737E, 0x7330,
	0x736D, 0x7379, 0x7333, 0x735B,
	0x735F, 0x7370, 0x737F, 0x7373,
	0x7E7E, 0x7E30, 0x7E6D, 0x7E79,
	0x7E33, 0x7E5B, 0x7E5F, 0x7E70 
};


void blink(int duration)
{
	DDRB |= 0x80;
	PORTB |= 0x80;
	_delay_ms(duration);
	clrBit(PORTB, 0x80);
	clrBit(DDRB, 0x80);
	_delay_ms(duration);
}

void displayMatrix(short bits, float brightness)
{
	int i = 0;
	int j = 0;
	
	DDRB = 0x7F;  //ensure everything is un-tristated
	PORTB = 0x7F; //and set high
	DDRD &= ~0x0C;
	setBit(DDRD, BIT(3) | BIT(4));
	
	for (i = 3; i < 5; i++)
	{
		//set digit high
		setBit(DDRD, 1 << i);
		setBit(PORTD, 1 << i);

		PORTB &= ((~bits >> ((i-3)*8)) & 0x7F);	//clear PORTB bit if corresponding bits bit is 1
		_delay_ms(DELAY_AMOUNT * brightness);

		PORTB |= 0x7F;		//set the pins back high to turn off display
		//set digit to tristate
		clrBit(PORTD, 1 << i);
		clrBit(DDRD, 1 << i);
	}
	
	DDRB = 0x00;  //ensure everything is tristated
	_delay_ms(DELAY_AMOUNT * (1 - brightness));	//dim the display
}

/*
	Displays the number on the digit displays
	PRECONDITION: Number must be between 44 and 107 inclusive
*/
void displayNumber(int number, float brightness)
{
	int fixedNumber = number;
	if (number > 107)	fixedNumber = 107;
	if (number < 44 )	fixedNumber = 44;
	//displayMatrix(lookup_matrix[fixedNumber - 44], brightness);
	displayBits = lookup_matrix[fixedNumber - 44];
}

void initializeTimer()
{
	TCCR1B |= (1 << WGM12); //enable CTC mode

	OCR1A = 16; //about 50 Hz with 1024 prescale

	TCCR1B |= (1 << CS10) | (1 << CS12); // Set up timer with 1024 prescale

	TIMSK1 |= (1 << OCIE1A); //enable timer1 CTC interrupt
	sei();			//enable global interrupts
}

void initializeSerial()
{
	UCSR0B |= (1 << RXEN0) | (1 << TXEN0);   // Turn on the transmission and reception circuitry
	UCSR0C |= (1 << UCSZ00) | (1 << UCSZ01); // Use 8-bit character sizes

	UBRR0H = (BAUD_PRESCALE >> 8); // Load upper 8-bits of the baud rate value into the high byte of the UBRR register
	UBRR0L = BAUD_PRESCALE; // Load lower 8-bits of the baud rate value into the low byte of the UBRR register
}

unsigned char serialReadByte()
{
	unsigned char receivedByte;
	while ((UCSR0A & (1 << RXC0)) == 0) {}; // Do nothing until data have been received and is ready to be read from UDR
	      receivedByte = UDR0; // Fetch the received byte value into the variable "ByteReceived"
	return receivedByte;
}

void serialWriteByte(unsigned char byteToSend)
{
	while ((UCSR0A & (1 << UDRE0)) == 0) {}; // Do nothing until UDR is ready for more data to be written to it
	      UDR0 = byteToSend; // Echo back the received byte back to the computer
}

// initialize adc
void adc_init()
{
    // AREF = AVcc
    ADMUX = (1<<REFS0);
 
    // ADC Enable and prescaler of 128
    // 1000000/128 = 7812
    // 8000000/128 = 62500
    ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
}
 
// read adc value
uint16_t adc_read(uint8_t ch)
{
    // select the corresponding channel 0~7
    // ANDing with '7' will always keep the value
    // of 'ch' between 0 and 7
    ch &= 0b00000111;  // AND operation with 7
    ADMUX = (ADMUX & 0xF8)|ch;     // clears the bottom 3 bits before ORing
 
    // start single conversion
    // write '1' to ADSC
    ADCSRA |= (1<<ADSC);
 
    // wait for conversion to complete
    // ADSC becomes '0' again
    // till then, run loop continuously
    while(ADCSRA & (1<<ADSC));
 
    return (ADC);
}

int calcTemp(int adcValue)
{
	return ((int) ((adcValue * (5.0 / 1024) - 0.75) / 0.018)) + 77;
}

int main(void)
{
	_delay_ms(3000);
	
	blink(500);
	
	initializeTimer();
	adc_init();
	initializeSerial();
	
	int i = 0;
	int j = 0;

	ISR_State = OFF;

	DDRB = 0x7F;	//set to outputs
	DDRD &= ~0x0C;	//tristate the two pins we use
	PORTB = 0x7F;	//put high
	PORTD &= ~0x0C;
	
	int adcValue = 0;
	int potValue = 0;
	float brightness;
	int temperatureValue = 0;
	
	while (1)
	{
		adcValue = adc_read(0);
		potValue = adc_read(1);
		isrStateCountSetPoint = potValue / 128;
		temperatureValue = calcTemp(adcValue + CALIBRATION);
		
		serialWriteByte(adcValue & 0xFF);

		//for (i = 0; i < 64; i++)
			displayNumber(temperatureValue, brightness);
		
		/**
		for (i = 0; i < 64; i++)
			for (j = 0; j < 30; j++)
				displayMatrix(lookup_matrix[i]);
		*/
	}
}
