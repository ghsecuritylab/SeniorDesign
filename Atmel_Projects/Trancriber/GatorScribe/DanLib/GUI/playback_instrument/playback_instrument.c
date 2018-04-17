/*
 * playback_instrument.c
 *
 * Created: 12/27/2017 3:37:01 AM
 *  Author: Daniel Gonzalez
 */ 

#include "asf.h"
#include "playback_instrument.h"
#include "LCDLib.h"
#include "instruments_img.h"
#include "fastmath.h"

#define NUMBER_OF_INSTRUMENTS 6

typedef struct instrument_coord
{
	int16_t x;
	int16_t y;
} instrument_coord_t;

static instrument_coord_t instrument_coordinates[] =
{
	{87, 134}, // Guitar
	{244, 125}, // Piano
	{396, 133}, // Violin
	{84, 258},	// Trumpet
	{241,254}, // Synth
	{390, 256} // Sax 
};

static midi_instrument_t midi_instruments[] = {GUITAR, PIANO, VIOLIN, TRUMPET, SYNTH, SAX}; 

static midi_instrument_t get_button_pressed(int16_t x, int16_t y)
{
	uint32_t min = 0xFFFF;
	int32_t x_diff;
	int32_t y_diff;
	uint32_t dist;
	midi_instrument_t instrumentPressed = midi_instruments[0];
	
	for (int i = 0; i < NUMBER_OF_INSTRUMENTS; i++)
	{
		x_diff = x - instrument_coordinates[i].x;
		y_diff = y - instrument_coordinates[i].y;
		dist = sqrt(x_diff*x_diff + y_diff*y_diff);
		if (dist < min)
		{
			min = dist;
			instrumentPressed = midi_instruments[i];
		}
	}
	return instrumentPressed;
}


midi_instrument_t instrument_menu(void)
{
	touch_t touched_point;
	
	gfx_draw_bitmap(&instruments_img, (gfx_get_width() - instruments_img.width) / 2, gfx_get_height() - instruments_img.height);
	
	/* Wait for touch */
	while (lcd_touched() == false);
	touch_handler(&touched_point);
			
	return get_button_pressed(touched_point.x, touched_point.y);
}