#include <avr/io.h>
// #include <util/delay.h>
#include <avr/interrupt.h>
#include <Arduino.h>

// This must be defined before including "ps2mouse.h"
//#define SERIAL_REPORTING

#include "ps2mouse.h"
//#include "LCDDraw.h"

PS2Mouse PS2(3);

ISR(INT0_vect) // PS/2 clock signal must be connected here
{
	PS2.interrupt_handler();
}

constexpr uint8_t SampleRate{ 100 };
constexpr uint8_t quadBits[4]{ 0b00, 0b10, 0b11, 0b01 };

uint8_t quadIndex_x{ 0 };
uint16_t dTicks_x;
bool positive_dx{ true }; // tiiä sitten tartteeko näitten olla volatile
uint8_t pulsesToDo_x;

uint8_t quadIndex_y{ 0 };
uint16_t dTicks_y;
bool positive_dy{ true };
uint8_t pulsesToDo_y;

// int main()
void setup()
{
#ifdef SERIAL_REPORTING
	Serial.begin(9600);
#endif

	DDRD &= ~(0b0100 | PS2.dataPinMask); // pins 2 and "dataPin" as input
	//PORTD |= (0b0100 | PS2.dataPinMask); // don't enable internal pull up resistors
	                                       // The mouse is supposed to already have them

	//DDRC  |=  0b111111;    // quadrature outputs, lmb, rmb
	DDRC  &= !0b111111;
	PORTC &= !0b111111;

	EICRA = (EICRA & ~(0b11)) | (0b10); // The falling edge of INT0 generates an interrupt request.
	EIMSK |= 1;                         // Bit 0 – INT0: External Interrupt Request 0 Enable
	sei();                              // enable interrupts

	TCCR1A &= ~( (1 << WGM10) | (1<< WGM11) );        // normal mode, no PWM
	TCCR1B = (0 << CS12) | (1 << CS11) | (1 << CS10); // timer 1 prescaler
	// TIMSK1 |= (1 << OCIE1A);                          // enable interrupt on comp a match

	PS2.begin(PS2Mouse::resolution::dpmm8, SampleRate);
#ifdef LCD_GRAPHICS
	SetupLCDCursor();
	DrawCursor(cursorGfx, {cursorPos.x, cursorPos.y});
#endif
}

void UpdatePulseRate(int16_t dx, int16_t dy)
{
	// 64 is timer1 prescaler factor
	static constexpr uint16_t ticksTillNextUpd = F_CPU / 64u / SampleRate;

	uint8_t oldSreg{ SREG };
	cli();
	
	if ( positive_dx )
		dx += pulsesToDo_x;
	else
		dx -= pulsesToDo_x;

	if ( dx != 0 )
	{
		positive_dx = (dx > 0);
		dx = abs(dx);
		pulsesToDo_x = min(254, dx);
		dTicks_x = ticksTillNextUpd / (pulsesToDo_x + 1);
		
		OCR1A = TCNT1 + dTicks_x;
		TIMSK1 |= (1 << OCIE1A);
	}

	if ( positive_dy )
		dy += pulsesToDo_y;
	else
		dy -= pulsesToDo_y;

	if ( dy != 0 )
	{
		positive_dy = (dy > 0);
		dy = abs(dy);
		pulsesToDo_y = min(254, dy);
		dTicks_y = ticksTillNextUpd / (pulsesToDo_y + 1);
		
		OCR1B = TCNT1 + dTicks_y;
		TIMSK1 |= (1 << OCIE1B);
	}

	SREG = oldSreg;
}

// x-suuntaiset pulssit. y-suuntaiset hoidetaan compb:llä
ISR(TIMER1_COMPA_vect)
{
	if ( pulsesToDo_x == 0 )
	{
		TIMSK1 &= ~(1 << OCIE1A);
		return;
	}
	--pulsesToDo_x;

	if ( positive_dx )
		quadIndex_x = (quadIndex_x + 1) % 4;
	else
		quadIndex_x = (quadIndex_x + 3) % 4;
	
	//PORTC = (PORTC & ~0b11) | quadBits[quadIndex_x];
	DDRC = (DDRC & ~0b11) | quadBits[quadIndex_x];
	OCR1A += dTicks_x;
}

ISR(TIMER1_COMPB_vect)
{
	if ( pulsesToDo_y == 0 )
	{
		TIMSK1 &= ~(1 << OCIE1B);
		return;
	}
	--pulsesToDo_y;

	if ( positive_dy )
		quadIndex_y = (quadIndex_y + 1) % 4;
	else
		quadIndex_y = (quadIndex_y + 3) % 4;
	
	//PORTC = (PORTC & ~0b1100) | (quadBits[quadIndex_y] << 2);
	DDRC = (DDRC & ~0b1100) | (quadBits[quadIndex_y] << 2);
	OCR1B += dTicks_y;
}

//while (1)
void loop()
{
#ifdef SERIAL_REPORTING
	if ( PS2.errorCode )
	{
		Serial.print("Error: ");
		Serial.println(PS2.errorCode, 16);
		PS2.errorCode = 0;
		PS2.clear(true);
		PS2.sendByte(PS2Mouse::code::Resend);
	}
		while ( PS2.available() )
		{
			Serial.print(PS2.read());
			Serial.print(' ');
		}
#endif

	if ( PS2.errorCode )
	{
		PS2.errorCode = 0;
		PS2.clear(true);
		PS2.sendByte(PS2Mouse::code::Resend);
	}

	while ( PS2.available() >= 3 )
	{
		auto rep = PS2.getReport();
		
#ifdef LCD_GRAPHICS
		bool cursorChanged{ false };
		if ( rep.LMB != buttons[0] || rep.RMB != buttons[1] )
		{
			buttons[0] = rep.LMB;
			buttons[1] = rep.RMB;
			cursorChanged = true;

			for (int8_t i = 0; i < cursorSize.y; i++)
				cursorGfx[i] = ((!rep.LMB && !rep.RMB) * cursorCross[i]) | (rep.LMB * cursorEx[i]) | (rep.RMB * cursorBox[i]);
		}

		cursorPos.x = Clamp8(cursorPos.x + rep.dx);
		cursorPos.y = Clamp8(cursorPos.y - rep.dy, canvas.h - 1);

		if ( cursorChanged || (rep.dx !=0) || (rep.dy !=0) )
			DrawCursor(cursorGfx, {cursorPos.x, cursorPos.y});
#endif

		UpdatePulseRate(-rep.dx, rep.dy);
		
		//uint8_t buttons{ ((!rep.LMB) << 4) | ((!rep.RMB) << 5) };
		uint8_t buttons{ ((rep.LMB) << 4) | ((rep.RMB) << 5) };
		//PORTC = (PORTC & ~0b110000) | buttons;
		DDRC = (DDRC & ~0b110000) | buttons;

		// TODO: Valita portti ja bitti MMB:lle. Eipä sillä että yksikään Amigan ohjelma käyttäisi keskinappia.
	}
}
