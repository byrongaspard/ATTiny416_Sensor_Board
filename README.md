# ATTiny416 Sensor Board

This project is my first attempt at embedded hardware/software design where I layout/route a custom PCB. In my professional career, I have done schematic capture, integrated Arduino and Raspberry Pi like development boards into custom solutions, and developed embedded software, but this will be my first time laying out a custom circuit card.

There were no real requirements for this design as the primary objective was to learn how to layout a PCB using KiCad. However, to make the design process a little more interesting than just a custom “Arduino”, I decided I wanted my board to have the following features:
-	UART communication to/from the microcontroller 
-	3.3V microcontroller (instead of 5V)
-	Includes SPI flash for recording data
-	Accepts/regulates power from multiple inputs:
    - 5 V from USB
    - 30V from an external power source
 
The following image is the of the final board. The sections that follow will describe the more interesting portions of the design.

![Board Front](Pictures/Board_Front.png)

## Relevent Repository Links:

- [KiCad Schematics](KiCad/): Contains the KiCad schematics for the electrical design
    -  [Schematic Plots](KiCad/Plots/): Contains the PDF schematics plots for the electrical design
- [SPICE files](SPICE): Contains the LTSpice models used in the design
- [Code](Code): Contains the test code developed for the design

## Hardware Design

### Power OR Spice Modeling

![LTSpice Schematic](Pictures/Power_OR_SPICE_Schematic.png)

![LTSpice Results](Pictures/Power_OR_SPICE_Results.png)

### PCB Design, Layout, and Manufacture

![PCB Layout](Pictures/PCB_Layout.png)
 
## Software Design

### Software Testing Results

![Booting](Pictures/Booting.png)

![Start Flash Dump](Pictures/Start_Flash_Dump.png)

![End Flash Dump](Pictures/End_Flash_Dump.png)

![Erasing](Pictures/Erase_Flash.png)


