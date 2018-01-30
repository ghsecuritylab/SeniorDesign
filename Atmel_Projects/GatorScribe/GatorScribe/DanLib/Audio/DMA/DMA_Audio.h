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

/** Micro-block w-length for single transfer  */
#define BUF_SIZE          4096	// 4096 in total, 2048 left & 2048 right 
#define BUF_SIZE_PER_CHANNEL (BUF_SIZE >> 1) // 2048 

// 1024 
#define PROCESS_BUF_SIZE (BUF_SIZE_PER_CHANNEL >> 1) // input decimated by 2 -- every other sample 	
/********************************** Defines End **********************************/

/********************************** Externs Start **********************************/
extern volatile bool dataReceived;
extern volatile float32_t *processBuffer;
extern volatile uint16_t *outBuffer; 
/********************************** Externs End **********************************/

/********************************** Public Functions Start **********************************/
void configure_xdma(void);
/********************************** Public Functions End **********************************/

#endif /* DMA_AUDIO_H_ */

