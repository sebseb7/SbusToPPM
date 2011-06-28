#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>

#include "eeprom.h"

//*****************************************************************************
// 
uint8_t ee_mode EEMEM = 1;

//*****************************************************************************
// 

//*****************************************************************************
// 
void ReadParameter (void)
{
	MODE = eeprom_read_byte(&ee_mode);
}

//*****************************************************************************
// 
void WriteParameter (void)
{
	eeprom_write_byte(&ee_mode, MODE);
}
