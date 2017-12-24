/*
 * keyboard_coordinates.h
 *
 * Created: 12/24/2017 6:12:09 AM
 *  Author: Daniel Gonzalez
 */ 


#ifndef KEYBOARD_COORDINATES_H_
#define KEYBOARD_COORDINATES_H_


typedef struct key_coord
{
	int16_t x;
	int16_t y;
} key_coord_t;

void get_key(int16_t x, int16_t y, uint32_t case_option, char *key_pressed); 






#endif /* KEYBOARD_COORDINATES_H_ */