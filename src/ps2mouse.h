#pragma once
//#include <avr/interrupt.h>
//#include <util/delay.h>
#include <Arduino.h>
#include "circbuff.h"

#ifdef SERIAL_REPORTING
#include <HardwareSerial.h>
#else
#define FAKE_SERIAL_DELAY 50
#endif

class PS2Mouse
{
private:
	volatile int8_t nextBit;
	volatile uint8_t checksum;
	volatile uint8_t theByte;

	volatile bool sendSuccess;
	const bool pullUp;
	CircularBuffer<uint8_t, 30> buffer;

public:
	enum code : uint8_t
	{
		Reset                = 0xff,
		Error                = 0xfc, // also: self-test failed
		Resend               = 0xfe,
		SetDefaults          = 0xf6,
		DisableDataReporting = 0xf5,
		EnableDataReporting  = 0xf4,
		// Valid sample rates are 10, 20, 40, 60, 80, 100, and 200 samples/sec.
		SetSampleRate        = 0xf3,
		GetDeviceID          = 0xf2,
		SetRemoteMode        = 0xf0,
		SetWrapMode          = 0xee,
		ResetWrapMode        = 0xec,
		ReadData             = 0xeb,
		SetStreamMode        = 0xea,
		StatusRequest        = 0xe9,
		SetResolution        = 0xe8,
		SetScaling_2_1       = 0xe7,
		SetScaling_1_1       = 0xe6,
		// hiireltä:
		SelfTestPassed       = 0xaa,
		Acknowledge          = 0xfa
	};

	enum resolution : uint8_t
	{
		dpmm1 = 0, dpmm2 = 1, dpmm4 = 2, dpmm8 = 3
	};

	struct Report
	{
		int16_t dx, dy;
		bool LMB, MMB, RMB;

		explicit Report(uint8_t (&packet)[3])
		: dx{ packet[1] - 0x100 * ( (packet[0] & 0b00010000) != 0 ) }
		, dy{ packet[2] - 0x100 * ( (packet[0] & 0b00100000) != 0 ) }
		, LMB( packet[0] & 0b001 )
		, MMB( packet[0] & 0b100 )
		, RMB( packet[0] & 0b010 )
		{}
	};
	

	volatile bool receiving, sending;
	const uint8_t dataPinMask;
	volatile uint8_t errorCode;
	static constexpr uint8_t clockPinMask{ 1 << 2 }; // external interrupt 0, pin D 2
	static constexpr uint8_t powerPinMask{ 1 << 5 };

	// Tähän voisi tehä niin, että const jäseninä eicra_shift ja eimsk_mask, jotka
	// konstruktööri laskee.  ja clockpinmaskin kans. Sitten porttien alustukset vois laittaa inittiin.
	// jne.
	explicit PS2Mouse(uint8_t PortDDataPin)
	: pullUp(PORTD & 0b0100)  // hetkinen! Tämähän tapahtuu ennen beginiä tai mainia.
	, dataPinMask(1 << PortDDataPin)
	, errorCode(0)
	{
		DDRB |= powerPinMask;
		// PORTB &= ~powerPinMask;
		power(false);
	}

	PS2Mouse(const PS2Mouse&) = delete;
	PS2Mouse(PS2Mouse&&) = delete;
	PS2Mouse& operator=(const PS2Mouse&) = delete;
	PS2Mouse& operator=(PS2Mouse&&) = delete;

	Report getReport()
	{
		uint8_t input[3];
		input[0] = buffer.pop_front();

		if ( (input[0] & 0b1000) == 0) // desync!
		{
#ifdef SERIAL_REPORTING
			Serial.println("desync!");
#endif
			clear(true);
			sendByte(PS2Mouse::code::Resend);
		}

		input[1] = buffer.pop_front();
		input[2] = buffer.pop_front();

		return Report(input);
	}

	void power(bool on)
	{
		// Oletetaan, että sähöt lähtee portista B
		if ( on )
			PORTB |= powerPinMask;
		else
			PORTB &= ~powerPinMask;
	}
	
	void clear(bool bufferToo = true)
	{
		checksum = 1;
		nextBit = -1;
		theByte = 0;
		if ( bufferToo )
			buffer.clear();
	}
	
	// void init()
	// {
	// 	receiving = true;
	// 	sending = false;
	// 	clear();
	// }
	
