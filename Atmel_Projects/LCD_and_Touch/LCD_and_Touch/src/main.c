#include "asf.h"
#include "LCDLib.h"
#include "keyboard.h"

const char example_string[] = "BOOP!";

int main(void)
{
	board_init();
	sysclk_init();
	lcd_init(); 

	/* Draw the keyboard at the bottom of the screen */
	//gfx_draw_bitmap(&keyboard, (gfx_get_width() - keyboard.width) / 2, gfx_get_height() - keyboard.height);
	
	touch_t touched_point;
	while (1) {
		if (lcd_touched()) 
		{
			touch_handler(&touched_point);
			gfx_draw_string_aligned(example_string,
				touched_point.x, touched_point.y, &sysfont,
				GFX_COLOR_TRANSPARENT, GFX_COLOR_RED,
				TEXT_POS_CENTER_X, TEXT_ALIGN_LEFT);
		}
	}
}
