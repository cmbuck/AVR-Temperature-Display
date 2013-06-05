#include <avr/io.h>
#include <util/delay.h>

#define DELAY_AMOUNT	5
#define setBit(a, b)	(a) |= (b)
#define clrBit(a, b)	(a) &= ~(b)

const short lookup_matrix[64] = 
{	0x3333, 0x335B, 0x335F, 0x3370, 0x337F, 0x3373, 0x5B7C, 0x5B30,
	0x5B6F, 0x5B7B, 0x5B33, 0x5B5B, 0x5B5F, 0x5B70, 0x5B7F, 0x5B73,
	0x5F7C, 0x5F30, 0x5F6F, 0x5F7B, 0x5F33, 0x5F5B, 0x5F5F, 0x5F70,
	0x5F7F, 0x5F73, 0x707C, 0x7030, 0x706F, 0x707B, 0x7033, 0x705B,
	0x705F, 0x7070, 0x707F, 0x7073, 0x7F7C, 0x7F30, 0x7F6F, 0x7F7B,
	0x7F33, 0x7F5B, 0x7F5F, 0x7F70, 0x7F7F, 0x7F73, 0x737C, 0x7330,
	0x736F, 0x737B, 0x7333, 0x735B, 0x735F, 0x7370, 0x737F, 0x7373,
	0x7E7C, 0x7E30, 0x7E6F, 0x7E7B, 0x7E33, 0x7E5B, 0x7E5F, 0x7E70	};

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
		//blink(250);
		setBit(DDRD, 1 << i); 
		setBit(PORTD, 1 << i);
		/*
		for (j = 0; j < 7; j++)
		{
			//set column to respective passed bit
			if (bits & (1 << (4*i + j)))
			setBit(DDRB, 1 << j);
			_delay_ms(DELAY_AMOUNT);
			clrBit(DDRB, 1 << j);
		}*/
		PORTB &= ((~bits >> (i*8)) & 0x7F);
		//_delay_ms(DELAY_AMOUNT);
		blink(DELAY_AMOUNT);
		PORTB |= 0x7F;
		//set digit to tristate
		clrBit(PORTD, 1 << i);
		clrBit(DDRD, 1 << i);
	}
	
	DDRB = 0x00;  //ensure everything is tristated
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