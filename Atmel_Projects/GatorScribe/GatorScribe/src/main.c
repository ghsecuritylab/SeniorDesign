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

const float ONEQTR_PI = M_PI / 4.0;
const float THRQTR_PI = 3.0 * M_PI / 4.0;
static inline float atan2_approximation(float y, float x)
{
	float r, angle;
	float abs_y = abs(y) + 1e-10f;      // kludge to prevent 0/0 condition
	if ( x < 0.0f )
	{
		r = (x + abs_y) / (abs_y - x);
		angle = THRQTR_PI;
	}
	else
	{
		r = (x - abs_y) / (x + abs_y);
		angle = ONEQTR_PI;
	}
	angle += (0.1963f * r * r - 0.9817f) * r;
	if ( y < 0.0f )
		return( -angle );     // negate if in quad III or IV
	else
		return( angle );
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
COMPILER_ALIGNED(WIN_SIZE) static float  x_filt[WIN_SIZE]; 
COMPILER_ALIGNED(WIN_SIZE) static float workingBuffer[WIN_SIZE]; 
COMPILER_ALIGNED(WIN_SIZE) static float workingBuffer2[WIN_SIZE]; // needed since the fft corrupts the input ughhhh 
COMPILER_ALIGNED(WIN_SIZE) static float _norm[WIN_SIZE]; 
COMPILER_ALIGNED(WIN_SIZE) static float _phas[WIN_SIZE]; 
volatile float  inputPitch; 

COMPILER_ALIGNED(WIN_SIZE) static float harmonized_output[WIN_SIZE];

volatile float temp1[] = {1.0, 2.0, 3.0, 4.0};
volatile float temp2[] = {1.0, 2.0, 3.0, 4.0};
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
	mags_and_phases->length = FRAME_SIZE_2 + 1; 
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
	
	arm_rfft_fast_instance_f32 fftInstance;
	arm_rfft_fast_init_f32(&fftInstance, WIN_SIZE);
	
	while(1)
	{
		if (dataReceived)
		{	
			// store lp-filtered values into last quarter of buffer 
			// TODO: going to have to end up changing DMA buffers again to do pitch detections every 1024 samples 
			apply_lp_filter((float  *)processBuffer, &x_filt[WIN_SIZE-STEP_SIZE], NEW_DATA_SIZE);
			
			// can use arm function! arm_power_f32 
			//power = get_average_power((float  *)&x[PROCESS_BUF_SIZE]);
										
			// apply hanning window 
			arm_mult_f32(x_filt, (float32_t *)hanning, workingVec->data, WIN_SIZE); 
			
			// save input since fft changes it 
			arm_copy_f32(workingVec->data, workingVec2->data, workingVec->length); 
							
			// take fft of the windowed signal
			arm_rfft_fast_f32(&fftInstance, workingVec->data, yin_instance->samples_fft->data, 0);
			
			// compute magnitude and phase 
			arm_cmplx_mag_f32(yin_instance->samples_fft->data, mags_and_phases->norm, mags_and_phases->length); 
			arm_scale_f32(mags_and_phases->norm, 2.0, mags_and_phases->norm, mags_and_phases->length); 
			if (yin_instance->samples_fft->data[0] < 0) 
				mags_and_phases->phas[0] = PI;
			else 
				mags_and_phases->phas[0] = 0.0;			
			if (yin_instance->samples_fft->data[1] < 0)
				mags_and_phases->phas[1] = PI;
			else
				mags_and_phases->phas[1] = 0.0;
			
			for (j = 2, i = 2; i < yin_instance->samples_fft->length; i+=2, j++)
			{
				mags_and_phases->phas[j] = atan2_approximation(yin_instance->samples_fft->data[i+1], yin_instance->samples_fft->data[i]); 
			}
			
			// compute pitch -- requires prior fft in yin_instance
			inputPitch = yin_get_pitch(yin_instance, workingVec2, &fftInstance);
							
			// debug frequency detection
			sprintf(str, "%f", inputPitch);
			printf("Freq: %s\n\r", str);
							
			float  desiredPitch = 440.0;
			float  pitch_shift = 1 + (inputPitch - desiredPitch)/desiredPitch;

			pitch_shift_do(harmonized_output, pitch_shift, mags_and_phases);
			
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
			
			// shift input back one quarter 
			arm_copy_f32(&x_filt[STEP_SIZE], &x_filt[0], WIN_SIZE-STEP_SIZE); 
			dataReceived = false; 
		}
	}
}

