/*
 * WM8904_Driver.h
 *
 * Created: 12/8/2017 7:33:28 PM
 *  Author: Daniel Gonzalez
 */ 


#ifndef WM8904_DRIVER_H_
#define WM8904_DRIVER_H_

/** Wav slot per frame */
#define SLOT_BY_FRAME           (1)

/** Bits per slot */
#define BITS_BY_SLOT            (16)

void configure_ssc(void); 
void configure_codec(void);

#endif /* WM8904_DRIVER_H_ */