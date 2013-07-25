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
 *  Main source file for the dashkey open-source keyboard controller
 *  The code is responsible for key detection, USB interfacing, macro support and (optionally) LED output
 */

#include "dashkey.h"

//eeprom_write_block((const void*)&x, (void*)&eeWayPts, 1);
//eeprom_read_block((void*)&y, (const void*)&eeWayPts, 1);
//pgm_read_byte(&(keycodes[i][j]))

#ifdef LED_MATRIX_SUPPORT
	uint8_t ledshift = 0; //#power transistor and PWM selection variable
	uint8_t ledmodeset = 0; //#The currently set LED mode. Can be overriden by Fn
	uint8_t	ledanimationkey = 0; //#Used for tracking iterations in LED animations
	bool 	ledanimationtoggle = 0; //#Used for tracking  state flips or ticks in LED animations
	uint8_t keytrails[12][2] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}; //tracks pressed keys and their timeouts for keytrails
	uint8_t ledmatrix[8][16]; //#stores matrix of active LEDs. One LED is stored per byte, with the upper 4 bits being D/C and the lowest bit being an on/off state.
							  //Upper 3 bits of the lower nibble are brightness value (0-7)
							  //0-7 is MSB, 8-15 is LSB (MSB is sent first). 5-7 are null.

#endif

uint8_t testcount = 0; //Test count to flash the teensy's built-in LED

uint8_t keyshift = 0; //#shift variable for keymatrix
uint8_t keymatrixA[16]; //#stores matrix of detected keys

uint8_t reportcodes[6] = {0}; //#stores set of current keys to report
uint8_t newcodes[6] = {0}; //stores just-acquired set of keys for debouncing/input
uint8_t oldcodes[6] = {0}; //stores set of previously acquired (5ms or 10ms ago) keys for debouncing
uint8_t reportmodifiers = 0; //#stores set of current modifiers to report
uint8_t newmodifiers = 0; //stores set of just-acquired modifiers  for debouncing/input
uint8_t oldmodifiers = 0; //stores set of previously acquired  (5ms ago) modifiers  for debouncing

//Macro variables
uint8_t macromatrix[16]; //Stores a bit matrix of keys currently set to use a macro key instead of their default behaviour.
						 //Used for doing a quick check if we should iterate through the macro list or not to find an active key
uint8_t macrolist[18];      //Stores the array of active macro key to keycode mappings. Currently long enough to have 18 macros, will need to extend
                  	  	 //if we want to support MR, M1-3 and LCD keys as well
uint8_t reportmacros[3]; //bitstuffed array containing current macros to report. Space for 24 keys, currently using 18
uint8_t newmacros[3];	 //bitstuffed array containing just-acquired macro keys
uint8_t oldmacros[3];	//bitstuffed array containing previously acquired macros for debouncing
uint8_t macro_waitforkey; //Used for timing the addition of macro keys to the array when setting

//TODO: We should migrate this into eeprom so that it can be reprogrammed
uint8_t keycodes[16][8] PROGMEM = {{0x2E, 0x25, 0x36, 0x03, 0x0E, 0x3F, 0x0C, 0x30}, //keycode matrix; edit this to change keymap. Fn slotted in as 0xFF
							       {0x41, 0x26, 0x37, 0x65, 0x0F, 0x03, 0x12, 0x40},
							       {0x22, 0x21, 0x19, 0x05, 0x09, 0x0A, 0x15, 0x17},
							       {0x23, 0x24, 0x10, 0x11, 0x0D, 0x0B, 0x18, 0x1C},
							       {0x35, 0x1E, 0x1D, 0x03, 0x04, 0x29, 0x14, 0x2B},
							       {0xE0, 0x3E, 0xE4, 0x03, 0x04, 0x03, 0x48, 0x06},
							       {0x3B, 0x20, 0x06, 0x03, 0x07, 0x3D, 0x08, 0x3C},
							       {0x3A, 0x1F, 0x1B, 0x03, 0x16, 0x03, 0x1A, 0x39},
							       {0x4A, 0x4D, 0x03, 0x50, 0x58, 0x52, 0x57, 0x07},
							       {0x03, 0x03, 0x03, 0x03, 0xE5, 0x03, 0x03, 0xE1},
							       {0x49, 0x03, 0x54, 0x4F, 0x5A, 0x62, 0x60, 0x5D},
							       {0x4B, 0x4E, 0x55, 0x56, 0x5B, 0x63, 0x61, 0x5E},
							       {0x42, 0x43, 0x28, 0x45, 0x31, 0x44, 0x1A, 0x2A},
							       {0x4C, 0x03, 0x53, 0x51, 0x59, 0x2C, 0x5F, 0x5C},
							       {0x2D, 0x27, 0x03, 0x38, 0x33, 0x34, 0x13, 0x2F},
							       {0x03, 0x46, 0x03, 0xE6, 0xFF, 0xE2, 0x47, 0xE3}};

#ifdef LED_MATRIX_SUPPORT
//TODO: We should migrate this into eeprom so that it can be reprogrammed
uint8_t keytoled[7][16] PROGMEM = {{0x07, 0x07, 0x07, 0x07, 0x1B, 0x5C, 0x3C, 0x3B, 0x3A, 0x4B, 0x5B, 0x6B, 0x00, 0x7B, 0x01, 0x11}, //key to LED translation table
								   {0x7C, 0x6C, 0x10, 0x20, 0x1A, 0x4A, 0x2B, 0x5A, 0x7A, 0x4C, 0x2A, 0x2C, 0x6A, 0x1C, 0x19, 0x29},
								   {0x39, 0x49, 0x59, 0x69, 0x79, 0x0F, 0x1F, 0x2F, 0x41, 0x08, 0x5F, 0x0A, 0x3D, 0x3F, 0x4F, 0x30},
								   {0x40, 0x50, 0x07, 0x21, 0x31, 0x09, 0x02, 0x12, 0x22, 0x0B, 0x18, 0x28, 0x38, 0x48, 0x58, 0x68},
								   {0x78, 0x0E, 0x1E, 0x2E, 0x3E, 0x4E, 0x5E, 0x6E, 0x7E, 0x6F, 0x7F, 0x04, 0x60, 0x70, 0x44, 0x23},
								   {0x03, 0x13, 0x42, 0x14, 0x24, 0x34, 0x73, 0x63, 0x53, 0x52, 0x62, 0x72, 0x51, 0x61, 0x71, 0x54},
								   {0x64, 0x74, 0x33, 0x43, 0x07, 0x6D, 0x07, 0x07, 0x0D, 0x0C, 0x2D, 0x1D, 0x7D, 0x32, 0x4D, 0x5D}};
#endif

