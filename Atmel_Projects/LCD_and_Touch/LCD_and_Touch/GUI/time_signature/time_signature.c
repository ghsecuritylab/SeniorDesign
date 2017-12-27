/*
 * time_signature.c
 *
 * Created: 12/27/2017 5:25:06 AM
 *  Author: Daniel Gonzalez
 */ 


#include "asf.h"
#include "time_signature.h"
#include "LCDLib.h"
#include "time_signatures_img.h"
#include "fastmath.h"

typedef struct time_signature_coord
{
	int16_t x;
	int16_t y;
} time_signature_coord_t;

static time_signature_coord_t time_signature_coordinates[] =
{
	{146, 128}, // 4/4
	{345, 128}, // 3/4
	{146, 222}, // 2/4
	{345, 222}	// 6/8
};

static time_signature_t time_signatures[] = 
{
	{4,4, FOUR_FOUR},
	{3,4, THREE_FOUR},
	{2,4, TWO_FOUR}, 
	{6,8, SIX_EIGHT}
}; 

static time_signature_t get_button_pressed(int16_t x, int16_t y)
{
	uint32_t min = 0xFFFF;
	int32_t x_diff;
	int32_t y_diff;
	uint32_t dist;
	time_signature_t timeSignaturePressed = time_signatures[0];
	
	for (int i = 0; i < 4; i++)
	{
		x_diff = x - time_signature_coordinates[i].x;
		y_diff = y - time_signature_coordinates[i].y;
		dist = sqrt(x_diff*x_diff + y_diff*y_diff);
		if (dist < min)
		{
			min = dist;
			timeSignaturePressed = time_signatures[i];
		}
	}
	return timeSignaturePressed;
}

void time_signature_menu(time_signature_t *tsig)
{
	touch_t touched_point;

	gfx_draw_bitmap(&time_signature_img, (gfx_get_width() - time_signature_img.width) / 2, gfx_get_height() - time_signature_img.height);
	
	/* Wait for touch */
	while (lcd_touched() == false);
	touch_handler(&touched_point);
	
	*tsig = get_button_pressed(touched_point.x, touched_point.y);
}
