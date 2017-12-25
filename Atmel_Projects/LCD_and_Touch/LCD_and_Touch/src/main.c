#include "asf.h"
#include "LCDLib.h"
#include "keyboard.h"
#include "keyboard_coordinates.h"

const char example_string[] = "BOOP!";

int main(void)
{
	board_init();
	sysclk_init();
	lcd_init(); 

	/* Draw the keyboard at the bottom of the screen */
	gfx_draw_bitmap(&keyboard, (gfx_get_width() - keyboard.width) / 2, gfx_get_height() - keyboard.height);
	
	touch_t touched_point;
	char keyPressed[2] = " ";  
	uint16_t xCoord = 2; 
	uint32_t case_option = 0; 
	while (1) {
		if (lcd_touched()) 
		{
			touch_handler(&touched_point);
			get_key(touched_point.x, touched_point.y, case_option, &keyPressed[0]); 
			gfx_draw_string_aligned((const char *)keyPressed,
				xCoord, 2, &sysfont,
				GFX_COLOR_TRANSPARENT, GFX_COLOR_RED,
				TEXT_POS_LEFT, TEXT_ALIGN_LEFT);
				
				xCoord += 9; 
		}
	}
}