//G-key matrix; translates a G-key number (G1-G18) into its byte/bit value. Stored with the bit value in 4 LSB and byte value in 4 MSB
//The rest of the array stores, in order, MR, M1-M3, LCD0-LCD4
uint8_t gcodes[27] PROGMEM = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x10, 0x21, 0x32,
							   0x43, 0x54, 0x65, 0x02, 0x13, 0x24, 0x35, 0x46, 0x76,
							   0x62, 0x50, 0x61, 0x72, 0x77, 0x17, 0x27, 0x37, 0x47};

uint8_t funcmode = 0; //#Set to 1 when the Function key is held down, 2 when in macro programming mode
uint8_t update_keys = 0; //#keymatrix updater count; used to time 5ms increments to refill keymatrix
uint8_t update_keys_max = 2; //Maximum value for update_keys checking; used for debounce tweaking
uint8_t frame_update = 0; //#counter tracking timing for animation updates. 5ms base timer.

AppPtr_t BootStartPtr = (AppPtr_t)0x7000;

/** Buffer to hold the previously generated Keyboard HID report, for comparison purposes inside the HID class driver. */
static uint8_t PrevKeyboardHIDReportBuffer[sizeof(USB_KeyboardReport_Data_t)];

/** Buffer to hold the previously generated Keyboard HID report, for comparison purposes inside the HID class driver. */
static uint8_t PrevMacroHIDReportBuffer[sizeof(USB_MacroReport_Data_t)];

/** Buffer to hold the previously generated Keyboard HID report, for comparison purposes inside the HID class driver. */
static uint8_t PrevMediaHIDReportBuffer[sizeof(USB_MediaReport_Data_t)];


/** LUFA HID Class driver interface configuration and state information. This structure is
 *  passed to all HID Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_HID_Device_t Keyboard_HID_Interface =
 	{
		.Config =
			{
				.InterfaceNumber              = 0,

				{
					.Address              = KEYBOARD_EPADDR,
					.Size                 = HID_EPSIZE,
					.Banks                = 1,
				},

				.PrevReportINBuffer           = PrevKeyboardHIDReportBuffer,
				.PrevReportINBufferSize       = sizeof(PrevKeyboardHIDReportBuffer),
			},
    };

/** LUFA HID Class driver interface configuration and state information. This structure is
 *  passed to all HID Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_HID_Device_t Macro_HID_Interface =
 	{
		.Config =
			{
				.InterfaceNumber              = 0,

				{
					.Address              = MACRO_EPADDR,
					.Size                 = HID_EPSIZE,
					.Banks                = 1,
				},

				.PrevReportINBuffer           = PrevMacroHIDReportBuffer,
				.PrevReportINBufferSize       = sizeof(PrevMacroHIDReportBuffer),
			},
    };

/** LUFA HID Class driver interface configuration and state information. This structure is
 *  passed to all HID Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_HID_Device_t Media_HID_Interface =
 	{
		.Config =
			{
				.InterfaceNumber              = 0,

				{
					.Address              = MEDIA_EPADDR,
					.Size                 = HID_EPSIZE,
					.Banks                = 1,
				},

				.PrevReportINBuffer           = PrevMediaHIDReportBuffer,
				.PrevReportINBufferSize       = sizeof(PrevMediaHIDReportBuffer),
			},
    };

/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
#ifdef OPENCLACK //If we're using openclack, we read two bytes at a time
	static inline uint8_t readmatrixrow(void) {
		//TODO: Input a return for the first byte read
		return ~PINB;
	}
	static inline uint8_t readmatrixrow2(void) {
		return ~PINF;
	}
#else //Dashkey-custom reads a single byte and iterates more rows instead
	static inline uint8_t readmatrixrow(void) {
		return ~((PIND&0b01111111)<<1 | ((PINC>>2)&0b00000001)); //amalgamate PC2 and PD0-6 into an 8-bit int containing a single keymatrix row
	}
#endif

#ifndef OPENCLACK	//We only need SPI functionality if we're using dashkey-custom
static inline void LATCH_KEY(void) {PORTB |= 0x01; PORTB &= 0xFE;} //flips !SS (PB0) high then low to latch keymatrix driver
static inline void LATCH_LED(void) {PORTB |= (1<<PB5); PORTB &= ~(1<<PB5);} //flips PB5 high then low to latch LED drivers

/*
static inline void SPI8_Send(uint8_t cData) //Sends a single SPI byte. Typically unused, kept for compatibility
{
	SPDR = cData; //Start transmission
	while(!(SPSR & (1<<SPIF))); //Wait for the transmission to complete
}
*/

static inline void SPI16_Send(uint8_t bighalf,uint8_t smallhalf) //Sends two SPI bytes
{
	SPDR = bighalf; //Start transmission
	while(!(SPSR & (1<<SPIF))); //Wait for the transmission to complete
	SPDR = smallhalf; //Start transmission
	while(!(SPSR & (1<<SPIF))); //Wait for the transmission to complete
}

//These two static inlines are used to disable and enable TIMER0's interrupt briefly to prevent TIMER0 from pushing out an SPI update during led_driver updates which can cause flickering
static inline void et0i(void) {TIMSK0 |= (1 << 1);} // Enable TIMER0's CTC interrupt
static inline void dt0i(void) {TIMSK0 &= ~(1 << 1);} // Disable TIMER0's CTC interrupt
#endif


int main(void)
{
	SetupHardware();
	sei();

	for (;;)
	{
		HID_Device_USBTask(&Keyboard_HID_Interface);
		HID_Device_USBTask(&Macro_HID_Interface);
		HID_Device_USBTask(&Media_HID_Interface);
		USB_USBTask();
		process_keys();
#ifdef LED_MATRIX_SUPPORT
		led_driver();
#endif
	}
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
//TODO: Break this functionality out to allow support for different board layouts, like the LED and button files. We can hook the MCU type definition to decide what to use.
void SetupHardware()
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1<<WDRF);
	wdt_disable();

	/* Disable clock division */
    CLKPR=0x80; //set the prescaler change enable bit high
	CLKPR=0x00; //remove the /8 clock prescaler division and set change enable low

	/* Hardware Initialization */
#ifndef OPENCLACK
	DDRB |= (1<<DDB5)|(1<<DDB4); //set LED driver latch and dataflash !CS as output
	PORTB |= (1<<PB4); //Set dataflash !CS pin high at startup (chip disabled)
	PORTB &= ~(1<<PB5); //Set LED driver latch low at startup
	SPI_MasterInit();
#else
	DDRD |= (1<<DDD6); //Enable the PD6 LED pin
	PORTD |= (1<<PD6);
	DDRC |= 0b10011111; //Set the row pins as outputs
#endif
	LEDs_Init();
	KeyMatrix_Init();
	USB_Init();
	Timer_Init();

}

