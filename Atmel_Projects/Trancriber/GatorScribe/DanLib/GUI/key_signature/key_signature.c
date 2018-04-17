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
	// First Row
	{58, 66},  // C Major
	{149, 66}, // C minor
	{240, 66}, // C# Major
	{331, 66}, // C# minor
	{422, 66}, // Db minor
	// Second Row
	{58, 111},  // D Major
	{149,111},  // D minor
	{240, 111}, // D# minor
	{331, 111}, // Eb Major
	{422, 111}, // Eb minor
	// Third Row
	{58, 156},  // E Major
	{149, 156}, // E minor
	{240, 156}, // F Major
	{331, 156}, // F minor
	{422, 156}, // F# Major
	// Fourth Row
	{58, 201},  // F# minor
	{149, 201}, // Gb Major
	{240, 201}, // G Major
	{331, 201}, // G minor
	{422, 201}, // G# minor
	// Fifth Row
	{58, 246},  // Ab Major
	{149, 246}, // Ab minor
	{240, 246}, // A Major
	{331, 246}, // A minor
	{422, 246}, // A# minor
	//Sixth Row
	{58, 291},  // Bb Major
	{149, 291}, // Bb minor
	{240, 291}, // B Major
	{331, 291}, // B minor
	{422, 291}, // Cb Major
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
