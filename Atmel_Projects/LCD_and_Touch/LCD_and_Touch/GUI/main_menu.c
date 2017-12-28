/*
 * main_menu.c
 *
 * Created: 12/26/2017 3:20:08 AM
 *  Author: Daniel Gonzalez
 */ 

#include "asf.h"
#include "main_menu.h"
#include "main_menu_img.h"
#include "playback_instrument.h"
#include "song_title.h"
#include "tempo.h"
#include "time_signature.h"
#include "key_signature.h"
#include "LCDLib.h"
#include "fastmath.h"
#include <string.h>

#define NUMBER_OF_MAIN_MENU_BUTTONS 6

#define TITLE_TEXT_Y 58
#define TITLE_TEXT_X 85
#define BPM_TEXT_X	134
#define BPM_TEXT_Y	242
#define INSTRUMENT_TEXT_X 341 
#define INSTRUMENT_TEXT_Y BPM_TEXT_Y
#define TIME_SIG_TEXT_X BPM_TEXT_X
#define TIME_SIG_TEXT_Y 151
#define KEY_SIG_TEXT_X INSTRUMENT_TEXT_X
#define KEY_SIG_TEXT_Y TIME_SIG_TEXT_Y

typedef struct button_coord
{
	int16_t x;
	int16_t y;
} button_coord_t;

static button_coord_t button_coordinates[] =
{
	{433, 25},  // Start
	{44, 65},   // Title
	{132, 250}, // Tempo
	{132, 158},	// Time
	{340, 156},	// Key 
	{336, 250}	// Instrument 
};

typedef enum button
{
	NONE = -1,
	START = 0,
	TITLE = 1,
	TEMPO = 2,
	TIME_SIGNATURE = 3, 
	KEY_SIGNATURE = 4, 
	PLAYBACK_INSTRUMENT = 5
} button_t;

static button_t get_button_pressed(int16_t x, int16_t y)
{
	uint32_t min = 100;
	int32_t x_diff;
	int32_t y_diff;
	uint32_t dist;
	button_t buttonPressed = NONE;
	
	for (int i = 0; i < NUMBER_OF_MAIN_MENU_BUTTONS; i++)
	{
		x_diff = x - button_coordinates[i].x;
		y_diff = y - button_coordinates[i].y;
		dist = sqrt(x_diff*x_diff + y_diff*y_diff);
		if (dist < min)
		{
			min = dist;
			buttonPressed = (button_t)i;
		}
	}
	
	return buttonPressed;
}

static void print_bpm(uint32_t bpm)
{
	char bpm_str[] = {0, 0, 0,' ','B','P','M',0};
	int tempBpm; 
	
	/* Create string */
	if (bpm < 100)
	{
		bpm_str[0] = ' ';
		bpm_str[1] =  0x30 + bpm / 10;
		bpm_str[2] = 0x30 + bpm % 10;
	}
	else
	{
		bpm_str[0] = 0x30 + bpm / 100;
		tempBpm = bpm % 100;
		bpm_str[1] = 0x30 + tempBpm / 10;
		bpm_str[2] = 0x30 + tempBpm % 10;
	}
	gfx_draw_string_aligned((const char *)&bpm_str[0],
		BPM_TEXT_X, BPM_TEXT_Y, &sysfont,
		GFX_COLOR_TRANSPARENT, GFX_COLOR_WHITE,
		TEXT_POS_CENTER_X, TEXT_ALIGN_LEFT);
}

static void print_title(char *stg)
{
	gfx_draw_string_aligned((const char *)stg,
	TITLE_TEXT_X, TITLE_TEXT_Y, &sysfont,
	GFX_COLOR_TRANSPARENT, GFX_COLOR_WHITE,
	TEXT_POS_LEFT, TEXT_ALIGN_LEFT);
}

static void print_instrument(uint8_t instrument)
{
	char instrument_string[20]; 
	
	switch (instrument)
	{
		case PIANO:   strcpy(instrument_string, "Piano");       break; 
		case GUITAR:  strcpy(instrument_string, "Guitar");      break; 
		case TRUMPET: strcpy(instrument_string, "Trumpet");     break; 
		case SYNTH:	  strcpy(instrument_string, "Synth Lead");  break; 
		case SAX:     strcpy(instrument_string, "Soprano Sax"); break; 
		case VIOLIN:  strcpy(instrument_string, "Violin");      break; 
		default: break; 
	}
		
	gfx_draw_string_aligned((const char *)instrument_string,
		INSTRUMENT_TEXT_X, INSTRUMENT_TEXT_Y, &sysfont,
		GFX_COLOR_TRANSPARENT, GFX_COLOR_WHITE,
		TEXT_POS_CENTER_X, TEXT_ALIGN_LEFT);
}

