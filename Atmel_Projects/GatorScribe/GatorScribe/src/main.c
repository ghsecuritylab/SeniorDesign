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

const float poly_term_1 = -0.0464964749f; 
const float poly_term_2 = 0.15931422f; 
const float poly_term_3 = 0.327622764f; 
static inline float atan2_approximation(float y, float x)
{
	float y_abs = y; 
	float x_abs = x; 
	
	if (y_abs < 0) y_abs *= -1; 
	if (x_abs < 0) x_abs *= -1; 

	float a = min(x_abs, y_abs) / max(x_abs, y_abs); 
	float s = a * a; 
	float r = ((poly_term_1 * s + poly_term_2) * s - poly_term_3) * s * a + a; 
	if (y_abs > x_abs) 
		r =  M_PI_2 - r; 
	if (x < 0)
		r = M_PI - r; 
	if (y < 0)
		r = -r; 
		
	return r; 
}


// LP filter 10-4000Hz
static const float  lp_filter[] = {0.0027, 0.0103, 0.0258, 0.0499, 0.0801, 0.1105, 0.1332, 0.1416, 0.1332, 0.1105, 0.0801, 0.0499, 0.0258, 0.0103, 0.0027};
static const uint32_t lp_filter_length = 15;

static const float lp_filter_10000[] = {0.0607  ,  0.2266  ,  0.3323  ,  0.2266 ,   0.0607}; 
static const uint32_t lp_filter_10000_length = 5;

static inline void apply_lp_filter(float  *src, float  *dest, uint32_t sig_length)
{
	
	// can use arm convolution for this!
	uint32_t j,i; 
	/*
	for (j = 0; j < lp_filter_2000_length; j++)
		dest[j] = src[j];
		*/ 
	uint32_t k = 0; 
	for(j = lp_filter_10000_length; j < sig_length + lp_filter_10000_length; j++, k++)
	{
		dest[k] = 0;
		for(i = 0; i < lp_filter_10000_length; i++)
		{
			dest[k] += src[j-i]*lp_filter_10000[i];
		}
	}
	
	//arm_copy_f32(src, dest, sig_length); 
}

// defines for circular filtered buffer 
COMPILER_ALIGNED(WIN_SIZE) static float  x_filt[WIN_SIZE]; 
COMPILER_ALIGNED(WIN_SIZE) static float workingBuffer[WIN_SIZE]; 
COMPILER_ALIGNED(WIN_SIZE) static float workingBuffer2[WIN_SIZE]; // needed since the fft corrupts the input ughhhh 

volatile float  inputPitch; 

COMPILER_ALIGNED(STEP_SIZE) static volatile float harmonized_output[2*STEP_SIZE];
COMPILER_ALIGNED(STEP_SIZE) static volatile float harmonized_output_filt[STEP_SIZE];

volatile float pitch_shift; 

COMPILER_ALIGNED(FRAME_SIZE_2) static volatile float _phas[FRAME_SIZE_2];
COMPILER_ALIGNED(FRAME_SIZE_2) static volatile float _norm[FRAME_SIZE_2];

static const float32_t frequencies[128] = {
	8.176,8.662,9.177,9.723,10.301,10.913,11.562,12.250,12.978,13.750,14.568,15.434,16.352,17.324,18.354,19.445,20.602,21.827,23.125,24.500,
	25.957,27.500,29.135,30.868,32.703,34.648,36.708,38.891,41.203,43.654,46.249,48.999,51.913,55.000,58.270,61.735,65.406,69.296,73.416,
	77.782,82.407,87.307,92.499,97.999,103.826,110.000,116.541,123.471,130.813,138.591,146.832,155.563,164.814,174.614,184.997,195.998,
	207.652,220.000,233.082,246.942,261.626,277.183,293.665,311.127,329.628,349.228,369.994,391.995,415.305,440.000,466.164,493.883,
	523.251,554.365,587.330,622.254,659.255,698.456,739.989,783.991,830.609,880.000,932.328,987.767,1046.502,1108.731,1174.659,1244.508,
	1318.510,1396.913,1479.978,1567.982,1661.219,1760.000,1864.655,1975.533,2093.005,2217.461,2349.318,2489.016,2637.020,2793.826,2959.955,
	3135.963,3322.438,3520.000,3729.310,3951.066,4186.009,4434.922,4698.636,4978.032,5274.041,5587.652,5919.911,6271.927,6644.875,7040.000,
	7458.620,7902.133,8372.018,8869.844,9397.273,9956.063,10548.080,11175.300,11839.820,12543.850
};