void Timer_Init(void){
	   TCCR0A |= (1 << 1); // Configure timer 0 for CTC mode (would use WGM01, but it's pointing to 3 for some reason...)
	   TIMSK0 |= (1 << 1); // Enable TIMER0's CTC interrupt
	   OCR0A   = 16; // Set CTC compare value to 16 (64uS at 16MHz AVR clock), with a prescaler of 64
	   TCCR0B |= ((1 << CS00) | (1 << CS01)); // Start timer at Fcpu/64, giving us a resolution of 4uS

	   TCCR1B |= (1 << WGM12) | (1 << CS11) | (1 << CS10); //set the 16-bit timer to CTC mode and use fclk/64 prescaler
	   OCR1AH = 0x04; //set the compare value to 1250, resulting in a 5ms (200Hz)  trigger, suitable for animation timing and debouncing
	   OCR1AL = 0xE2;
	   TIMSK1 |= (1<<1); //Enable TIMER1's CTC interrupt
}

#ifdef LED_MATRIX_SUPPORT
	void led_driver(void){ //LED control function, responsible for populating ledmatrix[][] with the right values to get sent out on timed interrupt
						   //In cases where it matters (when we clear then set, or set then clear LEDs in a single operation) we disable then re-enable the timer to prevent flickering
		switch((ledmodeset&(0xFF*!(bool)funcmode)) | ((funcmode)<<6)) //Selects an LED display pattern. Bitstuff the funcmode value into ledmodeset to detect Fn cases
		{
			case 0: //All LEDs off case
				if(frame_update >= 20) {
					ledanim_clear();
					frame_update = 0;
				}
				break;
			case 1: //All LEDs off case with lit status LEDs
				if(frame_update >= 20) {
					dt0i(); //Disables TIMER0's interrupt to prevent flickering
					ledanim_clear();
					if(LEDs_GetLEDs()&0x80) ledanim_lightkey(0x53, 0x07); //Num Lock
					if(LEDs_GetLEDs()&0x40) ledanim_lightkey(0x39, 0x07); //Caps Lock
					if(LEDs_GetLEDs()&0x20) ledanim_lightkey(0x47, 0x07); //Scroll Lock
					et0i(); //Enables TIMER0's interrupt again
					frame_update = 0;
				}
				break;
			case 2: //All LEDs on case
				if(frame_update >= 20) {
					ledanim_full(0x07);
					frame_update = 0;
				}
				break;
			case 3: //All LEDs on case with unlit status LEDs
				if(frame_update >= 20) {
					dt0i();
					ledanim_full(0x07);
					if(LEDs_GetLEDs()&0x80) ledanim_clearkey(0x53); //Num Lock
					if(LEDs_GetLEDs()&0x40) ledanim_clearkey(0x39); //Caps Lock
					if(LEDs_GetLEDs()&0x20) ledanim_clearkey(0x47); //Scroll Lock
					et0i();
					frame_update = 0;
				}
				break;
			case 4: //All LEDs on at minimum brightness
				if(frame_update >= 20) {
					ledanim_full(0x00);
					frame_update = 0;
				}
				break;
	/*		case 5: //Thatch pattern case
				if(frame_update >= 40) {
					dt0i();
					ledanim_full(0x07);
					ledanim_clearrow(1);
					ledanim_clearrow(3);
					ledanim_clearrow(5);
					ledanim_clearrow(7);
					//TODO: We don't have flipcolumn and fliprow functions yet
		//			ledanim_flipcolumn(0);
		//			ledanim_flipcolumn(2);
		//			ledanim_flipcolumn(4);
		//			ledanim_flipcolumn(9);
		//			ledanim_flipcolumn(11);
		//			ledanim_flipcolumn(13);
		//			ledanim_flipcolumn(15);
					et0i();
					frame_update = 0;
				}
				break;
	*/		case 6: //Alphabet soup
				if(frame_update >= 10) {
					dt0i();
					ledanim_clear();
					ledanim_lightkey(ledanimationkey+0x04,0x07);
					et0i();
					if(ledanimationkey>=0x19) ledanimationkey = 0x00;
					else ledanimationkey++;
					frame_update = 0;
				}
				break;
			case 7: //Fade test
				if(frame_update >= 20) {
					ledanim_full(ledanimationkey);
					if (ledanimationkey >= 7) ledanimationtoggle = 1;
					if (ledanimationkey == 0) ledanimationtoggle = 0;
					if(ledanimationtoggle) ledanimationkey--;
					else ledanimationkey++;
					frame_update = 0;
				}
				break;
			case 8: //SX Fading Logo  Test
				if(frame_update >= 40) {
					ledanim_clear();
					ledanim_lightkey(0x21,ledanimationkey);
					ledanim_lightkey(0x22,ledanimationkey);
					ledanim_lightkey(0x08,ledanimationkey);
					ledanim_lightkey(0x09,ledanimationkey);
					ledanim_lightkey(0x06,ledanimationkey);
					ledanim_lightkey(0x1B,ledanimationkey);
					ledanim_lightkey(0x24,ledanimationkey);
					ledanim_lightkey(0x26,ledanimationkey);
					ledanim_lightkey(0x18,ledanimationkey);
					ledanim_lightkey(0x0C,ledanimationkey);
					ledanim_lightkey(0x0D,ledanimationkey);
					ledanim_lightkey(0x0E,ledanimationkey);
					ledanim_lightkey(0x11,ledanimationkey);
					ledanim_lightkey(0x36,ledanimationkey);
					if (ledanimationkey >= 7) ledanimationtoggle = 1;
					if (ledanimationkey == 0) ledanimationtoggle = 0;
					if(ledanimationtoggle) ledanimationkey--;
					else ledanimationkey++;
					frame_update = 0;
				}
				break;
			case 9: //Key Trails
				if(frame_update >= 10) {
					ledanim_clear();
					uint8_t matrixrow;
					uint8_t matrixcol;
					for(matrixrow = 0;matrixrow <=5;matrixrow++) {
						if(reportcodes[matrixrow] == 0) break; //break at end of report
						for(matrixcol = 0; matrixcol <=11; matrixcol++) {
							if (reportcodes[matrixrow] == keytrails[matrixcol][0]) {
								keytrails[matrixcol][1] = 10;
								break;
							}

						}
						if(matrixcol == 12) {
							for(matrixcol = 0; matrixcol <=11; matrixcol++) {
								if (keytrails[matrixcol][1] == 0) {
									keytrails[matrixcol][0] = reportcodes[matrixrow];
									keytrails[matrixcol][1] = 10;
									break;
								}

							}
						}
					}

					for(matrixcol = 0; matrixcol <=11; matrixcol++) {
						if(keytrails[matrixcol][1] != 0) {
							if(keytrails[matrixcol][1] > 0x07) ledanim_lightkey(keytrails[matrixcol][0], 0x07);
							else ledanim_lightkey(keytrails[matrixcol][0], keytrails[matrixcol][1]);
							keytrails[matrixcol][1]--;
						}
					}
	//				if (ledanimationkey >= 7) ledanimationtoggle = 1;
	//				if (ledanimationkey == 0) ledanimationtoggle = 0;
	//				if(ledanimationtoggle) ledanimationkey--;
	//				else ledanimationkey++;
					frame_update = 0;
				}
				break;
			case 64: //Fn on indicator case
				if(frame_update >= 20) {
					dt0i();
					ledanim_clear();
					ledanim_lightkey(ledmodeset+0x1E, 0x07); //keys 1 through 0, based on mode
					if(update_keys_max == 1) ledanim_lightkey(0x4B,0x07); //If using fast debouncing (5ms), light PgUP
					else ledanim_lightkey(0x4E,0x07); //If using slow debouncing (10ms), light PgDn
	//				if(CLKSEL1&0x80) ledanim_lightkey(0x3E, 0x07); //Used for diagnostic output
	//				if(CLKSEL1&0x40) ledanim_lightkey(0x3F, 0x07);
	//				if(CLKSEL1&0x20) ledanim_lightkey(0x40, 0x07);
	//				if(CLKSEL1&0x10) ledanim_lightkey(0x41, 0x07);
	//				if(CLKSEL1&0x08) ledanim_lightkey(0x42, 0x07);
	//				if(CLKSEL1&0x04) ledanim_lightkey(0x43, 0x07);
	//				if(CLKSEL1&0x04) ledanim_lightkey(0x43, 0x07);
	//				if(CLKSEL0&0x01) ledanim_lightkey(0x45, 0x07);
					et0i();
					frame_update= 0;
				}
				break;
			case 5:
			case 128: //Fn on macro setting case
				if(frame_update >= 20) {
					dt0i();
					ledanim_clear();
					//Light keys set as macro keys
					for(ledanimationkey = 0;ledanimationkey < 18;ledanimationkey++) {
						if(macrolist[ledanimationkey] != 0) {
							ledanim_lightkey(macrolist[ledanimationkey],0x07);
						}
					}
					et0i();
					frame_update = 0;
				}
				break;
			default:
				break;
		}
	}

	void ledanim_clear(void) { //Clears the entire LED matrix table (sets brightness values to full, but in off state)
		uint8_t ledmatrixcolumn;
		uint8_t ledmatrixrow;
		for(ledmatrixrow = 0;ledmatrixrow<8;ledmatrixrow++) {
			for(ledmatrixcolumn = 0; ledmatrixcolumn <5; ledmatrixcolumn++) {
				ledmatrix[ledmatrixrow][ledmatrixcolumn] = 0x0E; //
			}
			for(ledmatrixcolumn = 8; ledmatrixcolumn <16; ledmatrixcolumn++) {
				ledmatrix[ledmatrixrow][ledmatrixcolumn] = 0x0E; //
			}
		}
	}

	void ledanim_full(uint8_t brightness) { //Fills the entire LED matrix table at a given brightness
		uint8_t ledmatrixcolumn;
		uint8_t ledmatrixrow;
		for(ledmatrixrow = 0;ledmatrixrow<8;ledmatrixrow++) {
			for(ledmatrixcolumn = 0; ledmatrixcolumn <5; ledmatrixcolumn++) {
				ledmatrix[ledmatrixrow][ledmatrixcolumn] = 0x01 | ((brightness&0x07)<<1); //
			}
			for(ledmatrixcolumn = 8; ledmatrixcolumn <16; ledmatrixcolumn++) {
				ledmatrix[ledmatrixrow][ledmatrixcolumn] = 0x01 | ((brightness&0x07)<<1); //
			}
		}
	}

	void ledanim_flip(void) {  //Inverts the current LED matrix table. Does not take brightness into account; uses whatever is already set.
		uint8_t ledmatrixcolumn;
		uint8_t ledmatrixrow;
		for(ledmatrixrow = 0;ledmatrixrow<8;ledmatrixrow++) {
			for(ledmatrixcolumn = 0; ledmatrixcolumn <5; ledmatrixcolumn++) {
				ledmatrix[ledmatrixrow][ledmatrixcolumn] ^= 0x01; //Flips the first bit
			}
			for(ledmatrixcolumn = 8; ledmatrixcolumn <16; ledmatrixcolumn++) {
				ledmatrix[ledmatrixrow][ledmatrixcolumn] ^= 0x01; //Flips the first bit
			}
		}
	}

	void ledanim_lightkey(uint8_t keycode, uint8_t brightness) { //Lights a single key at the given brightness
		uint8_t location;
		if((keycode&0xF0) == 0xE0) {//This is a modifier in the 0xEn block, so use different read syntax
			location = pgm_read_byte(&(keytoled[0x06][(keycode&0x07)+0x08]));
		}
		else location = pgm_read_byte(&(keytoled[keycode>>4][keycode&0x0F])); //grab a location key out of the translation matrix
		ledmatrix[location>>4][location&0x0F] = 0x01 | ((brightness&0x07)<<1); //Sets the first bit high and the next 3 bits to the brightness value
	}

	void ledanim_clearkey(uint8_t keycode) { //Clears a single key (off). Doesn't affect set brightness value.
		uint8_t location;
		if((keycode&0xF0) == 0xE0) {//This is a modifier in the 0xEn block, so use different read syntax
			location = pgm_read_byte(&(keytoled[0x06][(keycode&0x07)+0x08]));
		}
		else location = pgm_read_byte(&(keytoled[(keycode>>4)&0x0F][keycode&0x0F])); //grab a location key out of the translation matrix
		ledmatrix[location>>4][location&0x0F] &= 0xFE; //Sets the first bit low
	}

	void ledanim_flipkey(uint8_t keycode) { //Inverts a single key. Does not account for brightness, leaves as-is.
		uint8_t location;
		if((keycode&0xF0) == 0xE0) {//This is a modifier in the 0xEn block, so use different read syntax
			location = pgm_read_byte(&(keytoled[0x06][(keycode&0x07)+0x08]));
		}
		else location = pgm_read_byte(&(keytoled[(keycode>>4)&0x0F][keycode&0x0F])); //grab a location key out of the translation matrix
		ledmatrix[location>>4][location&0x0F] ^= 0x01; //Flips the first bit
	}

	void ledanim_lightcolumn(uint8_t ledmatrixcolumn, uint8_t brightness) { //Lights a single column (between 0-4 and 8-15)
		uint8_t ledmatrixrow;
		ledmatrixcolumn &= 0x0F;
		for(ledmatrixrow = 0;ledmatrixrow<8;ledmatrixrow++) {
				ledmatrix[ledmatrixrow][ledmatrixcolumn] = 0x01 | ((brightness&0x07)<<1); //Light the key at the given brightness
		}
	}

	void ledanim_lightrow(uint8_t ledmatrixrow, uint8_t brightness) { //Lights a single row (between 0 and 8) at the given brightness
		uint8_t ledmatrixcolumn;
		for(ledmatrixcolumn = 0; ledmatrixcolumn <5; ledmatrixcolumn++) {
			ledmatrix[ledmatrixrow][ledmatrixcolumn] = 0x01 | ((brightness&0x07)<<1); //Light the key at the given brightness
		} //We skip 5-7 as they're always zero with the current design
		for(ledmatrixcolumn = 8; ledmatrixcolumn <16; ledmatrixcolumn++) {
			ledmatrix[ledmatrixrow][ledmatrixcolumn] = 0x01 | ((brightness&0x07)<<1); //Light the key at the given brightness
		}
	}

	void ledanim_clearcolumn(uint8_t ledmatrixcolumn) { //Clears a single column (doesn't affect brightness value)
		uint8_t ledmatrixrow;
		ledmatrixcolumn &= 0x0F;
		for(ledmatrixrow = 0;ledmatrixrow<8;ledmatrixrow++) {
				ledmatrix[ledmatrixrow][ledmatrixcolumn] &= 0xFE; //Clear the key's on state
		}
	}

	void ledanim_clearrow(uint8_t ledmatrixrow) { //Clears a single row (doesn't affect brightness value)
		uint8_t ledmatrixcolumn;
		for(ledmatrixcolumn = 0; ledmatrixcolumn <5; ledmatrixcolumn++) {
			ledmatrix[ledmatrixrow][ledmatrixcolumn] &= 0xFE; //Clear the key's on state
		}
		for(ledmatrixcolumn = 8; ledmatrixcolumn <16; ledmatrixcolumn++) {
			ledmatrix[ledmatrixrow][ledmatrixcolumn] &= 0xFE; //Clear the key's on state
		}
	}
