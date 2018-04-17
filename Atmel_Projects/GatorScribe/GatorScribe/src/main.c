#include "asf.h"
#include "DanLib.h"

void USART_SERIAL_ISR_HANDLER(void)
{
	uint32_t received_byte = 0; 
	uint32_t dw_status = usart_get_status(USART_SERIAL);
	if (dw_status & US_CSR_RXRDY) {
		usart_read(USART_SERIAL, &received_byte);
		
	}
}
static void configure_uart(void)
{
	usart_serial_options_t usart_console_settings = {
		USART_SERIAL_BAUDRATE,
		USART_SERIAL_CHAR_LENGTH,
		USART_SERIAL_PARITY,
		USART_SERIAL_STOP_BIT
	};
	usart_serial_init(USART_SERIAL, &usart_console_settings);
	usart_enable_tx(USART_SERIAL);
	usart_enable_rx(USART_SERIAL);
	usart_enable_interrupt(USART_SERIAL, US_IER_RXRDY);
	NVIC_SetPriority(USART1_IRQn, 2);
	NVIC_ClearPendingIRQ(USART1_IRQn);
	NVIC_EnableIRQ(USART1_IRQn);
}

static void configure_console(void)
{
	const usart_serial_options_t uart_serial_options = {
		.baudrate = CONF_UART_BAUDRATE,
		.charlength = CONF_UART_CHAR_LENGTH,
		.paritytype = CONF_UART_PARITY,
		.stopbits = CONF_UART_STOP_BITS,
	};

	/* Configure console UART. */
	sysclk_enable_peripheral_clock(CONSOLE_UART_ID);
	stdio_serial_init(CONF_UART, &uart_serial_options);
}

extern uint32_t max_power; 
int main(void)
{
	sysclk_init();
	board_init();
	//lcd_init(); 
	audio_init();
	//configure_uart(); // can use this for just rx! 
	configure_console(); 
	
	/* Initial Gatorscribe params */ 
	uint32_t bpm = 100;
	midi_instrument_t playback_instrument = PIANO;
	time_signature_t time_signature = {4,4, FOUR_FOUR};
	key_signature_t key_signature = {C_MAJOR, MAJOR};
	char title[MAX_TITLE_SIZE] = "Title Here";
	
	midi_event_t events_in_time[MAX_NUM_EVENTS];
	uint32_t number_of_events = 0; 
	max_power = 100; // minimum initial max power

	while(1)
	{
		// wait for uart to get bpm, playback_instrument, time_signature, key_signature, and title 
		
		start_recording(events_in_time, &number_of_events, bpm, playback_instrument, time_signature, key_signature, title);
		write_midi_file(bpm, playback_instrument, &time_signature, &key_signature, title, events_in_time, number_of_events);
		while(1); 
	}
}

