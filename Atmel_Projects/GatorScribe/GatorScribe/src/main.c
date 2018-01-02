#include <asf.h>
#include "DanLib.h"
#include <math.h>
#include "arm_math.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define PITCH_BUF_SIZE 1600
#define LCD_DELAY 50

extern volatile bool outOfTime; 
static volatile float32_t oldPitch = 0.0;
static volatile float32_t pitch = -1;
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
			if ((int)oldPitch != (int)pitch)
			{
				sprintf(str, " %.2f", oldPitch);
				gfx_draw_string_aligned((const char *)str,
				150, 150, &sysfont,
				GFX_COLOR_TRANSPARENT, GFX_COLOR_BLACK,
				TEXT_POS_LEFT, TEXT_ALIGN_LEFT);
				
				sprintf(str, " %.2f", pitch);
				gfx_draw_string_aligned((const char *)str,
				150, 150, &sysfont,
				GFX_COLOR_TRANSPARENT, GFX_COLOR_WHITE,
				TEXT_POS_LEFT, TEXT_ALIGN_LEFT);
				oldPitch = pitch;
			}
			lcd_refresh = 0;
		}
		if (dataReceived)
		{
			

			pitch = Yin_getPitch((int16_t *)processBuffer);
			
			
			dataReceived = false; 
		}
		
	}
}

