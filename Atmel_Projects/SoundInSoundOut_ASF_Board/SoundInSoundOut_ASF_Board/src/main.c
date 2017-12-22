/**
 * \file
 *
 * \brief Audio sine tone using SSC on WM8904.
 *
 * Copyright (c) 2017-2018 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */
/*
 * Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
 */

/**
 * \mainpage Audio sine tone using SSC on WM8904
 *
 * \section Purpose
 *
 * This example uses the Synchronous Serial Controller (SSC) of SAM devices
 * to output an audio stream through the on-board WM8904 CODEC with options of
 * SAM device acting as Master or Slave.
 *
 * \section Requirements
 *
 * This example can be used with SAM Xplained board kits with the on-board WM8904
 * CODEC.
 *
 * \section Description
 * This program plays sine tone stored as a lookup table. The sine tone audio is of
 * 16-bit data width and 48000 Hz sampling rate.
 * The audio stream is sent through the SSC interface connected to the on-board WM8904,
 * enabling the sound to be audible using a pair of headphones.
 *
 *
 * The code can be roughly broken down as follows:
 * - Enable the clock.
 * - Initialize and configure the Codec.
 * - Configure and enable the SSC.
 * - Configure and enable the XDMA.
 *
 * \section Usage
 *
 * -# Build the program and download it into the Xplained board.
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
	-- WM8904 example xxx --
	-- SAMxx-xx
	-- Compiled: xxx xx xxxx xx:xx:xx --
\endcode
 */

#include <asf.h>

/// @cond 0
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/// @endcond

#define STRING_EOL    "\r"
#define STRING_HEADER "-- Audio sine tone using SSC on WM8904 --\r\n" \
"-- "BOARD_NAME" --\r\n" \
"-- Compiled: "__DATE__" "__TIME__" --"STRING_EOL

//#define PCK2 PMC_MCKR_CSS_MAIN_CLK
#define PCK2 PMC_MCKR_CSS_SLOW_CLK

/** Features of the stored sine tone */
/** Sampling rate */
#define SAMPLE_RATE             (48000)
/** slot per sample */
#define SLOT_BY_FRAME           (2)
/** Bits per slot */
#define BITS_BY_SLOT            (16)



/* Input Master Clock to SSC peripheral */
#define INPUT_MCK_TO_SSC		120000000

/** XDMA channel used in this example. */
#define XDMA_CH_SSC_TX    0
/** XDMA Descriptor */
#define TOTAL_BUFFERS     2
#define MAX_DMA_SIZE            240
static lld_view1 linklist_write[TOTAL_BUFFERS];
static uint16_t AudioBuffer[MAX_DMA_SIZE*(BITS_BY_SLOT / 8)]=
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


/**
 *  \brief Configure UART console.
 */
static void configure_console(void)
{
	const usart_serial_options_t uart_serial_options =
	{
		.baudrate = CONF_UART_BAUDRATE,
		.charlength = CONF_UART_CHAR_LENGTH,
		.paritytype = CONF_UART_PARITY,
		.stopbits = CONF_UART_STOP_BITS,
	};

	/* Configure console UART. */
	sysclk_enable_peripheral_clock(CONSOLE_UART_ID);
	stdio_serial_init(CONF_UART, &uart_serial_options);
}

/**
 *  \brief Configure SSC in Master or Slave mode
 *  (based on the value of macro PCK2) for Tx operation only
 */