	void begin(resolution Resolution, uint8_t SampleRate)
	{
		receiving = true;
		sending = false;
		clear(true);

#ifdef SERIAL_REPORTING
		Serial.println("\n\nstarting up");
#endif
		power(1);
		
		while ( available() < 2);

#ifdef SERIAL_REPORTING
		Serial.println("\nsetting rez");
#else
		delay(FAKE_SERIAL_DELAY);
#endif
		errorCode = 0;
		clear();
		sendByte(PS2Mouse::code::SetResolution);
#ifdef SERIAL_REPORTING
		if ( WaitForAck() )
			Serial.println("Ack'd");
		else
			Serial.println("Not ack'd");
#else
		WaitForAck();
		delay(FAKE_SERIAL_DELAY);
#endif

		sendByte(Resolution);
#ifdef SERIAL_REPORTING
		if ( WaitForAck() )
			Serial.println("Ack'd");
		else
			Serial.println("Not ack'd");
#else
		WaitForAck();
		delay(FAKE_SERIAL_DELAY);
#endif
		
		// Valid sample rates are 10, 20, 40, 60, 80, 100, and 200
		if ( (SampleRate != 10u) && (SampleRate != 20u)
		  && (SampleRate != 40u) && (SampleRate != 60u)
		  && (SampleRate != 80u) && (SampleRate != 100u)
		  && (SampleRate != 200u) )
		{
			SampleRate = 10;
		}

#ifdef SERIAL_REPORTING
		Serial.println("\nsetting sample rate");
#else
		delay(FAKE_SERIAL_DELAY);
#endif
		errorCode = 0;
		clear();
		sendByte(PS2Mouse::code::SetSampleRate);
#ifdef SERIAL_REPORTING
		if ( WaitForAck() )
			Serial.println("Ack'd");
		else
			Serial.println("Not ack'd");
#else
		WaitForAck();
		delay(FAKE_SERIAL_DELAY);
#endif

#ifdef SERIAL_REPORTING
		sendByte(SampleRate);
		if ( WaitForAck() )
			Serial.println("Ack'd");
		else
			Serial.println("Not ack'd");
#else
		sendByte(SampleRate);
		WaitForAck();
		delay(FAKE_SERIAL_DELAY);
#endif

#ifdef SERIAL_REPORTING
		Serial.println("\nenabling data reporting");
#else
		delay(FAKE_SERIAL_DELAY);
#endif
		errorCode = 0;
		clear();
		sendByte(PS2Mouse::code::EnableDataReporting);
#ifdef SERIAL_REPORTING
		if ( WaitForAck() )
			Serial.println("Ack'd");
		else
			Serial.println("Not ack'd");
#else
		WaitForAck();
#endif
	}

	uint8_t available() const
	{
		return buffer.size();
	}

	uint8_t peek() const
	{
		return buffer.peek_front();
	}
	
	uint8_t read()
	{
		return buffer.pop_front();
	}

	bool WaitForAck(uint16_t timeout_ms = 500)
	{
		auto loppu = millis() + timeout_ms;

		while ( millis() < loppu )
		{
			if ( buffer.size() > 0 )
				return (buffer.pop_front() == code::Acknowledge);
		}

		return false;
	}

	void prepareToSend(uint8_t dataByte)
	{
		EIMSK &= ~1; // disable clock line interrupt
		theByte = dataByte; // the byte to be sent
		checksum = 1;
		sendSuccess = false;
		receiving = false;
		sending = true; // redundant?
	}

	bool sendByte()
	{
		if ( !sending )
			return false;
		
#ifdef SERIAL_REPORTING
		uint8_t dataByte = theByte;
#endif

		PORTD &= ~clockPinMask;    // pull clock low
		DDRD |= clockPinMask;
		//_delay_us(105);            // keep clock low for at least 100 us
		delayMicroseconds(105);
		nextBit = -1;
		DDRD |= dataPinMask;
		PORTD &= ~dataPinMask;     // data low. This is the start bit?
		                           // it is not!
		if ( pullUp )
			PORTD |= clockPinMask; // re-enable internal pull up
		DDRD &= ~clockPinMask;     // 3) release clock
		EIMSK |= 1;

		auto timeOutMillis = millis() + 18;
#ifdef SERIAL_REPORTING
		Serial.print("Sending byte: ");
		Serial.println(dataByte, 16);
#endif
		
		while ( sending )          // wait for the ISR handle the rest...
		{
			if ( millis() > timeOutMillis )
			{
				EIMSK &= ~1; // disable clock line interrupt
				PORTD &= ~clockPinMask;    // pull clock low
				DDRD |= clockPinMask;

				PORTD |= dataPinMask; // Release the Data line (high)
				DDRD &= ~dataPinMask;
				if ( !pullUp )
					PORTD &= ~dataPinMask; // re-disable internal pull up
				
				receiving = true;
				sending = false;
				clear(false);
#ifdef SERIAL_REPORTING
				Serial.println("Send timeout");
#endif
				break;
			}
		}

#ifdef SERIAL_REPORTING
		if ( sendSuccess )
			Serial.print("Sent byte: ");
		else
			Serial.print("Failed to send byte: ");
		Serial.println(dataByte, 16);
#endif

		if ( pullUp )
			PORTD |= clockPinMask; // re-enable internal pull up
		DDRD &= ~clockPinMask;     // release clock
		EIMSK |= 1;

		return sendSuccess;
	}

