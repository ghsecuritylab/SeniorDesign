/*
 * DMA_Audio.c
 *
 * Created: 12/8/2017 7:39:33 PM
 *  Author: Daniel Gonzalez
 */ 

#include <asf.h>
#include "DMA_Audio.h"

/********************************** Defines **********************************/
/** Micro-block length for single transfer  */
#define MICROBLOCK_LEN          0x100

/** XDMA Descriptor */
#define TOTAL_BUFFERS     2

/** Size of each buffer */
#define BUF_SIZE (TOTAL_BUFFERS * MICROBLOCK_LEN * (BITS_BY_SLOT/8))
/********************************** Defines **********************************/


/********************************** Static Variables **********************************/
static lld_view1 linklist_write[TOTAL_BUFFERS];
static lld_view1 linklist_read[TOTAL_BUFFERS];

static uint16_t AudioBuffer[BUF_SIZE];

volatile int sin_idx = 0; 
static const uint16_t SIN_WAVE[44] = {32767,37420,41978,46350,50446,54185,57489,60292,62538,64180,65185,65534,
	65218,	64245,	62634,	60418,	57641,	54361,	50643,	46562,	42202,	37651,	33000,	28345,	23780,	19397,	15285,
	11527,	8199,	5369,	3095,	1422,	384,	2,	284,	1225,	2804,	4991,	7741,	10998,	14696,	18761,	23109,	27653};
/********************************** Static Variables **********************************/


/********************************** Externs **********************************/
extern volatile int flag; 
/********************************** Externs **********************************/

/* XDMAC interrupt handler */ 
void XDMAC_Handler(void)
{
	uint32_t dma_status;

	dma_status = xdmac_channel_get_interrupt_status(XDMAC, XDMA_CH_SSC_RX);
	if (dma_status & XDMAC_CIS_BIS)
	{
		int i;
		flag = 0;
		for (i = 0; i < BUF_SIZE; i++)
		{
			AudioBuffer[i] +=  SIN_WAVE[sin_idx++];

			if (sin_idx == 44) 
				sin_idx = 0;
		}
	}
}

/********************************** Public Functions **********************************/
void configure_xdma(void)
{
	uint16_t *src;
	uint8_t i;

	xdmac_channel_config_t xdmac_channel_cfg = {0};

	/* Initialize and enable DMA controller */
	pmc_enable_periph_clk(ID_XDMAC);
	
	/*Enable XDMA interrupt */
	NVIC_ClearPendingIRQ(XDMAC_IRQn);
	NVIC_SetPriority(XDMAC_IRQn ,1);
	NVIC_EnableIRQ(XDMAC_IRQn);

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
	src = &AudioBuffer[0];
	for(i = 0; i < TOTAL_BUFFERS; i++) {
		linklist_read[i].mbr_ubc = XDMAC_UBC_NVIEW_NDV1
		| XDMAC_UBC_NDE_FETCH_EN
		| XDMAC_UBC_NSEN_UPDATED
		| XDMAC_CUBC_UBLEN(MICROBLOCK_LEN);
		linklist_read[i].mbr_sa  = (uint32_t)&(SSC->SSC_RHR);
		linklist_read[i].mbr_da = (uint32_t)(src);
		if ( i == (TOTAL_BUFFERS - 1)) {
			linklist_read[i].mbr_nda = (uint32_t)&linklist_read[0];
			} else {
			linklist_read[i].mbr_nda = (uint32_t)&linklist_read[i + 1];
		}
		src += MICROBLOCK_LEN ;
	}

	xdmac_channel_set_descriptor_control(XDMAC, XDMA_CH_SSC_RX, XDMAC_CNDC_NDVIEW_NDV1 |
	XDMAC_CNDC_NDE_DSCR_FETCH_EN |
	XDMAC_CNDC_NDSUP_SRC_PARAMS_UPDATED |
	XDMAC_CNDC_NDDUP_DST_PARAMS_UPDATED);
	xdmac_channel_set_descriptor_addr(XDMAC, XDMA_CH_SSC_RX, (uint32_t)(&linklist_read[0]), 0);

	xdmac_enable_interrupt(XDMAC, XDMA_CH_SSC_RX);
	xdmac_channel_enable_interrupt(XDMAC, XDMA_CH_SSC_RX, XDMAC_CIE_BIE);

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

	src = &AudioBuffer[0];
	for(i = 0; i < TOTAL_BUFFERS; i++) {
		linklist_write[i].mbr_ubc = XDMAC_UBC_NVIEW_NDV1
		| XDMAC_UBC_NDE_FETCH_EN
		| XDMAC_UBC_NSEN_UPDATED
		| XDMAC_CUBC_UBLEN(MICROBLOCK_LEN);
		linklist_write[i].mbr_sa = (uint32_t)(src);
		linklist_write[i].mbr_da = (uint32_t)&(SSC->SSC_THR);
		if ( i == (TOTAL_BUFFERS - 1 )) {
			linklist_write[i].mbr_nda = (uint32_t)&linklist_write[0];
			} else {
			linklist_write[i].mbr_nda = (uint32_t)&linklist_write[i+1];
		}
		src += MICROBLOCK_LEN;
	}

	xdmac_channel_set_descriptor_control(XDMAC, XDMA_CH_SSC_TX, XDMAC_CNDC_NDVIEW_NDV1
	| XDMAC_CNDC_NDE_DSCR_FETCH_EN
	| XDMAC_CNDC_NDSUP_SRC_PARAMS_UPDATED
	| XDMAC_CNDC_NDDUP_DST_PARAMS_UPDATED);
	xdmac_channel_set_descriptor_addr(XDMAC, XDMA_CH_SSC_TX, (uint32_t)(&linklist_write[0]), 0);

	xdmac_enable_interrupt(XDMAC, XDMA_CH_SSC_TX);
	xdmac_channel_enable_interrupt(XDMAC, XDMA_CH_SSC_TX, XDMAC_CIE_BIE);

}