#include "asf.h"
#include "DanLib.h"
#include "hanning.h"
#include "cvec.h"
#include "fft.h"

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

static float32_t get_average_power (float32_t *buffer)
{
	uint32_t i;
	float32_t p = 0.0;
	for ( i = 0; i < (PROCESS_BUF_SIZE); i++)
	{
		p = p + buffer[i]*buffer[i];
	}
	return p ;
}

// LP filter 10-4000Hz
static const float32_t lp_filter[] = {0.0027, 0.0103, 0.0258, 0.0499, 0.0801, 0.1105, 0.1332, 0.1416, 0.1332, 0.1105, 0.0801, 0.0499, 0.0258, 0.0103, 0.0027};
static const uint32_t lp_filter_length = 15;

static void apply_lp_filter(const float32_t *src, float32_t *dest, uint32_t sig_length)
{
	uint32_t j,i; 
	for (j = 0; j < lp_filter_length; j++)
		dest[j] = src[j];
	for(j = lp_filter_length; j < sig_length; j++)
	{
		dest[j] = 0;
		for(i = 0; i < lp_filter_length; i++)
		{
			dest[j] += src[j-i]*lp_filter[i];
		}
	}
}

static float32_t x[2*PROCESS_BUF_SIZE]; // 2*bufsize to hold past values (values are lp-filtered) 
static smpl_t workingBuffer[PROCESS_BUF_SIZE]; 

int main(void)
{
	sysclk_init();
	board_init();
	lcd_init(); 
	audio_init();
	configure_console();
	aubio_pitchyinfast_t *yin_instance = new_aubio_pitchyinfast(PROCESS_BUF_SIZE);
	PSOLA_init(PROCESS_BUF_SIZE);
	gfx_draw_filled_rect(0, 0, gfx_get_width(), gfx_get_height(), GFX_COLOR_BLACK);

	//q15_t input15[PROCESS_BUF_SIZE];
	uint32_t i;
	float32_t power;
	cvec_t *mags_and_phases = new_cvec(PROCESS_BUF_SIZE); 
	printf("Hellooooo\n\n\n\r"); 
	char str[20]; 
	
	while(1)
	{
		if (dataReceived)
		{	
			// store lp-filtered values 
			apply_lp_filter(processBuffer, &x[PROCESS_BUF_SIZE], PROCESS_BUF_SIZE);
			
			power = get_average_power((float32_t *)&x[PROCESS_BUF_SIZE]);
			
			fvec_t workingVec;
			workingVec.length = PROCESS_BUF_SIZE;
			workingVec.data = workingBuffer; 
			uint32_t starting_index = PROCESS_BUF_SIZE/NUM_OF_OVERLAPS; 
			
			// take fft of the windowed signal and get mag & phase
			aubio_fft_do_complex(yin_instance->fft, &workingVec, yin_instance->samples_fft);
			aubio_fft_get_spectrum((const fvec_t *)yin_instance->samples_fft, mags_and_phases);
						
			// compute pitch -- requires prior fft in yin_instance
			float32_t inputPitch = aubio_pitchyinfast_do(yin_instance, &workingVec);
			
			for (i = 0; i < 1; i++) // should loop for NUM_OF_OVERLAPS
			{							
				float32_t *temp_input = &x[starting_index + i*STEP_SIZE]; 
				// apply hanning window
				for (i = 0; i < workingVec.length; i++)
					workingVec.data[i] = temp_input[i] * hanning[i];
							
				// take fft of the windowed signal and get mag & phase
				aubio_fft_do_complex(yin_instance->fft, &workingVec, yin_instance->samples_fft);
				aubio_fft_get_spectrum((const fvec_t *)yin_instance->samples_fft, mags_and_phases);
							
							
				// debug frequency detection
				sprintf(str, "%f", inputPitch);
				printf("Freq: %s\n\r", str);
							
				float32_t desiredPitch = 440.0;
				float32_t pitch_shift = 1 + (inputPitch - desiredPitch)/desiredPitch;
				pitch_shift_do((float32_t *)&processBuffer, pitch_shift, mags_and_phases);
			}
			 
				
			// shift input back 
			memmove(&x[0], &x[PROCESS_BUF_SIZE], PROCESS_BUF_SIZE*sizeof(float32_t)); 
			
			// TODO: interpolate 
		
			int processIdx = 0;
			for(i = 0; i < IO_BUF_SIZE; i+=4)
			{
				outBuffer[i] = (uint16_t)(int16_t)(processBuffer[processIdx++] * INT16_MAX); // sound in / sound out
				outBuffer[i+1] = outBuffer[i];
				outBuffer[i+2] = outBuffer[i];
				outBuffer[i+3] = outBuffer[i];
			}
			dataReceived = false; 
		}
	}
}

