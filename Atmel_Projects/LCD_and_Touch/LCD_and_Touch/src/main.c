#include "asf.h"
#include "LCDLib.h"
#include "main_menu.h"

int main(void)
{
	board_init();
	sysclk_init();
	lcd_init(); 

	start_gatorscribe(); 
	
	while(1);  
}
