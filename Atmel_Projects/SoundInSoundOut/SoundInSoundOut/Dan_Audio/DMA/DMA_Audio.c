/*
 * DMA_Audio.c
 *
 * Created: 12/8/2017 7:39:33 PM
 *  Author: Daniel Gonzalez
 */

#include <asf.h>
#include "DMA_Audio.h"

/********************************** Defines **********************************/

/********************************** Defines **********************************/


/********************************** Static Variables **********************************/
//COMPILER_WORD_ALIGNED
static lld_view1 linklist_write[2];
//COMPILER_WORD_ALIGNED
static lld_view1 linklist_read[2];

//COMPILER_ALIGNED(8)
volatile uint16_t ReadBuffer[2*MICROBLOCK_LEN];
//COMPILER_ALIGNED(8)
volatile uint16_t WriteBuffer[2*MICROBLOCK_LEN];
//COMPILER_ALIGNED(8)
volatile uint16_t ProcessBuffer[MICROBLOCK_LEN]; 

/********************************** Static Variables **********************************/


/********************************** Externs **********************************/
//extern volatile int flag;
/********************************** Externs **********************************/
/*
//  XDMAC interrupt handler 
void XDMAC_Handler(void)
{
    uint32_t dma_status;
    
    dma_status = xdmac_channel_get_interrupt_status(XDMAC, XDMA_CH_SSC_RX);
    if (dma_status & XDMAC_CIS_BIS)
    {
        flag = 1;
    }
}
*/ 

/********************************** Public Functions **********************************/
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
    src = (uint16_t *)&ReadBuffer[0];
    linklist_read[0].mbr_ubc = XDMAC_UBC_NVIEW_NDV1
		| XDMAC_UBC_NDE_FETCH_EN
		| XDMAC_UBC_NSEN_UPDATED
		| XDMAC_CUBC_UBLEN(MICROBLOCK_LEN);
    linklist_read[0].mbr_sa  = (uint32_t)&(SSC->SSC_RHR);
    linklist_read[0].mbr_da = (uint32_t)(src);
    linklist_read[0].mbr_nda = (uint32_t)&linklist_read[1];
	src += MICROBLOCK_LEN; 
	linklist_read[1].mbr_ubc = XDMAC_UBC_NVIEW_NDV1
	    | XDMAC_UBC_NDE_FETCH_EN
	    | XDMAC_UBC_NSEN_UPDATED
	    | XDMAC_CUBC_UBLEN(MICROBLOCK_LEN);
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
    
    src = (uint16_t *)&WriteBuffer[0];
    linklist_write[0].mbr_ubc = XDMAC_UBC_NVIEW_NDV1
		 | XDMAC_UBC_NDE_FETCH_EN
		 | XDMAC_UBC_NSEN_UPDATED
			| XDMAC_CUBC_UBLEN(MICROBLOCK_LEN);
    linklist_write[0].mbr_sa = (uint32_t)(src);
    linklist_write[0].mbr_da = (uint32_t)&(SSC->SSC_THR);
    linklist_write[0].mbr_nda = (uint32_t)&linklist_write[1];
	
	src += MICROBLOCK_LEN;
	linklist_write[1].mbr_ubc = XDMAC_UBC_NVIEW_NDV1
	    | XDMAC_UBC_NDE_FETCH_EN
	    | XDMAC_UBC_NSEN_UPDATED
	    | XDMAC_CUBC_UBLEN(MICROBLOCK_LEN);
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

