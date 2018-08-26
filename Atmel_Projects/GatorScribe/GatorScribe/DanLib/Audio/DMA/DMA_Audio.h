/*
 * DMA_Audio.h
 *
 * Created: 12/8/2017 7:39:46 PM
 *  Author: Daniel Gonzalez
 */


#ifndef DMA_AUDIO_H_
#define DMA_AUDIO_H_

#include <math.h>

/********************************** Defines Start **********************************/
/** XDMA channels used */
#define XDMA_CH_SSC_RX    0
#define XDMA_CH_SSC_TX    1

#define SAMPLE_RATE 47250.0f //45750.0f

/** Micro-block w-length for single transfer  */
#define IO_BUF_SIZE          1024	// x in total, x/2 left & x/2 right 
#define IO_BUF_SIZE_PER_CHANNEL (IO_BUF_SIZE >> 1) 

#define WIN_SIZE IO_BUF_SIZE_PER_CHANNEL 
#define WIN_SIZE_D2 (WIN_SIZE>>1)
/********************************** Defines End **********************************/

/********************************** Externs Start **********************************/
extern volatile bool dataReceived;
extern float processBuffer[IO_BUF_SIZE_PER_CHANNEL];
extern volatile uint16_t *sound_out; 
/********************************** Externs End **********************************/

/********************************** Public Functions Start **********************************/
void configure_xdma(void);
/********************************** Public Functions End **********************************/

#endif /* DMA_AUDIO_H_ */

