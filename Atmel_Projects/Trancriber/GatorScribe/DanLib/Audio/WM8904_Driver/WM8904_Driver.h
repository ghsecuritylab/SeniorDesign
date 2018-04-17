/*
 * WM8904_Driver.h
 *
 * Created: 12/8/2017 7:33:28 PM
 *  Author: Daniel Gonzalez
 */ 


#ifndef WM8904_DRIVER_H_
#define WM8904_DRIVER_H_

/********************************** Defines Start **********************************/
/** Wav slot per frame */
#define SLOT_BY_FRAME           (1)

/** Bits per slot */
#define BITS_BY_SLOT            (16)

#define SAMPLE_RATE_24k 3 
#define SAMPLE_RATE_32k 4 
#define SAMPLE_RATE_48k 5

#define SAMPLE_RATE SAMPLE_RATE_48k
/********************************** Defines End **********************************/

/********************************** Public Functions Start **********************************/
void configure_ssc(void); 
void configure_codec(void);
/********************************** Public Functions End **********************************/


#endif /* WM8904_DRIVER_H_ */