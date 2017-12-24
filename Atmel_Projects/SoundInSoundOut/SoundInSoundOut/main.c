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
#include "DanLib.h"

static int16_t sin_wave[240] = 
{
	0,           0,
	1714, 		 1714,
	3425, 		 3425,
	5125, 		 5125,
	6812, 		 6812,
	8480, 		 8480,
	10125, 		 10125,
	11742, 		 11742,
	13327, 		 13327,
	14876, 		 14876,
	16383, 		 16383,
	17846, 		 17846,
	19260, 		 19260,
	20621, 		 20621,
	21925, 		 21925,
	23170, 		 23170,
	24350, 		 24350,
	25465, 		 25465,
	26509, 		 26509,
	27481, 		 27481,
	28377, 		 28377,
	29196, 		 29196,
	29934, 		 29934,
	30591, 		 30591,
	31163, 		 31163,
	31650, 		 31650,
	32051, 		 32051,
	32364, 		 32364,
	32587, 		 32587,
	32722, 		 32722,
	32767, 		 32767,
	32722, 		 32722,
	32587, 		 32587,
	32364, 		 32364,
	32051, 		 32051,
	31650, 		 31650,
	31163, 		 31163,
	30591, 		 30591,
	29934, 		 29934,
	29196, 		 29196,
	28377, 		 28377,
	27481, 		 27481,
	26509, 		 26509,
	25465, 		 25465,
	24350, 		 24350,
	23170, 		 23170,
	21925, 		 21925,
	20621, 		 20621,
	19260, 		 19260,
	17846, 		 17846,
	16383, 		 16383,
	14876, 		 14876,
	13327, 		 13327,
	11742, 		 11742,
	10125, 		 10125,
	8480, 		 8480,
	6812, 		 6812,
	5125, 		 5125,
	3425, 		 3425,
	1714, 		 1714,
	0, 		 0,
	-1715, 		 -1715,
	-3426, 		 -3426,
	-5126, 		 -5126,
	-6813, 		 -6813,
	-8481, 		 -8481,
	-10126, 	 -10126,
	-11743, 	 -11743,
	-13328, 	 -13328,
	-14877, 	 -14877,
	-16384, 	 -16384,
	-17847, 	 -17847,
	-19261, 	 -19261,
	-20622, 	 -20622,
	-21926, 	 -21926,
	-23171, 	 -23171,
	-24351, 	 -24351,
	-25466, 	 -25466,
	-26510, 	 -26510,
	-27482, 	 -27482,
	-28378, 	 -28378,
	-29197, 	 -29197,
	-29935, 	 -29935,
	-30592, 	 -30592,
	-31164, 	 -31164,
	-31651, 	 -31651,
	-32052, 	 -32052,
	-32365, 	 -32365,
	-32588, 	 -32588,
	-32723, 	 -32723,
	-32768, 	 -32768,
	-32723, 	 -32723,
	-32588, 	 -32588,
	-32365, 	 -32365,
	-32052, 	 -32052,
	-31651, 	 -31651,
	-31164, 	 -31164,
	-30592, 	 -30592,
	-29935, 	 -29935,
	-29197, 	 -29197,
	-28378, 	 -28378,
	-27482, 	 -27482,
	-26510, 	 -26510,
	-25466, 	 -25466,
	-24351, 	 -24351,
	-23171, 	 -23171,
	-21926, 	 -21926,
	-20622, 	 -20622,
	-19261, 	 -19261,
	-17847, 	 -17847,
	-16384, 	 -16384,
	-14877, 	 -14877,
	-13328, 	 -13328,
	-11743, 	 -11743,
	-10126, 	 -10126,
	-8481, 		 -8481,
	-6813, 		 -6813,
	-5126, 		 -5126,
	-3426, 		 -3426,
	-1715, 		 -1715
};

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
	//delay_ms(300);
	// TODO: Might want to go back and wait for first transfer later 
    ssc_enable_tx(SSC);
    xdmac_channel_enable(XDMAC, XDMA_CH_SSC_TX);
    
	int sinIdx = 0; 
    while(1)
    {
		
		if (dataReceived)
		{
			for(int i = 0; i < BUF_SIZE; i++)
			{
				
				outBuffer[i] = inBuffer[i]; 
				//outBuffer[i] = (uint16_t) ( ( (int32_t)((int16_t)inBuffer[i]) + (int32_t)(sin_wave[sinIdx++]/16) ) / 2 ); 
				//if(sinIdx == 240) sinIdx = 0; 
			}
			dataReceived = 0; 
		}
    }
}

