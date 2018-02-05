#include "asf.h"
#include "DanLib.h"
#include "arm_math.h"

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

static float  get_average_power (float  *buffer)
{
	uint32_t i;
	float  p = 0.0;
	for ( i = 0; i < (WIN_SIZE); i++)
	{
		p = p + buffer[i]*buffer[i];
	}
	return p ;
}

// LP filter 10-4000Hz
static const float  lp_filter[] = {0.0027, 0.0103, 0.0258, 0.0499, 0.0801, 0.1105, 0.1332, 0.1416, 0.1332, 0.1105, 0.0801, 0.0499, 0.0258, 0.0103, 0.0027};
static const uint32_t lp_filter_length = 15;

static inline void apply_lp_filter(float  *src, float  *dest, uint32_t sig_length)
{
	/*
	// can use arm convolution for this!
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
	*/ 
	arm_copy_f32(src, dest, sig_length); 
}

// defines for circular filtered buffer 
#define CIRC_MASK (WIN_SIZE-1)
COMPILER_ALIGNED(WIN_SIZE) static float  x_circle_filt[WIN_SIZE]; 
COMPILER_ALIGNED(WIN_SIZE) static float workingBuffer[WIN_SIZE]; 
COMPILER_ALIGNED(WIN_SIZE) static float workingBuffer2[WIN_SIZE]; // needed since the fft corrupts the input ughhhh 
COMPILER_ALIGNED(WIN_SIZE) static float _norm[WIN_SIZE]; 
COMPILER_ALIGNED(WIN_SIZE) static float _phas[WIN_SIZE]; 
volatile float  inputPitch; 

int main(void)
{
	sysclk_init();
	board_init();
	lcd_init(); 
	audio_init();
	configure_console();
	yin_t *yin_instance = yin_init();
	PSOLA_init();
	gfx_draw_filled_rect(0, 0, gfx_get_width(), gfx_get_height(), GFX_COLOR_BLACK);
	
	uint32_t i,j;
	float  power;
	cvec_t *mags_and_phases = (cvec_t*)calloc(sizeof(cvec_t), 1); 
	mags_and_phases->length = WIN_SIZE/2 + 1; 
	mags_and_phases->norm = _norm; 
	mags_and_phases->phas = _phas; 
	
	fvec_t *workingVec = (fvec_t*)calloc(sizeof(fvec_t), 1);
	workingVec->length = WIN_SIZE;
	workingVec->data = workingBuffer; 
	
	fvec_t *workingVec2 = (fvec_t*)calloc(sizeof(fvec_t), 1);
	workingVec2->length = WIN_SIZE;
	workingVec2->data = workingBuffer2;
	
	printf("Starting Program\n\n\n\r"); 
	char str[20]; 
	uint32_t circ_buff_idx = 0; 
	
	arm_rfft_fast_instance_f32 fftInstance;
	arm_rfft_fast_init_f32(&fftInstance, WIN_SIZE);
	
	while(1)
	{
		if (dataReceived)
		{	
			// store lp-filtered values 
			uint32_t fill_index = (circ_buff_idx & CIRC_MASK); 
			apply_lp_filter((float  *)processBuffer, &x_circle_filt[fill_index], NEW_DATA_SIZE);
			
			// can use arm function! arm_power_f32 
			//power = get_average_power((float  *)&x[PROCESS_BUF_SIZE]);
						
			uint32_t starting_index = (fill_index + NEW_DATA_SIZE) & CIRC_MASK; 
				
			// apply hanning window - might consider using arm mult function and just shifting data instead of circular buffer 
			// could do a benchmark for this 
			for (j = starting_index, i = 0; j < starting_index + workingVec->length; j++, i++)
				workingVec->data[i] = x_circle_filt[j & CIRC_MASK] * hanning[i];
			
			// save input since fft changes it 
			arm_copy_f32(workingVec->data, workingVec2->data, workingVec->length); 
							
			// take fft of the windowed signal and get mag & phase
			arm_rfft_fast_f32(&fftInstance, workingVec->data, yin_instance->samples_fft->data, 0);
			
			// compute pitch -- requires prior fft in yin_instance
			inputPitch = yin_get_pitch(yin_instance, workingVec2, &fftInstance);
							
			// debug frequency detection
			sprintf(str, "%f", inputPitch);
			printf("Freq: %s\n\r", str);
							
			float  desiredPitch = 440.0;
			float  pitch_shift = 1 + (inputPitch - desiredPitch)/desiredPitch;

			//pitch_shift_do(temp_buffer, pitch_shift, mags_and_phases);
			
			// TODO: interpolate 
			// TODO: keep in mind you have the 48KHz information from the inBuffer that you can use for the voice 
			int processIdx = 0; 
			for(i = 0; i < IO_BUF_SIZE; i+=4)
			{
				outBuffer[i] = (uint16_t)(int16_t)(processBuffer[processIdx++] * INT16_MAX); // sound in / sound out
				outBuffer[i+1] = outBuffer[i]; 
				outBuffer[i+2] = outBuffer[i]; 
				outBuffer[i+3] = outBuffer[i]; 
			}
			circ_buff_idx += NEW_DATA_SIZE; 
			dataReceived = false; 
		}
	}
}

