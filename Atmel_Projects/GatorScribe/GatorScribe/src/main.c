#include "asf.h"
#include "DanLib.h"


/**
 * \brief Configure the console UART.
 *
 *   - 115200 baud rate
 *   - 8 bits of data
 *   - No parity
 *   - 1 stop bit
 *   - No flow control
 */
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

int main(void)
{
	sysclk_init();
	board_init();
	lcd_init(); 
	audio_init();
	configure_console();
	
	/* Initial Gatorscribe params */ 
	uint32_t bpm = 100;
	midi_instrument_t playback_instrument = PIANO;
	time_signature_t time_signature = {4,4, FOUR_FOUR};
	key_signature_t key_signature = {C_MAJOR, MAJOR};
	char title[MAX_TITLE_SIZE] = "Title Here";
	
	midi_note_t notes_in_time[1000];
	midi_event_t events_in_time[1000];
	uint32_t number_of_events = 0; 

	while(1)
	{
		main_menu(&bpm, &playback_instrument, &time_signature, &key_signature, &title[0]);
		gfx_draw_filled_rect(0, 0, gfx_get_width(), gfx_get_height(), GFX_COLOR_BLACK);
		start_recording(notes_in_time, bpm, playback_instrument, time_signature, key_signature, title);
		convert_midi_notes_to_events(notes_in_time, events_in_time, &number_of_events); 
		write_midi_file(bpm, playback_instrument, &time_signature, &key_signature, title, events_in_time, number_of_events);
	}
}

