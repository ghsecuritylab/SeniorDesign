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
 *  \page gmac_lwip GMAC lwIP Example
 *
 *  \section Purpose
 *
 *  This project implements webserver example by using lwIP stack, It enables
 *  the device to act as a web server, sending a very short page when accessed
 *  through a browser.
 *
 *  \section Requirements
 *
 * This package can be used with SAMV71 Xplained Ultra board or SAME70 Xplained board.
 *
 *  \section Description
 *
 *  Please refer to the lwIP documentation for more information about
 *  the TCP/IP stack and the webserver example.
 *
 *  By default, the example does not use DHCP. If you want to use DHCP,
 *  please open file lwipopts.h and define "LWIP_DHCP" and "LWIP_UDP" to 1.
 *
 *  \section Usage
 *
 *  -# Build the program and download it inside the board.
 *     Please refer to the Getting Started with SAM V71/E70 Microcontrollers.pdf
 *  -# On the computer, open and configure a terminal application
 *     (e.g. HyperTerminal on Microsoft Windows) with these settings:
 *    - 115200 baud rate
 *    - 8 bits of data
 *    - No parity
 *    - 1 stop bit
 *    - No flow control
 *  -# Connect an Ethernet cable between the evaluation board and the network.
 *      The board may be connected directly to a computer; in this case,
 *      make sure to use a cross/twisted wired cable such as the one provided
 *      with the evaluation kit.
 *  -# Start the application. It will display the following message on the
 *  terminal:
 *    \code
 *    -- GMAC lwIP Example xxx --
 *    -- xxxxxx-xx
 *    -- Compiled: xxx xx xxxx xx:xx:xx --
 *      MAC 3a:1f:34:08:54:54
 *    - Host IP  192.168.1.3
 *    - Gateway IP 192.168.1.2
 *    - Net Mask 255.255.255.0
 *    \endcode
 * -# Type the IP address (Host IP in the debug log) of the device in a web
 *    browser, like this:
 *    \code
 *    http://192.168.1.3
 *    \endcode
 *    The page generated by lwIP will appear in the web browser, like below:
 *    \code
 *    Small test page.#
 *    \endcode
 *
 *  \note
 *  Make sure the IP address of the device and the computer are in the same
 *  network.
 *
 *  \section References
 *  - gmac_lwip/main.c
 *  - gmacb.h
 *  - gmacd.h
 *  - gmac.h
 *  - liblwip.h
 */

/** \file
 *
 *  This file contains all the specific code for the gmac_lwip example.
 *
 */

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/


#include <board.h>
#include <liblwip.h>
#include "httpd.h"

/*---------------------------------------------------------------------------
 *         Local Define
 *---------------------------------------------------------------------------*/

/** TWI clock frequency in Hz. */
#define TWCK            400000
/** Slave address of twi_eeprom AT24MAC.*/
#define AT24MAC_SERIAL_NUM_ADD  0x5F
/** Page size of an AT24MAC402 chip (in bytes)*/
#define PAGE_SIZE       16
/** Page numbers of an AT24MAC402 chip */
#define EEPROM_PAGES    16
/** EEPROM Pins definition */
#define BOARD_PINS_TWI_EEPROM PINS_TWI0
/** TWI0 peripheral ID for EEPROM device*/
#define BOARD_ID_TWI_EEPROM   ID_TWIHS0
/** TWI0 base address for EEPROM device */
#define BOARD_BASE_TWI_EEPROM TWIHS0
/*----------------------------------------------------------------------------
 *        Types
 *----------------------------------------------------------------------------*/

/* Timer for calling lwIP tmr functions without system */
typedef struct _timers_info {
	uint32_t timer;
	uint32_t timer_interval;
	void (*timer_func)(void);
} timers_info;

/*---------------------------------------------------------------------------
 *         Variables
 *---------------------------------------------------------------------------*/
/** TWI driver instance.*/
static Twid twid;

/* lwIP tmr functions list */
static timers_info timers_table[] = {
	/* LWIP_TCP */
	{ 0, TCP_FAST_INTERVAL,     tcp_fasttmr},
	{ 0, TCP_SLOW_INTERVAL,     tcp_slowtmr},
	/* LWIP_ARP */
	{ 0, ARP_TMR_INTERVAL,      etharp_tmr},
	/* LWIP_DHCP */
#if LWIP_DHCP
	{ 0, DHCP_COARSE_TIMER_SECS, dhcp_coarse_tmr},
	{ 0, DHCP_FINE_TIMER_MSECS,  dhcp_fine_tmr},
#endif
};

/** The PINs for TWI*/
static const Pin twiPins[]      = BOARD_PINS_TWI_EEPROM;

/** The MAC address used for demo */
static uint8_t GMacAddress[6] = {0x3a, 0x1f, 0x34, 0x08, 0x54, 0x54};

/* The IP address used for demo (ping ...) */
static uint8_t gIpAddress[4] = {192, 168, 1, 3};

/* Set the default router's IP address. */
static const uint8_t gGateWay[4] = {192, 168, 1, 2};
/* The NetMask address */
static const uint8_t gNetMask[4] = {255, 255, 255, 0};

/*----------------------------------------------------------------------------
 *        Local functions
 *----------------------------------------------------------------------------*/
/**
 * Process timing functions
 */
