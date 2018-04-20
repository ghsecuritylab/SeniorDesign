/*
 * song_title.h
 *
 * Created: 12/27/2017 1:00:12 AM
 *  Author: Daniel Gonzalez
 */ 


#ifndef SONG_TITLE_H_
#define SONG_TITLE_H_

enum CASE {
	LOWER_CASE = 0,
	UPPER_CASE = 1
};

#define MAX_TITLE_SIZE 30
void titleMenu(char *str); 


#endif /* SONG_TITLE_H_ */