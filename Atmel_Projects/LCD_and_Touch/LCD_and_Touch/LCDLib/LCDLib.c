/*
 * LCDLib.c
 *
 * Created: 12/23/2017 12:30:34 AM
 *  Author: Daniel Gonzalez
 */ 

#include "LCDLib.h"
#include "asf.h"

/*********************************** Public Functions Start ***********************************/
void lcd_init(void)
{
	gfx_init();
	mxt_init();
}
/*********************************** Public Functions End ***********************************/