static void timers_update(void)
{
	static uint32_t last_time;
	uint32_t cur_time, time_diff, idxtimer;
	timers_info *ptmr_inf;

	cur_time = sys_get_ms();

	if (cur_time >= last_time)
		time_diff = cur_time - last_time;
	else
		time_diff = 0xFFFFFFFF - last_time + cur_time;

	if (time_diff) {
		last_time = cur_time;

		for (idxtimer = 0;
			idxtimer < (sizeof(timers_table) / sizeof(timers_info));
			idxtimer ++) {
			ptmr_inf = &timers_table[idxtimer];
			ptmr_inf->timer += time_diff;

			if (ptmr_inf->timer > ptmr_inf->timer_interval) {
				if (ptmr_inf->timer_func)
					ptmr_inf->timer_func();

				ptmr_inf->timer -= ptmr_inf->timer_interval;
			}
		}
	}
}

/*----------------------------------------------------------------------------
 *        Exported functions
 *----------------------------------------------------------------------------*/

/**
 * Gmac interrupt handler
 */
void GMAC_Handler(void)
{
	GMACD_Handler(&gGmacd, GMAC_QUE_0);
}

/**
 *  \brief gmac_lwip example entry point.
 *
 *  \return Unused (ANSI-C compatibility).
 */
int main(void)
{
	struct ip_addr ipaddr, netmask, gw;
	struct netif NetIf, *netif;

#if LWIP_DHCP
	u8_t dhcp_state = DHCP_INIT;
#endif
	uint8_t  OrigiGMacAddr[16];
	uint32_t i;

	/* Disable watchdog */
	WDT_Disable(WDT);

	printf("-- GMAC lwIP Example %s --\n\r", SOFTPACK_VERSION);
	printf("-- %s\n\r", BOARD_NAME);
	printf("-- Compiled: %s %s With %s--\n\r", __DATE__, __TIME__ ,
			COMPILER_NAME);

	SCB_EnableICache();
	SCB_EnableDCache();

	/* Configure systick for 1 ms. */
	TimeTick_Configure();

	/* Configure TWI pins. */
	PIO_Configure(twiPins, PIO_LISTSIZE(twiPins));
	/* Enable TWI */
	PMC_EnablePeripheral(BOARD_ID_TWI_EEPROM);
	TWI_ConfigureMaster(BOARD_BASE_TWI_EEPROM, TWCK, BOARD_MCK);
	TWID_Initialize(&twid, BOARD_BASE_TWI_EEPROM);
	/* Display MAC & IP settings */
	TWID_Read(&twid, AT24MAC_SERIAL_NUM_ADD, 0x9A, 1, OrigiGMacAddr, PAGE_SIZE, 0);

	if ((OrigiGMacAddr[0] == 0xFC) && (OrigiGMacAddr[1] == 0xC2)
		&& (OrigiGMacAddr[2] == 0x3D)) {
		for (i = 0; i < 6; i++)
			GMacAddress[i] = OrigiGMacAddr[i];
	}

	printf("-- MAC %x:%x:%x:%x:%x:%x\n\r",
			GMacAddress[0], GMacAddress[1], GMacAddress[2],
			GMacAddress[3], GMacAddress[4], GMacAddress[5]);

#if !LWIP_DHCP
	printf(" - Host IP  %d.%d.%d.%d\n\r",
			gIpAddress[0], gIpAddress[1],
			gIpAddress[2], gIpAddress[3]);
	printf(" - GateWay IP  %d.%d.%d.%d\n\r",
			gGateWay[0], gGateWay[1], gGateWay[2], gGateWay[3]);
	printf(" - Net Mask  %d.%d.%d.%d\n\r",
			gNetMask[0], gNetMask[1], gNetMask[2], gNetMask[3]);
#else
	printf(" - DHCP Enabled\n\r");
#endif

	/* Initialize system timing */
	sys_init_timing();

	/* Initialize lwIP modules */
	lwip_init();

	/* Initialize net interface for lwIP */
	gmacif_setmac((u8_t *)GMacAddress);

#if !LWIP_DHCP
	IP4_ADDR(&gw, gGateWay[0], gGateWay[1], gGateWay[2], gGateWay[3]);
	IP4_ADDR(&ipaddr, gIpAddress[0], gIpAddress[1], gIpAddress[2], gIpAddress[3]);
	IP4_ADDR(&netmask, gNetMask[0], gNetMask[1], gNetMask[2], gNetMask[3]);
#else
	IP4_ADDR(&gw, 0, 0, 0, 0);
	IP4_ADDR(&ipaddr, 0, 0, 0, 0);
	IP4_ADDR(&netmask, 0, 0, 0, 0);
#endif
	netif = netif_add(&NetIf, &ipaddr, &netmask, &gw, NULL, gmacif_init, ip_input);
	netif_set_default(netif);
	netif_set_up(netif);

	/* Initialize http server application */
	if (ERR_OK != httpd_init()) {
		printf("httpd_init ERR_OK!");
		return -1;
	}

	printf("Type the IP address of the device in a web browser,\
		http://%d.%d.%d.%d \n\r",
			gIpAddress[0], gIpAddress[1], gIpAddress[2], gIpAddress[3]);

	while (1) {
		/* Run periodic tasks */
		timers_update();
		/* Run polling tasks */
		gmacif_poll(netif);
	}
}
