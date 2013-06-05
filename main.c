#include <avr/io.h>
#include <util/delay.h>

#define DELAY_AMOUNT	1
#define setBit(a, b)	(a) |= (b)
#define clrBit(a, b)	(a) &= ~(b)

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

void displayMatrix(short bits)
{
	int i = 0;
	int j = 0;
	
	DDRB = 0x7F;  //ensure everything is un-tristated
	PORTB = 0x7F; //and set high
	DDRD &= ~0x03;
	
	for (i = 0; i < 2; i++)
	{
		//set digit high
		setBit(DDRD, 1 << i); 
		setBit(PORTD, 1 << i);

		PORTB &= ((~bits >> (i*8)) & 0x7F);	//clear PORTB bit if corresponding bits bit is 1
		_delay_ms(DELAY_AMOUNT);

		PORTB |= 0x7F;		//set the pins back high to turn off display
		//set digit to tristate
		clrBit(PORTD, 1 << i);
		clrBit(DDRD, 1 << i);
	}
	
	DDRB = 0x00;  //ensure everything is tristated
	_delay_ms(DELAY_AMOUNT);	//dim the display
}

/*
	Displays the number on the digit displays
	PRECONDITION: Number must be between 44 and 107 inclusive
*/
void displayNumber(int number)
{
	displayMatrix(lookup_matrix[number - 44]);
}

int main(void)
{
	_delay_ms(3000);
	
	blink(500);

	int i = 0;
	int j = 0;

	DDRB = 0x00;	//set to inputs (tristate)
	DDRD &= ~0x03;	//tristate the two pins we use
	PORTB = 0x00;	//no pull-ups (tristate)
	PORTD &= ~0x03;
	
	while (1)
	{
		for (i = 0; i < 64; i++)
			for (j = 0; j < 30; j++)
				displayMatrix(lookup_matrix[i]);
	}
}