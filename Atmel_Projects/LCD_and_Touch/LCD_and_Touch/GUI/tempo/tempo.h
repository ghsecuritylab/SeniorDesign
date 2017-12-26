/*
 * tempo.h
 *
 * Created: 12/26/2017 3:18:21 AM
 *  Author: Daniel Gonzalez
 */ 


#ifndef TEMPO_H_
#define TEMPO_H_

typedef struct button_coord
{
	int16_t x;
	int16_t y;
} button_coord_t;

int tempoMenu(int initial_bpm); 

#endif /* TEMPO_H_ */