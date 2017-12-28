/*
 * Touch.c
 *
 * Created: 12/24/2017 3:26:32 AM
 *  Author: Daniel Gonzalez
 */ 

/* Relies on ASF/common/components/touch/mxt */ 
#include "asf.h"
#include "touch.h"

/*********************************** Global Variables Start ***********************************/
static struct mxt_device device;
/*********************************** Global Variables End ***********************************/

/*********************************** Static Functions Start ***********************************/
static void mxt_make_highchg(struct mxt_device *data)
{
	struct mxt_conf_messageprocessor_t5 message;
	int count = 350;
	/* Read dummy message to make high CHG pin */
	do
	{
		mxt_read_message(data, &message);
	} while (--count);
}

static void get_finger_display_coordinates(const struct mxt_touch_event *touch_event, touch_t *touch_data)
{
	/* Display X-coordinate */ 
	touch_data->x = ((uint32_t)(4096 - touch_event->x) * gfx_get_width()) / 4096;

	/* Display Y-coordinate */ 
	touch_data->y = ((uint32_t)(4096 - touch_event->y) * gfx_get_height()) / 4096;

	/* Save the scaled size of the touch */
	touch_data->size = (touch_event->size * 4);
}
/*********************************** Static Functions End ***********************************/

/*********************************** Public Functions Start ***********************************/
void touch_handler(touch_t *touched_point)
{
	struct mxt_touch_event touch_event;	
	mxt_read_touch_event(&device, &touch_event);
	
	get_finger_display_coordinates(&touch_event, touched_point); 
	
	mxt_make_highchg(&device);
}

bool lcd_touched(void)
{
	if (mxt_is_message_pending(&device))
		return true; 
	else 
		return false; 
}

void mxt_init(void)
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
	status = mxt_init_device(&device, MAXTOUCH_TWI_INTERFACE,
			MAXTOUCH_TWI_ADDRESS, MAXTOUCH_XPRO_CHG_PIO);
	Assert(status == STATUS_OK);

	/* Issue soft reset of maXTouch device by writing a non-zero value to
	 * the reset register */
	mxt_write_config_reg(&device, mxt_get_object_address(&device,
			MXT_GEN_COMMANDPROCESSOR_T6, 0)
			+ MXT_GEN_COMMANDPROCESSOR_RESET, 0x01);

	/* Wait for the reset of the device to complete */
	delay_ms(MXT_RESET_TIME);

	// Write data to configuration registers in T7 configuration object 
	mxt_write_config_reg(&device, mxt_get_object_address(&device,
			MXT_GEN_POWERCONFIG_T7, 0) + 0, 0x20);
	mxt_write_config_reg(&device, mxt_get_object_address(&device,
			MXT_GEN_POWERCONFIG_T7, 0) + 1, 0x10);
	mxt_write_config_reg(&device, mxt_get_object_address(&device,
			MXT_GEN_POWERCONFIG_T7, 0) + 2, 0x4b);
	mxt_write_config_reg(&device, mxt_get_object_address(&device,
			MXT_GEN_POWERCONFIG_T7, 0) + 3, 0x84);

	// Write predefined configuration data to configuration objects 
	mxt_write_config_object(&device, mxt_get_object_address(&device,
			MXT_GEN_ACQUISITIONCONFIG_T8, 0), &t8_object);
	mxt_write_config_object(&device, mxt_get_object_address(&device,
			MXT_TOUCH_MULTITOUCHSCREEN_T9, 0), &t9_object);
	mxt_write_config_object(&device, mxt_get_object_address(&device,
			MXT_SPT_CTE_CONFIGURATION_T46, 0), &t46_object);
	mxt_write_config_object(&device, mxt_get_object_address(&device,
			MXT_PROCI_SHIELDLESS_T56, 0), &t56_object);

	// Issue recalibration command to maXTouch device by writing a non-zero
	 // value to the calibrate register 
	mxt_write_config_reg(&device, mxt_get_object_address(&device, MXT_GEN_COMMANDPROCESSOR_T6, 0) + MXT_GEN_COMMANDPROCESSOR_CALIBRATE, 0x01); 
	
	delay_ms(100); 
	
	// Make chg pin high 
	mxt_make_highchg(&device);
}

/*********************************** Public Functions End ***********************************/
