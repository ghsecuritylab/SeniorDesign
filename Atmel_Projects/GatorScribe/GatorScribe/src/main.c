#include "asf.h"
#include "DanLib.h"
#include "arm_math.h"
#include <math.h>

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

static const float32_t midi_note_frequencies[128] = {
	8.176,8.662,9.177,9.723,10.301,10.913,11.562,12.250,12.978,13.750,14.568,15.434,16.352,17.324,18.354,19.445,20.602,21.827,23.125,24.500,
	25.957,27.500,29.135,30.868,32.703,34.648,36.708,38.891,41.203,43.654,46.249,48.999,51.913,55.000,58.270,61.735,65.406,69.296,73.416,
	77.782,82.407,87.307,92.499,97.999,103.826,110.000,116.541,123.471,130.813,138.591,146.832,155.563,164.814,174.614,184.997,195.998,
	207.652,220.000,233.082,246.942,261.626,277.183,293.665,311.127,329.628,349.228,369.994,391.995,415.305,440.000,466.164,493.883,
	523.251,554.365,587.330,622.254,659.255,698.456,739.989,783.991,830.609,880.000,932.328,987.767,1046.502,1108.731,1174.659,1244.508,
	1318.510,1396.913,1479.978,1567.982,1661.219,1760.000,1864.655,1975.533,2093.005,2217.461,2349.318,2489.016,2637.020,2793.826,2959.955,
	3135.963,3322.438,3520.000,3729.310,3951.066,4186.009,4434.922,4698.636,4978.032,5274.041,5587.652,5919.911,6271.927,6644.875,7040.000,
	7458.620,7902.133,8372.018,8869.844,9397.273,9956.063,10548.080,11175.300,11839.820,12543.850
};

static inline void get_frequency_from_all(float32_t frequency, float *closest_note, uint32_t *closest_note_idx)
{
	uint32_t lo = 12; // lowest at C0
	uint32_t hi = 127;
	uint32_t mid;
	uint32_t d1;
	uint32_t d2;
	while (lo < hi)
	{
		mid = (hi + lo) >> 1;
		d1 = Abs(midi_note_frequencies[mid] - frequency);
		d2 = Abs(midi_note_frequencies[mid+1] - frequency);
		if (d2 <= d1)
		{
			lo = mid+1;
		}
		else
		{
			hi = mid;
		}
	}
	*closest_note = midi_note_frequencies[hi];
	*closest_note_idx = hi; 
}

/*************** Application code buffers and consts start ***************/
COMPILER_ALIGNED(WIN_SIZE) static float out_buffer[WIN_SIZE];

#define CIRC_BUF_SIZE 16384
#define CIRC_MASK (CIRC_BUF_SIZE-1)
COMPILER_ALIGNED(CIRC_BUF_SIZE) static float dry_circ_buffer[CIRC_BUF_SIZE];
COMPILER_ALIGNED(CIRC_BUF_SIZE) static float delay_circ_buffer[CIRC_BUF_SIZE];

/*************** Application code buffers and consts end ***************/

/*************** UART Communication Start ***************/
#define USART_SERIAL                 USART1
#define USART_SERIAL_ID              ID_USART1  
#define USART_SERIAL_ISR_HANDLER     USART1_Handler
#define USART_SERIAL_BAUDRATE        115200
#define USART_SERIAL_CHAR_LENGTH     US_MR_CHRL_8_BIT
#define USART_SERIAL_PARITY          US_MR_PAR_NO
#define USART_SERIAL_STOP_BIT        US_MR_NBSTOP_1_BIT

volatile harmony_t harmony_list_a[MAX_NUM_SHIFTS]; 
volatile harmony_t harmony_list_b[MAX_NUM_SHIFTS];
volatile harmony_t *harmony_list_read = harmony_list_a; 
volatile harmony_t *harmony_list_fill = harmony_list_b; 
volatile uint32_t harmony_idx = 0;  

volatile bool waiting_for_harm_volume = false;
volatile float harm_volume = 1.0f;

volatile bool waiting_for_dry_volume = false;
volatile float dry_volume = 1.0f;
 
volatile bool waiting_for_master_volume = false;
volatile float master_volume = 1.0f;

volatile bool waiting_for_pitch_bend = false;
volatile uint32_t pitch_bend = NO_PITCH_BEND;

volatile bool waiting_for_reverb_volume = false;
volatile float reverb_volume = 0.0f; 

volatile bool waiting_for_delay_volume = false;
volatile float delay_volume = 0.0f;

volatile bool waiting_for_delay_speed = false;
volatile uint32_t delay_speed = 10000;

