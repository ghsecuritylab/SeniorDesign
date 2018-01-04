#include <asf.h>
#include "DanLib.h"
#include <math.h>
#include "arm_math.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define PITCH_BUF_SIZE 2048
#define LCD_DELAY 50

extern volatile bool outOfTime; 
static volatile midi_note_t oldNote = {0, 0};
static volatile midi_note_t note = {-1,0};
static volatile int lcd_refresh = 0;
int main(void)
{
	sysclk_init();
	board_init();
	lcd_init(); 
	Yin_init(PITCH_BUF_SIZE, YIN_DEFAULT_THRESHOLD);
	audio_init();

	char str[20]; 

	//start_gatorscribe();
	int16_t *audio; 
	while(1)
	{
		lcd_refresh++;
		if (lcd_refresh == LCD_DELAY)
		{
			if (oldNote.note_number != note.note_number)
			{
				get_midi_note_name(str, oldNote.note_number); 
				gfx_draw_string_aligned((const char *)str,
				150, 150, &sysfont,
				GFX_COLOR_TRANSPARENT, GFX_COLOR_BLACK,
				TEXT_POS_LEFT, TEXT_ALIGN_LEFT);
				
				get_midi_note_name(str, note.note_number);
				gfx_draw_string_aligned((const char *)str,
				150, 150, &sysfont,
				GFX_COLOR_TRANSPARENT, GFX_COLOR_WHITE,
				TEXT_POS_LEFT, TEXT_ALIGN_LEFT);
				oldNote.note_number = note.note_number;
			}
			lcd_refresh = 0;
		}
		if (dataReceived)
		{
			
			get_midi_note((int16_t *)processBuffer, (midi_note_t *)&note); 
			
			dataReceived = false; 
		}
		
	}
}

