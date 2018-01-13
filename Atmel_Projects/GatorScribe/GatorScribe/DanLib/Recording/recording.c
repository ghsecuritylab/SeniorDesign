/*
 * recording.c
 *
 * Created: 1/10/2018 9:34:37 AM
 *  Author: Daniel Gonzalez
 */ 

#include <asf.h>
#include "recording.h"
#include "rtt.h"

/**************************** Defines Start *********************************/
/* How many samples to process for a 16th note */ 
#define PITCH_BUF_SIZE 2048

/**************************** Defines End *********************************/

/**************************** Global Variables Start *********************************/
volatile bool recording = false;
volatile bool metronome_on = false; 
extern volatile bool outOfTime;
/**************************** Global Variables End *********************************/

/**************************** Private Variables Start *********************************/
static volatile bool note_16_received = false;
static volatile midi_note_t oldNote = {0, 0};
static volatile midi_note_t note = {-1,0};

static volatile uint8_t sixteenth_note_cnt = 0; 

//static midi_note_t notes_in_time[1000];
//static midi_event_t events_in_time[1000];
/**************************** Private Variables End *********************************/

/**************************** Private Functions Start *********************************/

/**
 * \brief RTT configuration function.
 *
 * Configure the RTT to generate a one second tick, which triggers the RTTINC
 * interrupt.
 * 
 * RTT -> 32KHz counter 
 */
static void configure_rtt(uint32_t count_8_note)
{
	uint32_t ul_previous_time;

	/* Configure RTT for a tick interrupt per 16th note specified by count */
	rtt_sel_source(RTT, false);
	rtt_init(RTT, count_8_note);

	ul_previous_time = rtt_read_timer_value(RTT);
	while (ul_previous_time == rtt_read_timer_value(RTT));

	/* Enable RTT interrupt */
	NVIC_DisableIRQ(RTT_IRQn);
	NVIC_ClearPendingIRQ(RTT_IRQn);
	NVIC_SetPriority(RTT_IRQn, 0);
	NVIC_EnableIRQ(RTT_IRQn);
	rtt_enable_interrupt(RTT, RTT_MR_RTTINCIEN);
}

/**************************** Private Functions End *********************************/

/**************************** RTT Interrupt Handler Start *********************************/

/**
 * \brief Interrupt handler for the RTT.
 *
 * Display the current time on the terminal.
 */
void RTT_Handler(void)
{
	uint32_t ul_status;

	/* Get RTT status */
	ul_status = rtt_get_status(RTT);

	/* Timer overflow */
	if ((ul_status & RTT_SR_RTTINC) == RTT_SR_RTTINC) 
	{	
		if (processPingMode)
		{
			processBuffer = processPingBuffer;
			fillBuffer = processPongBuffer;
			processPingMode = !processPingMode;
		}
		else
		{
			processBuffer = processPongBuffer;
			fillBuffer = processPingBuffer;
			processPingMode = !processPingMode;
		}
		note_16_received = true; 
		
		if ((sixteenth_note_cnt & 0x03) == 3)
		{
			metronome_on = true; 
			sixteenth_note_cnt = 0; 
		}
		else
		{
			sixteenth_note_cnt++; 
		}
	}
}

/**************************** RTT Interrupt Handler End *********************************/

/**************************** Public Functions Start *********************************/
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
void start_recording(uint32_t bpm, midi_instrument_t playback_instrument, time_signature_t time_signature , key_signature_t key_signature, char *title)
{
	recording = true; 
	char str[20]; 
	
	uint32_t timer_count_for_16th_note = 32768 * 15 / bpm; 
	configure_console(); 
	
	sixteenth_note_cnt = 0; 
	metronome_on = false; 
	configure_rtt(timer_count_for_16th_note);
	
	//yin_init((uint32_t)(23250.0f * 30.0f /(float)bpm), 0.05);
	//yin_init(timer_count_for_16th_note/4, 0.05);
	uint32_t yin_buffer_size = (uint32_t)(-20*bpm + 2000 + 1600); // allows for 1600 at 100bpm, and 2200 at 70bpm 
	yin_init(yin_buffer_size, 0.2);
	//yin_init(2000, 0.05);
	// not fast enough for 2000 @ 100bpm 

	// todo: count down metronome per time signature 
	// if current note length is 16th and next 16th is none, make the note an 8th 
	
	while(1)
	{
		if (note_16_received)
		{
			get_midi_note_name(str, note.note_number);
			printf("Beat %d : %s\n\r", ((sixteenth_note_cnt-2) & 3) + 1, str); 
			//oldNote.note_number = note.note_number;
			
			get_midi_note((int16_t *)&processBuffer[600], (midi_note_t *)&note);
			
			
				
			note_16_received = false;
		}
	}
	
	yin_free_buffer(); 
}

/**************************** Public Functions End *********************************/
