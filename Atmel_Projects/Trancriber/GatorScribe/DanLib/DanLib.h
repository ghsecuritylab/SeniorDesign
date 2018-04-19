/*
 * DanLib.h
 *
 * Created: 12/22/2017 4:17:28 AM
 *  Author: Daniel Gonzalez
 */ 


#ifndef DANLIB_H_
#define DANLIB_H_

#include "audio.h"
#include "LCDLib.h"
#include "main_menu.h"
#include "audio_to_midi.h"
#include "recording.h"
#include "MidiFile.h"
#include "DYWA/dywapitchtrack.h"
#include "pitchyinfast.h"

#define DISABLE_SYSTICK() (SysTick->CTRL  = 0)
#define ENABLE_SYSTICK() (SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk)

#define NUM_AVG_TAPS 4 

#define USART_SERIAL                 USART1
#define USART_SERIAL_ID              ID_USART1
#define USART_SERIAL_ISR_HANDLER     USART1_Handler
#define USART_SERIAL_BAUDRATE        115200
#define USART_SERIAL_CHAR_LENGTH     US_MR_CHRL_8_BIT
#define USART_SERIAL_PARITY          US_MR_PAR_NO
#define USART_SERIAL_STOP_BIT        US_MR_NBSTOP_1_BIT


#endif /* DANLIB_H_ */