static void print_time_signature(time_signature_t * tsig)
{
	char time_str[5]; 
	
	if (tsig->sig == FOUR_FOUR)
		strcpy(time_str, "4/4"); 
	else if (tsig->sig == THREE_FOUR)
		strcpy(time_str, "3/4"); 
	else if (tsig->sig == TWO_FOUR)
		strcpy(time_str, "2/4");
	else 
		strcpy(time_str, "6/8");
		
	gfx_draw_string_aligned((const char *)time_str,
		TIME_SIG_TEXT_X, TIME_SIG_TEXT_Y, &sysfont,
		GFX_COLOR_TRANSPARENT, GFX_COLOR_WHITE,
		TEXT_POS_CENTER_X, TEXT_ALIGN_LEFT);
}

static void print_key_signature(key_signature_t * ksig)
{
	char key_str[10];
	
	switch (ksig->mode)
	{
		case MAJOR: 
			switch (ksig->key)
			{
				case C_MAJOR:  strcpy(key_str, "C Major");  break;	
				case G_MAJOR:  strcpy(key_str, "G Major");  break;
				case D_MAJOR:  strcpy(key_str, "D Major");  break;
				case A_MAJOR:  strcpy(key_str, "A Major");  break;
				case E_MAJOR:  strcpy(key_str, "E Major");  break;
				case B_MAJOR:  strcpy(key_str, "B Major");  break;
				case Fs_MAJOR: strcpy(key_str, "F# Major"); break;
				case Cs_MAJOR: strcpy(key_str, "C# Major"); break;
				case F_MAJOR:  strcpy(key_str, "F Major");  break;
				case Bb_MAJOR: strcpy(key_str, "Bb Major"); break;
				case Eb_MAJOR: strcpy(key_str, "Eb Major"); break;
				case Ab_MAJOR: strcpy(key_str, "Ab Major"); break;
				case Db_MAJOR: strcpy(key_str, "Db Major"); break;
				case Gb_MAJOR: strcpy(key_str, "Gb Major"); break;
				case Cb_MAJOR: strcpy(key_str, "Cb Major"); break;	
				default: break; 		
			}
			break; 
		case MINOR: 
			switch (ksig->key)
			{
				case A_MINOR:  strcpy(key_str, "A Minor");  break;
				case E_MINOR:  strcpy(key_str, "E Minor");  break;
				case B_MINOR:  strcpy(key_str, "B Minor");  break;
				case Fs_MINOR: strcpy(key_str, "F# Minor"); break;
				case Cs_MINOR: strcpy(key_str, "C# Minor"); break;
				case Gs_MINOR: strcpy(key_str, "G# Minor"); break;
				case Ds_MINOR: strcpy(key_str, "D# Minor"); break;
				case As_MINOR: strcpy(key_str, "A# Minor"); break;
				case D_MINOR:  strcpy(key_str, "D Minor");  break;
				case G_MINOR:  strcpy(key_str, "G Minor");  break;
				case C_MINOR:  strcpy(key_str, "C Minor");  break;
				case F_MINOR:  strcpy(key_str, "F Minor");  break;
				case Bb_MINOR: strcpy(key_str, "Bb Minor"); break;
				case Eb_MINOR: strcpy(key_str, "Eb Minor"); break;
				case Ab_MINOR: strcpy(key_str, "Ab Minor"); break;	
				default: break; 
			}
			break; 
		default: break;  
	}
	
	gfx_draw_string_aligned((const char *)key_str,
		KEY_SIG_TEXT_X, KEY_SIG_TEXT_Y, &sysfont,
		GFX_COLOR_TRANSPARENT, GFX_COLOR_WHITE,
		TEXT_POS_CENTER_X, TEXT_ALIGN_LEFT);
}

void start_gatorscribe(void)
{
	touch_t touched_point;
	button_t button_pressed;
	bool tappedNewMenu = true; 
	char title[MAX_TITLE_SIZE] = " "; 
	uint32_t bpm = 100;
	midi_instrument_t playback_instrument = PIANO; 
	time_signature_t time_signature = {4,4, FOUR_FOUR}; 
	key_signature_t key_signature = {C_MAJOR, MAJOR};

	while(1)
	{
		if (tappedNewMenu)
		{
			gfx_draw_bitmap(&main_menu_img, (gfx_get_width() - main_menu_img.width) / 2, gfx_get_height() - main_menu_img.height);
			print_bpm(bpm);
			print_title(title);
			print_instrument(playback_instrument); 
			print_time_signature(&time_signature); 
			print_key_signature(&key_signature);
			tappedNewMenu = false; 
		}

		/* Wait for touch */
		while (lcd_touched() == false);
		touch_handler(&touched_point);
		
		button_pressed = get_button_pressed(touched_point.x, touched_point.y); 
		
		if (button_pressed == TEMPO)
		{
			bpm = tempoMenu(bpm);
			tappedNewMenu = true; 
		}
		else if (button_pressed == TITLE)
		{
			titleMenu(&title[0]); 
			tappedNewMenu = true;
		}
		else if (button_pressed == PLAYBACK_INSTRUMENT)
		{
			playback_instrument = instrument_menu();
			tappedNewMenu = true;
		}
		else if (button_pressed == TIME_SIGNATURE)
		{
			time_signature_menu(&time_signature); 
			tappedNewMenu = true;
		}
		else if (button_pressed == KEY_SIGNATURE)
		{
			key_signature_menu(&key_signature);
			tappedNewMenu = true;
		}
	}
}