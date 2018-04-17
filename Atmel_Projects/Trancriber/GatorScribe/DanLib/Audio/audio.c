/*
 * audio.c
 *
 * Created: 12/28/2017 6:56:04 PM
 *  Author: Daniel Gonzalez
 */ 

#include "asf.h"
#include "audio.h"
#include <pio.h>

#include "DMA_Audio.h"
#include "WM8904_Driver.h"

void audio_init(void)
{
	/* Initialize WM8904 TWI interface*/
	if (wm8904_twi_init() != TWIHS_SUCCESS) {
		printf("-E-\tWM8904 initialization failed.\r");
		while (1) {
			/* Capture error */
		}
	}
		
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
	// might want to enable in the first xdma interrupt (if you care)
	ssc_enable_tx(SSC);
	xdmac_channel_enable(XDMAC, XDMA_CH_SSC_TX);
}
