/*
 * tempo.c
 *
 * Created: 12/26/2017 3:18:09 AM
 *  Author: Daniel Gonzalez
 */ 

#include "asf.h"
#include "tempo.h"
#include "LCDLib.h"
#include "tapTempo_img.h"
#include "fastmath.h"

#define DISABLE_SYSTICK() (SysTick->CTRL  = 0) 
#define NUMBER_OF_TEMPO_BUTTONS 4

#define TEXT_X 195
#define TEXT_Y 173
#define BPM_TEXT_X (TEXT_X + 25)
#define INDICATOR_TEXT_X 282
#define INDICATOR_TEXT_Y (TEXT_Y + 8)
#define INDICATOR_DURATION 100

#define BPM_BUF_SIZE 4
#define BPM_MASK 3

typedef enum button
{
	BACK = 0,
	TAP = 1,
	UP = 2,
	DOWN = 3
} button_t; 


typedef struct button_coord
{
	int16_t x;
	int16_t y;
} button_coord_t;

static button_coord_t button_coordinates[] = 
{
	{47, 19}, // Back 
	{73, 160}, // Tap 
	{389, 152}, // Up
	{389, 209}	// Down 
}; 


/* Initial BPM */ 
static int bpm[BPM_BUF_SIZE]; 
static float weights[BPM_BUF_SIZE] = {0.5, 0.25, 0.15, 0.10};

/* Timing counters */ 
static volatile int msCount;
static volatile int led_indicator_freq;
static volatile int led_indicator_duration;
	
static button_t get_button_pressed(int16_t x, int16_t y) 
{
	uint32_t min = 0xFFFF;
	int32_t x_diff;
	int32_t y_diff;
	uint32_t dist;
	button_t buttonPressed = 0; 
	
	for (int i = 0; i < NUMBER_OF_TEMPO_BUTTONS; i++)
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

/* Systick Interrupt Handler */ 
void SysTick_Handler(void)
{
	msCount++;
	led_indicator_freq--;
	led_indicator_duration++;
}

int tempoMenu(uint32_t initial_bpm) 
{
	uint32_t bpmIdx = 0;
	uint32_t averageBpm = initial_bpm;
	uint32_t tempBpm;	// used for printing 
	touch_t touched_point;
	button_t button_pressed;
	
	/* BPM string */
	char count_str[4] = " ";
	char bpm_str[] = {' ','B','P','M',0};
		
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
		tempBpm = averageBpm % 100;
		count_str[1] = 0x30 + tempBpm / 10;
		count_str[2] = 0x30 + tempBpm % 10;
	}
	
	/* Initial bpm values */ 
	for (int i = 0; i < BPM_BUF_SIZE; i++)
	{
		bpm[i] = initial_bpm; 
	}
	
	/* Initial timer counters */ 
	uint32_t current_ms_count =  60000 / initial_bpm; 
	led_indicator_freq = current_ms_count; 
	led_indicator_duration = 0; 
	msCount = 0; 

	/* Draw tempo menu */ 
	gfx_draw_bitmap(&tap_tempo_img, (gfx_get_width() - tap_tempo_img.width) / 2, gfx_get_height() - tap_tempo_img.height);

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
	button_pressed = get_button_pressed(touched_point.x, touched_point.y); 
	
	if (button_pressed == UP)
	{
		bpm[bpmIdx & BPM_MASK] = bpm[bpmIdx & BPM_MASK] + 1;
		current_ms_count++;
		bpmIdx++; 
	}
	else if (button_pressed == DOWN)
	{
		bpm[bpmIdx & BPM_MASK] = bpm[bpmIdx & BPM_MASK] - 1;
		current_ms_count--;
		bpmIdx++; 
	}
	else if (button_pressed == BACK)
	{
		DISABLE_SYSTICK();
		return initial_bpm;
	}
	
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
		
		/* Handle touch */
		touch_handler(&touched_point);
		button_pressed = get_button_pressed(touched_point.x, touched_point.y);

		if (button_pressed == UP)
		{
			averageBpm = averageBpm + 1;
			bpm[bpmIdx & BPM_MASK] = averageBpm; 
			current_ms_count = current_ms_count + 6;
		}
		else if (button_pressed == DOWN)
		{
			averageBpm = averageBpm - 1; 
			bpm[bpmIdx & BPM_MASK] = averageBpm;
			current_ms_count = current_ms_count - 6;
		}
		else if (button_pressed == TAP)
		{
			/* Calculate and store bpm value, then reset */
			bpm[bpmIdx & BPM_MASK] = 60000 / msCount;
			current_ms_count = msCount;
			msCount = 0;
			
			/* Compute average BPM */
			averageBpm = bpm[bpmIdx & BPM_MASK]*weights[0];
			for(int i = 1; i < BPM_BUF_SIZE; i++)
			{
				averageBpm += bpm[(bpmIdx+i) & BPM_MASK]*weights[i];
			}
		}
		else
		{
			break;	// BACK 
		}
		bpmIdx++;

		/* Draw BPM light indicator */
		gfx_draw_filled_circle(INDICATOR_TEXT_X, INDICATOR_TEXT_Y, 8, GFX_COLOR_GREEN, GFX_WHOLE);
		led_indicator_duration = 0;
		
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
			tempBpm = averageBpm % 100;
			count_str[1] = 0x30 + tempBpm / 10;
			count_str[2] = 0x30 + tempBpm % 10;
		}
	
		/* Print new BPM */
		gfx_draw_string_aligned((const char *)&count_str[0],
			TEXT_X, TEXT_Y, &sysfont,
			GFX_COLOR_TRANSPARENT, GFX_COLOR_WHITE,
			TEXT_POS_LEFT, TEXT_ALIGN_LEFT);
	}
	
	/* Disable Systick */ 
	DISABLE_SYSTICK(); 
	
	return averageBpm; 
}