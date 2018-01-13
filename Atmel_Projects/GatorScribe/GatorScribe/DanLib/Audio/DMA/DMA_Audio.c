/*
 * DMA_Audio.c
 *
 * Created: 12/8/2017 7:39:33 PM
 *  Author: Daniel Gonzalez
 */

#include <asf.h>
#include "DMA_Audio.h"
#include "clicks.h"


/********************************** Static Variables Start **********************************/
COMPILER_WORD_ALIGNED
static lld_view1 linklist_write[2];
COMPILER_WORD_ALIGNED
static lld_view1 linklist_read[2];

volatile static bool inPingMode = 1;
volatile static bool outPingMode = 1;


static int clickIdx = 0;
volatile static uint32_t metronome_time_elapsed = 0; 
/********************************** Static Variables End **********************************/

/********************************** Public Variables Start **********************************/
uint16_t inPingBuffer[BUF_SIZE];
uint16_t inPongBuffer[BUF_SIZE];
uint16_t outPingBuffer[BUF_SIZE];
uint16_t outPongBuffer[BUF_SIZE];

COMPILER_WORD_ALIGNED
volatile int16_t processPingBuffer[PROCESS_BUF_SIZE];	
COMPILER_WORD_ALIGNED
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
extern volatile bool metronome_on; 
extern volatile bool one_beat; 
extern volatile bool up_beat; 
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
		if (metronome_on)
		{	
			if (one_beat)
			{
				for(int i = 0; i < BUF_SIZE; i++)
				{
					outBuffer[i] = (uint16_t) ( ( (int32_t)((int16_t)inBuffer[i]) + (int32_t)(click_high[clickIdx++]) ) / 2 );
					if(clickIdx == CLICK_LENGTH) 
						clickIdx = 0;
				}
			}
			else if (up_beat)
			{
				for(int i = 0; i < BUF_SIZE; i++)
				{
					outBuffer[i] = (uint16_t) ( ( (int32_t)((int16_t)inBuffer[i]) + (int32_t)(click_low[clickIdx++]/6) ) / 2 );
					if(clickIdx == CLICK_LENGTH)
						clickIdx = 0;
				}
			}
			else
			{
				for(int i = 0; i < BUF_SIZE; i++)
				{
					outBuffer[i] = (uint16_t) ( ( (int32_t)((int16_t)inBuffer[i]) + (int32_t)(click_low[clickIdx++]) ) / 2 );
					if(clickIdx == CLICK_LENGTH)
						clickIdx = 0;
				}				
			}
			metronome_time_elapsed += BUF_SIZE_PER_CHANNEL; 
			if (metronome_time_elapsed >= CLICK_DURATION)
			{ 
				metronome_on = false; 
				metronome_time_elapsed = 0; 
				clickIdx = 0; 
			}
		}
		else 
		{
			for(int i = 0; i < BUF_SIZE; i++)
				outBuffer[i] = (uint16_t)((int16_t)inBuffer[i] / 2);	
		}
		
		// Fill process buffer 
		for(int i = 0; i < BUF_SIZE; i++)
		{
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

