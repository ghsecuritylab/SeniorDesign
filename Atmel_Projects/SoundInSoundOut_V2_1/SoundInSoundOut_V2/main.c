/* ---------------------------------------------------------------------------- */
/*                  Atmel Microcontroller Software Support                      */
/*                       SAM Software Package License                           */
/* ---------------------------------------------------------------------------- */
/* Copyright (c) 2015, Atmel Corporation                                        */
/*                                                                              */
/* All rights reserved.                                                         */
/*                                                                              */
/* Redistribution and use in source and binary forms, with or without           */
/* modification, are permitted provided that the following condition is met:    */
/*                                                                              */
/* - Redistributions of source code must retain the above copyright notice,     */
/* this list of conditions and the disclaimer below.                            */
/*                                                                              */
/* Atmel's name may not be used to endorse or promote products derived from     */
/* this software without specific prior written permission.                     */
/*                                                                              */
/* DISCLAIMER:  THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR   */
/* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE   */
/* DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,      */
/* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT */
/* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,  */
/* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF    */
/* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING         */
/* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, */
/* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                           */
/* ---------------------------------------------------------------------------- */

/**
 * \page ssc_dma_audio SSC with DMA Audio Example
 *
 * \section Purpose
 *
 * This example uses the Synchronous Serial Controller (SSC) of an SAMV7x
 * microcontroller to output an audio stream through the on-board WM8904 CODEC.
 *
 * \section Requirements
 *
 * This package can be used with SAM V71 Xplained Ultra board with external
 * codec WM8904 components.
 *
 * \section Description
 * This program plays a WAV file from PC via Line-In. The audio stream is
 * sent through the SSC interface connected to the on-board WM8904, enabling
 * the sound to be audible using a pair of headphones.
 *
 * \section Usage
 *  -# Build the program and download it inside the SAM V71 Xplained Ultra board.
 *     Please refer to the Getting Started with SAM V71 Microcontrollers.pdf
 * -# On the computer, open and configure a terminal application
 *    (e.g. HyperTerminal on Microsoft Windows) with these settings:
 *   - 115200 baud rate
 *   - 8 bits of data
 *   - No parity
 *   - 1 stop bit
 *   - No flow control
 * -# Start the application.
 * -# In the terminal window, the following text should appear:
 *    \code
 *     -- SSC DMA Audio Example xxx --
 *      -- SAMxxxxx-xx
 *     -- Compiled: xxx xx xxxx xx:xx:xx --
 *    \endcode
 * The user can then choose any of the available options to perform the
 * described action.
 *
 * \section References
 * - ssc_dma_audio/main.c
 * - ssc.c
 * - twi.c
 * - twid.c
 * - xdmac.c
 * - xdmad.c
 */

/**
 * \file
 *
 * This file contains all the specific code for the SSC audio example.
 */


/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/

#include "board.h"

/*----------------------------------------------------------------------------
 *        Local definitions
 *----------------------------------------------------------------------------*/
#define I2S_SLAVE_TX_SETTING     ((SSC_TCMR_CKS_TK) |     \
			(SSC_TCMR_CKO_NONE) |                        \
			(SSC_TCMR_START_TF_EDGE) |                   \
			(SSC_TCMR_STTDLY(1)) |                       \
			(SSC_TCMR_PERIOD(0)))

#define I2S_SLAVE_TX_FRM_SETTING ((SSC_TFMR_DATLEN(BITS_BY_SLOT - 1)) | \
			(SSC_TFMR_MSBF) |                                          \
			(SSC_TFMR_DATNB(SLOT_BY_FRAME - 1)) |                      \
			(SSC_TFMR_FSOS_NONE))


#define I2S_SLAVE_RX_SETTING     ((SSC_RCMR_CKS_TK) |   \
			(SSC_RCMR_CKO_NONE) |                      \
			(SSC_RCMR_CKI) |                           \
			(SSC_RCMR_START_RF_EDGE) |                 \
			(SSC_RCMR_STTDLY(1)) |                     \
			(SSC_RCMR_PERIOD(0)))

#define I2S_SLAVE_RX_FRM_SETTING ((SSC_RFMR_DATLEN(BITS_BY_SLOT - 1)) | \
			(SSC_RFMR_MSBF) |                                          \
			(SSC_RFMR_DATNB(SLOT_BY_FRAME - 1)) |                      \
			(SSC_RFMR_FSOS_NONE))


/** Master clock frequency in Hz */
#define SSC_MCK                 BOARD_MCK

