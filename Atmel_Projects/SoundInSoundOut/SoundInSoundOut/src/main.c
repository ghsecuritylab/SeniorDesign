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

extern volatile uint16_t ReadBuffer[2*MICROBLOCK_LEN];
extern volatile uint16_t WriteBuffer[2*MICROBLOCK_LEN];
extern volatile uint16_t ProcessBuffer[MICROBLOCK_LEN];

/*
const int16_t SIN_WAVE[44] = {0,4653,9211,13583,17679,21418,24722,27525,29771,31413,
	32418,32767,32451,31478,29867,27651,24874,21594,17876,13795,9435,4884,233,-4422,
	-8987,-13370,-17482,-21240,-24568,-27398,-29672,-31345,-32383,-32765,-32483,-31542,
	-29963,-27776,-25026,-21769,-18071,-14006,-9658,-5114}; 
	*/ 

const uint16_t SIN_WAVE[44] = {32767,37420,41978,46350,50446,54185,57489,60292,62538,64180,65185,65534,
	65218,	64245,	62634,	60418,	57641,	54361,	50643,	46562,	42202,	37651,	33000,	28345,	23780,	19397,	15285,
11527,	8199,	5369,	3095,	1422,	384,	2,	284,	1225,	2804,	4991,	7741,	10998,	14696,	18761,	23109,	27653};

volatile int readPtrFlag = 0;
volatile int writePtrFlag = 0; 
volatile int sinIdx = 0;
volatile uint16_t *readPtr;
volatile uint16_t *writePtr;

/* XDMAC interrupt handler */
void XDMAC_Handler(void)
{
	uint32_t dma_status;
	
	dma_status = xdmac_channel_get_interrupt_status(XDMAC, XDMA_CH_SSC_RX);
	SCB_CleanDCache(); 	
	SCB_InvalidateDCache();
	if (dma_status & XDMAC_CIS_BIS)
	{
		flag = 1;
		
		if (readPtrFlag == 0)
		{
			readPtr = &ReadBuffer[0];
			readPtrFlag = 1;
		}
		else
		{
			readPtr = &ReadBuffer[MICROBLOCK_LEN];
			readPtrFlag = 0; 
		}
	}
	
	dma_status = xdmac_channel_get_interrupt_status(XDMAC, XDMA_CH_SSC_TX);
	if (dma_status & XDMAC_CIS_BIS)
	{
		if (writePtrFlag == 0)
		{
			writePtr = &WriteBuffer[0];
			writePtrFlag = 1; 
		}
		else
		{
			writePtr = &WriteBuffer[MICROBLOCK_LEN];
			writePtrFlag = 0; 
		}
		
		volatile int i;
		volatile int16_t readValue = 0;
		//volatile uint16_t a = 0; volatile uint16_t b = 0;
		for (i = 0; i < MICROBLOCK_LEN; i++)
		{
			readValue = readPtr[i] - 0x7FFF;
					
			// readValue = (int16_t)(2 * readValue);
			//a = (uint16_t)( (readValue/2) + (SIN_WAVE[sinIdx]/2));
			//b = (uint16_t)( ((uint32_t)readValue + (uint32_t)SIN_WAVE[sinIdx]) * 0.5);
			//writePtr[i] = (uint16_t)( (readValue/2) + (SIN_WAVE[sinIdx]/2));
					
			//readValue = readPtr[i+1];
			//writePtr[i] = (uint16_t)( ((uint32_t)readValue + (uint32_t)SIN_WAVE[sinIdx]) /2);
					
			//writePtr[i] = SIN_WAVE[sinIdx];
			readValue = readValue ;
			writePtr[i] = readValue + 0x7FFF;
			//ProcessBuffer[i] = readPtr[i];
			if ((i&1))
				sinIdx++;
					
			if (sinIdx == 44)
			sinIdx = 0;
		}
		
	}

}

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
    
    flag = 0;
    while(1)
    {
        
        if(flag)
        {
            pio_set(PIOA, PIO_PA23);
			flag = 0;		
        }
    }
}

