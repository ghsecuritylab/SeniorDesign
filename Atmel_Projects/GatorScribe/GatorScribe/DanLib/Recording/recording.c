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

/* Used for displaying current note for debug */ 
#define LCD_DELAY 50
/**************************** Defines End *********************************/

/**************************** Global Variables Start *********************************/
volatile bool recording = false;
extern volatile bool outOfTime;
/**************************** Global Variables End *********************************/

/**************************** Private Variables Start *********************************/
static volatile bool note_16_received = false;
static volatile midi_note_t oldNote = {0, 0};
static volatile midi_note_t note = {-1,0};
static volatile int lcd_refresh = 0;

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
static void configure_rtt(uint32_t count_16_note)
{
	uint32_t ul_previous_time;

	/* Configure RTT for a tick interrupt per 16th note specified by count */
	rtt_sel_source(RTT, false);
	rtt_init(RTT, count_16_note);

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
	}
}

/**************************** RTT Interrupt Handler End *********************************/

/**************************** Public Functions Start *********************************/

void start_recording(uint32_t bpm, midi_instrument_t playback_instrument, time_signature_t time_signature , key_signature_t key_signature, char *title)
{
	recording = true; 
	char str[20]; 
	yin_init(PITCH_BUF_SIZE, YIN_DEFAULT_THRESHOLD);
	
	uint32_t timer_count_for_16th_note = 32768 * 15 / bpm; 
	
	configure_rtt(timer_count_for_16th_note);
	
	// count down metronome per time signature 
	// once the one beat hits, switch the dma buffer 
	// dma buffer starts getting filled 
	// once the next 16th note hits, switch buffers and process previous buffer
	// this is accurate as shit 
	
	while(1)
	{
		lcd_refresh++;
		if (lcd_refresh == LCD_DELAY)
		{
			if (oldNote.note_number != note.note_number)
			{
				get_midi_note_name(str, oldNote.note_number);
				gfx_draw_string_aligned((const char *)str,
					150, 150, &sysfont,
					GFX_COLOR_TRANSPARENT, GFX_COLOR_BLACK,
					TEXT_POS_LEFT, TEXT_ALIGN_LEFT);
				
				get_midi_note_name(str, note.note_number);
				gfx_draw_string_aligned((const char *)str,
					150, 150, &sysfont,
					GFX_COLOR_TRANSPARENT, GFX_COLOR_WHITE,
					TEXT_POS_LEFT, TEXT_ALIGN_LEFT);
				oldNote.note_number = note.note_number;
			}
			lcd_refresh = 0;
		}
		if (note_16_received)
		{
			get_midi_note((int16_t *)processBuffer, (midi_note_t *)&note);	
			note_16_received = false;
		}
	}
	
	yin_free_buffer(); 
}

/**************************** Public Functions End *********************************/
