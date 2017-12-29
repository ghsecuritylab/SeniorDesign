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
#define SAMPLE_RATE             (48000)

/** XDMA channels used */
#define XDMA_CH_SSC_RX    0
#define XDMA_CH_SSC_TX    1

/** Micro-block length for single transfer  */
#define BUF_SIZE          512
/********************************** Defines End **********************************/

/********************************** Externs Start **********************************/
extern volatile uint16_t *inBuffer;
extern volatile uint16_t *outBuffer;
extern volatile int32_t dataReceived;
/********************************** Externs End **********************************/


/********************************** Public Functions Start **********************************/
void configure_xdma(void);
/********************************** Public Functions End **********************************/



#endif /* DMA_AUDIO_H_ */