#endif

//pgm_read_byte(&(keytoled[i][j]))

#ifdef OPENCLACK //We use all of PB and PF as inputs for openclack
	void KeyMatrix_Init(void) {
		DDRB &= 0b00000000; //set PB as input
		DDRF &= 0b00000000; //set PF as input
		PORTB |= 0b11111111; //enable pull-up on PB
		PORTF |= 0b11111111; //enable pull-up on PF
	}
#else //We use PD plus PC2 for dashkey-custom
void KeyMatrix_Init(void) {
	DDRC &= 0b11111011; //set PC2 as input
	DDRD &= 0b10000000; //set PD0-6 as input
	PORTC |= 0b00000100; //enable pull-up on PC2
	PORTD |= 0b01111111; //enable pull-up on PD0-6
}
#endif

void process_keys(void) { //Handles debouncing and report-stuffing.  Should act every 5ms, otherwise terminate immediately
	uint8_t UsedKeyCodes = 0;
	uint8_t matrixrow;
	uint8_t matrixcol;
	uint8_t shiftval; //shift value for report-stuffing loop

	//This block handles converting the current keymatrix into newcodes[] and newmodifiers
	if(update_keys >= update_keys_max) {
		newmodifiers = 0;  //Clear the modifiers variable
		for(matrixrow = 0; matrixrow <6; matrixrow++) newcodes[matrixrow] = 0; //Clear the codes array
		newmacros[0]=0;newmacros[1]=0;newmacros[2]=0; //Reset newmacros
		for(matrixrow = 0; matrixrow <16; matrixrow++) { //Loop through all 16 rows, checking for pressed keys
			shiftval = 1;
			matrixcol = 0;
			while(shiftval) { //Loops through the byte, checking each bit for pressed keys
				if (keymatrixA[matrixrow] == 0) break; //if the row is equal to zero no point doing further work on it - no switches are pressed
				if (keymatrixA[matrixrow]&shiftval) { //the key at matrixrow & shiftval (matrixcol) is pressed down

					if(macromatrix[matrixrow]&shiftval) { //Pre-check if it's a macro-rebound key (the key at matrixrow & shiftval is *also* macro-modified
						uint8_t target_keycode = pgm_read_byte(&(keycodes[matrixrow][matrixcol])); //Store the keycode to compare against active macros
						bool found_macro = false;
						uint8_t target_macro = 0;
						for(;target_macro < 18;target_macro++) {
							if(target_keycode == macrolist[target_macro]) {
								found_macro = true;
								break;
							}
						}
						if(found_macro) {
							newmacros[target_macro/8] |= 1<<(target_macro%8); //slots a bit in
						}
					}
					else { //Otherwise check modifiers then regular keys
					  if((pgm_read_byte(&(keycodes[matrixrow][matrixcol]))&0xF8) == 0xE0) { //mask to see if keycode is in modifier range (0xE0-0xE7)
						  newmodifiers |= 1<<(pgm_read_byte(&(keycodes[matrixrow][matrixcol]))&0x07);//if so, stuff modifier instead of main keycode set
					  }
					  else {
						  if(UsedKeyCodes<6) {
							  newcodes[UsedKeyCodes++] = pgm_read_byte(&(keycodes[matrixrow][matrixcol]));
						  }
					  }
					}

				}
				shiftval <<= 1;
				matrixcol++;
			}
		}

		update_keys = 0;

		//Debounce by ORing newcodes[] with oldcodes[] and stuff result into reportcodes[]
		reportmodifiers = oldmodifiers | newmodifiers;
		reportmacros[0] = oldmacros[0] | newmacros[0]; //We stuff the macro report the same way as the modifier report
		reportmacros[1] = oldmacros[1] | newmacros[1];
		reportmacros[2] = oldmacros[2] | newmacros[2];
		for(matrixrow = 0; matrixrow < 6; matrixrow++) { //First, stuff reportcodes[] with oldcodes[]'s data. Don't break early if we find zeros as we're using this to clear reportcodes[]
			reportcodes[matrixrow] = oldcodes[matrixrow];
		}
		for(;matrixrow <6; matrixrow++) { //continue loop with new behavior once oldcodes has been fully read. Loop through remaining report slots and fill with newcodes[] that don't match existing values
			for(matrixcol = 0; matrixcol < 6; matrixcol++) { //have to check each newcodes position for each slot
				  if(newcodes[matrixcol] == 0) reportcodes[matrixrow] = 0x00; break; //Don't bother checking the leftover zero-values of newcodes[]
				  for(shiftval = 0;shiftval < 6;shiftval++) if((reportcodes[shiftval]) && (reportcodes[shiftval] == newcodes[matrixcol])) break;
				  if(shiftval == 6) reportcodes[matrixrow] = newcodes[matrixcol]; //If we make  it all the way through the loop there were no matches; stuff it
			}
		}

		//Swap newcodes[] into oldcodes[]
		oldmodifiers = newmodifiers;
		oldmacros[0] = newmacros[0];
		oldmacros[1] = newmacros[1];
		oldmacros[2] = newmacros[2];
		for(matrixrow  = 0; matrixrow < 6; matrixrow++) {
			oldcodes[matrixrow] = newcodes[matrixrow];
		}

	}
}

