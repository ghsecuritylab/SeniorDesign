/*
 * song_title.c
 *
 * Created: 12/27/2017 1:00:25 AM
 *  Author: Daniel Gonzalez
 */ 

#include "asf.h"
#include "LCDLib.h"
#include "title_img.h"
#include "song_title.h"
#include "keyboard_coordinates.h"

#define CHAR_WIDTH 10
#define TITLE_Y 86
#define TITLE_START_X 85
#define CAPS_X 4 
#define CAPS_Y 305
#define KEY_BACKGROUND_COLOR 0x5ACB

void titleMenu(char *str)
{
	gfx_draw_bitmap(&title_img, (gfx_get_width() - title_img.width) / 2, gfx_get_height() - title_img.height);
	
	touch_t touched_point;
	uint32_t str_len = 0; 
    uint32_t cursor = TITLE_START_X; 
	bool case_option = UPPER_CASE; 
	char keyPressed[2] = " ";
	char * tempStr = str; 
	char caps_stg[] = "CAPS"; 
	
	gfx_draw_string_aligned((const char *)str,
		cursor, TITLE_Y, &sysfont,
		GFX_COLOR_TRANSPARENT, GFX_COLOR_WHITE,
		TEXT_POS_LEFT, TEXT_ALIGN_LEFT);
	
	while(*(str++)) 
	{
		str_len++;
		cursor += CHAR_WIDTH; 
	}
	str = tempStr; 
	
	// need to add bounds check and key check 
	while(keyPressed[0] != RETURN)
	{
		if (case_option == UPPER_CASE)
		{
			gfx_draw_string_aligned((const char *)caps_stg,
				CAPS_X, CAPS_Y, &sysfont,
				GFX_COLOR_TRANSPARENT, GFX_COLOR_WHITE,
				TEXT_POS_LEFT, TEXT_ALIGN_LEFT);
		}
		else
		{
			gfx_draw_string_aligned((const char *)caps_stg,
				CAPS_X, CAPS_Y, &sysfont,
				GFX_COLOR_TRANSPARENT, KEY_BACKGROUND_COLOR,
				TEXT_POS_LEFT, TEXT_ALIGN_LEFT);
		}
		
		if (lcd_touched())
		{
			touch_handler(&touched_point);
			get_key(touched_point.x, touched_point.y, case_option, &keyPressed[0]);
			
			if (keyPressed[0] == RETURN)
			{
				continue;
			}
			else if (keyPressed[0] == SHIFT)
			{
				case_option = !case_option;
			}
			else if (keyPressed[0] == BACKSPACE)
			{
				if (str_len > 0)
				{
					cursor -= CHAR_WIDTH; 
					str_len--; 
					gfx_draw_string_aligned((const char *)&str[str_len],
						cursor, TITLE_Y, &sysfont,
						GFX_COLOR_TRANSPARENT, GFX_COLOR_BLACK,
						TEXT_POS_LEFT, TEXT_ALIGN_LEFT); 
					str[str_len] = 0; 
				}
				if (str_len == 0)
					case_option = UPPER_CASE;
			}
			else 
			{
				if (str_len <= MAX_TITLE_SIZE)
				{
					gfx_draw_string_aligned((const char *)&keyPressed[0],
					cursor, TITLE_Y, &sysfont,
					GFX_COLOR_TRANSPARENT, GFX_COLOR_WHITE,
					TEXT_POS_LEFT, TEXT_ALIGN_LEFT);
					cursor += CHAR_WIDTH;
					str[str_len++] = keyPressed[0];
					if (case_option == UPPER_CASE)
					case_option = LOWER_CASE;
				}
			}
		}
	}
}

