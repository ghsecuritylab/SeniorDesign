/*
 * Touch.h
 *
 * Created: 12/24/2017 3:26:46 AM
 *  Author: Daniel Gonzalez
 */ 


#ifndef TOUCH_H_
#define TOUCH_H_

#include <compiler.h>
#include <board.h>
#include "asf.h"
#include "gfx.h"

/*********************************** Defines Start ***********************************/
#define MAXTOUCH_TWI_INTERFACE           TWIHS0
#define MAXTOUCH_TWI_ADDRESS             0x4A
#define MAXTOUCH_XPRO_CHG_PIO			 PIO_PD28_IDX
#define MAXTOUCH_CHG_PIN				 PIO_PD28

/*********************************** Defines End ***********************************/

/*********************************** Data Structures Start ***********************************/
typedef struct touch {
	gfx_coord_t x; /**< X-coordinate on display */
	gfx_coord_t y; /**< Y-coordinate on display */
	uint8_t size;  /**< Size of the detected touch */
}touch_t;
/*********************************** Data Structures End ***********************************/

/*********************************** Public Functions Start ***********************************/
void touch_handler(touch_t *touched_point); 
bool lcd_touched(void); 
void mxt_init(void); 
/*********************************** Public Functions End ***********************************/

#endif /* TOUCH_H_ */