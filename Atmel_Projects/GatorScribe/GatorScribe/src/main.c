#include <asf.h>
#include "DanLib.h"

int main(void)
{
	sysclk_init();
	board_init();
	lcd_init();
	audio_init(); 
	start_gatorscribe();
	while(1); 
}

