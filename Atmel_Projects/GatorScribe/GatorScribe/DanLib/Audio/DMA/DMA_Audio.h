/*
 * DMA_Audio.h
 *
 * Created: 12/8/2017 7:39:46 PM
 *  Author: Daniel Gonzalez
 */


#ifndef DMA_AUDIO_H_
#define DMA_AUDIO_H_

/********************************** Defines Start **********************************/
/** Sampling rate */
//#define SAMPLE_RATE             (44100)

/** XDMA channels used */
#define XDMA_CH_SSC_RX    0
#define XDMA_CH_SSC_TX    1

/** Micro-block length for single transfer  */
#define BUF_SIZE          1024

#define TOTAL_PROCESS_BUFFERS 8 
#define PROCESS_BUF_SIZE (BUF_SIZE >> 1)
/********************************** Defines End **********************************/

/********************************** Externs Start **********************************/
extern volatile uint16_t *inBuffer;
extern volatile uint16_t *outBuffer;
extern volatile bool dataReceived;
extern volatile int16_t *processBuffer;
/********************************** Externs End **********************************/


/********************************** Public Functions Start **********************************/
void configure_xdma(void);
/********************************** Public Functions End **********************************/



#endif /* DMA_AUDIO_H_ */