static float get_frequency(float32_t frequency)
{
	uint32_t lo = 12; // lowest at C0
	uint32_t hi = 127;
	uint32_t mid;
	uint32_t d1;
	uint32_t d2;
	while (lo < hi)
	{
		mid = (hi + lo) >> 1;
		d1 = abs(frequencies[mid] - frequency);
		d2 = abs(frequencies[mid+1] - frequency);
		if (d2 <= d1)
		{
			lo = mid+1;
		}
		else
		{
			hi = mid;
		}
	}
	return frequencies[hi];
}

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
			// store lp-filtered values into last quarter of buffer 
			// TODO: going to have to end up changing DMA buffers again to do pitch detections every 1024 samples 
			//apply_lp_filter((float  *)processBuffer, &x_filt[WIN_SIZE-STEP_SIZE], STEP_SIZE);
			arm_copy_f32((float  *)processBuffer, &x_filt[WIN_SIZE-STEP_SIZE], STEP_SIZE); 
			
			// can use arm function! arm_power_f32 
			//power = get_average_power((float  *)&x[PROCESS_BUF_SIZE]);
										
			// apply hanning window 
			arm_mult_f32(x_filt, (float32_t *)hanning, workingVec->data, WIN_SIZE); 
			
			// save input since fft changes it 
			arm_copy_f32(workingVec->data, workingVec2->data, workingVec->length); 
							
			// take fft of the windowed signal
			arm_rfft_fast_f32(&fftInstance, workingVec->data, yin_instance->samples_fft->data, 0);
						
			// compute magnitude and phase 
			arm_cmplx_mag_f32(yin_instance->samples_fft->data, mags_and_phases->norm, WIN_SIZE >> 1); 
			arm_scale_f32(mags_and_phases->norm, 2.0, mags_and_phases->norm, mags_and_phases->length); 
				
			for (j = 0, i = 0; i < WIN_SIZE; i+=2, j++)
			{
				mags_and_phases->phas[j] = atan2_approximation(yin_instance->samples_fft->data[i+1], yin_instance->samples_fft->data[i]); 
			}
			
			// compute pitch -- requires prior fft in yin_instance -- corrupts samples_fft->data 
		    inputPitch = yin_get_pitch(yin_instance, workingVec2, &fftInstance);
			//inputPitch = 440.0; 
			// debug frequency detection
			sprintf(str, "%f", inputPitch);
			printf("Freq: %s\n\r", str);
							
			float  desiredPitch = get_frequency(inputPitch); 
			pitch_shift = 1.0 - (inputPitch - desiredPitch)/desiredPitch;
			//pitch_shift = 1.5; 
		    pitch_shift_do(&harmonized_output[STEP_SIZE], pitch_shift, mags_and_phases, &fftInstance);
			
			// can probably use circular buffer here if filtering needed 
			apply_lp_filter(&harmonized_output[STEP_SIZE - lp_filter_10000_length], harmonized_output_filt, STEP_SIZE); 
			
			for (i = 0; i < STEP_SIZE; i++)
			{
				harmonized_output[i] = harmonized_output[i + STEP_SIZE]; 
			}
			
			// TODO: interpolate 
			// TODO: keep in mind you have the 48KHz information from the inBuffer that you can use for the original voice 
			int processIdx = 0; 
			for(i = 0; i < IO_BUF_SIZE; i+=4)
			{
				outBuffer[i] = (uint16_t)(int16_t)(harmonized_output_filt[processIdx++] * INT16_MAX); // sound in / sound out
				outBuffer[i+1] = outBuffer[i]; 
				outBuffer[i+2] = outBuffer[i]; 
				outBuffer[i+3] = outBuffer[i]; 
			}
			
			// shift input back one quarter 
			//arm_copy_f32(&x_filt[STEP_SIZE], &x_filt[0], WIN_SIZE-STEP_SIZE); 
			for (i = 0; i < WIN_SIZE-STEP_SIZE; i++)
				x_filt[i] = x_filt[i + STEP_SIZE]; 
			
			dataReceived = false; 
		}
	}
}