#ifndef OPENCLACK
void SPI_MasterInit(void)
{
	PRR0 &= 0xFB; //set SPI powersave bit low to activate it
	/* Set MOSI, SCK and SS to output*/
	DDRB |= (1<<DDB2)|(1<<DDB1)|(1<<DDB0);
	/* Enable SPI, MSB first, Master, clock idles low, rising edge data, set clock rate fck/4 */
	SPCR |= (1<<SPE)|(0<<DORD)|(1<<MSTR)|(0<<CPOL)|(0<<CPHA)|(0<<SPR1)|(0<<SPR0);

	//Set the SPI2X bit high, switching to fck/2 (should be 8MHz clockrate)
	SPSR |= (1<<SPI2X);
	PORTB &= 0xFE; //set !SS output low at initialise (latch input on LEDdrv)
}
#endif

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
//	LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
//	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	//ConfigSuccess &= HID_Device_ConfigureEndpoints(&Device_HID_Interface);
	ConfigSuccess &= HID_Device_ConfigureEndpoints(&Keyboard_HID_Interface);
	ConfigSuccess &= HID_Device_ConfigureEndpoints(&Macro_HID_Interface);
	ConfigSuccess &= HID_Device_ConfigureEndpoints(&Media_HID_Interface);

	USB_Device_EnableSOFEvents();

