#include "asf.h"
#include "DanLib.h"
#include "ioport.h"
#include "pio.h"

static time_signature_t time_signatures[] =
{
	{4,4, FOUR_FOUR},
	{3,4, THREE_FOUR},
	{2,4, TWO_FOUR},
	{6,8, SIX_EIGHT}
};

static key_signature_t key_signatures[] =
{
	{C_MAJOR,  MAJOR},
	{C_MINOR,  MINOR},
	{Cs_MAJOR, MAJOR},
	{Cs_MINOR, MINOR},
	{Db_MAJOR, MAJOR},
	{D_MAJOR,  MAJOR},
	{D_MINOR,  MINOR},
	{Ds_MINOR, MINOR},
	{Eb_MAJOR, MAJOR},
	{Eb_MINOR, MINOR},
	{E_MAJOR,  MAJOR},
	{E_MINOR,  MINOR},
	{F_MAJOR,  MAJOR},
	{F_MINOR,  MINOR},
	{Fs_MAJOR, MAJOR},
	{Fs_MINOR, MINOR},
	{Gb_MAJOR, MAJOR},
	{G_MAJOR,  MAJOR},
	{G_MINOR,  MINOR},
	{Gs_MINOR, MINOR},
	{Ab_MAJOR, MAJOR},
	{Ab_MINOR, MINOR},
	{A_MAJOR,  MAJOR},
	{A_MINOR,  MINOR},
	{As_MINOR, MINOR},
	{Bb_MAJOR, MAJOR},
	{Bb_MINOR, MINOR},
	{B_MAJOR,  MAJOR},
	{B_MINOR,  MINOR},
	{Cb_MAJOR, MAJOR}
};

/* Initial Gatorscribe params */
volatile uint32_t bpm;
volatile midi_instrument_t playback_instrument; 
volatile time_signature_t time_sig; 
volatile key_signature_t key_sig;
volatile char title[MAX_TITLE_SIZE] = "Title Here";
volatile bool start; 
volatile bool tap; 
volatile uint32_t millisecond_cnt;

	
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
	
	/* Configure stdio UART. */
	sysclk_enable_peripheral_clock(CONSOLE_UART_ID);
	stdio_serial_init(CONF_UART, &usart_console_settings);
	
	usart_enable_tx(USART_SERIAL);
	usart_enable_rx(USART_SERIAL);
	usart_enable_interrupt(USART_SERIAL, US_IER_RXRDY);
	NVIC_SetPriority(USART1_IRQn, 2);
	NVIC_ClearPendingIRQ(USART1_IRQn);
	NVIC_EnableIRQ(USART1_IRQn);
}

void SysTick_Handler(void)
{
	millisecond_cnt++; 
}

static void start_handler(const uint32_t id, const uint32_t index)
{
	if ((id == ID_PIOA) && (index == PIO_PA9)){
		start = !start; 
	}
}

static void tap_handler(const uint32_t id, const uint32_t index)
{
	if ((id == ID_PIOC) && (index == PIO_PC13)){
		if (millisecond_cnt > 100)
		{
			tap = true; 
		}
	}
}


extern uint32_t max_power; 
int main(void)
{
	sysclk_init();
	board_init();
	//lcd_init(); 
	audio_init();
	configure_uart(); 
	
	bpm = 100;
	playback_instrument = PIANO;
	time_sig = time_signatures[0];
	key_sig = key_signatures[0];
	start = false; 
	tap = false; 
	millisecond_cnt = 0; 
	
	pio_set_input(PIOA, PIO_PA9, PIO_PULLUP); 
	pio_set_debounce_filter(PIOA, PIO_PA9, 10); 
	pio_handler_set(PIOA, ID_PIOA, PIO_PA9, PIO_IT_EDGE, &start_handler);
	pio_handler_set_priority(PIOA, PIOA_IRQn, 2);
	pio_enable_interrupt(PIOA, PIO_PA9);
	NVIC_ClearPendingIRQ(PIOA_IRQn); 
	NVIC_EnableIRQ(PIOA_IRQn);
	
	pio_set_input(PIOC, PIO_PC13, PIO_PULLUP);
	pio_set_debounce_filter(PIOC, PIO_PC13, 10);
	pio_handler_set(PIOC, ID_PIOC, PIO_PC13, PIO_IT_FALL_EDGE, &tap_handler);
	pio_handler_set_priority(PIOC, PIOC_IRQn, 2);
	pio_enable_interrupt(PIOC, PIO_PC13);
	NVIC_ClearPendingIRQ(PIOC_IRQn);
	NVIC_EnableIRQ(PIOC_IRQn);
	
	/* Initialize and start 1ms systick timer */
	SysTick_Config(sysclk_get_cpu_hz()/1000);
	
	midi_event_t events_in_time[MAX_NUM_EVENTS];
	uint32_t number_of_events = 0; 
	max_power = 100; // minimum initial max power
	
	uint32_t millisecond_cnt_buffer[NUM_AVG_TAPS]; 
	
	while(1)
	{
		for (uint32_t i = 0; i < NUM_AVG_TAPS; i++)
			millisecond_cnt_buffer[i] = 0;
		uint32_t bpm_total = 0;
		bool first_tap = false; 
		uint32_t curr_tap_idx = 0; 
		while(!start)
		{
			if (tap)
			{
				if (first_tap == true)
				{
					bpm_total -= millisecond_cnt_buffer[curr_tap_idx];
					millisecond_cnt_buffer[curr_tap_idx] = millisecond_cnt; 
					bpm_total += millisecond_cnt_buffer[curr_tap_idx++];  
					bpm = 60000 / (bpm_total / 4);
					if (curr_tap_idx == NUM_AVG_TAPS) 
						curr_tap_idx = 0; 
				}
				else 
					first_tap = true; 
				
				tap = false;
				millisecond_cnt = 0;
			}
		} 
		delay_ms(100); 
		start = true; 
		DISABLE_SYSTICK(); 
		start_recording(events_in_time, &number_of_events, bpm, (midi_instrument_t)playback_instrument, (time_signature_t)time_sig, (key_signature_t)key_sig, (char *)title);
		write_midi_file(bpm, (midi_instrument_t)playback_instrument, (time_signature_t *)&time_sig,  (key_signature_t *)&key_sig, (char *)title, events_in_time, number_of_events);
		delay_ms(100); 
		start = false; 
		tap = false; 
		millisecond_cnt = 0; 
		ENABLE_SYSTICK(); 
		//SysTick_Config(sysclk_get_cpu_hz()/1000);
	}
}

