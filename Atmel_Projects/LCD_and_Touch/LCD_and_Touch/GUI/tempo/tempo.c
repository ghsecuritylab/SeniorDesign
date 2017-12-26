/*
 * tempo.c
 *
 * Created: 12/26/2017 3:18:09 AM
 *  Author: Daniel Gonzalez
 */ 

#include "asf.h"
#include "tempo.h"
#include "LCDLib.h"
#include "tapTempoImg.h"

#define TEXT_X 195
#define TEXT_Y 173
#define BPM_TEXT_X (TEXT_X + 25)
#define INDICATOR_TEXT_X 282
#define INDICATOR_TEXT_Y (TEXT_Y + 8)
#define INDICATOR_DURATION 100

#define BPM_BUF_SIZE 4
#define BPM_MASK 3

/* Initial BPM */ 
static int bpm[BPM_BUF_SIZE]; 
static float weights[BPM_BUF_SIZE] = {0.5, 0.25, 0.15, 0.10};

/* Timing counters */ 
static volatile int msCount;
static volatile int led_indicator_freq;
static volatile int led_indicator_duration;

/* BPM string */ 
static char count_str[9] = {'1','0','0',0};
static char bpm_str[] = {' ','B','P','M',0};

/* Systick Interrupt Handler */ 
void SysTick_Handler(void)
{
	msCount++;
	led_indicator_freq--;
	led_indicator_duration++;
}

int tempoMenu(int initial_bpm) 
{
	int bpmIdx = 0;
	int averageBpm = 0;
	touch_t touched_point;
	
	/* Initial bpm values */ 
	for (int i = 0; i < BPM_BUF_SIZE; i++)
	{
		bpm[i] = initial_bpm; 
	}
	
	/* Initial timer counters */ 
	int current_ms_count =  60000 / initial_bpm; 
	led_indicator_freq = current_ms_count; 
	led_indicator_duration = 0; 
	msCount = 0; 

	/* Draw tempo menu */ 
	gfx_draw_bitmap(&tapTempoImg, (gfx_get_width() - tapTempoImg.width) / 2, gfx_get_height() - tapTempoImg.height);

	/* Print initial BPM value */
	gfx_draw_string_aligned((const char *)&count_str[0],
	TEXT_X, TEXT_Y, &sysfont,
	GFX_COLOR_TRANSPARENT, GFX_COLOR_WHITE,
	TEXT_POS_LEFT, TEXT_ALIGN_LEFT);

	/* Print "BPM" string */ 
	gfx_draw_string_aligned((const char *)&bpm_str[0],
	BPM_TEXT_X, TEXT_Y, &sysfont,
	GFX_COLOR_TRANSPARENT, GFX_COLOR_WHITE,
	TEXT_POS_LEFT, TEXT_ALIGN_LEFT);

	/* Initialize and start 1ms systick timer */ 
	SysTick_Config(sysclk_get_cpu_hz()/1000);
	
	/* Wait for first touch */ 
	while (lcd_touched() == false)
	{
		/* Handle initial flashing of BPM */ 
		if (led_indicator_freq == 0)
		{
			gfx_draw_filled_circle(INDICATOR_TEXT_X, INDICATOR_TEXT_Y, 8, GFX_COLOR_GREEN, GFX_WHOLE);
			led_indicator_freq = current_ms_count;
			led_indicator_duration = 0;
		}
	
		if(led_indicator_duration >= INDICATOR_DURATION)
		{
			gfx_draw_filled_circle(INDICATOR_TEXT_X, INDICATOR_TEXT_Y, 8, GFX_COLOR_BLACK, GFX_WHOLE);
			led_indicator_duration = 0;
		}
	}
	touch_handler(&touched_point);
	
	/* Reset timer counters */ 
	led_indicator_duration = 0;
	led_indicator_freq = current_ms_count;
	msCount = 0;
	
	while(1)
	{
		/* wait for next touch */
		while (lcd_touched() == false)
		{
			/* Flashing of current BPM */ 
			if (led_indicator_freq == 0)
			{
				gfx_draw_filled_circle(INDICATOR_TEXT_X, INDICATOR_TEXT_Y, 8, GFX_COLOR_GREEN, GFX_WHOLE);
				led_indicator_freq = current_ms_count;
				led_indicator_duration = 0;
			}
		
			if(led_indicator_duration >= INDICATOR_DURATION)
			{
				gfx_draw_filled_circle(INDICATOR_TEXT_X, INDICATOR_TEXT_Y, 8, GFX_COLOR_BLACK, GFX_WHOLE);
				led_indicator_duration = 0;
			}
		}
	
		/* Calculate and store bpm value, then reset */ 
		bpm[bpmIdx & BPM_MASK] = 60000 / msCount;
		current_ms_count = msCount;
		msCount = 0;
	
		/* Draw BPM light indicator */
		gfx_draw_filled_circle(INDICATOR_TEXT_X, INDICATOR_TEXT_Y, 8, GFX_COLOR_GREEN, GFX_WHOLE);
		led_indicator_duration = 0;

		/* Compute average BPM */
		averageBpm = bpm[bpmIdx & BPM_MASK]*weights[0];
		for(int i = 1; i < BPM_BUF_SIZE; i++)
		{
			averageBpm += bpm[(bpmIdx+i) & BPM_MASK]*weights[i];
		}
		bpmIdx++;
		
		/* Cap BPM */ 
		if (averageBpm > 300)
			averageBpm = 300; 
	
		/* Update frequency of indicator with average */
		led_indicator_freq = 60000 / averageBpm;
	
		/* Erase old BPM */
		gfx_draw_string_aligned((const char *)&count_str[0],
		TEXT_X, TEXT_Y, &sysfont,
		GFX_COLOR_TRANSPARENT, GFX_COLOR_BLACK,
		TEXT_POS_LEFT, TEXT_ALIGN_LEFT);

		/* Create new BPM value string */ 
		if (averageBpm < 100)
		{
			count_str[0] = ' ';
			count_str[1] =  0x30 + averageBpm / 10;
			count_str[2] = 0x30 + averageBpm % 10;
		}
		else
		{
			count_str[0] = 0x30 + averageBpm / 100;
			averageBpm = averageBpm % 100;
			count_str[1] = 0x30 + averageBpm / 10;
			count_str[2] = 0x30 + averageBpm % 10;
		}
	
		/* Print new BPM */
		gfx_draw_string_aligned((const char *)&count_str[0],
		TEXT_X, TEXT_Y, &sysfont,
		GFX_COLOR_TRANSPARENT, GFX_COLOR_WHITE,
		TEXT_POS_LEFT, TEXT_ALIGN_LEFT);

		/* Handle touch */ 
		touch_handler(&touched_point);
	}
	
	/* Disable Systick */ 
	
	return averageBpm; 
}