//	LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	HID_Device_ProcessControlRequest(&Keyboard_HID_Interface);
	HID_Device_ProcessControlRequest(&Macro_HID_Interface);
	HID_Device_ProcessControlRequest(&Media_HID_Interface);
}

/** Event handler for the USB device Start Of Frame event. */
void EVENT_USB_Device_StartOfFrame(void)
{
	//HID_Device_MillisecondElapsed(&Device_HID_Interface);
	HID_Device_MillisecondElapsed(&Keyboard_HID_Interface);
	HID_Device_MillisecondElapsed(&Macro_HID_Interface);
	HID_Device_MillisecondElapsed(&Media_HID_Interface);
}

/** HID class driver callback function for the creation of HID reports to the host.
 *
 *  \param[in]     HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in,out] ReportID    Report ID requested by the host if non-zero, otherwise callback should set to the generated report ID
 *  \param[in]     ReportType  Type of the report to create, either HID_REPORT_ITEM_In or HID_REPORT_ITEM_Feature
 *  \param[out]    ReportData  Pointer to a buffer where the created report should be stored
 *  \param[out]    ReportSize  Number of bytes written in the report (or zero if no report is to be sent
 *
 *  \return Boolean true to force the sending of the report, false to let the library determine if it needs to be sent
 */
bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo, uint8_t* const ReportID,
                                         const uint8_t ReportType, void* ReportData, uint16_t* const ReportSize)
{
	uint8_t  keycodeiterate = 0;
	uint8_t  innermatrix = 0;

	if (HIDInterfaceInfo == &Keyboard_HID_Interface || HIDInterfaceInfo == &Media_HID_Interface ) { //Do our usual matrix calculations if we're sending a keyboard or media report
			//Special tasks, with Fn modifier
		if((keymatrixA[15]&(1<<4)) && !(keymatrixA[5]&(1<<2)))  { //Detect Fn button being pressed down. Maybe we should refactor to use debounced? Takes more calculations...
			funcmode = 1; //Set funcmode to 1 so that the LED routine knows to show the Fn-modifier layout
			for(keycodeiterate = 0;keycodeiterate<6;keycodeiterate++) {
				if(reportcodes[keycodeiterate]==0) break; //if a position is zero, all others after it will be too so break out of loop
				if(reportcodes[keycodeiterate] == 0x29) runbootloader(); //If ESC key is pressed, trigger a bootloader restart to put keyboard into programming mode
				#ifdef LED_MATRIX_SUPPORT
					if(reportcodes[keycodeiterate]>=0x1E && reportcodes[keycodeiterate]<=0x27) ledmodeset = reportcodes[keycodeiterate]-0x1E;
				#endif
				if(reportcodes[keycodeiterate] == 0x4B) update_keys_max = 1; //If PgUP, require one timer tick to stuff a new report (shortest possible latency)
				if(reportcodes[keycodeiterate] == 0x4E) update_keys_max = 2; //If PgDN, require two timer ticks to stuff a new report (no double-presses)
			}
		}
		else {
			if((keymatrixA[15]&(1<<4)) && (keymatrixA[5]&(1<<2))) {
				if (funcmode == 1) resetmacros();
				funcmode = 2;
			}
			else {
				funcmode = 0;
			}
		}

		if(funcmode == 2) { //We're taking in a new set of macros to modify
			if(macro_waitforkey > 0) {
				if(reportcodes[0] == 0xFF && reportcodes[1] == 0x00) reportcodes[0] = 0; //Pre-interpreter to force a zero on nothing but Fn
				if(reportcodes[0] == 0xFF && reportcodes[1] != 0x00) reportcodes[0] = reportcodes[1]; //Pre-interpreter to copy the useful key to the 1st position if it shows up 2nd
				reportmodifiers &= ~0b00010000; //masks out right control (which we're using as our modifier-hold to be in macro setting mode)

				if(reportcodes[0] == 0 && reportmodifiers == 0 && macro_waitforkey%2) { //We're waiting for a key but don't have it yet - do nothing
				}
				if(reportcodes[0] == 0 && reportmodifiers == 0 && !(macro_waitforkey%2)) { //We're waiting for a release and just got it - reset to next state
					macro_waitforkey++;
					if(macro_waitforkey >= 37) macro_waitforkey = 0; //Block further table additions until we release/reset
				}
				if(reportcodes[0] != 0 && reportmodifiers == 0 && macro_waitforkey%2) {//We're waiting for a key and just got a keycode - add it to the macrolist and reset to next state
					macrolist[macro_waitforkey/2] = reportcodes[0];
					for(keycodeiterate = 0; keycodeiterate < 16;keycodeiterate++){
						for(innermatrix = 0;innermatrix < 8;innermatrix++){
							if(reportcodes[0] == pgm_read_byte(&(keycodes[keycodeiterate][innermatrix]))) {
								macromatrix[keycodeiterate] |= 1<<innermatrix;
							}
						}
					}
					macro_waitforkey++;
				}
				if(reportcodes[0] == 0 && reportmodifiers != 0 && macro_waitforkey%2) {//We're waiting for a key and just got a modifier - add it to the macrolist and reset to next state
					uint8_t modifier_position = 0;
					while (reportmodifiers >>= 1) //Finds the highest bit position that has a 1 in it; we use this to add to DF to get modifier report codes back (E0-E7)
					{
					  modifier_position++;
					}
					reportmodifiers = 1<<modifier_position; //restore reportmodifiers to prevent weirdness on the next pass
					macrolist[macro_waitforkey/2] = 0xE0+modifier_position;
					for(keycodeiterate = 0; keycodeiterate < 16;keycodeiterate++){
						for(innermatrix = 0;innermatrix < 8;innermatrix++){
							if(0xE0+modifier_position == pgm_read_byte(&(keycodes[keycodeiterate][innermatrix]))) {
								macromatrix[keycodeiterate] |= 1<<innermatrix;
							}
						}
					}
					macro_waitforkey++;
				}

				if((reportcodes[0] != 0 || reportmodifiers != 0) && !(macro_waitforkey%2)) {//We're waiting for a release and haven't gotten it yet - keep waiting
				}
			}
		}

		if(funcmode == 1) { //If we're in Func mode, the only keys we want to send are MediaController report keys
			if(HIDInterfaceInfo == &Media_HID_Interface) {
				USB_MediaReport_Data_t* MediaReport = (USB_MediaReport_Data_t*)ReportData;

				//TODO: Break the hardcoded codes here out to an array so it can be more easily changed
				for(keycodeiterate = 0; keycodeiterate < 6; keycodeiterate++) {
					if(reportcodes[keycodeiterate] == 0x00) break; //A zero means the rest of the array is empty; break out
					if(reportcodes[keycodeiterate] == 0x3A) MediaReport->Mute = 1;
					if(reportcodes[keycodeiterate] == 0x3D) MediaReport->PlayPause = 1;
					if(reportcodes[keycodeiterate] == 0x3C) MediaReport->VolumeUp = 1;
					if(reportcodes[keycodeiterate] == 0x3B) MediaReport->VolumeDown = 1;
					if(reportcodes[keycodeiterate] == 0x3E) MediaReport->PreviousTrack = 1;
					if(reportcodes[keycodeiterate] == 0x3F) MediaReport->NextTrack = 1;
				}
				*ReportID   = MEDIA_REPORTID_MediaReport;
				*ReportSize = sizeof(USB_MediaReport_Data_t);
			}
/*			else { //Not sure if we need to make this so complicated; can we just ignore and not pack a reportID or reportsize at all?
				USB_KeyboardReport_Data_t* KeyboardReport = (USB_KeyboardReport_Data_t*)ReportData;

				*ReportID   = KEYBOARD_REPORTID_KeyboardReport;
				*ReportSize = sizeof(USB_KeyboardReport_Data_t);

			}
*/		}

		if(funcmode == 0) { //If we're out of Func mode, keyboard behaves and reports normally
			if(HIDInterfaceInfo == &Keyboard_HID_Interface) {
				USB_KeyboardReport_Data_t* KeyboardReport = (USB_KeyboardReport_Data_t*)ReportData;

				KeyboardReport->Modifier = reportmodifiers; //Update Modifier byte with report data
				for(keycodeiterate = 0; keycodeiterate < 6; keycodeiterate++) {
					if(reportcodes[keycodeiterate] == 0x00) break; //A zero means the rest of the array is empty; break out
					KeyboardReport->KeyCode[keycodeiterate] = reportcodes[keycodeiterate]; //Update report keycodes
				}
				*ReportID   = KEYBOARD_REPORTID_KeyboardReport;
				*ReportSize = sizeof(USB_KeyboardReport_Data_t);
			}
/*			else {//Not sure if we need to make this so complicated; can we just ignore and not pack a reportID or reportsize at all?
				USB_MediaReport_Data_t* MediaReport = (USB_MediaReport_Data_t*)ReportData;

				*ReportID   = MEDIA_REPORTID_MediaReport;
				*ReportSize = sizeof(USB_MediaReport_Data_t);
			}
*/		}

		return false;
	}
	else {
		USB_MacroReport_Data_t* MacroReport = (USB_MacroReport_Data_t*)ReportData;
		if(!(keymatrixA[15]&(1<<4))) { //Only send macros if the function key is not held down
			for(keycodeiterate = 0; keycodeiterate <3;keycodeiterate++) {
				if(reportmacros[keycodeiterate]) {
					for(innermatrix = 0; innermatrix <8; innermatrix++) {
						if(reportmacros[keycodeiterate]&(1<<innermatrix)) {
							pack_gkey(MacroReport, (innermatrix)+(keycodeiterate*8));
							//pack_gkey(MacroReport, 5);
						}
					}
				}
			}
		}

		*ReportID   = MACRO_REPORTID_MacroReport;
		*ReportSize = sizeof(USB_MacroReport_Data_t);

		return false;
	}
}

