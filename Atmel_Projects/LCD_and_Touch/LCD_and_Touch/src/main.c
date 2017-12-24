#include <board.h>
#include <sysclk.h>
#include "asf.h"
#include "conf_board.h"
#include "conf_example.h"

#include "LCDLib.h"
#include "Keyboard.h"
#define MAXTOUCH_XPRO_CHG_PIO     PIO_PD28_IDX
const char example_string[] = "Touch!";

static void mxt_make_highchg(struct mxt_device *data)
{
	struct mxt_conf_messageprocessor_t5 message;
	int count = 200;
	/* Read dummy message to make high CHG pin */
	do 
	{
		mxt_read_message(data, &message);
	} while (--count); 
}

static void mxt_init(struct mxt_device *device)
{
	enum status_code status;

	/* T8 configuration object data */
	uint8_t t8_object[] = {
		0x0d, 0x00, 0x05, 0x0a, 0x4b, 0x00, 0x00,
		0x00, 0x32, 0x19
	};

	/* T9 configuration object data */
	uint8_t t9_object[] = {
		0x8B, 0x00, 0x00, 0x0E, 0x08, 0x00, 0x80,
		0x32, 0x05, 0x02, 0x0A, 0x03, 0x03, 0x20,
		0x02, 0x0F, 0x0F, 0x0A, 0x00, 0x00, 0x00,
		0x00, 0x18, 0x18, 0x20, 0x20, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x02,
		0x02
	};

	/* T46 configuration object data */
	uint8_t t46_object[] = {
		0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x03,
		0x00, 0x00
	};
	
	/* T56 configuration object data */
	uint8_t t56_object[] = {
		0x02, 0x00, 0x01, 0x18, 0x1E, 0x1E, 0x1E,
		0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E,
		0x1E, 0x1E, 0x1E, 0x1E, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00
	};

	/* TWI configuration */
	twihs_master_options_t twi_opt = {
		.speed = MXT_TWI_SPEED,
		.chip  = MAXTOUCH_TWI_ADDRESS,
	};
	
	/* Configure IRQ pin for MaxTouch */
	pio_configure(PIOD, PIO_INPUT, PIO_PD28, PIO_PULLUP);

	status = (enum status_code)twihs_master_setup(MAXTOUCH_TWI_INTERFACE, &twi_opt);
	Assert(status == STATUS_OK);

	/* Initialize the maXTouch device */
	status = mxt_init_device(device, MAXTOUCH_TWI_INTERFACE,
			MAXTOUCH_TWI_ADDRESS, MAXTOUCH_XPRO_CHG_PIO);
	Assert(status == STATUS_OK);

	/* Issue soft reset of maXTouch device by writing a non-zero value to
	 * the reset register */
	mxt_write_config_reg(device, mxt_get_object_address(device,
			MXT_GEN_COMMANDPROCESSOR_T6, 0)
			+ MXT_GEN_COMMANDPROCESSOR_RESET, 0x01);

	/* Wait for the reset of the device to complete */
	delay_ms(MXT_RESET_TIME);

	// Write data to configuration registers in T7 configuration object 
	mxt_write_config_reg(device, mxt_get_object_address(device,
			MXT_GEN_POWERCONFIG_T7, 0) + 0, 0x20);
	mxt_write_config_reg(device, mxt_get_object_address(device,
			MXT_GEN_POWERCONFIG_T7, 0) + 1, 0x10);
	mxt_write_config_reg(device, mxt_get_object_address(device,
			MXT_GEN_POWERCONFIG_T7, 0) + 2, 0x4b);
	mxt_write_config_reg(device, mxt_get_object_address(device,
			MXT_GEN_POWERCONFIG_T7, 0) + 3, 0x84);

	// Write predefined configuration data to configuration objects 
	mxt_write_config_object(device, mxt_get_object_address(device,
			MXT_GEN_ACQUISITIONCONFIG_T8, 0), &t8_object);
	mxt_write_config_object(device, mxt_get_object_address(device,
			MXT_TOUCH_MULTITOUCHSCREEN_T9, 0), &t9_object);
	mxt_write_config_object(device, mxt_get_object_address(device,
			MXT_SPT_CTE_CONFIGURATION_T46, 0), &t46_object);
	mxt_write_config_object(device, mxt_get_object_address(device,
			MXT_PROCI_SHIELDLESS_T56, 0), &t56_object);

	// Issue recalibration command to maXTouch device by writing a non-zero
	 // value to the calibrate register 
	mxt_write_config_reg(device, mxt_get_object_address(device, MXT_GEN_COMMANDPROCESSOR_T6, 0) + MXT_GEN_COMMANDPROCESSOR_CALIBRATE, 0x01); 
	
	delay_ms(100); 
	// Make chg pin high 
	mxt_make_highchg(device);
}



#define MAX_ENTRIES        2
#define STRING_LENGTH     40

static void Touch_Handler(struct mxt_device *device)
{
	uint8_t i = 0;

	// Temporary touch event data struct
	struct mxt_touch_event touch_event;

	// Collect touch events and put the data in a string,
	// maximum 2 events at the time
	do {
		// Temporary buffer for each new touch event line
		//char buf[STRING_LENGTH];
			
		// Read next next touch event in the queue, discard if read fails
		if (mxt_read_touch_event(device, &touch_event) != STATUS_OK) {
			continue;
		}

		// Format a new entry in the data string that will be sent over USART
		//sprintf(buf, "Nr: %1d, X:%4d, Y:%4d, Status:0x%2x\n\r",
		//touch_event.id, touch_event.x, touch_event.y,
		//touch_event.status);

		// Add the new string to the string buffer
		//strcat(tx_buf, buf);
		i++;

		//Check if there is still messages in the queue and
		//if we have reached the maximum numbers of events
	} while ((mxt_is_message_pending(device)) & (i < MAX_ENTRIES));
	
	mxt_make_highchg(device); 
}

int main(void)
{
	struct mxt_device device; 
	struct mxt_touch_event touch_event;
	board_init();
	sysclk_init();

	gfx_init();
	mxt_init(&device);

	/* Draw the keyboard at the bottom of the screen */
	gfx_draw_bitmap(&keyboard, (gfx_get_width() - keyboard.width) / 2, gfx_get_height() - keyboard.height);
	int ycoord = 2; 	
	while (1) {
		if (mxt_is_message_pending(&device)) {
			Touch_Handler(&device);
			gfx_draw_string_aligned(example_string,
				0, ycoord, &sysfont,
				GFX_COLOR_TRANSPARENT, GFX_COLOR_RED,
				TEXT_POS_LEFT, TEXT_ALIGN_LEFT);
			
			ycoord+= 20;
			//delay_ms(1000);
		}
	}
}
