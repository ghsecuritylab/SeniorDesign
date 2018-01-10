#include <asf.h>
#include "DanLib.h"

int main(void)
{
	sysclk_init();
	board_init();
	lcd_init(); 
	audio_init();
	
	/* Initial Gatorscribe params */ 
	uint32_t bpm = 100;
	midi_instrument_t playback_instrument = PIANO;
	time_signature_t time_signature = {4,4, FOUR_FOUR};
	key_signature_t key_signature = {C_MAJOR, MAJOR};
	char title[MAX_TITLE_SIZE] = " ";

	main_menu(&bpm, &playback_instrument, &time_signature, &key_signature, &title[0]);
	gfx_draw_filled_rect(0, 0, gfx_get_width(), gfx_get_height(), GFX_COLOR_BLACK);
	start_recording(bpm, playback_instrument, time_signature, key_signature, title); 
	while(1)
	{

		
	}
}

