OpenClack.TKL
=========

#####Preface:

Software in this project is licensed under the GPL.  See GPL.md
Hardware in this project is licensed under the TAPR.  See TAPR.md

This repository contains all the source files needed to create a 87 key (tenkeyless) mechanical keyboard.
In order to actually build this keep in mind you'll need access to the following.
- PCB fabrication.
- CNC, laser or waterjet cutting.
- Soldering iron and supplies.
- A source for your switches, stabilizers and electronic components.

#####The keyboard:

Prototype images: http://imgur.com/a/Rx5gJ https://imgur.com/a/tNWkU

- Tenkeyless layout.
- Compatible with Cherry MX switches and both Cherry and Costar stabilizers.
- LED support for each key.
- Does not require a resistor for each LED.
- N-Key roll-over, no ghosting, 1 diode per switch.
- Firmware which allows for custom key mapping, macros, de-bouncing and LED control.
- Mini-B connector.
- Teensy 2.0++ controller.

#####Project files:

- /dashkey/*
 - Firmware for the keyboard, codenamed DashKey.
- /fabrication/*
 - Final project files to be sent off and fabricated.
- /kicad_files/*
 - Kicad project.  Open the file "openclack-tkl.pro".
- /notes/*
 - Notes explaining the LED configuration, parts list, info on the boost converter, resistors.
- openclack.skp
 - Rough 3D render of the project (SketchUp).

###Instructions:

#####Ordering parts:
WIP

#####Assembly:
WIP

#####Loading Firmware:
WIP

#####Sources/Thanks:

Thanks to the folks at http://deskthority.net/ for helping out when I had questions about measurements!
Also a big thanks to BathroomEpiphanies.  Wouldn't have been able to do it without your amazing wiki.
[Check it out here!](http://deskthority.net/wiki/KiCAD_keyboard_PCB_design_guide)