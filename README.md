# OpenClack.TKL

## Preface
This repository contains all the source files needed to create a 87 key (tenkeyless) mechanical keyboard.</br>
In order to actually build this keep in mind you'll need access to the following.
* PCB fabrication.
* CNC, laser or waterjet cutting.
* Soldering iron and supplies.
* A source for your switches, stabilizers and electronic components.

## Licensing
Software in this project is licensed under the GPL.  See GPL.md</br>
Hardware in this project is licensed under the TAPR.  See TAPR.md

## Details
* Tenkeyless layout.
* Compatible with Cherry MX switches and both Cherry and Costar stabilizers.
* LED support for each key.
* Does not require a resistor for each LED.
* N-Key roll-over, no ghosting, 1 diode per switch.
* Firmware which allows for custom key mapping, macros, de-bouncing and LED control.
* Mini-B connector.
* Teensy 2.0++ controller.
Some images of a completed prototype based on the designs in this repo are in the "prototype_images" folder.
![Plate](/prototype_images/plate.jpg)![PCB](/prototype_images/teensy0.JPG)

## Project Files
* /dashkey/* - Firmware for the keyboard, codenamed DashKey.  Coded by SiberX.
* /fabrication/* - Final project files to be sent off and fabricated.
* /kicad_files/* - Kicad project.  Open the file "openclack-tkl.pro".
* /notes/* - Notes explaining the LED configuration, parts list, info on the boost converter, resistors.
* openclack.skp - Rough 3D render of the project (SketchUp).

## Instructions
There's currently no guide on how to do this but if you're technically inclined it should be doable with all the files contained herein.  If someone wants to develop a guide feel free to contribute!

## Loading Firmware
WIP

## Sources/Thanks
Thanks to the folks at http://deskthority.net/ for helping out when I had questions about measurements!</br>
Also a big thanks to BathroomEpiphanies.  Wouldn't have been able to do it without your amazing wiki.  [Check it out here!](http://deskthority.net/wiki/KiCAD_keyboard_PCB_design_guide)
