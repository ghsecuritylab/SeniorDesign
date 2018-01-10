/*
 * DMA_Audio.c
 *
 * Created: 12/8/2017 7:39:33 PM
 *  Author: Daniel Gonzalez
 */

#include <asf.h>
#include "DMA_Audio.h"

#define SIN_WAVE_LENGTH 240 
/********************************** Static Variables Start **********************************/
COMPILER_WORD_ALIGNED
static lld_view1 linklist_write[2];
COMPILER_WORD_ALIGNED
static lld_view1 linklist_read[2];

volatile static bool inPingMode = 1;
volatile static bool outPingMode = 1;

static int16_t sin_wave[SIN_WAVE_LENGTH] =
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
static int sinIdx = 0;
/********************************** Static Variables End **********************************/

/********************************** Public Variables Start **********************************/
uint16_t inPingBuffer[BUF_SIZE];
uint16_t inPongBuffer[BUF_SIZE];
uint16_t outPingBuffer[BUF_SIZE];
uint16_t outPongBuffer[BUF_SIZE];

volatile int16_t processPingBuffer[PROCESS_BUF_SIZE];	
volatile int16_t processPongBuffer[PROCESS_BUF_SIZE];
volatile int16_t *processBuffer = processPongBuffer; 
volatile int16_t *fillBuffer = processPingBuffer;
volatile uint16_t *inBuffer = inPingBuffer;
volatile uint16_t *outBuffer = outPingBuffer;

volatile bool processPingMode = 1;

volatile bool outOfTime = 0; 
/********************************** Public Variables End **********************************/

/********************************** Extern Variables Start **********************************/
extern volatile bool recording; 

/******************************* XDMAC Interrupt Handler Start *******************************/ 
void XDMAC_Handler(void)
{
    uint32_t dma_status;
    
    dma_status = xdmac_channel_get_interrupt_status(XDMAC, XDMA_CH_SSC_RX);
    if (dma_status & XDMAC_CIS_BIS)
    {
		if(inPingMode)
		{
			inBuffer = inPingBuffer; 
		}
		else 
		{
			inBuffer = inPongBuffer; 
		}
		inPingMode = !inPingMode; 
		int processIdx = 0; 
		for(int i = 0; i < BUF_SIZE; i++)
		{
						
			outBuffer[i] = inBuffer[i];
			//outBuffer[i] = (uint16_t) ( ( (int32_t)((int16_t)inBuffer[i]) + (int32_t)(sin_wave[sinIdx++]/16) ) / 2 );
			//if(sinIdx == SIN_WAVE_LENGTH) sinIdx = 0;

			/* Check if divisible by 4 for decimation by 2 */ 
			if ((i & 0x03) == 0)
				fillBuffer[processIdx++] = (int16_t)inBuffer[i]; 
		}
		
		if (processPingMode)
		{
			if (fillBuffer == &processPingBuffer[PROCESS_BUF_SIZE - (BUF_SIZE >> 1)])
			{
				// out of time 				
			}	
			else 
			{
				fillBuffer += PROCESS_BUF_SIZE_INCREMENT; 
			}
		}
		else 
		{
			if (fillBuffer == &processPongBuffer[PROCESS_BUF_SIZE - (BUF_SIZE >> 1)])
			{
				// out of time 
			}
			else
			{
				fillBuffer += PROCESS_BUF_SIZE_INCREMENT;
			}
		}
    }
	
	dma_status = xdmac_channel_get_interrupt_status(XDMAC, XDMA_CH_SSC_TX);
	if (dma_status & XDMAC_CIS_BIS)
	{
		if(outPingMode)
		{
			outBuffer = outPingBuffer; 
		}
		else
		{
			outBuffer = outPongBuffer; 
		}
		outPingMode = !outPingMode; 
	}
}
/******************************* XDMAC Interrupt Handler End *******************************/