void resetmacros(void) {
	uint8_t temp = 0;
	for(;temp < 18; temp++) {
		macrolist[temp] = 0; //Empty the extant macro list
	}
	for(temp = 0;temp < 16; temp++) {
		macromatrix[temp] = 0; //Empty the key check quicklist
	}
	macro_waitforkey = 1; //Wait for a keypress
}

//Stuffs the Macro report with a converted G-key value.
void pack_gkey(USB_MacroReport_Data_t *MacroReport, uint8_t gkey) {
	uint8_t gkey_bytebit = pgm_read_byte(&(gcodes[gkey]));
	uint8_t gkey_byte = (gkey_bytebit>>4)&0x0F; //Upper 4 bits are the byte value, use in the case
	uint8_t gkey_bit = gkey_bytebit&0x0F; //Lower 4 bits are the bit position in the byte
	switch(gkey_byte) {
	case 0:
		MacroReport->Byte0 |= 1<<gkey_bit; //Shift by number of bits for this character
	break;
	case 1:
		MacroReport->Byte1 |= 1<<gkey_bit;
	break;
	case 2:
		MacroReport->Byte2 |= 1<<gkey_bit;
	break;
	case 3:
		MacroReport->Byte3 |= 1<<gkey_bit;
	break;
	case 4:
		MacroReport->Byte4 |= 1<<gkey_bit;
	break;
	case 5:
		MacroReport->Byte5 |= 1<<gkey_bit;
	break;
	case 6:
		MacroReport->Byte6 |= 1<<gkey_bit;
	break;
	case 7:
		MacroReport->Byte7 |= 1<<gkey_bit;
	break;
	default:  //Do nothing if we get something malformed
	break;
	}
}



