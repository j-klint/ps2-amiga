/*
void ListenFor(uint16_t ms)
{
	auto loppu = millis() + ms;
	do
	{
		if ( PS2.errorCode )
		{
			Serial.print("Error: ");
			Serial.println(PS2.errorCode, 16);
			PS2.errorCode = 0;
		}
		
		while ( PS2.available() > 0 )
		{
			Serial.print(PS2.read(), 16);
			Serial.print(" ");
		}
	} while ( millis() < loppu );

	Serial.println();
}



*/