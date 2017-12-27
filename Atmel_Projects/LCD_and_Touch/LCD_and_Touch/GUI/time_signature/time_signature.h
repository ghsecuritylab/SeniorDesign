/*
 * time_signature.h
 *
 * Created: 12/27/2017 5:25:21 AM
 *  Author: Daniel Gonzalez
 */ 


#ifndef TIME_SIGNATURE_H_
#define TIME_SIGNATURE_H_

typedef enum time_signature_identifier
{
	FOUR_FOUR = 0,
	THREE_FOUR = 1,
	TWO_FOUR = 2, 
	SIX_EIGHT = 3 
}time_signature_identifier_t;

typedef struct time_signature 
{
	uint8_t top; 
	uint8_t bottom; 
	time_signature_identifier_t sig; 
} time_signature_t;

void time_signature_menu(time_signature_t *tsig); 


#endif /* TIME_SIGNATURE_H_ */