volatile bool waiting_for_delay_feedback = false;
volatile float delay_feedback = 0.2f;

volatile bool waiting_for_chorus_volume = false;
volatile float chorus_volume = 0.0f;

volatile bool waiting_for_chorus_speed = false;
volatile float chorus_speed = 0.02f;

volatile bool autotune = true; 
void USART_SERIAL_ISR_HANDLER(void)
{
	uint32_t dw_status = usart_get_status(USART_SERIAL);
	if (dw_status & US_CSR_RXRDY) {
		uint32_t received_byte;
		usart_read(USART_SERIAL, &received_byte);
		//usart_write(USART_SERIAL, received_byte); // for debug 
		
		if (waiting_for_harm_volume)
		{
			harm_volume = (float)received_byte / 127.0f; 
			waiting_for_harm_volume = false; 
		}
		else if (waiting_for_master_volume)
		{
			master_volume = 1.2f*(float)received_byte / 127.0f;
			waiting_for_master_volume = false;
		}
		else if (waiting_for_dry_volume)
		{
			dry_volume = 0.1f + 0.9f*(float)received_byte / 127.0f;
			waiting_for_dry_volume = false;
		}
		else if (waiting_for_pitch_bend)
		{
			pitch_bend = received_byte;
			waiting_for_pitch_bend = false;
		}
		else if (waiting_for_reverb_volume)
		{
			reverb_volume = (float)received_byte / 127.0f;;
			waiting_for_reverb_volume = false;
		}
		else if (waiting_for_delay_volume)
		{
			delay_volume = 0.7f * (float)received_byte / 127.0f;
			waiting_for_delay_volume = false;
		}
		else if (waiting_for_delay_speed)
		{
			// first value = longest delay
			// first val - second val = shortest delay 
			delay_speed = 16200 - 14000 * (float)received_byte / 127.0f;
			waiting_for_delay_speed = false;
		}
		else if (waiting_for_delay_feedback)
		{
			delay_feedback = 0.8f * (float)received_byte / 127.0f;
			waiting_for_delay_feedback = false;
		}
		else if (waiting_for_chorus_volume)
		{
			chorus_volume = (float)received_byte / 127.0f;;
			waiting_for_chorus_volume = false;
		}
		else if (waiting_for_chorus_speed)
		{
			chorus_speed = 0.05f + 2.0f*(float)received_byte / 127.0f;;
			waiting_for_chorus_speed = false;
		}
		else if (received_byte == HARMONY_VOLUME_FLAG) 
		{
			waiting_for_harm_volume = true; 
		}
		else if (received_byte == DRY_VOLUME_FLAG)
		{
			waiting_for_dry_volume = true;
		}
		else if (received_byte == MASTER_VOLUME_FLAG)
		{
			waiting_for_master_volume = true;
		}
		else if (received_byte == PITCH_BEND_FLAG)
		{
			waiting_for_pitch_bend = true;
		}
		else if (received_byte == REVERB_VOLUME_FLAG)
		{
			waiting_for_reverb_volume = true;
		}
		else if (received_byte == DELAY_VOLUME_FLAG)
		{
			waiting_for_delay_volume = true;
		}
		else if (received_byte == DELAY_SPEED_FLAG)
		{
			waiting_for_delay_speed = true;
		}
		else if (received_byte == DELAY_FEEDBACK_FLAG)
		{
			waiting_for_delay_feedback = true;
		}
		else if (received_byte == CHORUS_VOLUME_FLAG)
		{
			waiting_for_chorus_volume = true;
		}
		else if (received_byte == CHORUS_SPEED_FLAG)
		{
			waiting_for_chorus_speed = true;
		}
		else if (received_byte == AUTOTUNE_FLAG)
		{
			autotune = !autotune; 
		}
		else if (received_byte != 0 && harmony_idx < MAX_NUM_SHIFTS)
		{
			harmony_list_fill[harmony_idx].freq = midi_note_frequencies[received_byte]; 
			harmony_list_fill[harmony_idx].idx = received_byte; 
			harmony_idx++;
		}
		else 
		{
			harmony_list_fill[harmony_idx].freq = END_OF_SHIFTS; 
			harmony_list_fill[harmony_idx].idx = 0; // dont care 
			harmony_t *temp = (harmony_t *)harmony_list_read; 
			harmony_list_read = harmony_list_fill;		
			harmony_list_fill = temp; 
			harmony_idx = 0; 
		}
	}
}
/*************** UART Communication End ***************/


