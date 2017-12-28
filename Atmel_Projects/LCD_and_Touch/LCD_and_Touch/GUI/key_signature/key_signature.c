/*
 * key_signature.c
 *
 * Created: 12/27/2017 6:41:07 PM
 *  Author: Daniel Gonzalez
 */ 

#include "asf.h"
#include "key_signature.h"
#include "LCDLib.h"
#include "key_signatures_img.h"
#include "fastmath.h"

#define NUMBER_OF_KEY_SIGNATURES 30

typedef struct key_signature_coord
{
	int16_t x;
	int16_t y;
} key_signature_coord_t;

static key_signature_coord_t key_signature_coordinates[] =
{
	// TODO 
	{146, 128}, 
	{345, 128}, 
	{146, 222}, 
	{345, 222}	
};

static key_signature_t key_signatures[] =
{
	{C_MAJOR,  MAJOR},
	{C_MINOR,  MINOR},
	{Cs_MAJOR, MAJOR},
	{Cs_MINOR, MINOR},
	{Db_MAJOR, MAJOR},
	{D_MAJOR,  MAJOR},
	{D_MINOR,  MINOR},
	{Ds_MINOR, MINOR},
	{Eb_MAJOR, MAJOR},
	{Eb_MINOR, MINOR},
	{E_MAJOR,  MAJOR},
	{E_MINOR,  MINOR},
	{F_MAJOR,  MAJOR},
	{F_MINOR,  MINOR},
	{Fs_MAJOR, MAJOR},
	{Fs_MINOR, MINOR},
	{Gb_MAJOR, MAJOR},
	{G_MAJOR,  MAJOR},
	{G_MINOR,  MINOR},
	{Gs_MINOR, MINOR},
	{Ab_MAJOR, MAJOR},
	{Ab_MINOR, MINOR},
	{A_MAJOR,  MAJOR},
	{A_MINOR,  MINOR},
	{As_MINOR, MINOR},
	{Bb_MAJOR, MAJOR},
	{Bb_MINOR, MINOR},
	{B_MAJOR,  MAJOR},
	{B_MINOR,  MINOR},
	{Cb_MAJOR, MAJOR}
};

static key_signature_t get_button_pressed(int16_t x, int16_t y)
{
	uint32_t min = 0xFFFF;
	int32_t x_diff;
	int32_t y_diff;
	uint32_t dist;
	key_signature_t keySignaturePressed = key_signatures[0];
	
	for (int i = 0; i < NUMBER_OF_KEY_SIGNATURES; i++)
	{
		x_diff = x - key_signature_coordinates[i].x;
		y_diff = y - key_signature_coordinates[i].y;
		dist = sqrt(x_diff*x_diff + y_diff*y_diff);
		if (dist < min)
		{
			min = dist;
			keySignaturePressed = key_signatures[i];
		}
	}
	return keySignaturePressed;
}


void key_signature_menu(key_signature_t *key_sig)
{
	touch_t touched_point;

	gfx_draw_bitmap(&key_signature_img, (gfx_get_width() - key_signature_img.width) / 2, gfx_get_height() - key_signature_img.height);
	
	/* Wait for touch */
	while (lcd_touched() == false);
	touch_handler(&touched_point);
	
	*key_sig = get_button_pressed(touched_point.x, touched_point.y);
	
}
