/*
 * DMA_Audio.h
 *
 * Created: 12/8/2017 7:39:46 PM
 *  Author: Daniel Gonzalez
 */


#ifndef DMA_AUDIO_H_
#define DMA_AUDIO_H_

#include <math.h>

// #define AUTOTUNE

/********************************** Defines Start **********************************/
/** XDMA channels used */
#define XDMA_CH_SSC_RX    0
#define XDMA_CH_SSC_TX    1

/** Micro-block w-length for single transfer  */
#define IO_BUF_SIZE          4096	// 1024 in total, 512 left & 512 right 
#define IO_BUF_SIZE_PER_CHANNEL (IO_BUF_SIZE >> 1) 

#define NUM_OF_OVERLAPS 4
#define WIN_SIZE 1024 // ((IO_BUF_SIZE_PER_CHANNEL*NUM_OF_OVERLAPS) >> 1) // decimated by 2, length 1024
#define NEW_DATA_SIZE ((IO_BUF_SIZE >> 1)>>1)  // one channel, decimated 
/********************************** Defines End **********************************/

/********************************** Externs Start **********************************/
extern volatile bool dataReceived;
extern volatile float *processBuffer;
extern volatile uint16_t *outBuffer; 
/********************************** Externs End **********************************/

/********************************** Public Functions Start **********************************/
void configure_xdma(void);
/********************************** Public Functions End **********************************/

#endif /* DMA_AUDIO_H_ */