/** MAX size of the recorded sound */
#define MAX_RECORD_SIZE         0xFFFFFFFF

/** MAX size of one DMA transfer */
#define MAX_DMA_SIZE            0x1000

/** TWI clock */
#define TWI_CLOCK               400000

/** WAV feature. */
#define SAMPLE_RATE             (48000)
#define SLOT_BY_FRAME           (1)
#define BITS_BY_SLOT            (16)

/** DMA Descriptor */
#define TOTAL_Buffers            2
#define AUDIO_IF                SSC

/*----------------------------------------------------------------------------
 *        Local variables
 *----------------------------------------------------------------------------*/

/** List of pins to configure. */
static const Pin pinsSsc[] = {PIN_TWI_TWD0, PIN_TWI_TWCK0, PIN_SSC_TD,
			PIN_SSC_TK, PIN_SSC_TF, PIN_SSC_RD,  PIN_SSC_RK, PIN_SSC_RF, PIN_PCK2};

/** Global DMA driver for all transfer */
static sXdmad dmad;
/** DMA channel for RX */
static uint32_t sscDmaRxChannel;
/** DMA channel for TX */
static uint32_t sscDmaTxChannel;

//static sXdmadCfg xdmadCfg;

/*
COMPILER_ALIGNED(32) static LinkedListDescriporView1 dmaWriteLinkList[TOTAL_Buffers];
COMPILER_ALIGNED(32) static LinkedListDescriporView1 dmaReadLinkList[TOTAL_Buffers];
*/
#define BUF_SIZE          MAX_DMA_SIZE

static uint16_t inPingBuffer[BUF_SIZE];
static uint16_t inPongBuffer[BUF_SIZE];
static uint16_t outPingBuffer[BUF_SIZE];
static uint16_t outPongBuffer[BUF_SIZE];

volatile static uint32_t inPingPong = 1; 
volatile static uint32_t outPingPong = 1; 
volatile static uint16_t *inBuffer = inPingBuffer; 
volatile static uint16_t *outBuffer = outPingBuffer;
volatile static int32_t dataReceived = 0; 


/** TWI instance*/
static Twid twid;

/*----------------------------------------------------------------------------
 *        Local functions
 *----------------------------------------------------------------------------*/

/**
 * ISR for DMA interrupt
 */
void XDMAC_Handler(void)
{
	XDMAD_Handler(&dmad);
}

/**
 * \brief TWI interrupt handler. Forwards the interrupt to the TWI driver handler.
 */
void TWIHS0_Handler(void)
{
	TWID_Handler(&twid);
}

/*
 * \brief Callback function for SSC Rx.
 * */
static void sscDmaRxClk(uint32_t Channel, void* pArg)
{
	Xdmac *pXdmac = dmad.pXdmacs;		
	if (inPingPong) 
	{
		XDMAC_SetDestinationAddr(pXdmac, sscDmaRxChannel, (uint32_t)inPongBuffer); 
		inBuffer = inPingBuffer; 
	}
	else 
	{
		XDMAC_SetDestinationAddr(pXdmac, sscDmaRxChannel, (uint32_t)inPingBuffer);
		inBuffer = inPongBuffer;
	}
	inPingPong = !inPingPong; 
	dataReceived = 1; 
	XDMAC_EnableChannel(pXdmac, sscDmaRxChannel);
}

/*
 * \brief Callback function for SSC Tx.
 * */
static void sscDmaTxClk(uint32_t Channel, void* pArg)
{
	Xdmac *pXdmac = dmad.pXdmacs;
	if (outPingPong)
	{
		XDMAC_SetSourceAddr(pXdmac, sscDmaTxChannel, (uint32_t)outPongBuffer);
		outBuffer = outPingBuffer;
	}
	else
	{
		XDMAC_SetSourceAddr(pXdmac, sscDmaTxChannel, (uint32_t)outPingBuffer);
		outBuffer = outPongBuffer;
	}
	outPingPong = !outPingPong;
	XDMAC_EnableChannel(pXdmac, sscDmaTxChannel);
}

/**
 * \brief DMA driver configuration
 */
