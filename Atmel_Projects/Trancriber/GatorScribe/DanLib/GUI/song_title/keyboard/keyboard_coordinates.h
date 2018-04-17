/*
 * keyboard_coordinates.h
 *
 * Created: 12/24/2017 6:12:09 AM
 *  Author: Daniel Gonzalez
 */ 


#ifndef KEYBOARD_COORDINATES_H_
#define KEYBOARD_COORDINATES_H_

#define BACKSPACE   0x08
#define RETURN      0x0D
#define SHIFT       0x0F
#define SPACE       0x20

void get_key(int16_t x, int16_t y, uint32_t case_option, char *key_pressed); 






#endif /* KEYBOARD_COORDINATES_H_ */