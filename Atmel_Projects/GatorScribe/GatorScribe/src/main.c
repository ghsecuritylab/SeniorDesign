#include "asf.h"
#include "DanLib.h"
#include "arm_math.h"
#include <math.h>

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Standard demo includes. */
#include "TimerDemo.h"
#include "QueueOverwrite.h"
#include "EventGroupsDemo.h"
#include "IntSemTest.h"
#include "TaskNotify.h"
void vApplicationMallocFailedHook( void );
void vApplicationIdleHook( void );
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName );
void vApplicationTickHook( void );

extern const float hanning[1024];

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

static inline float get_frequency_from_key_C(float32_t frequency)
{
	uint32_t lo = 0;
	uint32_t hi = 51; 
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

static inline float get_frequency_from_all(float32_t frequency)
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
	return midi_note_frequencies[hi];
}

static float  get_average_power (float  *buffer, uint32_t size)
{
	uint32_t i;
	float  p = 0.0;
	for ( i = 0; i < size; i++)
	{
		p = p + buffer[i]*buffer[i];
	}
	return p;
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

/*************** Application code buffers and consts start ***************/

COMPILER_ALIGNED(WIN_SIZE) static float mixed_buffer[WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE) static float prev_input[WIN_SIZE];


/*************** Application code buffers and consts end ***************/

#define USART_SERIAL                 USART1
#define USART_SERIAL_ID              ID_USART1  
#define USART_SERIAL_ISR_HANDLER     USART1_Handler
#define USART_SERIAL_BAUDRATE        115200
#define USART_SERIAL_CHAR_LENGTH     US_MR_CHRL_8_BIT
#define USART_SERIAL_PARITY          US_MR_PAR_NO
#define USART_SERIAL_STOP_BIT        US_MR_NBSTOP_1_BIT

volatile float harmony_list_a[11]; 
volatile float harmony_list_b[11];
volatile float *harmony_list_read = harmony_list_a; 
volatile float *harmony_list_fill = harmony_list_b; 
volatile uint32_t harmony_idx = 0;  
void USART_SERIAL_ISR_HANDLER(void)
{
	uint32_t dw_status = usart_get_status(USART_SERIAL);
	if (dw_status & US_CSR_RXRDY) {
		uint32_t received_byte;
		usart_read(USART_SERIAL, &received_byte);
		//usart_write(USART_SERIAL, received_byte); // for debug 
		if (received_byte != 0 && harmony_idx < MAX_NUM_SHIFTS)
		{
			harmony_list_fill[harmony_idx] = midi_note_frequencies[received_byte]; 
			harmony_idx++;
		}
		else 
		{
			harmony_list_fill[harmony_idx] = END_OF_SHIFTS; 
			float *temp = (float *)harmony_list_read; 
			harmony_list_read = harmony_list_fill; 
			harmony_list_fill = temp; 
			harmony_idx = 0; 
		}
	}
}

int main(void)
{
	sysclk_init();
	board_init();
	SCB_DisableICache(); 
	lcd_init(); 
	SCB_EnableICache();
	audio_init();
	//configure_console();
	PSOLA_init(); 
	 
	SCB_DisableICache(); 
	gfx_draw_filled_rect(100, 100, 20, 20, GFX_COLOR_YELLOW);
	gfx_draw_filled_rect(200, 100, 20, 20, GFX_COLOR_YELLOW);
	gfx_draw_filled_rect(80, 180, 20, 20, GFX_COLOR_YELLOW);
	gfx_draw_filled_rect(100, 200, 20, 20, GFX_COLOR_YELLOW);
	gfx_draw_filled_rect(120, 220, 20, 20, GFX_COLOR_YELLOW);
	gfx_draw_filled_rect(140, 220, 20, 20, GFX_COLOR_YELLOW);
	gfx_draw_filled_rect(160, 220, 20, 20, GFX_COLOR_YELLOW);
	gfx_draw_filled_rect(180, 220, 20, 20, GFX_COLOR_YELLOW);
	gfx_draw_filled_rect(200, 200, 20, 20, GFX_COLOR_YELLOW);
	gfx_draw_filled_rect(220, 180, 20, 20, GFX_COLOR_YELLOW);
	SCB_EnableICache(); 
	
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
	
	// for serial debug 
	//char *str = (char *)calloc(20, sizeof(char)); 
	
	/*************** Application code variables start ***************/
	uint32_t i;
	
	float inputPitch; 

	for (i = 0; i < 11; i++)
	{
		harmony_list_a[i] = 0.0f; harmony_list_b[i] = 0.0f; 
	}
	
	
	float oneOverInputPitch, pitch_shift;
	bool prev_harmonize_option = false;
	bool new_harmonize_option = false;
	float harmony_shifts[MAX_NUM_SHIFTS+1]; arm_fill_f32(NO_SHIFT, harmony_shifts, MAX_NUM_SHIFTS);
	harmony_shifts[MAX_NUM_SHIFTS] = END_OF_SHIFTS;
	
	arm_fill_f32(0.0f, prev_input, WIN_SIZE); 
	/*************** Application code variables end ***************/

	while(1)
	{
		if (dataReceived)
		{	
		    inputPitch = computeWaveletPitch((float  *)processBuffer);
				
			//closest_note = get_frequency_from_all(inputPitch);

			if (inputPitch > MINIMUM_PITCH)
			{
				oneOverInputPitch = 1.0f / inputPitch;
				i = 0;
				while(harmony_list_read[i] > 1.0f && i < MAX_NUM_SHIFTS)
				{
					pitch_shift = 1.0f - (inputPitch-harmony_list_read[i])*oneOverInputPitch;
					harmony_shifts[i] = pitch_shift; 
					i++; 
				}
				if (i == 0)
				{
					harmony_shifts[0] = NO_SHIFT; 
					harmony_shifts[1] = END_OF_SHIFTS; 
				}
				else 
				{
					harmony_shifts[i] = END_OF_SHIFTS; 
				}
				new_harmonize_option = true; 
			} 
			else 
			{
				new_harmonize_option = false; 
				inputPitch = MINIMUM_PITCH; 
				harmony_shifts[0] = NO_SHIFT; // forces no pitch shift ... might need to revisit if this is a good idea 
				harmony_shifts[1] = END_OF_SHIFTS; 	
			}
			
			// return pitch shifted data from previous samples block  
			create_harmonies((float  *)processBuffer, mixed_buffer, inputPitch, harmony_shifts); 
			
			if (prev_harmonize_option)
			{
				arm_scale_f32(mixed_buffer, 0.95f, mixed_buffer, WIN_SIZE);
				arm_add_f32(prev_input, mixed_buffer, mixed_buffer, WIN_SIZE);
				//arm_add_f32((float *)processBuffer, mixed_buffer, mixed_buffer, WIN_SIZE);
				arm_scale_f32(mixed_buffer, (float)INT16_MAX * 0.5f , mixed_buffer, WIN_SIZE);
			}
			else 
			{
				arm_scale_f32(prev_input, (float)INT16_MAX, mixed_buffer, WIN_SIZE); 
				//arm_scale_f32((float *)processBuffer, (float)INT16_MAX, mixed_buffer, WIN_SIZE); 
			}
			prev_harmonize_option = new_harmonize_option; 
			arm_copy_f32((float *)processBuffer, prev_input, WIN_SIZE); 
			

			uint32_t idx = 0; 
			for(i = 0; i < IO_BUF_SIZE; i+=2)
			{
				outBuffer[i] = (uint16_t)(int16_t)(mixed_buffer[idx++]);  
				outBuffer[i+1] = outBuffer[i]; 
			}
			
			dataReceived = false; 
		}
	}
}

void vApplicationMallocFailedHook( void )
{
	/* Called if a call to pvPortMalloc() fails because there is insufficient
	free memory available in the FreeRTOS heap.  pvPortMalloc() is called
	internally by FreeRTOS API functions that create tasks, queues, software
	timers, and semaphores.  The size of the FreeRTOS heap is set by the
	configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */

	/* Force an assert. */
	configASSERT( ( volatile void * ) NULL );
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected. */

	/* Force an assert. */
	configASSERT( ( volatile void * ) NULL );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
volatile size_t xFreeHeapSpace;

	/* This is just a trivial example of an idle hook.  It is called on each
	cycle of the idle task.  It must *NOT* attempt to block.  In this case the
	idle task just queries the amount of FreeRTOS heap that remains.  See the
	memory management section on the http://www.FreeRTOS.org web site for memory
	management options.  If there is a lot of heap memory free then the
	configTOTAL_HEAP_SIZE value in FreeRTOSConfig.h can be reduced to free up
	RAM. */
	xFreeHeapSpace = xPortGetFreeHeapSize();

	/* Remove compiler warning about xFreeHeapSpace being set but never used. */
	( void ) xFreeHeapSpace;
}
/*-----------------------------------------------------------*/

void vApplicationTickHook( void )
{
	#if mainCREATE_SIMPLE_BLINKY_DEMO_ONLY == 0
	{
		/* The full demo includes a software timer demo/test that requires
		prodding periodically from the tick interrupt. */
		vTimerPeriodicISRTests();

		/* Call the periodic queue overwrite from ISR demo. */
		vQueueOverwritePeriodicISRDemo();

		/* Call the periodic event group from ISR demo. */
		vPeriodicEventGroupsProcessing();

		/* Call the code that uses a mutex from an ISR. */
		vInterruptSemaphorePeriodicTest();

		/* Call the code that 'gives' a task notification from an ISR. */
		xNotifyTaskFromISR();
	}
	#endif
}
/*-----------------------------------------------------------*/

/* Just to keep the linker happy. */
int __write( int x );
int __write( int x )
{
	return x;
}


