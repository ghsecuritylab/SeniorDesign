/*
 * DMA_Audio.h
 *
 * Created: 12/8/2017 7:39:46 PM
 *  Author: Daniel Gonzalez
 */


#ifndef DMA_AUDIO_H_
#define DMA_AUDIO_H_

/********************************** Defines **********************************/
/** Wav feature. */
#define SAMPLE_RATE             (44100)



/** XDMA channel used in this example. */
#define XDMA_CH_SSC_RX    0
#define XDMA_CH_SSC_TX    1

/** Micro-block length for single transfer  */
#define MICROBLOCK_LEN          512
#define BUF_SIZE          512

/** Size of each buffer */
// #define BUF_SIZE (TOTAL_BUFFERS * MICROBLOCK_LEN * (BITS_BY_SLOT/8))

/********************************** Defines **********************************/


/********************************** Public Functions **********************************/
void configure_xdma(void);
/********************************** Public Functions **********************************/



#endif /* DMA_AUDIO_H_ */

