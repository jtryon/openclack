/*
             LUFA Library
     Copyright (C) Dean Camera, 2011.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2011  Dean Camera (dean [at] fourwalledcubicle [dot] com)

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
 *  Header file for Descriptors.c.
 */

#ifndef _DESCRIPTORS_H_
#define _DESCRIPTORS_H_

	/* Includes: */
		#include <avr/pgmspace.h>

		#include <LUFA/Drivers/USB/USB.h>

	/* Type Defines: */
		/** Type define for the device configuration descriptor structure. This must be defined in the
		 *  application code, as the configuration descriptor contains several sub-descriptors which
		 *  vary between devices, and which describe the device's usage to the host.
		 */
		typedef struct
		{
			USB_Descriptor_Configuration_Header_t Config;
			USB_Descriptor_Interface_t            HID1_KeyboardInterface;
			USB_HID_Descriptor_HID_t              HID1_KeyboardHID;
			USB_Descriptor_Endpoint_t             HID1_ReportINEndpoint;
			USB_Descriptor_Interface_t            HID2_MacroInterface;
			USB_HID_Descriptor_HID_t              HID2_MacroHID;
			USB_Descriptor_Endpoint_t             HID2_ReportINEndpoint;
			USB_Descriptor_Endpoint_t             HID2_ReportOUTEndpoint;
		} USB_Descriptor_Configuration_t;


/*		typedef struct
		{
			USB_Descriptor_Configuration_Header_t Config;
			USB_Descriptor_Interface_t            HID1_KeyboardInterface;
			USB_HID_Descriptor_HID_t              HID1_KeyboardHID;
			USB_Descriptor_Endpoint_t             HID1_ReportINEndpoint;
			USB_Descriptor_Interface_t            HID2_MouseInterface;
			USB_HID_Descriptor_HID_t              HID2_MouseHID;
			USB_Descriptor_Endpoint_t             HID2_ReportINEndpoint;
		} USB_Descriptor_Configuration_t;
*/

	/* Macros: */
		/** Endpoint number of the KEYB reporting IN endpoint. */
		//#define KEYBOARD_IN_EPNUM              1
		#define KEYBOARD_EPADDR              (ENDPOINT_DIR_IN | 1)

		/** Endpoint number of the macro reporting IN endpoint. */
		//#define MACRO_IN_EPNUM              2
		#define MACRO_EPADDR              (ENDPOINT_DIR_IN | 2)

		/** Endpoint number of the macro reporting OUT endpoint. */
		#define MACRO_OUT_EPADDR              (ENDPOINT_DIR_OUT | 3)

		/** Size in bytes of each of the HID reporting IN. */
		#define HID_EPSIZE                32

	/* Enums: */
		/** Enum for the HID report IDs used in the device. */
		enum
		{
			KEYBOARD_REPORTID_KeyboardReport = 0x01, /**< Report ID for the Keyboard report within the device. */
			KEYBOARD_REPORTID_MediaReport = 0x02, /**< Report ID for the Media controller report within the device. */
		} KEYBOARD_Report_IDs;

		enum
		{
			MACRO_REPORTID_KeyboardReport = 0x01, /**< Report ID for the Keyboard report within the device. */
			MACRO_REPORTID_MacroReport = 0x02, /**< Report ID for the LCD display */
			MACRO_REPORTID_LCDReport = 0x03, /**< Report ID for the G15 macro keys */
		} MACRO_Report_IDs;

	/* Function Prototypes: */
		uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
		                                    const uint8_t wIndex,
		                                    const void** const DescriptorAddress)
		                                    ATTR_WARN_UNUSED_RESULT ATTR_NON_NULL_PTR_ARG(3);

#endif