/* 
	Returns bent pitch in *pitch 
	Bend ranges from 0 - 127: 
		0 = -2 semitones 
		127 = +2 semitones 
		64 = +0 semitones 
*/ 
static inline void bend_pitch(float *pitch, uint32_t pitch_idx, uint32_t bend)
{
	if (pitch_idx < 0 || pitch_idx > 127 || bend < 0 || bend > 127)
		return; 
	
	float bend_difference; 
	if (pitch_bend > 64)
	{
		bend_difference = midi_note_frequencies[pitch_idx+2] - midi_note_frequencies[pitch_idx];
		*pitch += (((float)pitch_bend - 63.0f) * ONE_OVER_64) * bend_difference;
	}
	else //if (pitch_bend < 64)
	{
		bend_difference = midi_note_frequencies[pitch_idx] - midi_note_frequencies[pitch_idx-2];
		*pitch += (((float)pitch_bend - 64.0f) * ONE_OVER_64) * bend_difference;
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

// uncomment to communicate to pc console over uart for debug 
//#define USING_CONSOLE

int main(void)
{
	sysclk_init();
	board_init();
 	SCB_DisableICache(); 
// 	lcd_init(); 
 	SCB_EnableICache();
	audio_init();
#ifdef USING_CONSOLE
	configure_console();
#endif 
	PSOLA_init(); 
	configure_uart(); 
	 
	 // draw smiley face 
// 	SCB_DisableICache(); 
// 	gfx_draw_filled_rect(100, 100, 20, 20, GFX_COLOR_YELLOW);
// 	gfx_draw_filled_rect(200, 100, 20, 20, GFX_COLOR_YELLOW);
// 	gfx_draw_filled_rect(80, 180, 20, 20, GFX_COLOR_YELLOW);
// 	gfx_draw_filled_rect(100, 200, 20, 20, GFX_COLOR_YELLOW);
// 	gfx_draw_filled_rect(120, 220, 20, 20, GFX_COLOR_YELLOW);
// 	gfx_draw_filled_rect(140, 220, 20, 20, GFX_COLOR_YELLOW);
// 	gfx_draw_filled_rect(160, 220, 20, 20, GFX_COLOR_YELLOW);
// 	gfx_draw_filled_rect(180, 220, 20, 20, GFX_COLOR_YELLOW);
// 	gfx_draw_filled_rect(200, 200, 20, 20, GFX_COLOR_YELLOW);
// 	gfx_draw_filled_rect(220, 180, 20, 20, GFX_COLOR_YELLOW);
// 	SCB_EnableICache(); 
	
	
	// for serial debug 
	//char *str = (char *)calloc(20, sizeof(char)); 
	
	/*************** Application code variables start ***************/
	uint32_t i;
	
	for (i = 0; i < MAX_NUM_SHIFTS; i++)
	{
		harmony_list_a[i].freq = 0.0f; harmony_list_b[i].freq = 0.0f; 
		harmony_list_a[i].idx = 0.0f; harmony_list_b[i].idx = 0.0f; 
	}
	float inputPitch; 
	float oneOverInputPitch = 1.0f;
	float pitch_shift, power;
	float harmony_shifts[MAX_NUM_SHIFTS+1];
	harmony_shifts[0] = NO_SHIFT;
	harmony_shifts[1] = END_OF_SHIFTS; 
	harmony_shifts[MAX_NUM_SHIFTS] = END_OF_SHIFTS; // should never change 
	uint32_t num_of_shifts = 0; 
	uint32_t circ_buf_idx = 0; 
	float closest_note = 0; 
	float desired_pitch; 
	uint32_t in_pitch_idx = 0; 
	uint32_t sin_cnt = 0; 
	uint32_t chorus_delay; 
	arm_fill_f32(0.0f, dry_circ_buffer, CIRC_BUF_SIZE);
	arm_fill_f32(0.0f, delay_circ_buffer, CIRC_BUF_SIZE);
	/*************** Application code variables end ***************/
	
	while(1)
	{
		if (dataReceived)
		{	
			dataReceived = false; 
			
			// get pitch 
			inputPitch = computeWaveletPitch((float  *)processBuffer);
			if (inputPitch < MINIMUM_PITCH) 
				inputPitch = MINIMUM_PITCH; 
			oneOverInputPitch = 1.0f / inputPitch;
			
			// auto tune 
			{
				get_frequency_from_all(inputPitch, &closest_note, &in_pitch_idx);
			
				if (autotune)
					desired_pitch = closest_note;
				else 
					desired_pitch = inputPitch;
				
				if (pitch_bend < 56 || pitch_bend > 72) // higher bounds for noise affecting pitch bend wheel 
					bend_pitch(&desired_pitch, in_pitch_idx, (uint32_t)pitch_bend);
				
				pitch_shift = 1.0f - (inputPitch-desired_pitch)*oneOverInputPitch;
				harmony_shifts[0] = pitch_shift ;
				num_of_shifts = 1;  
			}
				
			// calculate power 
			arm_power_f32((float  *)processBuffer, WIN_SIZE>>2, &power);
		
			// determine whether you should add any harmonies 
			if (inputPitch > MINIMUM_PITCH && power > POWER_THRESHOLD)
			{
				i = 0;
				while(harmony_list_read[i].freq > 1.0f && i < MAX_NUM_SHIFTS-1)
				{
					if (Abs(harmony_list_read[i].freq - closest_note) > 1.0f) // don't harmonies input pitch twice 
					{
						desired_pitch = harmony_list_read[i].freq; 
						if (pitch_bend != 64)
							bend_pitch(&desired_pitch, harmony_list_read[i].idx, (uint32_t)pitch_bend);
							
						pitch_shift = 1.0f - (inputPitch-desired_pitch)*oneOverInputPitch;
						
						if (pitch_shift > 0.1f && pitch_shift < 6.0f) // range check 
							harmony_shifts[num_of_shifts++] = pitch_shift;
					}
					i++; 
				}
				harmony_shifts[num_of_shifts] = END_OF_SHIFTS; 
			} 
			else 
			{
				// only do autotune 
				harmony_shifts[1] = END_OF_SHIFTS; 	
				num_of_shifts = 1; 
			}
			
			// return pitch shifted data from previous samples block  
			create_harmonies((float  *)processBuffer, out_buffer, inputPitch, harmony_shifts, (float)harm_volume, (float)dry_volume); 
			
			// save dry audio 
			for (i = 0; i < WIN_SIZE; i++)
			{
				dry_circ_buffer[circ_buf_idx++ & CIRC_MASK] = out_buffer[i];
			}
			
			// Add audio effects 
			uint32_t curr_idx = circ_buf_idx - (uint32_t)WIN_SIZE;
			// chorus params -- could make speed a param 
			float n_freq = chorus_speed / PSOLA_SAMPLE_RATE; 
			uint32_t num_samples_in_period = 1 / n_freq; 
			for (i = 0; i < WIN_SIZE; i++, curr_idx++)
			{				
				out_buffer[i] = (1.0f - 0.5*(delay_volume + chorus_volume + reverb_volume)) * out_buffer[i]; 
						
				// chorus
				chorus_delay = (0.008f + 0.001f *  arm_cos_f32(2.0f*(float)M_PI * (float)sin_cnt++ * n_freq)) * PSOLA_SAMPLE_RATE;
				if (sin_cnt == num_samples_in_period)
					sin_cnt = 0;
				out_buffer[i] += chorus_volume * (0.2f* (dry_circ_buffer[(curr_idx - chorus_delay)  & CIRC_MASK] +
														dry_circ_buffer[(curr_idx - 199 - chorus_delay)  & CIRC_MASK] +
														dry_circ_buffer[(curr_idx - 401 - chorus_delay)  & CIRC_MASK] +
														dry_circ_buffer[(curr_idx - 601 - chorus_delay)  & CIRC_MASK] +
														dry_circ_buffer[(curr_idx - 809 - chorus_delay)  & CIRC_MASK]));			
	
				// delay
				delay_circ_buffer[curr_idx & CIRC_MASK] = out_buffer[i] + delay_feedback * delay_circ_buffer[(curr_idx - delay_speed)  & CIRC_MASK];	
				out_buffer[i] += delay_volume * delay_circ_buffer[curr_idx & CIRC_MASK];
				
				// reverb
				out_buffer[i] += reverb_volume * 0.33f *
						(dry_circ_buffer[(curr_idx - 2001)  & CIRC_MASK] +
						dry_circ_buffer[(curr_idx - 1503)  & CIRC_MASK] + 
						dry_circ_buffer[(curr_idx - 1203)  & CIRC_MASK] ); 
			}
	
			// scale output 
			arm_scale_f32(out_buffer, (float)INT16_MAX * master_volume, out_buffer, WIN_SIZE);
 			//arm_scale_f32(processBuffer, (float)INT16_MAX, out_buffer, WIN_SIZE); // sound in / sound out 
			
			// Sound out 
			uint32_t idx = 0; 
			for(i = 0; i < IO_BUF_SIZE; i+=2)
			{
				outBuffer[i] = (uint16_t)(int16_t)(out_buffer[idx++]);  
				outBuffer[i+1] = outBuffer[i]; 
			}
			
			// check if we're too slow 
			if (dataReceived)
			{
// 				while(1)
// 				{
// 					// taking too long ... never 
// 				}
			}
			else 
				dataReceived = false; 
		}
	}
}