static void configure_ssc(void)
{
	clock_opt_t tx_clk_option;
	data_frame_opt_t tx_data_frame_option;

	/* Initialize clock */
	pmc_enable_periph_clk(ID_SSC);

	/* Reset SSC */
	ssc_reset(SSC);
	
	if(PCK2 == PMC_MCKR_CSS_SLOW_CLK)
	{
		/* Transmitter clock mode configuration. */
		tx_clk_option.ul_cks = SSC_TCMR_CKS_TK;
		tx_clk_option.ul_cko = SSC_TCMR_CKO_NONE;
		tx_clk_option.ul_cki = 0;
		tx_clk_option.ul_ckg = SSC_TCMR_CKG_CONTINUOUS;
		tx_clk_option.ul_start_sel = SSC_TCMR_START_TF_EDGE;
		tx_clk_option.ul_sttdly = 1;
		tx_clk_option.ul_period = BITS_BY_SLOT - 1;

		/* Transmitter frame mode configuration. */
		tx_data_frame_option.ul_datlen = BITS_BY_SLOT - 1;
		tx_data_frame_option.ul_msbf = SSC_TFMR_MSBF;
		tx_data_frame_option.ul_datnb = 0;
		tx_data_frame_option.ul_fslen = BITS_BY_SLOT - 1;
		tx_data_frame_option.ul_fslen_ext = 0;
		tx_data_frame_option.ul_fsos = SSC_TFMR_FSOS_NONE;
		tx_data_frame_option.ul_fsedge = SSC_TFMR_FSEDGE_POSITIVE;
	}
	else
	{
		ssc_set_clock_divider(SSC, ((BITS_BY_SLOT*SLOT_BY_FRAME)*SAMPLE_RATE), INPUT_MCK_TO_SSC);
		/* Transmitter clock mode configuration. */
		tx_clk_option.ul_cks = SSC_TCMR_CKS_MCK;
		tx_clk_option.ul_cko = SSC_TCMR_CKO_CONTINUOUS;
		tx_clk_option.ul_cki = 0;
		tx_clk_option.ul_ckg = SSC_TCMR_CKG_CONTINUOUS;
		tx_clk_option.ul_start_sel = SSC_TCMR_START_TF_EDGE;
		tx_clk_option.ul_sttdly = 1;
		tx_clk_option.ul_period = BITS_BY_SLOT - 1;

		/* Transmitter frame mode configuration. */
		//tx_data_frame_option.ul_datlen = BITS_BY_SLOT - 1;
		tx_data_frame_option.ul_datlen = BITS_BY_SLOT - 1;				
		tx_data_frame_option.ul_msbf = SSC_TFMR_MSBF;
		tx_data_frame_option.ul_datnb = SLOT_BY_FRAME - 1;
		tx_data_frame_option.ul_fslen = BITS_BY_SLOT - 1;
		tx_data_frame_option.ul_fslen_ext = 0;
		tx_data_frame_option.ul_fsos = SSC_TFMR_FSOS_NEGATIVE;
		tx_data_frame_option.ul_fsedge = SSC_TFMR_FSEDGE_POSITIVE;
	}
	ssc_set_transmitter(SSC, &tx_clk_option, &tx_data_frame_option);

	/* Disable transmitter first */
	ssc_disable_tx(SSC);

	/* Disable All Interrupt */
	ssc_disable_interrupt(SSC, 0xFFFFFFFF);
}

/**
 *  \brief Configure DMA
 */
static void configure_xdma(void)
{
	uint16_t *src;
	uint8_t i;

	xdmac_channel_config_t xdmac_channel_cfg = {0};

	/* Initialize and enable DMA controller */
	pmc_enable_periph_clk(ID_XDMAC);
	
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
	for(i = 0; i < TOTAL_BUFFERS; i++)
	{
		linklist_write[i].mbr_ubc = XDMAC_UBC_NVIEW_NDV1
									| XDMAC_UBC_NDE_FETCH_EN
									| XDMAC_UBC_NSEN_UPDATED
									| XDMAC_UBC_NDEN_UNCHANGED
									| XDMAC_CUBC_UBLEN(MAX_DMA_SIZE);
		linklist_write[i].mbr_sa = (uint32_t)(src);
		linklist_write[i].mbr_da = (uint32_t)&(SSC->SSC_THR);										
		if ( i == (TOTAL_BUFFERS - 1 ))
		{
			linklist_write[i].mbr_nda = (uint32_t)&linklist_write[0];
		}
		else
		{
			linklist_write[i].mbr_nda = (uint32_t)&linklist_write[i+1];
		}
	}
		
	xdmac_channel_set_descriptor_control(XDMAC, XDMA_CH_SSC_TX, 
										  XDMAC_CNDC_NDVIEW_NDV1
										| XDMAC_CNDC_NDE_DSCR_FETCH_EN
										| XDMAC_CNDC_NDSUP_SRC_PARAMS_UPDATED
										| XDMAC_CNDC_NDDUP_DST_PARAMS_UPDATED);
	xdmac_channel_set_descriptor_addr(XDMAC, XDMA_CH_SSC_TX, (uint32_t)(&linklist_write[0]), 0);
		
	xdmac_enable_interrupt(XDMAC, XDMA_CH_SSC_TX);
	xdmac_channel_enable_interrupt(XDMAC, XDMA_CH_SSC_TX, XDMAC_CIE_LIE);

	/*Enable XDMA interrupt */
	NVIC_ClearPendingIRQ(XDMAC_IRQn);
	NVIC_SetPriority(XDMAC_IRQn ,1);
	NVIC_EnableIRQ(XDMAC_IRQn);
}

/**
 *  \brief Configure Codec WM8904 in Master or Slave mode
 *  (based on the value of macro PCK2) for audio playback
 */
