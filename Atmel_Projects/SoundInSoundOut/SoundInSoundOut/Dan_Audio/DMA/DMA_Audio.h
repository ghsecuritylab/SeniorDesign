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

/** Wav slot per frame */
#define SLOT_BY_FRAME           (1)

/** Bits per slot */
#define BITS_BY_SLOT            (16)

/** XDMA channel used in this example. */
#define XDMA_CH_SSC_RX    0
#define XDMA_CH_SSC_TX    1

/********************************** Defines **********************************/


/********************************** Public Functions **********************************/
void configure_xdma(void); 
/********************************** Public Functions **********************************/



#endif /* DMA_AUDIO_H_ */