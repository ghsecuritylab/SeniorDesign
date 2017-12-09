/*
 * WM8904_Driver.c
 *
 * Created: 12/8/2017 7:33:04 PM
 *  Author: Daniel Gonzalez
 */ 

#include <asf.h>
#include "WM8904_Driver.h"

void configure_ssc(void)
{
	clock_opt_t tx_clk_option, rx_clk_option;
	data_frame_opt_t tx_data_frame_option, rx_data_frame_option;

	/* Initialize clock */
	pmc_enable_periph_clk(ID_SSC);

	/* Reset SSC */
	ssc_reset(SSC);

	/* Transmitter clock mode configuration. */
	tx_clk_option.ul_cks = SSC_TCMR_CKS_TK;
	tx_clk_option.ul_cko = SSC_TCMR_CKO_NONE;
	tx_clk_option.ul_cki = 0;
	tx_clk_option.ul_ckg = SSC_TCMR_CKG_CONTINUOUS;
	tx_clk_option.ul_start_sel = SSC_TCMR_START_TF_EDGE;
	tx_clk_option.ul_sttdly = SSC_TCMR_STTDLY(1);
	tx_clk_option.ul_period = SSC_TCMR_PERIOD(0);
	/* Transmitter frame mode configuration. */
	tx_data_frame_option.ul_datlen = BITS_BY_SLOT - 1;
	tx_data_frame_option.ul_msbf = SSC_TFMR_MSBF;
	tx_data_frame_option.ul_datnb = 0;
	tx_data_frame_option.ul_fslen = SLOT_BY_FRAME - 1;
	tx_data_frame_option.ul_fslen_ext = 0;
	tx_data_frame_option.ul_fsos = SSC_TFMR_FSOS_NONE;
	tx_data_frame_option.ul_fsedge = SSC_TFMR_FSEDGE_POSITIVE;
	/* Configure the SSC transmitter to I2S mode. */
	ssc_set_transmitter(SSC, &tx_clk_option, &tx_data_frame_option);

	/* Receiver clock mode configuration. */
	rx_clk_option.ul_cks = SSC_RCMR_CKS_TK;
	rx_clk_option.ul_cko = SSC_RCMR_CKO_NONE;
	rx_clk_option.ul_cki = SSC_RCMR_CKI;
	rx_clk_option.ul_ckg = SSC_RCMR_CKG_CONTINUOUS;
	rx_clk_option.ul_start_sel = SSC_RCMR_START_RF_EDGE;
	rx_clk_option.ul_sttdly = SSC_RCMR_STTDLY(1);
	rx_clk_option.ul_period = SSC_RCMR_PERIOD(0);
	/* Receiver frame mode configuration. */
	rx_data_frame_option.ul_datlen = BITS_BY_SLOT - 1;
	rx_data_frame_option.ul_msbf = SSC_RFMR_MSBF;
	rx_data_frame_option.ul_datnb = 0;
	rx_data_frame_option.ul_fslen = SLOT_BY_FRAME - 1;
	rx_data_frame_option.ul_fslen_ext = 0;
	rx_data_frame_option.ul_fsos = SSC_TFMR_FSOS_NONE;
	rx_data_frame_option.ul_fsedge = SSC_TFMR_FSEDGE_POSITIVE;
	/* Configure the SSC transmitter to I2S mode. */
	ssc_set_receiver(SSC, &rx_clk_option, &rx_data_frame_option);

	/* Disable transmitter first */
	ssc_disable_tx(SSC);
	ssc_disable_rx(SSC);

	/* Disable All Interrupt */
	ssc_disable_interrupt(SSC, 0xFFFFFFFF);
}


void configure_codec(void)
{
	uint16_t data = 0;
	/* check that WM8904 is present */
	wm8904_write_register(WM8904_SW_RESET_AND_ID, 0xFFFF);
	data = wm8904_read_register(WM8904_SW_RESET_AND_ID);
	if(data != 0x8904) {
		printf("WM8904 not found!\n\r");
		while(1);
	}

	wm8904_write_register(WM8904_BIAS_CONTROL_0, WM8904_ISEL_HP_BIAS);
	wm8904_write_register(WM8904_VMID_CONTROL_0, WM8904_VMID_BUF_ENA |
	WM8904_VMID_RES_FAST | WM8904_VMID_ENA);
	delay_ms(5);
	wm8904_write_register(WM8904_VMID_CONTROL_0, WM8904_VMID_BUF_ENA |
	WM8904_VMID_RES_NORMAL | WM8904_VMID_ENA);
	wm8904_write_register(WM8904_BIAS_CONTROL_0, WM8904_ISEL_HP_BIAS | WM8904_BIAS_ENA);
	wm8904_write_register(WM8904_POWER_MANAGEMENT_0, WM8904_INL_ENA | WM8904_INR_ENA);
	wm8904_write_register(WM8904_POWER_MANAGEMENT_2, WM8904_HPL_PGA_ENA | WM8904_HPR_PGA_ENA);
	wm8904_write_register(WM8904_DAC_DIGITAL_1, WM8904_DEEMPH(0));
	wm8904_write_register(WM8904_ANALOGUE_OUT12_ZC, 0x0000);
	wm8904_write_register(WM8904_CHARGE_PUMP_0, WM8904_CP_ENA);
	wm8904_write_register(WM8904_CLASS_W_0, WM8904_CP_DYN_PWR);
	wm8904_write_register(WM8904_FLL_CONTROL_1, 0x0000);
	wm8904_write_register(WM8904_FLL_CONTROL_2, WM8904_FLL_OUTDIV(7)| WM8904_FLL_FRATIO(4));
	wm8904_write_register(WM8904_FLL_CONTROL_3, WM8904_FLL_K(0x8000));
	wm8904_write_register(WM8904_FLL_CONTROL_4, WM8904_FLL_N(0xBB));
	wm8904_write_register(WM8904_FLL_CONTROL_1, WM8904_FLL_FRACN_ENA | WM8904_FLL_ENA);
	delay_ms(5);
	wm8904_write_register(WM8904_CLOCK_RATES_1, WM8904_CLK_SYS_RATE(3) | WM8904_SAMPLE_RATE(5));
	wm8904_write_register(WM8904_CLOCK_RATES_0, 0x0000);
	wm8904_write_register(WM8904_CLOCK_RATES_2,
	WM8904_SYSCLK_SRC | WM8904_CLK_SYS_ENA | WM8904_CLK_DSP_ENA);
	wm8904_write_register(WM8904_AUDIO_INTERFACE_1, WM8904_BCLK_DIR | WM8904_AIF_FMT_I2S);
	wm8904_write_register(WM8904_AUDIO_INTERFACE_2, WM8904_BCLK_DIV(8));
	wm8904_write_register(WM8904_AUDIO_INTERFACE_3, WM8904_LRCLK_DIR | WM8904_LRCLK_RATE(0x20));
	wm8904_write_register(WM8904_POWER_MANAGEMENT_6,
	WM8904_DACL_ENA | WM8904_DACR_ENA |
	WM8904_ADCL_ENA | WM8904_ADCR_ENA);
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
	delay_ms(100);
}
