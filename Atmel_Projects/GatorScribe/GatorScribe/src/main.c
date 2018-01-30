#include "asf.h"
#include "DanLib.h"
#include "hanning.h"
#include "cvec.h"

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
static uint32_t lp_filter_length = 15;
static float32_t y[PROCESS_BUF_SIZE]; // buffer for processed input
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
	uint32_t j,i;
	float32_t power;
	cvec_t *phase = new_cvec(PROCESS_BUF_SIZE); 
	printf("Hellooooo\n\n\n\r"); 
	char str[20]; 
	while(1)
	{
		if (dataReceived)
		{
			
			fvec_t input;
			input.data = (smpl_t *)processBuffer;
			input.length = PROCESS_BUF_SIZE;
			
			power = get_average_power((float32_t *)input.data);
				
			if (power > 0.1)
			{
				// LP-filtering		
				fvec_t processed_input;
				processed_input.data = (smpl_t *)y;
				processed_input.length = PROCESS_BUF_SIZE;
				for (j = 0; j < lp_filter_length; j++)
					processed_input.data[j] = input.data[j];
				for(j = lp_filter_length; j < input.length; j++)
				{
					processed_input.data[j] = 0;
					for(i = 0; i < lp_filter_length; i++)
					{
						processed_input.data[j] += input.data[j-i]*lp_filter[i];
					}
				}
				
				// apply hanning window
				for (i = 0; i < processed_input.length; i++)
					processed_input.data[i] *= hanning[i];
				
				float32_t freq = aubio_pitchyinfast_do(yin_instance, &processed_input, phase); 
				sprintf(str, "%f", freq);
				printf("Freq: %s\n\r", str); 
						
				pitchCorrect((float32_t *)processBuffer, YIN_FFT_SAMPLING_RATE, freq, 220.0); 
			}
			
			// TODO: interpolate 
		
			int processIdx = 0;
			for(i = 0; i < BUF_SIZE; i+=4)
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

