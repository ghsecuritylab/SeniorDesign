#include "asf.h"
#include "LCDLib.h"
#include "keyboard.h"
#include "instruments.h"
#include "keyboard_coordinates.h"
#include "tapTempo.h"
#include "board.h"


#define BACK_TEMPO_COLOR 0x5ACB
#define BPM_BUF_SIZE 4
#define BPM_MASK 3
volatile int bpm[BPM_BUF_SIZE] = {0, 0, 0, 0};
float weights[4] = {0.5, 0.25, 0.15, 0.10}; 

volatile int msCount = 0;
volatile char countStg[4] = " ";
volatile int led_indicator_freq = 0; 
volatile int current_ms_count = 0;
volatile int led_indicator_duration = 0; 


void SysTick_Handler()
{
	msCount++; 
	led_indicator_freq--; 
	led_indicator_duration++; 
}

int main(void)
{
	board_init();
	sysclk_init();
	lcd_init(); 
	
	msCount = sysclk_get_cpu_hz(); 
	SysTick_Config(sysclk_get_cpu_hz()/1000); 
	

	/* Draw the keyboard at the bottom of the screen */
	//gfx_draw_bitmap(&instruments, (gfx_get_width() - instruments.width) / 2, gfx_get_height() - instruments.height);
	//gfx_draw_bitmap(&keyboard, (gfx_get_width() - keyboard.width) / 2, gfx_get_height() - keyboard.height);
	gfx_draw_bitmap(&tapTempo, (gfx_get_width() - tapTempo.width) / 2, gfx_get_height() - tapTempo.height);
	touch_t touched_point;

	int bpmIdx = 0; 
	int averageBpm = 0; 
	bool secondTouchOccurred = 0; 
	bool firstReading = 1; 
	while(1)
	{
		if (lcd_touched())
		{
			msCount = 0; 
			/* Handle touch */
			touch_handler(&touched_point);
							
			while(1)
			{
				/* wait for next touch */ 
				while (lcd_touched() == false)
				{
					if(secondTouchOccurred == true){
						if (led_indicator_freq == 0)
						{
							gfx_draw_filled_circle(330, 184, 8, GFX_COLOR_GREEN, GFX_WHOLE);
							led_indicator_freq = current_ms_count;
							led_indicator_duration = 0;
						}
						
						if(led_indicator_duration >= 100)
						{
							gfx_draw_filled_circle(330, 184, 8, BACK_TEMPO_COLOR, GFX_WHOLE);
							led_indicator_duration = 0;
						}
					}
				}

				bpm[bpmIdx & BPM_MASK] = 60000 / msCount;
				
				if(firstReading)
				{
					led_indicator_duration = 0; 
					firstReading = false; 
					secondTouchOccurred = true; 
				}
				
				current_ms_count = msCount; 
				msCount = 0; 
				
				gfx_draw_filled_circle(330, 184, 8, GFX_COLOR_GREEN, GFX_WHOLE);
				led_indicator_duration = 0;
				
				averageBpm = bpm[bpmIdx & BPM_MASK]*weights[0];
				for(int i = 1; i < BPM_BUF_SIZE; i++)
				{
					averageBpm += bpm[(bpmIdx+i) & BPM_MASK]*weights[i];
				}
				bpmIdx++; 
				
				led_indicator_freq = 60000 / averageBpm;
			
				/* Erase old BPM */ 
				gfx_draw_string_aligned((const char *)countStg,
					285, 177, &sysfont,
					GFX_COLOR_TRANSPARENT, 0x5ACB,
					TEXT_POS_LEFT, TEXT_ALIGN_LEFT);
		
				if (averageBpm < 100)
				{
					countStg[0] = 0x30 + averageBpm / 10;
					countStg[1] = 0x30 + averageBpm % 10; 
					countStg[2] = 0;				
				}
				else 
				{
					countStg[0] = 0x30 + averageBpm / 100;
					averageBpm = averageBpm % 100;
					countStg[1] = 0x30 + averageBpm / 10;
					countStg[2] = 0x30 + averageBpm % 10;
				}

				/* Print new BPM */ 
				gfx_draw_string_aligned((const char *)countStg,
					285, 177, &sysfont,
					GFX_COLOR_TRANSPARENT, GFX_COLOR_WHITE,
					TEXT_POS_LEFT, TEXT_ALIGN_LEFT);
			
				touch_handler(&touched_point);
			}
		}
		
		
	}

	char keyPressed[2] = " ";  
	uint16_t xCoord = 2; 
	uint32_t case_option = 0; 
	while (1) 
	{
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