/********************************** Public Functions Start **********************************/
void configure_xdma(void)
{
    uint16_t *src;
    
    xdmac_channel_config_t xdmac_channel_cfg = {0};
    
    /* Initialize and enable DMA controller */
    pmc_enable_periph_clk(ID_XDMAC);
    
    /*Enable XDMA interrupt */
    NVIC_ClearPendingIRQ(XDMAC_IRQn);
    NVIC_SetPriority(XDMAC_IRQn ,1);
    NVIC_EnableIRQ(XDMAC_IRQn);
    
    /********** Read Buffer ********/
    /* Initialize channel config */
    xdmac_channel_cfg.mbr_cfg = XDMAC_CC_TYPE_PER_TRAN
    | XDMAC_CC_MBSIZE_SINGLE
    | XDMAC_CC_DSYNC_PER2MEM
    | XDMAC_CC_CSIZE_CHK_1
    | XDMAC_CC_DWIDTH_HALFWORD
    | XDMAC_CC_SIF_AHB_IF1
    | XDMAC_CC_DIF_AHB_IF0
    | XDMAC_CC_SAM_FIXED_AM
    | XDMAC_CC_DAM_INCREMENTED_AM
    | XDMAC_CC_PERID(33);
	
    xdmac_configure_transfer(XDMAC, XDMA_CH_SSC_RX, &xdmac_channel_cfg);
    
    /* Initialize linked list descriptor */
    src = (uint16_t *)&inPingBuffer[0];
    linklist_read[0].mbr_ubc = XDMAC_UBC_NVIEW_NDV1
		| XDMAC_UBC_NDE_FETCH_EN
		| XDMAC_UBC_NSEN_UPDATED
		| XDMAC_CUBC_UBLEN(BUF_SIZE);
    linklist_read[0].mbr_sa  = (uint32_t)&(SSC->SSC_RHR);
    linklist_read[0].mbr_da = (uint32_t)(src);
    linklist_read[0].mbr_nda = (uint32_t)&linklist_read[1];
	src = (uint16_t *)&inPongBuffer[0];; 
	linklist_read[1].mbr_ubc = XDMAC_UBC_NVIEW_NDV1
	    | XDMAC_UBC_NDE_FETCH_EN
	    | XDMAC_UBC_NSEN_UPDATED
	    | XDMAC_CUBC_UBLEN(BUF_SIZE);
	linklist_read[1].mbr_sa  = (uint32_t)&(SSC->SSC_RHR);
	linklist_read[1].mbr_da = (uint32_t)(src);
	linklist_read[1].mbr_nda = (uint32_t)&linklist_read[0];
    
    xdmac_channel_set_descriptor_control(XDMAC, XDMA_CH_SSC_RX, XDMAC_CNDC_NDVIEW_NDV1 |
                                         XDMAC_CNDC_NDE_DSCR_FETCH_EN |
                                         XDMAC_CNDC_NDSUP_SRC_PARAMS_UPDATED |
                                         XDMAC_CNDC_NDDUP_DST_PARAMS_UPDATED);
    xdmac_channel_set_descriptor_addr(XDMAC, XDMA_CH_SSC_RX, (uint32_t)(&linklist_read[0]), 0);
    
    xdmac_enable_interrupt(XDMAC, XDMA_CH_SSC_RX);
    xdmac_channel_enable_interrupt(XDMAC, XDMA_CH_SSC_RX, XDMAC_CIE_BIE);
    
    
    /******* Write buffer *******/
    xdmac_channel_cfg.mbr_cfg = XDMAC_CC_TYPE_PER_TRAN
    | XDMAC_CC_MBSIZE_SINGLE
    | XDMAC_CC_DSYNC_MEM2PER
    | XDMAC_CC_CSIZE_CHK_1
    | XDMAC_CC_DWIDTH_HALFWORD
    | XDMAC_CC_SIF_AHB_IF0
    | XDMAC_CC_DIF_AHB_IF1
    | XDMAC_CC_SAM_INCREMENTED_AM
    | XDMAC_CC_DAM_FIXED_AM
    | XDMAC_CC_PERID(32);
	
    xdmac_configure_transfer(XDMAC, XDMA_CH_SSC_TX, &xdmac_channel_cfg);
    
    src = (uint16_t *)&outPingBuffer[0];
    linklist_write[0].mbr_ubc = XDMAC_UBC_NVIEW_NDV1
		 | XDMAC_UBC_NDE_FETCH_EN
		 | XDMAC_UBC_NSEN_UPDATED
			| XDMAC_CUBC_UBLEN(BUF_SIZE);
    linklist_write[0].mbr_sa = (uint32_t)(src);
    linklist_write[0].mbr_da = (uint32_t)&(SSC->SSC_THR);
    linklist_write[0].mbr_nda = (uint32_t)&linklist_write[1];
	
	src = (uint16_t *)&outPongBuffer[0];
	linklist_write[1].mbr_ubc = XDMAC_UBC_NVIEW_NDV1
	    | XDMAC_UBC_NDE_FETCH_EN
	    | XDMAC_UBC_NSEN_UPDATED
	    | XDMAC_CUBC_UBLEN(BUF_SIZE);
	linklist_write[1].mbr_sa = (uint32_t)(src);
	linklist_write[1].mbr_da = (uint32_t)&(SSC->SSC_THR);
	linklist_write[1].mbr_nda = (uint32_t)&linklist_write[0];

    xdmac_channel_set_descriptor_control(XDMAC, XDMA_CH_SSC_TX, XDMAC_CNDC_NDVIEW_NDV1
                                         | XDMAC_CNDC_NDE_DSCR_FETCH_EN
                                         | XDMAC_CNDC_NDSUP_SRC_PARAMS_UPDATED
                                         | XDMAC_CNDC_NDDUP_DST_PARAMS_UPDATED);
    xdmac_channel_set_descriptor_addr(XDMAC, XDMA_CH_SSC_TX, (uint32_t)(&linklist_write[0]), 0);
    
    xdmac_enable_interrupt(XDMAC, XDMA_CH_SSC_TX);
    xdmac_channel_enable_interrupt(XDMAC, XDMA_CH_SSC_TX, XDMAC_CIE_BIE);
    
}
/********************************** Public Functions End **********************************/

