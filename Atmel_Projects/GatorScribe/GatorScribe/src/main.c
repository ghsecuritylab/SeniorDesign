#include <asf.h>
#include "DanLib.h"
//#include <math.h>
//#include "arm_math.h"

static volatile float pitch; 
extern volatile bool outOfTime; 
int main(void)
{
	sysclk_init();
	board_init();
	lcd_init(); 
	Yin_init(PROCESS_BUF_SIZE, YIN_DEFAULT_THRESHOLD);
	audio_init();

	
	//start_gatorscribe();
	int16_t *audio; 
	while(1)
	{
		if (dataReceived)
		{
			
			audio = (int16_t *)processBuffer; 
			/*
			for(int i = 0; i < TOTAL_PROCESS_BUFFERS; i++)
			{
				Yin_init(&yin, BUF_SIZE, 0.05);
				pitch += Yin_getPitch(&yin, audio);
				audio += BUF_SIZE;
			}
			*/
			pitch = Yin_getPitch((int16_t *)processBuffer);
			
			// Average pitch 
			//pitch = pitch / TOTAL_PROCESS_BUFFERS; 
			
			dataReceived = false; 
		}

	}
}

