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

volatile uint32_t key_root = KEY_OF_E; 
volatile harmony_t harmony_list[MAX_NUM_KEYS_HARMONIES]; 
volatile float harm_volume = 1.0f;
volatile float dry_volume = 1.0f;
volatile float master_volume = MASTER_VOL_BASE; 
volatile uint32_t pitch_bend = NO_PITCH_BEND;
volatile float reverb_volume = 0.0f; 
volatile float delay_volume = 0.0f;
volatile uint32_t delay_speed = 15500;
volatile float delay_feedback = 0.2f;
volatile float chorus_volume = 0.0f;
volatile float chorus_speed = 0.02f;
volatile uint32_t received_bytes[3];
volatile uint32_t uart_cnt = 0;
volatile uint32_t waiting_for_button_press = 0;
volatile bool chord_harmonies[9] = {false, false, false, false, false, false, false, false, false}; // last one is autotune  
volatile uint32_t prev_midi_status = 0; 
void USART_SERIAL_ISR_HANDLER(void)
{
	
	uint32_t dw_status = usart_get_status(USART_SERIAL);
	if (dw_status & US_CSR_RXRDY) {
		usart_read(USART_SERIAL, (uint32_t *)&received_bytes[uart_cnt++]);
		
		// [255, 255, key] would mean im sending the major key 
		/*
			60 C
			61 Db 
			62 D 
			63 Eb
			64 E 
			65 F 
			66 Gb
			67 G 
			68 Ab
			69 A  
			70 Bb
			71 B
		*/ 
		if (received_bytes[0] == CH_BUTTON && uart_cnt == 2 && prev_midi_status == 176)
		{
			uart_cnt = 0; 
			uint32_t *data1 = (uint32_t *)&received_bytes[1];
			chord_harmonies[*data1] = !chord_harmonies[*data1];
		}
		else if (uart_cnt == 3)
		{
			uart_cnt = 0; 
			uint32_t *message = (uint32_t *)&received_bytes[0]; 
			uint32_t *data1 = (uint32_t *)&received_bytes[1]; 
			uint32_t *data2 = (uint32_t *)&received_bytes[2];
			prev_midi_status = *message;  
			if (*message == 255 && *data1 == 255 && *data2 == 255)
			{
				// clear all harmonies 
				for(int i = 0; i < 9; i++)
					chord_harmonies[i] = false; 
					
				key_root = KEY_OF_E; 
			}
			else if (*message == 255 && *data1 == 255)
			{
				key_root = *data2; 
			}
			else if (*message == NOTE_ON)
			{
				for(int i = 0; i < MAX_NUM_KEYS_HARMONIES; i++)
				{
					if (harmony_list[i].active == false)
					{
						harmony_list[i].active = true; 
						harmony_list[i].freq = midi_note_frequencies[*data1]; 
						harmony_list[i].idx = *data1; 
						break; 
					}
				}
			}
			else if (*message == NOTE_OFF)
			{
				for (int i = 0; i < MAX_NUM_KEYS_HARMONIES; i++)
				{
					if (harmony_list[i].active == true && harmony_list[i].idx == *data1)
					{
						harmony_list[i].active = false; 
						break; 
					}
				}
			}
			else if (*message == SLIDER) 
			{
				switch(*data1)
				{
					case DRY_VOLUME_CH: 
						dry_volume = 0.1f + 0.9f*(float)*data2 / 127.0f; break; 
					case HARM_VOLUME_CH: 
						harm_volume = (float)*data2 / 127.0f; break; 
					case MASTER_VOLUME_CH: 
						master_volume = MASTER_VOL_BASE*(float)*data2 / 127.0f; break; 
					case REVERB_CH: 
						reverb_volume = (float)*data2 / 127.0f; break; 
					case CHORUS_VOLUME_CH: 
						chorus_volume = (float)*data2 / 127.0f; break;
					case CHORUS_SPEED_CH: 
						chorus_speed = 0.05f + 2.0f*(float)*data2 / 127.0f; break;
					case DELAY_FEEDBACK_CH: 
						delay_feedback = 0.8f * (float)*data2 / 127.0f; break;
					case DELAY_SPEED_CH: 
						delay_speed = 16200 - 14000 * (float)*data2 / 127.0f; break; 
					case DELAY_VOLUME_CH: 
						delay_volume = 0.7f * (float)*data2 / 127.0f; break; 
					default: break; 
				}
			}
			else if (*message == PITCH_WHEEL)
			{
				pitch_bend = *data2; 
			}
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

static inline float powerf(float base, int32_t exponent)
{
	float result = 1.0;
	uint32_t exp_abs = Abs(exponent);
	while (exp_abs)
	{
		result *= base;
		exp_abs--;
	}
	if (exponent < 0)
	return 1.0/result;
	else
	return result;
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
	audio_init();
#ifdef USING_CONSOLE
	configure_console();
#endif 
	PSOLA_init(); 
	configure_uart(); 
	usart_write(USART_SERIAL, 0x30);
	
	/*************** Application code variables start ***************/
	uint32_t i;
	
	for (i = 0; i < MAX_NUM_KEYS_HARMONIES; i++)
	{
		harmony_list[i].freq = 0.0f; 
		harmony_list[i].idx = 0; 
		harmony_list[i].active = false; 
	}
	float inputPitch; 
	float oneOverInputPitch = 1.0f;
	float power;
	float harmony_shifts[MAX_NUM_SHIFTS+1]; arm_fill_f32(1.0f, harmony_shifts, MAX_NUM_SHIFTS); 
	harmony_shifts[0] = NO_SHIFT;
	harmony_shifts[1] = END_OF_SHIFTS; 
	harmony_shifts[MAX_NUM_SHIFTS] = END_OF_SHIFTS; // should never change 
	uint32_t num_of_shifts = 0; 
	chord_t chord_freqs[8]; 
	for(i = 0; i < 8; i++)
	{
		chord_freqs[i].freq = 0.0f; chord_freqs[i].active = false; 
	} 
	uint32_t circ_buf_idx = 0; 
	float closest_note_freq = 0; 
	uint32_t closest_note_number = 0; 
	float desired_pitch; 
	uint32_t sin_cnt = 0; 
	uint32_t chorus_delay; 
	arm_fill_f32(0.0f, dry_circ_buffer, CIRC_BUF_SIZE);
	arm_fill_f32(0.0f, delay_circ_buffer, CIRC_BUF_SIZE);
	scale_t major[] = {W,W,H,W,W,W,H}; 
	uint32_t harmony_steps[] = {2, 2, 1}; // third, fifth, sixth
	float scale_correct_history[SCALE_CORRECT_HISTORY_SIZE]; arm_fill_f32(SCALE_NONE, scale_correct_history, SCALE_CORRECT_HISTORY_SIZE); 
	uint32_t scale_correct_idx = 0; 
	/*************** Application code variables end ***************/
	
	while(1)
	{
		if (dataReceived)
		{	
			dataReceived = false; 
			
			// get pitch 
			inputPitch = computeWaveletPitch(processBuffer);
			if (inputPitch < MINIMUM_PITCH) 
				inputPitch = MINIMUM_PITCH; 
			oneOverInputPitch = 1.0f / inputPitch;
			
			// find closest musical pitch 
			get_frequency_from_all(inputPitch, &closest_note_freq, &closest_note_number);
				
			// find number of semitones from root 
			float scale_pitch = closest_note_freq;
			uint32_t shifted_note_number = closest_note_number; 
			while (shifted_note_number < key_root)
				shifted_note_number += 12; 
			uint32_t number_of_semitones_from_root = shifted_note_number - key_root;
			while(number_of_semitones_from_root > 12)
				number_of_semitones_from_root -= 12;
						
			// adjust for pitch in between notes in major scale 	
			if (number_of_semitones_from_root == 1 || number_of_semitones_from_root == 3 ||
			number_of_semitones_from_root == 6 || number_of_semitones_from_root == 8 || number_of_semitones_from_root == 10 )
			{
				float low_avg = 0.5f * (midi_note_frequencies[closest_note_number] + midi_note_frequencies[closest_note_number-1]); 
				float hi_avg = 0.5f * (midi_note_frequencies[closest_note_number] + midi_note_frequencies[closest_note_number+1]);
				if (inputPitch < low_avg)
				{
					number_of_semitones_from_root -=1;
					scale_pitch = midi_note_frequencies[closest_note_number - 1];
					scale_correct_history[scale_correct_idx++ & SCALE_CORRECT_HISTORY_MASK] = SCALE_DOWN; 
				}
				else if (inputPitch > hi_avg)
				{
					number_of_semitones_from_root +=1;
					scale_pitch = midi_note_frequencies[closest_note_number + 1];
					scale_correct_history[scale_correct_idx++ & SCALE_CORRECT_HISTORY_MASK] = SCALE_UP;
				}
				else
				{
					float avg_direction = 0.0f; 
					arm_mean_f32(scale_correct_history, SCALE_CORRECT_HISTORY_SIZE, &avg_direction); 
					if(avg_direction < 0.0f)
					{
						number_of_semitones_from_root -=1;
						scale_pitch = midi_note_frequencies[closest_note_number - 1];
						scale_correct_history[scale_correct_idx++ & SCALE_CORRECT_HISTORY_MASK] = SCALE_DOWN;
					}
					else 
					{
						number_of_semitones_from_root +=1;
						scale_pitch = midi_note_frequencies[closest_note_number + 1];
						scale_correct_history[scale_correct_idx++ & SCALE_CORRECT_HISTORY_MASK] = SCALE_UP;
					}
				}
			}	
			else
			{
				scale_correct_history[scale_correct_idx++ & SCALE_CORRECT_HISTORY_MASK] = SCALE_NONE;
			}		
			
			// find index in scale where the pitch lies 
			uint32_t interval_idx = 0;
			uint32_t scale_step = 0;
			for (i = 0; i < 7; i++)
			{
				scale_step += major[i];
				if (number_of_semitones_from_root == scale_step)
				{
					interval_idx = i+1;
					if (interval_idx == 7)
						interval_idx = 0;
					break;
				}
			}
			
			// regular voice 
			harmony_shifts[0] = 1.0f; 
			num_of_shifts = 1;  
			
			// calculate power 
			arm_power_f32(processBuffer, WIN_SIZE>>2, &power);
		
			// determine whether you should add any harmonies 
			if (inputPitch > MINIMUM_PITCH && power > POWER_THRESHOLD)
			{
				// chord harmonies 
				uint32_t chord_idx = 0;	
				uint32_t saved_interval_idx = interval_idx; 	
				
				// autotune 
				bool *autotune = (bool *)&chord_harmonies[8]; 
				if (*autotune)
				{
					desired_pitch = scale_pitch;
	
					if (pitch_bend < 56 || pitch_bend > 72) 
						bend_pitch(&desired_pitch, closest_note_number, (uint32_t)pitch_bend);
				
					harmony_shifts[0] = 1.0f - (inputPitch-desired_pitch)*oneOverInputPitch;
				}
				
				// octave down
				if(chord_harmonies[chord_idx] == true)
				{
					if (*autotune)
						desired_pitch = scale_pitch*powerf(1.059463094359f, -12);
					else
						desired_pitch = closest_note_freq*powerf(1.059463094359f, -12);
					if (pitch_bend < 56 || pitch_bend > 72)
						bend_pitch(&desired_pitch, closest_note_number, (uint32_t)pitch_bend);
					harmony_shifts[num_of_shifts++] = 1.0f - (inputPitch-desired_pitch)*oneOverInputPitch;
					chord_freqs[chord_idx].active = true;
					chord_freqs[chord_idx].freq = desired_pitch;
				} else chord_freqs[chord_idx].active = false;
				chord_idx++;
						
				// low harmonies 
				int32_t steps_to_harmony = -12;
				for (i = 0; i < 3; i++, chord_idx++)
				{
					for (uint32_t j = 0; j < harmony_steps[i]; j++, interval_idx++)
						steps_to_harmony += major[interval_idx % 7];
					if(chord_harmonies[chord_idx] == true)
					{
						desired_pitch = scale_pitch*powerf(1.059463094359f, steps_to_harmony);
						if (pitch_bend < 56 || pitch_bend > 72)
							bend_pitch(&desired_pitch, closest_note_number, (uint32_t)pitch_bend);
						harmony_shifts[num_of_shifts++] = 1.0f - (inputPitch-desired_pitch)*oneOverInputPitch;
						chord_freqs[chord_idx].active = true;
						chord_freqs[chord_idx].freq = desired_pitch;
					} else chord_freqs[chord_idx].active = false;
				}
				
				// high harmonies 
				steps_to_harmony = 0;
				interval_idx = saved_interval_idx;  	
				for (i = 0; i < 3; i++, chord_idx++)
				{
					for (uint32_t j = 0; j < harmony_steps[i]; j++, interval_idx++)
						steps_to_harmony += major[interval_idx % 7];
					if(chord_harmonies[chord_idx] == true)
					{
						desired_pitch = scale_pitch*powerf(1.059463094359f, steps_to_harmony);
						if (pitch_bend < 56 || pitch_bend > 72)
							bend_pitch(&desired_pitch, closest_note_number, (uint32_t)pitch_bend);
						harmony_shifts[num_of_shifts++] = 1.0f - (inputPitch-desired_pitch)*oneOverInputPitch;
						chord_freqs[chord_idx].active = true;
						chord_freqs[chord_idx].freq = desired_pitch;
					} else chord_freqs[chord_idx].active = false; 
				}
				
				// octave up
				if(chord_harmonies[chord_idx] == true)
				{
					if (*autotune)
						desired_pitch = scale_pitch*powerf(1.059463094359f, 12);
					else
						desired_pitch = closest_note_freq*powerf(1.059463094359f, 12);
					if (pitch_bend < 56 || pitch_bend > 72)
						bend_pitch(&desired_pitch, closest_note_number, (uint32_t)pitch_bend);
					harmony_shifts[num_of_shifts++] = 1.0f - (inputPitch-desired_pitch)*oneOverInputPitch;
					chord_freqs[chord_idx].active = true;
					chord_freqs[chord_idx].freq = desired_pitch;
				} else chord_freqs[chord_idx].active = false;

				// keyboard harmonies  
				float pitch_shift; 
				for (i = 0; i < MAX_NUM_KEYS_HARMONIES; i++)
				{
					if (harmony_list[i].active)
					{
						if (Abs(harmony_list[i].freq - harmony_shifts[0]) > 8.0f) // don't harmonize input pitch twice
						{
							desired_pitch = harmony_list[i].freq;
							bool already_harmonized = false; 
							for (int k = 0; k < 8; k++)
							{
								if (chord_freqs[k].active && Abs(desired_pitch - chord_freqs[k].freq) < 1.0f)
								{
									already_harmonized = true; 
									break; 
								}
							}
							
							if (already_harmonized == false)
							{
								if (pitch_bend != 64)
									bend_pitch(&desired_pitch, harmony_list[i].idx, (uint32_t)pitch_bend);
							
								pitch_shift = 1.0f - (inputPitch-desired_pitch)*oneOverInputPitch;
							
								if (pitch_shift > 0.1f && pitch_shift < 6.0f) // range check
									harmony_shifts[num_of_shifts++] = pitch_shift;
							}
						}
					}
				}
			} 
			
			harmony_shifts[num_of_shifts] = END_OF_SHIFTS; 
			
			create_harmonies(processBuffer, out_buffer, inputPitch, harmony_shifts, (float)harm_volume, (float)dry_volume); 
			
			// save dry audio 
			for (i = 0; i < WIN_SIZE; i++)
			{
				dry_circ_buffer[circ_buf_idx++ & CIRC_MASK] = out_buffer[i];
			}
			
			// Add audio effects 
			uint32_t curr_idx = circ_buf_idx - (uint32_t)WIN_SIZE;
			// chorus params 
			float n_freq = chorus_speed / SAMPLE_RATE; 
			uint32_t num_samples_in_period = 1 / n_freq; 
			for (i = 0; i < WIN_SIZE; i++, curr_idx++)
			{				
				out_buffer[i] = (1.0f - 0.5*(delay_volume + 0.5f*(chorus_volume + reverb_volume))) * out_buffer[i]; 
						
				// chorus
				chorus_delay = (0.008f + 0.003f *  arm_cos_f32(2.0f*(float)M_PI * (float)sin_cnt++ * n_freq)) * SAMPLE_RATE;
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
			
			// debug sound in / sound out 
 			//arm_scale_f32(processBuffer, (float)INT16_MAX, out_buffer, WIN_SIZE); 
			
			// Sound out 
			uint32_t idx = 0; 
			for(i = 0; i < IO_BUF_SIZE; i+=2)
			{
				outBuffer[i] = (uint16_t)(int16_t)(out_buffer[idx++]);  
				outBuffer[i+1] = outBuffer[i]; 
			}
			
			// check if we're too slow ... uncomment for debug 
// 			if (dataReceived)
// 			{
// 				while(1)
// 				{
// 					// taking too long 
// 				}
// 			}

		}
	}
}