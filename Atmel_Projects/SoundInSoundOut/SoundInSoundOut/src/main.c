/**
 * \SoundInSoundOut
 *
 * \section Purpose
 *
 * This example uses the Synchronous Serial Controller (SSC) of SAM devices
 * to output an audio stream through the on-board WM8904 CODEC.
 *
 * \section Requirements
 *
 *
 * \section Description
 * This program plays sound from PC via Line-In. The audio stream is
 * sent through the SSC interface connected to the on-board WM8904, enabling
 * the sound to be audible using a pair of headphones.
 *
 * The code can be roughly broken down as follows:
 * - Enable the clock.
 * - Initialize and configure the Codec.
 * - Configure and enable the SSC.
 * - Configure and enable the XDMA.
 *
 */

#include <asf.h>
#include <pio.h>
#include "WM8904_Driver.h"
#include "DMA_Audio.h" 

volatile int flag; 


int main(void)
{
	/* Initialize the SAM system. */
	sysclk_init();
	board_init();

	/* Initialize WM8904 TWI interface*/
	if (wm8904_twi_init() != TWIHS_SUCCESS) {
		printf("-E-\tWM8904 initialization failed.\r");
		while (1) {
			/* Capture error */
		}
	}

	/* Configure test LED0 */ 
	pmc_enable_periph_clk(ID_PIOA);
	pio_set_output(PIOA, PIO_PA23, LOW, DISABLE, ENABLE);
	pio_clear(PIOA, PIO_PA23);

	/* Configure CODEC */
	configure_codec();

	/* Configure SSC */
	configure_ssc();

	/* Configure XDMA */
	configure_xdma();

	/* Enable the DAC master clock */
	pmc_pck_set_prescaler(PMC_PCK_2, PMC_MCKR_PRES_CLK_1);
	pmc_pck_set_source(PMC_PCK_2, PMC_MCKR_CSS_SLOW_CLK);
	pmc_enable_pck(PMC_PCK_2);
	
	/* Start playing */
	ssc_enable_rx(SSC);
	xdmac_channel_enable(XDMAC, XDMA_CH_SSC_RX);
	delay_ms(300);
	ssc_enable_tx(SSC);
	xdmac_channel_enable(XDMAC, XDMA_CH_SSC_TX);
	
	flag = 1;

	while(1)
	{
		
		if(!flag)
		{
			pio_set(PIOA, PIO_PA23);
		}
	}
}
