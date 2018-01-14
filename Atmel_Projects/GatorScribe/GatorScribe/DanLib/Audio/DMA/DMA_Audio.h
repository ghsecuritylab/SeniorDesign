/*
 * DMA_Audio.h
 *
 * Created: 12/8/2017 7:39:46 PM
 *  Author: Daniel Gonzalez
 */


#ifndef DMA_AUDIO_H_
#define DMA_AUDIO_H_

#include "arm_math.h"
/********************************** Defines Start **********************************/
/** XDMA channels used */
#define XDMA_CH_SSC_RX    0
#define XDMA_CH_SSC_TX    1

/** Micro-block wlength for single transfer  */
#define BUF_SIZE          128	// 128 in total, 64 left & 64 right 
#define BUF_SIZE_PER_CHANNEL (BUF_SIZE >> 1)

#define PROCESS_BUF_SIZE 16384	
#define PROCESS_BUF_SIZE_INCREMENT (BUF_SIZE_PER_CHANNEL >> 1)	// left channel, decimated by 2
/********************************** Defines End **********************************/

/********************************** Externs Start **********************************/
extern volatile uint16_t *inBuffer;
extern volatile uint16_t *outBuffer;
extern volatile bool dataReceived;
extern volatile float32_t *processBuffer;
extern volatile float32_t *fillBuffer; 
extern volatile bool processPingMode; 

extern volatile float32_t processPingBuffer[PROCESS_BUF_SIZE];
extern volatile float32_t processPongBuffer[PROCESS_BUF_SIZE];
/********************************** Externs End **********************************/

/********************************** Public Functions Start **********************************/
void configure_xdma(void);
/********************************** Public Functions End **********************************/

#endif /* DMA_AUDIO_H_ */

