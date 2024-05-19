#pragma once
#include <stdint.h>
#include <LiquidCrystal.h>
#define LCD_GRAPHICS

LiquidCrystal lcd(5, 7, 8, 9, 10, 11);

struct Vec2
{
	int8_t x, y;
	constexpr Vec2(int8_t x, int8_t y) : x(x), y(y) {}
	constexpr Vec2(   int x,    int y) : x(x), y(y) {}
};

struct Rect { int8_t x, y, w, h; };


constexpr Vec2 cursorSize{ 5, 5 };
constexpr Vec2 cursorPoint{ 2, 2 };
constexpr Rect canvas{ 0, 0, 16 * 6 - 1, 2 * 9 - 1 };
Vec2 cursorPos{2, 2};
bool buttons[2]{false, false};
Rect drawnSquares{ 0, 0, 1, 1 };

constexpr uint8_t cursorCross[cursorSize.y]
{
	0b00100,
	0b00100,
	0b11011,
	0b00100,
	0b00100
};

constexpr uint8_t cursorEx[cursorSize.y]
{
	0b10001,
	0b01010,
	0b00100,
	0b01010,
	0b10001
};

constexpr uint8_t cursorBox[cursorSize.y]
{
	0b01110,
	0b10001,
	0b10001,
	0b10001,
	0b01110
};

uint8_t cursorGfx[cursorSize.y];

void DrawCursor(const uint8_t* graphic, const Vec2 pos)
{
	// empty old boxes
	for ( uint8_t i = 0; i < drawnSquares.h; ++i )
	{
		lcd.setCursor(drawnSquares.x, drawnSquares.y + i);
		for ( uint8_t j = 0; j < drawnSquares.w; ++j )
		{
			lcd.write(' ');
		}
	}

	// calculate new boxes
	Vec2 topLeft{ pos.x - cursorPoint.x, pos.y - cursorPoint.y };
	Vec2 bottomRight{ topLeft.x + cursorSize.x - 1, topLeft.y + cursorSize.y - 1 };

#ifndef max
	auto max = [](int8_t a, int8_t b){ if ( a > b ) return a; else return b; };
#endif
#ifndef min
	auto min = [](int8_t a, int8_t b){ if ( a < b ) return a; else return b; };
#endif

	drawnSquares.x = max(0, (topLeft.x + 1) / 6);
	drawnSquares.y = max(0, (topLeft.y + 1) / 9);
	drawnSquares.w = 1 + ( min(15, bottomRight.x / 6) > drawnSquares.x ) ;
	drawnSquares.h = 1 + ( min( 1, bottomRight.y / 9) > drawnSquares.y ) ;

	// make new custom chars
	uint8_t custChar[8];
	for ( int8_t h = 0; h < drawnSquares.h; ++h )
	{
		for ( int8_t w = 0; w < drawnSquares.w; ++w )
		{
			Vec2 offset{ topLeft.x - 6 * (drawnSquares.x + w), topLeft.y - 9 * (drawnSquares.y + h) };

			for ( int8_t i = 0; i < 8; ++i )
			{
				int8_t row = i - offset.y;
				if ( row < 0 || row >= cursorSize.y )
					custChar[i] = 0;
				else
					custChar[i] = 0b11111 & ((offset.x < 0) ? (graphic[row] << -offset.x) : (graphic[row] >> offset.x));
			}
			
			lcd.createChar(2*h + w, custChar);
		}
	}

	// write to new boxes
	for ( uint8_t i = 0; i < drawnSquares.h; ++i )
	{
		lcd.setCursor(drawnSquares.x, drawnSquares.y + i);
		for ( uint8_t j = 0; j < drawnSquares.w; ++j )
		{
			lcd.write(i*2 + j);
		}
	}
}

void SetupLCDCursor()
{
	lcd.begin(16,2);
	memcpy(cursorGfx, cursorCross, cursorSize.y);
}

inline int8_t Clamp8(int16_t x, int8_t maximum = canvas.w - 1, int8_t minimum = 0)
{
	if ( x > maximum ) return maximum;
	if ( x < minimum ) return minimum;
	return static_cast<int8_t>(x);
}
