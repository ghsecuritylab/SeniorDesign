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

static const float key_C[] = {16.35	,
	18.35	,
	20.60	,
	21.83	,
	24.50	,
	27.50	,
	30.87	,
	32.70	,
	36.71	,
	41.20	,
	43.65	,
	49.00	,
	55.00	,
	61.74	,
	65.41	,
	73.42	,
	82.41	,
	87.31	,
	98.00	,
	110.00	,
	123.47	,
	130.81	,
	146.83	,
	164.81	,
	174.61	,
	196.00	,
	220.00	,
	246.94	,
	261.63	,
	293.66	,
	329.63	,
	349.23	,
	392.00	,
	440.00	,
	493.88	,
	523.25	,
	587.33	,
	659.25	,
	698.46	,
	783.99	,
	880.00	,
	987.77	,
	1046.50	,
	1174.66	,
	1318.51	,
	1396.91	,
	1567.98	,
	1760.00	,
	1975.53	,
	2093.00	,
	2349.32	,
2637.02};

static inline float get_frequency(float32_t frequency)
{
	uint32_t lo = 0; // 12; // lowest at C0
	uint32_t hi = 51; // 127;
	uint32_t mid;
	uint32_t d1;
	uint32_t d2;
	while (lo < hi)
	{
		mid = (hi + lo) >> 1;
		d1 = Abs(key_C[mid] - frequency);
		d2 = Abs(key_C[mid+1] - frequency);
		if (d2 <= d1)
		{
			lo = mid+1;
		}
		else
		{
			hi = mid;
		}
	}
	return key_C[hi];
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


static inline float atan2_approximation(float y, float x)
{
	float y_abs = Abs(y); 
	float x_abs = Abs(x); 

	float a = min(x_abs, y_abs) / max(x_abs, y_abs); 
	float s = a * a; 
	float r = ((-0.0464964749f * s + 0.15931422f) * s - 0.327622764f) * s * a + a; 
	if (y_abs > x_abs) 
		r =  M_PI_2 - r; 
	if (x < 0)
		r = M_PI - r; 
	if (y < 0)
		r = -r; 
		
	return r; 
}

static const float lp_filter_10000[] = {0.0607, 0.2266, 0.3323, 0.2266, 0.0607}; 
static const uint32_t lp_filter_10000_length = 5;

// defines for circular filtered buffer 
COMPILER_ALIGNED(WIN_SIZE) static float  x_in[WIN_SIZE]; 
COMPILER_ALIGNED(WIN_SIZE) static float workingBuffer[WIN_SIZE]; 
COMPILER_ALIGNED(WIN_SIZE) static float workingBuffer2[WIN_SIZE]; // needed since the fft corrupts the input ughhhh 

volatile float inputPitch; 

COMPILER_ALIGNED(STEP_SIZE) static float harmonized_output[2*STEP_SIZE];
COMPILER_ALIGNED(STEP_SIZE) static float harmonized_output_filt[2*STEP_SIZE];

volatile float pitch_shift; 

COMPILER_ALIGNED(FRAME_SIZE_2) static float _phas[FRAME_SIZE_2];
COMPILER_ALIGNED(FRAME_SIZE_2) static float _norm[FRAME_SIZE_2];

int main(void)
{
	sysclk_init();
	board_init();
	lcd_init(); 
	audio_init();
	configure_console();
	yin_t *yin_instance = yin_init();
	PSOLA_init();
	gfx_draw_filled_rect(100, 100, 20, 20, GFX_COLOR_YELLOW);
	
	uint32_t i,j;
	float  power;
	cvec_t *mags_and_phases = (cvec_t*)calloc(sizeof(cvec_t), 1); 
	mags_and_phases->length = FRAME_SIZE_2; 
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
			// store process buffer values into last quarter of input buffer 
			// TODO: going to have to end up changing DMA buffers again to do pitch detections every 1024 samples 
			arm_copy_f32((float  *)processBuffer, &x_in[WIN_SIZE-STEP_SIZE], STEP_SIZE); 
			
			// can use arm function! arm_power_f32 
			//power = get_average_power((float  *)&x[PROCESS_BUF_SIZE]);
										
			// apply hanning window 
			arm_mult_f32(x_in, (float32_t *)hanning, workingVec->data, WIN_SIZE); 
			
			// save input since fft changes it 
			arm_copy_f32(workingVec->data, workingVec2->data, workingVec->length); 
							
			// take fft of the windowed signal
			arm_rfft_fast_f32(&fftInstance, workingVec->data, yin_instance->samples_fft->data, 0);
						
			// compute magnitude and phase 
			arm_cmplx_mag_f32(yin_instance->samples_fft->data, mags_and_phases->norm, WIN_SIZE >> 1); 
			for (j = 0, i = 0; i < WIN_SIZE; i+=2, j++)
			{
				mags_and_phases->phas[j] = atan2_approximation(yin_instance->samples_fft->data[i+1], yin_instance->samples_fft->data[i]); 
			}
			
			// compute pitch -- requires prior fft in yin_instance -- corrupts samples_fft->data 
		    inputPitch = yin_get_pitch(yin_instance, workingVec2, &fftInstance);

			// debug frequency detection
			sprintf(str, "%f", inputPitch);
			printf("Freq: %s\n\r", str);
			
			if (inputPitch > 100)
			{
				float desiredPitch = get_frequency(inputPitch);
				pitch_shift = 2.0 - inputPitch/desiredPitch;
			}
			else 
				pitch_shift = 1.0; 

		    pitch_shift_do(&harmonized_output[lp_filter_10000_length], pitch_shift, mags_and_phases, &fftInstance);
			
			// lp - filter 10k cut off 
			arm_conv_f32(&harmonized_output[0], STEP_SIZE+lp_filter_10000_length, (float *)lp_filter_10000, lp_filter_10000_length, harmonized_output_filt); 
			
			// shift last filter length harmonized values for filter memory 
			arm_copy_f32(&harmonized_output[STEP_SIZE], &harmonized_output[0], lp_filter_10000_length);
			
			// TODO: keep in mind you have the 48KHz information from the inBuffer that you can use for the original voice 
			int processIdx = lp_filter_10000_length; 
			for(i = 0; i < IO_BUF_SIZE; i+=4)
			{
				outBuffer[i] = (uint16_t)(int16_t)(harmonized_output_filt[processIdx++] * (float)INT16_MAX); 
				outBuffer[i+1] = outBuffer[i]; 
				outBuffer[i+2] = outBuffer[i]; 
				outBuffer[i+3] = outBuffer[i];
			}
			
			// shift input back one quarter 
			arm_copy_f32(&x_in[STEP_SIZE], &x_in[0], WIN_SIZE-STEP_SIZE);
			
			dataReceived = false; 
		}
	}
}