/*

	USB_MediaReport_Data_t* MediaReport = (USB_MediaReport_Data_t*)ReportData;

	uint8_t JoyStatus_LCL    = Joystick_GetStatus();
	uint8_t ButtonStatus_LCL = Buttons_GetStatus();

	//Update the Media Control report with the user button pressess
	MediaReport->Mute          = ((ButtonStatus_LCL & BUTTONS_BUTTON1) ? true : false);
	MediaReport->PlayPause     = ((JoyStatus_LCL & JOY_PRESS) ? true : false);
	MediaReport->VolumeUp      = ((JoyStatus_LCL & JOY_UP)    ? true : false);
	MediaReport->VolumeDown    = ((JoyStatus_LCL & JOY_DOWN)  ? true : false);
	MediaReport->PreviousTrack = ((JoyStatus_LCL & JOY_LEFT)  ? true : false);
	MediaReport->NextTrack     = ((JoyStatus_LCL & JOY_RIGHT) ? true : false);

	*ReportSize = sizeof(USB_MediaReport_Data_t);

*/

void runbootloader(void) {
    TIMSK0 &= ~(1 << 1); // Disable CTC interrupt for TIMER0
    TIMSK1 &= ~(1 << 1); // Disable CTC interrupt for TIMER1
#ifndef OPENCLACK
    SPI16_Send(0x00, 0x00); //Clear the three driver chips before reset to prevent stuck LEDs and power draw and pin pulldown from keymatrix
    SPI16_Send(0x00, 0x00);
    SPI16_Send(0x00, 0x00);
	LATCH_KEY(); //Latch they key matrix driver
	LATCH_LED(); //Latch the LED drivers
#endif
    USB_Disable();
	BootStartPtr();
}

/** HID class driver callback function for the processing of HID reports from the host.
 *
 *  \param[in] HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in] ReportID    Report ID of the received report from the host
 *  \param[in] ReportType  The type of report that the host has sent, either HID_REPORT_ITEM_Out or HID_REPORT_ITEM_Feature
 *  \param[in] ReportData  Pointer to a buffer where the created report has been stored
 *  \param[in] ReportSize  Size in bytes of the received HID report
 */
void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                          const uint8_t ReportID,
                                          const uint8_t ReportType,
                                          const void* ReportData,
                                          const uint16_t ReportSize)
{
	if (HIDInterfaceInfo == &Keyboard_HID_Interface)
	{
		uint8_t  LEDMask   = 0;
		uint8_t* LEDReport = (uint8_t*)ReportData;

		if (*LEDReport & HID_KEYBOARD_LED_NUMLOCK)
		  LEDMask |= LEDS_LED1;

		if (*LEDReport & HID_KEYBOARD_LED_CAPSLOCK)
		  LEDMask |= LEDS_LED2;

		if (*LEDReport & HID_KEYBOARD_LED_SCROLLLOCK)
		  LEDMask |= LEDS_LED3;

		LEDs_SetAllLEDs(LEDMask);
	}
}


ISR(TIMER0_COMPA_vect)
{
	#ifdef LED_MATRIX_SUPPORT
		uint8_t led_msb = 0; //LED output values. Need to be populated by comparing brightness state from ledmatrix and brightness_pattern, then sent out over SPI
		uint8_t led_lsb = 0;
		uint8_t led_loop; //Loop value for counting through the 13 active (of 16 total) LED sink lines
		uint8_t led_row = ledshift&0x07; //select the current driver  row out of the increment
		uint8_t brightmax = ledshift>>3; //select the current brightness compare value out of the increment
	#endif

    // Code to execute on ISR fire here (every 64uS)
    // Need to do maintenance tasks, sample keys and write out 3 SPI bytes & latch
	// We don't technically need to be refreshing the keymatrix so often (we could do it 1/5 as often, as the existing code executes in roughly 1ms), but we have to send the word anyways so the overhead is just a couple instructions every 64uS at worst
	keymatrixA[keyshift] = readmatrixrow();
	#ifdef OPENCLACK
		keyshift++;
		keymatrixA[keyshift] = readmatrixrow2();
	#endif

	if(keyshift == 15) keyshift = 0; //increment keyshift
	else keyshift++;

	#ifdef LED_MATRIX_SUPPORT
		//Populate led_msb and led_lsb with data
		//Check if brightness position is less than the maximum specified in the LED byte. If so, set it on if it's on
		for(led_loop = 0;led_loop <5;led_loop++) { //If an LED's value is on and brightness isn't at the LED's max yet, add a one at the given position
			if((ledmatrix[led_row][led_loop]&0x01) & (brightmax <= (ledmatrix[led_row][led_loop]>>1)))	led_msb |= 1<<led_loop;
		}
		for(led_loop = 8;led_loop <16;led_loop++) { //If an LED's value is on and brightness isn't at the LED's max yet, add a one at the given position
			if((ledmatrix[led_row][led_loop]&0x01) & (brightmax <= (ledmatrix[led_row][led_loop]>>1)))	led_lsb |= 1<<(led_loop-8);
		}

		SPI16_Send((1<<led_row), 0x00); //select power transistor chip; 8LSB are N/C, drive one of the 8 MSBs
		SPI16_Send(led_msb, led_lsb); //send current LED driver word
	#endif

#ifndef OPENCLACK		//dashkey-custom shifts an SPI bit along
	if(keyshift&0x08) SPI16_Send((1<<(keyshift&0x07)), 0x00); //send the key matrix selection bit
	else SPI16_Send(0x00, (1<<(keyshift&0x07)));

	LATCH_KEY(); //Latch they key matrix driver
#else					//Whereas openclack simply sets a bit in PC (PC0-4, PC7) for ROW0-5 respectively
	uint8_t tempkey = (keyshift>>1)&0x0F; //shift keyshift right by one to /2, then mask it off to 0-15. This gives us a row count for openclack
	if(tempkey == 5) { //We're on the last active row, so we add an extra shift amount to our bit
		//PORTC = ((~(1<<(tempkey+2)))&0x9F) | (PINC&(LEDS_LED2|LEDS_LED3));
		PORTC |= 0x9F;
		PORTC &= ~(1<<(tempkey+2));
	}
	else {
		//PORTC = ((~(1<<tempkey))&0x9F) | (PINC&(LEDS_LED2|LEDS_LED3));
		PORTC |= 0x9F;
		PORTC &= (~(1<<tempkey))|0x80; //We keep the last pin high to prevent a duplicate of ROW5
	}
#endif

	#ifdef LED_MATRIX_SUPPORT
		LATCH_LED(); //Latch the LED drivers
		if(ledshift == 63) ledshift = 0; //increment ledshift
		else ledshift++;
	#endif
}

ISR(TIMER1_COMPA_vect)	//5ms timer, used for timing animations to LEDs and debouncing.
{						//Simply update_keys to signal when to trigger a key update and increment the frame update
	if(testcount == 255) {
		PIND = 0b01000000; //Flips pin PD6
		testcount = 0;
	}
	else {
		testcount++;
	}

	update_keys++;
	frame_update++;
}
