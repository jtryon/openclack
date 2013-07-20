/*
             LUFA Library
     Copyright (C) Dean Camera, 2010.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2010  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Header file for dashkey.c.
 */

#ifndef _DASHKEY_H_
#define _DASHKEY_H_

	//Leave defined to compile in support for key LED functionality; comment out for matrix only
	#define LED_MATRIX_SUPPORT

	/* Includes: */
	#include <avr/io.h>
	#include <avr/wdt.h>
	#include <avr/power.h>
	#include <avr/interrupt.h>

	#include <avr/pgmspace.h> //Allows use of Flash memory for storing variables

//	#include <util/delay.h>
	#include <stdbool.h>
	#include <string.h>

	#include "Descriptors.h"

	#include <LUFA/Version.h>
//	#include <LUFA/Drivers/Board/Joystick.h>
	#include <LUFA/Drivers/Board/LEDs.h>
//	#include <LUFA/Drivers/Board/Buttons.h>
	#include <LUFA/Drivers/USB/USB.h>

	/* Macros: */
	/** LED mask for the library LED driver, to indicate that the USB interface is not ready. */
//	#define LEDMASK_USB_NOTREADY      LEDS_LED1

	/** LED mask for the library LED driver, to indicate that the USB interface is enumerating. */
//	#define LEDMASK_USB_ENUMERATING  (LEDS_LED2 | LEDS_LED3)

	/** LED mask for the library LED driver, to indicate that the USB interface is ready. */
//	#define LEDMASK_USB_READY        (LEDS_LED2 | LEDS_LED4)

	/** LED mask for the library LED driver, to indicate that an error has occurred in the USB interface. */
//	#define LEDMASK_USB_ERROR        (LEDS_LED1 | LEDS_LED3)

	/** Type define for a Media Control HID report. This report contains the bits to match the usages defined
	 *  in the HID report of the device. When set to a true value, the relevant media controls on the host will
	 *  be triggered.
	 */
	typedef struct
	{
		unsigned Play           : 1;
		unsigned NextTrack      : 1;
		unsigned PreviousTrack  : 1;
		unsigned Stop           : 1;
		unsigned PlayPause      : 1;
		unsigned Mute           : 1;
		unsigned VolumeUp       : 1;
		unsigned VolumeDown     : 1;
	} ATTR_PACKED USB_MediaReport_Data_t;

	//Type define for a G15 macro report
	typedef struct
	{
		uint8_t Byte0;
		uint8_t Byte1;
		uint8_t Byte2;
		uint8_t Byte3;
		uint8_t Byte4;
		uint8_t Byte5;
		uint8_t Byte6;
		uint8_t Byte7;
	} ATTR_PACKED USB_MacroReport_Data_t;

	/* Function Prototypes: */
	typedef void (*AppPtr_t)(void) __attribute__ ((noreturn));
	void runbootloader(void);
	void resetmacros(void);

	void SetupHardware(void);
	void Timer_Init(void);
	void KeyMatrix_Init(void);

	void process_keys(void);

	#ifdef LED_MATRIX_SUPPORT
		//**********************************************
		//LED driver operations. Used for key animation
		//**********************************************
		//LED driver function. Selects an animation pattern and updates the ledmatrix[][] array.
		void led_driver(void);

		//Full-keyboard operations
		void ledanim_clear(void);
		void ledanim_full(uint8_t brightness);

		//Single-key operations
		void ledanim_lightkey(uint8_t keycode, uint8_t brightness);
		void ledanim_clearkey(uint8_t keycode);

		//Flip operations (turns off keys on and on keys off)
		void ledanim_flip(void);
		void ledanim_flipkey(uint8_t keycode);

		//Batch operations on full rows/columns
		void ledanim_lightcolumn(uint8_t ledmatrixcolumn, uint8_t brightness);
		void ledanim_lightrow(uint8_t ledmatrixrow, uint8_t brightness);
		void ledanim_clearcolumn(uint8_t ledmatrixcolumn);
		void ledanim_clearrow(uint8_t ledmatrixrow);
	#endif

	void SPI_MasterInit(void);
	void SPI_MasterTransmit(uint8_t cData);

	void EVENT_USB_Device_Connect(void);
	void EVENT_USB_Device_Disconnect(void);
	void EVENT_USB_Device_ConfigurationChanged(void);
	void EVENT_USB_Device_ControlRequest(void);
	void EVENT_USB_Device_StartOfFrame(void);

	bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
											 uint8_t* const ReportID,
											 const uint8_t ReportType,
											 void* ReportData,
											 uint16_t* const ReportSize);

	void pack_gkey(USB_MacroReport_Data_t *MacroReport, uint8_t gkey);

	void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
											  const uint8_t ReportID,
											  const uint8_t ReportType,
											  const void* ReportData,
											  const uint16_t ReportSize);

#endif