static void Dma_configure(void)
{
	sXdmad *pDmad = &dmad;

	/* Driver initialize */
	XDMAD_Initialize( pDmad, 0);
	/* Configure TWI interrupts */
	NVIC_ClearPendingIRQ(XDMAC_IRQn);
	NVIC_EnableIRQ(XDMAC_IRQn);
	/* Allocate DMA channels for SSC */
	sscDmaTxChannel = XDMAD_AllocateChannel(pDmad, XDMAD_TRANSFER_MEMORY, ID_SSC);
	sscDmaRxChannel = XDMAD_AllocateChannel(pDmad, ID_SSC, XDMAD_TRANSFER_MEMORY);
	if (sscDmaTxChannel == XDMAD_ALLOC_FAILED || sscDmaRxChannel == XDMAD_ALLOC_FAILED) 
	{
		printf("xDMA channel allocation error\n\r");
		while (1);
	}

	XDMAD_SetCallback( pDmad, sscDmaRxChannel, sscDmaRxClk, 0);
	XDMAD_SetCallback( pDmad, sscDmaTxChannel, sscDmaTxClk, 0);
	XDMAD_PrepareChannel(pDmad, sscDmaTxChannel);
	XDMAD_PrepareChannel(pDmad, sscDmaRxChannel);
}


/**
 * \brief Receive and play audio with DMA.
 */
static void PlayRecording(void)
{
	uint32_t xdmaCndc;
	sXdmadCfg xdmadCfg;

	/******************** In **************/ 
	/* Microblock Control Member */ 
	xdmadCfg.mbr_ubc = BUF_SIZE; 
	
	/* Source Address Member */ 
	xdmadCfg.mbr_sa = (uint32_t)&(AUDIO_IF->SSC_RHR);
	
	/* Destination Address Member */ 
	xdmadCfg.mbr_da = (uint32_t)&inPingBuffer[0]; 
	
	/* Configuration Register */
	xdmadCfg.mbr_cfg = XDMAC_CC_TYPE_PER_TRAN
		| XDMAC_CC_MBSIZE_SINGLE
		| XDMAC_CC_DSYNC_PER2MEM
		| XDMAC_CC_CSIZE_CHK_1
		| XDMAC_CC_DWIDTH_HALFWORD
		| XDMAC_CC_SIF_AHB_IF1
		| XDMAC_CC_DIF_AHB_IF0
		| XDMAC_CC_SAM_FIXED_AM
		| XDMAC_CC_DAM_INCREMENTED_AM
		| XDMAC_CC_PERID(XDMAIF_Get_ChannelNumber(ID_SSC, XDMAD_TRANSFER_RX));
	
	/* Block Control Member */ 
	xdmadCfg.mbr_bc = 0; 
	
	/* Data Stride Member */ 
	xdmadCfg.mbr_ds = 0; 
	
	/* Source Microblock Stride Member */ 
	xdmadCfg.mbr_sus = 0; 
	
	/* Destination Microblock Stride Member */ 
	xdmadCfg.mbr_dus = 0; 
	
	/* Next Descriptor Control Register */ 
	xdmaCndc = XDMAC_CNDC_NDE_DSCR_FETCH_DIS; 

	/*XDMAC_CIE_BIE make interrupts can be generated on per block basis*/
	XDMAD_ConfigureTransfer( &dmad, sscDmaRxChannel, &xdmadCfg, xdmaCndc,
			(uint32_t)&inPingBuffer[0], XDMAC_CIE_BIE);

	/***************** Out *****************/ 
	/* Microblock Control Member */
	xdmadCfg.mbr_ubc = BUF_SIZE;
	
	/* Source Address Member */
	xdmadCfg.mbr_sa = (uint32_t)&outPingBuffer[0];
	
	/* Destination Address Member */
	xdmadCfg.mbr_da = (uint32_t)&(AUDIO_IF->SSC_THR);	
	
	/* Configuration Register */
	xdmadCfg.mbr_cfg = XDMAC_CC_TYPE_PER_TRAN
	| XDMAC_CC_MBSIZE_SINGLE
	| XDMAC_CC_DSYNC_MEM2PER
	| XDMAC_CC_CSIZE_CHK_1
	| XDMAC_CC_DWIDTH_HALFWORD
	| XDMAC_CC_SIF_AHB_IF0
	| XDMAC_CC_DIF_AHB_IF1
	| XDMAC_CC_SAM_INCREMENTED_AM
	| XDMAC_CC_DAM_FIXED_AM
	| XDMAC_CC_PERID(XDMAIF_Get_ChannelNumber(ID_SSC, XDMAD_TRANSFER_TX));
	
	/* Block Control Member */
	xdmadCfg.mbr_bc = 0;
	
	/* Data Stride Member */
	xdmadCfg.mbr_ds = 0;
	
	/* Source Microblock Stride Member */
	xdmadCfg.mbr_sus = 0;
	
	/* Destination Microblock Stride Member */
	xdmadCfg.mbr_dus = 0;
	 
	XDMAD_ConfigureTransfer( &dmad, sscDmaTxChannel, &xdmadCfg, xdmaCndc,
			(uint32_t)&outPingBuffer[0], XDMAC_CIE_BIE);

	SSC_EnableReceiver(AUDIO_IF);
	XDMAD_StartTransfer( &dmad, sscDmaRxChannel);

	//Wait(50);
	/* Enable playback(SSC TX) */
	SSC_EnableTransmitter(AUDIO_IF);
	XDMAD_StartTransfer( &dmad, sscDmaTxChannel);

}