static void configure_codec(void)
{
	uint16_t data = 0;
	/* check that WM8904 is present */
	wm8904_write_register(WM8904_SW_RESET_AND_ID, 0xFFFF);
	data = wm8904_read_register(WM8904_SW_RESET_AND_ID);
	if(data != 0x8904) {
		printf("WM8904 not found!\n\r");
		while(1);
	}
	if(PCK2 == PMC_MCKR_CSS_SLOW_CLK)
	{
		wm8904_write_register(WM8904_BIAS_CONTROL_0, WM8904_ISEL_HP_BIAS);
		wm8904_write_register(WM8904_VMID_CONTROL_0, WM8904_VMID_BUF_ENA | 
								WM8904_VMID_RES_FAST | WM8904_VMID_ENA);
		delay_ms(5);
		wm8904_write_register(WM8904_VMID_CONTROL_0, WM8904_VMID_BUF_ENA | 	WM8904_VMID_RES_NORMAL | WM8904_VMID_ENA);
		wm8904_write_register(WM8904_BIAS_CONTROL_0, WM8904_ISEL_HP_BIAS | WM8904_BIAS_ENA);
		wm8904_write_register(WM8904_POWER_MANAGEMENT_0, WM8904_INL_ENA | WM8904_INR_ENA);
		wm8904_write_register(WM8904_POWER_MANAGEMENT_2, WM8904_HPL_PGA_ENA | WM8904_HPR_PGA_ENA);
		wm8904_write_register(WM8904_DAC_DIGITAL_1, WM8904_DEEMPH(0));
		wm8904_write_register(WM8904_ANALOGUE_OUT12_ZC, 0x0000);
		wm8904_write_register(WM8904_CHARGE_PUMP_0, WM8904_CP_ENA);
		wm8904_write_register(WM8904_CLASS_W_0, WM8904_CP_DYN_PWR);

		wm8904_write_register(WM8904_FLL_CONTROL_1, 0x0000);
		wm8904_write_register(WM8904_FLL_CONTROL_2, WM8904_FLL_OUTDIV(7)| 
								WM8904_FLL_FRATIO(4));
		wm8904_write_register(WM8904_FLL_CONTROL_3, WM8904_FLL_K(0x8000));
		wm8904_write_register(WM8904_FLL_CONTROL_4, WM8904_FLL_N(0xBB));
		wm8904_write_register(WM8904_FLL_CONTROL_1, WM8904_FLL_FRACN_ENA | 
								WM8904_FLL_ENA);

		delay_ms(5);
		wm8904_write_register(WM8904_CLOCK_RATES_1, WM8904_CLK_SYS_RATE(3) | 
								WM8904_SAMPLE_RATE(5));
		wm8904_write_register(WM8904_CLOCK_RATES_0, 0x0000);
		wm8904_write_register(WM8904_CLOCK_RATES_2,	WM8904_SYSCLK_SRC 
							| WM8904_CLK_SYS_ENA | WM8904_CLK_DSP_ENA);

		wm8904_write_register(WM8904_AUDIO_INTERFACE_1, WM8904_BCLK_DIR | 
								WM8904_AIF_WL_16BIT | WM8904_AIF_FMT_I2S);
		wm8904_write_register(WM8904_AUDIO_INTERFACE_2, WM8904_BCLK_DIV(8));
		wm8904_write_register(WM8904_AUDIO_INTERFACE_3, WM8904_LRCLK_DIR | 
								WM8904_LRCLK_RATE(0x20));

		wm8904_write_register(WM8904_POWER_MANAGEMENT_6, WM8904_DACL_ENA | 
								WM8904_DACR_ENA | WM8904_ADCL_ENA | WM8904_ADCR_ENA);
		delay_ms(5);
		wm8904_write_register(WM8904_ANALOGUE_LEFT_INPUT_0, WM8904_LIN_VOL(0x10));
		wm8904_write_register(WM8904_ANALOGUE_RIGHT_INPUT_0, WM8904_RIN_VOL(0x10));
		wm8904_write_register(WM8904_ANALOGUE_HP_0,
		WM8904_HPL_ENA | WM8904_HPR_ENA);
		wm8904_write_register(WM8904_ANALOGUE_HP_0,
		WM8904_HPL_ENA_DLY | WM8904_HPL_ENA |
		WM8904_HPR_ENA_DLY | WM8904_HPR_ENA);
		wm8904_write_register(WM8904_DC_SERVO_0,
		WM8904_DCS_ENA_CHAN_3 | WM8904_DCS_ENA_CHAN_2 |
		WM8904_DCS_ENA_CHAN_1 | WM8904_DCS_ENA_CHAN_0);
		wm8904_write_register(WM8904_DC_SERVO_1,
		WM8904_DCS_TRIG_STARTUP_3 | WM8904_DCS_TRIG_STARTUP_2 |
		WM8904_DCS_TRIG_STARTUP_1 | WM8904_DCS_TRIG_STARTUP_0);
		delay_ms(100);
		wm8904_write_register(WM8904_ANALOGUE_HP_0,
		WM8904_HPL_ENA_OUTP | WM8904_HPL_ENA_DLY | WM8904_HPL_ENA |
		WM8904_HPR_ENA_OUTP | WM8904_HPR_ENA_DLY | WM8904_HPR_ENA);
		wm8904_write_register(WM8904_ANALOGUE_HP_0,
		WM8904_HPL_RMV_SHORT | WM8904_HPL_ENA_OUTP | WM8904_HPL_ENA_DLY | WM8904_HPL_ENA |
		WM8904_HPR_RMV_SHORT | WM8904_HPR_ENA_OUTP | WM8904_HPR_ENA_DLY | WM8904_HPR_ENA);
		wm8904_write_register(WM8904_ANALOGUE_OUT1_LEFT, WM8904_HPOUT_VU | WM8904_HPOUTL_VOL(0x39));
		wm8904_write_register(WM8904_ANALOGUE_OUT1_RIGHT, WM8904_HPOUT_VU | WM8904_HPOUTR_VOL(0x39));
	}
	else
	{
		wm8904_write_register(WM8904_BIAS_CONTROL_0, WM8904_ISEL_HP_BIAS | WM8904_BIAS_ENA);

		wm8904_write_register(WM8904_VMID_CONTROL_0, WM8904_VMID_BUF_ENA | 	WM8904_VMID_RES_NORMAL | WM8904_VMID_ENA);
		delay_ms(5);

		wm8904_write_register(WM8904_POWER_MANAGEMENT_0, WM8904_INL_ENA | WM8904_INR_ENA);
		wm8904_write_register(WM8904_POWER_MANAGEMENT_0, WM8904_HPL_PGA_ENA | WM8904_HPR_PGA_ENA);
		wm8904_write_register(WM8904_POWER_MANAGEMENT_6, WM8904_DACL_ENA | WM8904_DACR_ENA | WM8904_ADCL_ENA | WM8904_ADCR_ENA);
		delay_ms(100);

		wm8904_write_register(WM8904_CLOCK_RATES_0, 0x845E);
		wm8904_write_register(WM8904_CLOCK_RATES_1,  WM8904_CLK_SYS_RATE(3) | WM8904_SAMPLE_RATE(5));								
		wm8904_write_register(WM8904_CLOCK_RATES_2, WM8904_CLK_SYS_ENA | WM8904_CLK_DSP_ENA);

		wm8904_write_register(WM8904_AUDIO_INTERFACE_1, 0x0002);		
		
		delay_ms(100);
		wm8904_write_register(WM8904_DAC_DIGITAL_1, 0x0000);
		wm8904_write_register(WM8904_ANALOGUE_LEFT_INPUT_0, WM8904_LIN_VOL(0x5));
		wm8904_write_register(WM8904_ANALOGUE_RIGHT_INPUT_0, WM8904_RIN_VOL(0x5));
		wm8904_write_register(WM8904_ANALOGUE_LEFT_INPUT_1, 0x0000);
		wm8904_write_register(WM8904_ANALOGUE_RIGHT_INPUT_1, 0x0000);

		delay_ms(100);
		wm8904_write_register(WM8904_ANALOGUE_OUT1_RIGHT, 0x00AD);
		wm8904_write_register(WM8904_DC_SERVO_0, WM8904_DCS_ENA_CHAN_1 | WM8904_DCS_ENA_CHAN_0);
		wm8904_write_register(WM8904_ANALOGUE_HP_0, 0x00FF);

		wm8904_write_register(WM8904_CHARGE_PUMP_0, WM8904_CP_ENA);
		wm8904_write_register(WM8904_CLASS_W_0, 0x0005);
		delay_ms(100);
	}
}

int main(void)
{
	/* Initialize the SAM system. */
	sysclk_init();
	board_init();

	/* Initialize the console UART. */
	configure_console();
	puts(STRING_HEADER);

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
	pmc_disable_pck(PMC_PCK_2);
	pmc_pck_set_prescaler(PMC_PCK_2, PMC_MCKR_PRES_CLK_1);
	if(PCK2 == PMC_MCKR_CSS_SLOW_CLK)
	{
		pmc_pck_set_source(PMC_PCK_2, PMC_MCKR_CSS_SLOW_CLK);
	}
	else
	{
		pmc_pck_set_source(PMC_PCK_2, PMC_MCKR_CSS_MAIN_CLK);
	}
	pmc_enable_pck(PMC_PCK_2);

	/* Start playing */
	delay_ms(300);
	ssc_enable_tx(SSC);
	xdmac_channel_enable(XDMAC, XDMA_CH_SSC_TX);

	while (1) {
	}
}

/// @cond 0
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/// @endcond
