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
#include "LCDLib.h"
#include "fastmath.h"
#include <string.h>

#define BPM_TEXT_X	98
#define BPM_TEXT_Y	242
#define TITLE_TEXT_Y 58
#define TITLE_TEXT_X 85
#define INSTRUMENT_TEXT_X 341 
#define INSTRUMENT_TEXT_Y 242
#define TIME_SIG_TEXT_X 134
#define TIME_SIG_TEXT_Y 151

typedef struct button_coord
{
	int16_t x;
	int16_t y;
} button_coord_t;

static button_coord_t button_coordinates[] =
{
	{433, 25}, // Start
	{44, 65}, // Title
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
	
	for (int i = 0; i < 6; i++)
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

static void print_bpm(int bpm)
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
		TEXT_POS_LEFT, TEXT_ALIGN_LEFT);
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
	
	if (instrument == PIANO)
		strcpy(instrument_string, "Piano"); 
	else if (instrument == GUITAR)
		strcpy(instrument_string, "Guitar"); 
	else if (instrument == TRUMPET)
		strcpy(instrument_string, "Trumpet"); 
	else if (instrument == SYNTH)
		strcpy(instrument_string, "Synth Lead"); 
	else if (instrument == SAX)
		strcpy(instrument_string, "Soprano Sax"); 
	else 
		strcpy(instrument_string, "Violin"); 
		
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


void start_g8rscribe(void)
{
	touch_t touched_point;
	button_t button_pressed;
	int bpm = 100; 
	bool tappedNewMenu = true; 
	char title[MAX_TITLE_SIZE] = " "; 
	midi_instrument_t playback_instrument = PIANO; 
	time_signature_t time_signature = {4,4, FOUR_FOUR}; 
	
	while(1)
	{
		if (tappedNewMenu)
		{
			gfx_draw_bitmap(&main_menu_img, (gfx_get_width() - main_menu_img.width) / 2, gfx_get_height() - main_menu_img.height);
			print_bpm(bpm);
			print_title(title);
			print_instrument(playback_instrument); 
			print_time_signature(&time_signature); 
			tappedNewMenu = false; 
		}

		/* Wait for first touch */
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
		
	}

	
}