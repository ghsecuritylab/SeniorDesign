/*
 * recording.c
 *
 * Created: 1/10/2018 9:34:37 AM
 *  Author: Daniel Gonzalez
 */ 

#include <asf.h>
#include "recording.h"
#include "rtt.h"
#include "pitchyinfast.h"

void touch_interrupt_handler(uint32_t x, uint32_t y); 

/**************************** Defines Start *********************************/
/* How many samples to process for a 16th note */ 
#define PITCH_BUF_SIZE 2048

/**************************** Defines End *********************************/

/**************************** Global Variables Start *********************************/
volatile bool recording = false;
volatile bool metronome_on = false; 
extern volatile bool outOfTime;
volatile bool one_beat = false;  
volatile bool up_beat = false; 
/**************************** Global Variables End *********************************/

/**************************** Private Variables Start *********************************/
static volatile bool note_16_received = false;

static volatile uint8_t sixteenth_note_cnt = 0; 
static volatile uint8_t beat_cnt = 0; 
static uint8_t number_of_beats_in_a_measure = 0; 

static time_signature_identifier_t time_sig = 0; 

/**************************** Private Variables End *********************************/

void touch_interrupt_handler(uint32_t x, uint32_t y)
{
	touch_t touched_point;
	touch_handler(&touched_point);
	recording = false; 
}

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
		one_beat = false; 
		up_beat = false; 
		
		// One beat, or quarter note has passed 
		if (sixteenth_note_cnt == 4)
		{
			metronome_on = true;
			sixteenth_note_cnt = 1;
			
			if (beat_cnt == number_of_beats_in_a_measure)
			{
				one_beat = true;
				beat_cnt = 1;
			}
			else
			{
				beat_cnt++;
			}
		}
		else
		{
			// Check for every other 16th note -> eigth note 
			if (sixteenth_note_cnt == 2)
			{
				metronome_on = true;
				up_beat = true;
			}
			sixteenth_note_cnt++;
		}
	}
}

/**************************** RTT Interrupt Handler End *********************************/

/**************************** Public Functions Start *********************************/
#define NOTE_BUF_SIZE 128 
#define NOTE_MASK (NOTE_BUF_SIZE-1)
static midi_note_t notes[NOTE_BUF_SIZE]; 
extern volatile int clickIdx; 
extern volatile bool start; 
void start_recording(midi_event_t *events, uint32_t *number_of_events, uint32_t bpm, midi_instrument_t playback_instrument, time_signature_t time_signature , key_signature_t key_signature, char *title, aubio_pitchyinfast_t *yin)
{		
	/******** midi event variables *********/ 
	*number_of_events = 0; 
	float current_rhythm = 0.25;  // 16th note
	int note_cnt = 0; 
	
	uint32_t midi_note_offset = 0; 
	if (playback_instrument == GUITAR)
		midi_note_offset = 12; 
	
	time_sig = time_signature.sig;
	if (time_sig == FOUR_FOUR)
		number_of_beats_in_a_measure = 4;
	else if (time_sig == THREE_FOUR)
		number_of_beats_in_a_measure = 3;
	else if (time_sig == TWO_FOUR)
		number_of_beats_in_a_measure = 2;
	else // 6/8
		number_of_beats_in_a_measure = 6; 
	
	sixteenth_note_cnt = 4; 
	beat_cnt = number_of_beats_in_a_measure; 
	one_beat = false; 
	up_beat = false; 
	metronome_on = false;
	recording = true;
	clickIdx = 0; 
	
	configure_rtt(32768 * 15 / bpm);
	
	// Wait a measure 
	int i; 
	for (i = 0; i < 4*number_of_beats_in_a_measure + 1;i++)
	{
		while(!note_16_received); 
		note_16_received = 0; 
	}

	uint32_t record_cnt = 0;
	while(recording && *number_of_events < MAX_NUM_EVENTS-1 && start == true)
	{
		if (note_16_received)
		{
			record_cnt++;
			get_midi_note((float32_t *)&processBuffer[0], &notes[note_cnt & NOTE_MASK], yin);
			if (notes[note_cnt & NOTE_MASK].note_number != NO_NOTE)
				notes[note_cnt & NOTE_MASK].note_number += midi_note_offset; 
			
			if (note_cnt > 0)
			{
				if (notes[note_cnt & NOTE_MASK].note_number != notes[(note_cnt-1) & NOTE_MASK].note_number 
					|| notes[note_cnt & NOTE_MASK].velocity > 1.3f*notes[(note_cnt-1) & NOTE_MASK].velocity)
				{
					events[*number_of_events].note_number = notes[(note_cnt-1) & NOTE_MASK].note_number;
					events[*number_of_events].velocity = 64;
					events[*number_of_events].rhythm = current_rhythm;
					(*number_of_events)++;
					current_rhythm = 0.25f;
				}
				else
				{
					current_rhythm += 0.25f;
				}
			}
			note_cnt++; 
			note_16_received = false;
		}
	}
	recording = false; 
	metronome_on = false;
	rtt_disable_interrupt(RTT, RTT_MR_RTTINCIEN);
	
	// add last note 
	events[*number_of_events].note_number = notes[(note_cnt-1) & NOTE_MASK].note_number;
	events[*number_of_events].velocity = 64;
	events[*number_of_events].rhythm = current_rhythm;
	(*number_of_events)++;
	
	
}

/**************************** Public Functions End *********************************/