/*----------------------------------------------------------------------------
 *         Global functions
 *----------------------------------------------------------------------------*/
/**
 * \brief Application entry point for ssc_dam_audio example.
 *
 * \return Unused (ANSI-C compatibility).
 */

uint16_t SIN_WAVE[44] = {32767,37420,41978,46350,50446,54185,57489,60292,62538,64180,65185,65534,
	65218,64245,62634,60418,57641,54361,50643,46562,42202,37651,33000,28345,23780,19397,15285,
11527,	8199,	5369,	3095,	1422,	384,	2,	284,	1225,	2804,	4991,	7741,	10998,	14696,	18761,	23109,	27653};

int main( void )
{
	uint16_t data = 0;
	/* Disable watchdog */
	WDT_Disable(WDT);

	/* Enable I and D cache */
	//SCB_EnableICache();
	//SCB_EnableDCache();

	/* Output example information */
	printf("-- SSC DMA Audio Example %s --\n\r", SOFTPACK_VERSION);
	printf("-- %s\n\r", BOARD_NAME);
	printf("-- Compiled: %s %s With %s--\n\r", __DATE__, __TIME__, COMPILER_NAME);

	/* Configure systick for 1 ms. */
	printf( "Configure system tick to get 1ms tick period.\n\r" );
	if (TimeTick_Configure())
		printf("-F- Systick configuration error\n\r" );

	/* Configure all pins */
	PIO_Configure(pinsSsc, PIO_LISTSIZE(pinsSsc));

	/* Configure SSC */
	SSC_Configure(AUDIO_IF , 0 , SSC_MCK);
	SSC_ConfigureReceiver(AUDIO_IF,I2S_SLAVE_RX_SETTING,I2S_SLAVE_RX_FRM_SETTING);
	SSC_DisableReceiver(AUDIO_IF);
	SSC_ConfigureTransmitter(AUDIO_IF,I2S_SLAVE_TX_SETTING,I2S_SLAVE_TX_FRM_SETTING);
	SSC_DisableTransmitter(AUDIO_IF);

	/* Configure DMA */
	Dma_configure();

	/* Configure and enable the TWI (required for accessing the DAC) */
	PMC_EnablePeripheral(ID_TWIHS0);
	TWI_ConfigureMaster(TWIHS0, TWI_CLOCK, BOARD_MCK);
	TWID_Initialize(&twid, TWIHS0);
	/* Configure TWI interrupts */
	NVIC_ClearPendingIRQ(TWIHS0_IRQn);
	NVIC_EnableIRQ(TWIHS0_IRQn);

	/* check that WM8904 is present */
	WM8904_Write(&twid,WM8904_SLAVE_ADDRESS, 22, 0);
	data=WM8904_Read(&twid, WM8904_SLAVE_ADDRESS, 0);
	if (data != 0x8904) {
		printf("WM8904 not found!\n\r");
		while (1);
	}
	/* Initialize the audio DAC */
	WM8904_Init(&twid, WM8904_SLAVE_ADDRESS, PMC_MCKR_CSS_SLOW_CLK);

	/* Enable the DAC master clock */
	PMC_ConfigurePCK2(PMC_MCKR_CSS_SLOW_CLK, PMC_MCKR_PRES_CLK_1);
	printf("Insert Line-in cable with PC Headphone output\n\r");
	
	PlayRecording();

	int i; 
	int sinIdx = 0; 

	while (1) {
		if (dataReceived == 1) {
			/*
			for(i = 0; i < BUF_SIZE; i++)
			{
				
				outBuffer[i] = SIN_WAVE[sinIdx] >> 8; //inBuffer[i]; 
				sinIdx++;  
				if(sinIdx == 44)
				{
					sinIdx = 0;
				}
				
			}
			*/
			

			for(i = 0; i < BUF_SIZE; i++)
			{
		
				outBuffer[i] = (uint16_t)( (int16_t)inBuffer[i] / 2); 
			}
			 

			dataReceived = 0; 
		}
	}
}