	bool sendByte(uint8_t n)
	{
		prepareToSend(n);
		return sendByte();
	}

	void interrupt_handler()
	{
		if ( receiving )
		{
			uint8_t readBit = ((PIND & dataPinMask) != 0);

			if ( nextBit < 0 ) // uusi tavu alkaa
			{
				if ( readBit == 0 )
				{
					nextBit = 0;
				}
				else // Väärä alkubitti
				{
					//clear();
					//prepareToSend(code::Resend);
					errorCode |= 0b10000000 | (0x0f & nextBit);
					nextBit = 0;
				}
			}
			else if ( nextBit < 8 ) // databitti
			{
				checksum ^= readBit;
				theByte |= (readBit << nextBit);
				++nextBit;
			}
			else if ( nextBit == 8 ) // parity bit
			{
				if ( checksum != readBit ) // checksum väärin
				{
					//clear();
					//prepareToSend(code::Resend);
					errorCode |= 0b10000000 | (0x0f & nextBit);
					++nextBit;
				}
				else // oll korrect
				{
					++nextBit;
				}
			}
			else if ( nextBit == 9 ) // stop bit oltava 1
			{
				if ( readBit != 1 ) // Väärä stop bit
				{
					//clear();
					//prepareToSend(code::Resend);
					errorCode |= 0b10000000 | (0x0f & nextBit);
				}
				// else // tavun luku onnistui
				// kuvitellaan toistaiseksi ettei väärä stop bit haittaa vaikka olisikin väärin
				{
					buffer.push(theByte);
					clear(false);
				}
			}
		}
		else // sending
		{
			//  4) Wait for the device to bring the Clock line low.
			//  5) Set/reset the Data line to send the first data bit
			//  6) Wait for the device to bring Clock high.
			//  7) Wait for the device to bring Clock low.
			//  8) Repeat steps 5-7 for the other seven data bits and the parity bit
			//  9) Release the Data line.
			// 10) Wait for the device to bring Data low.
			// 11) Wait for the device to bring Clock  low.
			// 12) Wait for the device to release Data and Clock

			if ( nextBit == -1 )
			{
				nextBit = 0;
			}
			else if ( nextBit < 8 ) // the data bits
			{
				uint8_t sendBit = 1 & (theByte >> nextBit);
				checksum ^= sendBit;
				sendBit *= dataPinMask;
				PORTD = (PORTD & ~dataPinMask) | sendBit;
				++nextBit;
			}
			else if ( nextBit == 8 ) // parity bit
			{
				uint8_t sendBit = checksum * dataPinMask;
				PORTD = (PORTD & ~dataPinMask) | sendBit;
				++nextBit;
			}
			else if ( nextBit == 9 ) // stop bit
			{
				PORTD |= dataPinMask; // 9) Release the Data line (high)
				DDRD &= ~dataPinMask;
				++nextBit;
			}
			else if ( nextBit == 10 ) // ack bit from device
			{
				uint8_t readBit = ((PIND & dataPinMask) != 0);

				if ( !pullUp )
					PORTD &= ~dataPinMask; // re-disable pull-up if need be

				if ( readBit == 0) // oll korrekt. Tavun lähetys onnistui
				{
					sendSuccess = true;
				}
				else
				{
					// virhe! Minkäs teet?
					sendSuccess = false;
					errorCode |= (0b01000000 | (0x0f & nextBit));
					++nextBit;
				}

				sending = false;
				receiving = true;
				clear(false);
			}
		}
	}
};
