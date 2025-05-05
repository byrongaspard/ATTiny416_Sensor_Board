# ATTiny416 Dev Board

This project is my first attempt at embedded hardware/software design where I layout/route a custom PCB. In my professional career, I have done schematic capture, integrated Arduino and Raspberry Pi like development boards into custom solutions, and developed embedded software, but this will be my first time laying out a custom circuit card.

There were no real requirements for this design as the primary objective was to learn how to layout a PCB using KiCad. However, to make the design process a little more interesting, I decided I wanted the board to have the following features:

- USB to UART communication to/from the microcontroller
- 3.3V microcontroller (instead of 5V)
- Includes SPI flash for recording data
- Accepts/regulates power from multiple inputs:
  - 5 V from USB
  - 30V from an external power source

The following image is the of the final board. The sections that follow will describe the more interesting portions of the design.

![Board Front](Pictures/Board_Front.png)

## Relevant Repository Links

- [KiCad Schematics](KiCad/): Contains the KiCad schematics for the electrical design
  - [Schematic Plots](KiCad/Plots/): Contains the PDF schematics plots for the electrical design
- [SPICE files](SPICE): Contains the LTSpice models used in the design
- [Code](Code): Contains the test code developed for the design

## Hardware Design

The first step in this hardware design was selecting a microcontroller. The ATTiny416 was chosen because of its interesting form factor. It comes in a very small (3x3mm) VQFN package while still providing a lot of the common/core Atmel peripherals found on larger microcontrollers. The USART and SPI peripheral were used in this project. This microcontroller also supports multiple logic levels, including 3.3V. The ATTiny416 is also very easy to integrate into a design as it requires almost no supporting external hardware.

A [ATTiny416 XPLAINED NANO](https://www.microchip.com/en-us/development-tool/attiny416-xnano) development board was purchased to support initial software development while waiting for the PCB to be manufactured/delivered.

### Power

The power design needed to provide the following features:

- Accept 5V USB input power
- Accept up to 30V of external input power
- Accept any combination of power inputs being present at the same time
- Preclude the backfeeding of power from input to input
- Provide a minimum of 50mA @ 3.3V whenever at least one input power is present
  - Provide uninterrupted power when power sources are applied and removed, so long as one input power source is present

The TPS7B6933 LDO linear regulator was selected to generate the 3.3V rail as the microcontroller, SPI flash, and USB to USART peripherals are all relatively low power devices and LDOs are among the simplest power supply designs to implement.
The TPS7B6933 can accept up to 40V (45V transient) of input voltage. The LDO is rated to provide 150mA of output current with a 14V input power source, therefore the maximum output current at 30V can be approximated to ~75mA, meeting the 50 mA requirement.

The simplest way to OR multiple power sources while precluding backfeeding of power would be a diode array. This was considered, but determined to not be feasible. The dropout voltage on the TPS7B6933 is 800mV, meaning 4.1V is the minimum input voltage required to maintain the 3.3V output (3.3V + 0.8V = 4.1V). Most diodes will have a forward voltage drop of approximately 0.7V. According to the USB spec, the minimum USB voltage available at peripherals is 4.35V. Therefore, after this 4.35V input USB power went through a diode, the voltage at the input to the LDO would be 3.65V (4.35V - 0.7V = 3.65V), which is below the TPS7B6933's minimum input voltage of 4.1V.

Therefore, an ideal diode circuit was designed to provide a smaller voltage drop when switching in the 5V USB input, while a regular diode was used to switch in the 30V input.
The DZDH0401DW P-Channel ideal diode controller was chosen primarily because of cost. An N-Channel ideal diode controller would have provided an even smaller voltage drop, but the design did not require this.
The DMP6110SVT was selected as the P-Channel FET primarily for it's relatively low R_DS(on) for a P-Channel FET of 130 mOhm. This R_DS(on) corresponds to a maximum voltage drop of 0.0065V across the FET at the maximum required 50mA output current (0.050A / 130mOhm = 0.0065V). This results in a minimum LDO input voltage of ~4.34VDC when only 5V USB power is present, well above the 4.1V minimum required by the TPS7B6933.

The BAT160 was selected as the diode to switch the 30V input. A LTSpice model was created to tune the ideal diode controller's resistors and to verify circuit functionality before manufacturing. The two images that follow are of the SPICE model where both 5V (4.35V) USB and 30V input power are switched while the 3.3V output is measured.

![LTSpice Schematic](Pictures/Power_OR_SPICE_Schematic.png)

The colors in the time-base results that follow correspond to the following measurement points:

- Green: 30V input power
- Red: 5V (4.35V) USB input power
- Blue: ORed power from the BAT160 and DZDH0401DW ideal diode circuit
- Pink: 3.3V output power

![LTSpice Results](Pictures/Power_OR_SPICE_Results.png)

Jumper resistors were also included in the design to allow the diode circuits to be reconfigured or bypassed entirely.

![Power Schematic](Pictures/Schematic_Power.png)

### USB to UART

The FT230 was selected to provide the USB to UART interface. This FTDI chip was chosen primarily for it's price. The FT230 datasheet recommended input USB data line filtering capacitors and termination resistors which were added to the design. LEDs were also added to CBUS1/2 on the FT230 to serve as Tx/Rx indicators.

The FT230 uses 5V logic levels on the USB inputs to comply with USB standards, but the UART output logic levels are configurable from 1.8-3.3V via the VCCIO input. The FT230 includes an internal 3.3V LDO on the 3V3OUT pin that can be connected directly to the VCCIO pin to provide the required 3.3V output logic levels. However, it is possible that the FT230's 3.3V regulator is outputting a higher voltage that the circuit card's main TPS7B6933 LDO. Therefore, the output of the design's main 3.3V LDO (TPS7B6933) was connected to the FT230's VCCIO input, instead of connecting the FT230's 3V3OUT.

Jumper resistors were included between the ATTiny416 and FT230 to support isolating the components if needed for debugging. Jumper resistors were also added to allow the FT230's 3V3OUT to be connected to VCCIO instead of the TPS7B6933's 3.3V output if needed.

The 5V USB power that is accepted on the mini-B USB input is also fused here before being utilized/regulated by the TPS7B6933.

![Power Schematic](Pictures/Schematic_UART.png)

### SPI Flash

The W25X20L SPI Flash chip was selected because it supported 3.3V logic levels. Otherwise, any generic SPI Flash chip would have been acceptable. The inverted logic write-protect, hold, and chip select signals were pulled up using external resistors. A debug header was added to all the SPI signal lines to support debugging with logic analyzers if needed. A 4.7uF decoupling capacitor was added to the W25X20L's input power line to provide more stable power, even though the datasheet did not explicitly it.

![Power Schematic](Pictures/Schematic_SPI_Flash.png)

### PCB Design, Layout, and Manufacture

A two layer board was designed, The top layer contained signals/power and the bottom later was a ground plane. All components were manually placed and all traces were manually routed. The USB data lines were routed as differential pairs. The 32kHz oscillator was surrounded by a guard trace as recommended by the ATTiny416 datasheet. Copper pours were used in the power supply design as was recommended by the manufacturer.

![PCB Layout](Pictures/PCB_Layout.png)

The several copies of the PCB were ordered along with a solder paste stencil, as I would be manually assembling this board to support bring-up testing.

## Bring-up Testing

The circuit card was assembled and brought up in the following stages:

1. Power
2. UART
3. Microcontroller
4. SPI Flash

The hardware design was functional and did not require rework.

### Programming

There are multiple 3rd part, DIY, and OEM programmers for the ATTiny416 and other Atmel microcontrollers that use the UPDI interface. The [ICD 5](http://microchip.com/en-us/development-tool/dv164055) from Microchip is the latest OEM programmer and debugger, but it is quite expensive at ~$400. Instead, the [ATTiny416 XPLAINED NANO](https://www.microchip.com/en-us/development-tool/attiny416-xnano) development board was modified to act as a programmer. The dev board was modified by removing R200, which connects the dev board's ATmega32U4 programmer to the dev board's ATTiny416.

![Modified ATTiny416 Programmer](Pictures/Bringup_Modified_Attiny416.png)

Finally, the GND and the ATmega32U4 programmer UPDI output pin could be connected to the programming header on the custom circuit card as shown in the figure below:

![Programming Setup](Pictures/Bringup_Programming.png)

When the ATTiny416 is running on 3.3V input power, none of it's GPIO pins are 5V tolerant, except the reset/programming pin. The reset/programming pin (PA0) can accept up to 12.5V to support the chip's high-voltage reset functionality when PA0 is configured by software as GPIO. This 12.5V tolerance allows 5V logic levels to be used when programming this 3.3V microcontroller without damage.

## Software Design

The objective of the software design was to verify the functionality of all hardware design. To accomplish this, test software was written that provides the following functionality:

- Write a known pattern of data to SPI Flash
- Provide an external command to export the contents of SPI Flash
- Provide an external command to erase the SPI Flash

The code was developed in the MPLAB X IDE and can be found [here](Code), but the diagram that follows describes the high-level code execution flow.

```mermaid
flowchart LR
    Z["Wait forever until reset (Infinite Loop)."]
    A[Start] --> B[Initialize uC GPIO and peripherals]
    B --> C{Dump Flash Command Asserted?}
    C --> |YES| D[Dump Flash over UART.]
    D --> Z
    C --> |NO| E{Erase Flash Command Asserted?}
    E --> |YES| F[Erase Flash and notify when erase is complete over UART.]
    F --> Z
    E --> |NO| G{Is storage remaining in Flash?}
    G --> |YES| H[Write data pattern to Flash.]
    H --> G
    G --> |NO| Z
```

The external commands to export and erase SPI Flash were implemented using two ATTiny416 GPIO interfaces:

- PA6 = ERASE_N
- PA7 = DUMP_N

The ATTiny416's internal pull-up resistors were enabled for the dump and erase pin (PA6 & PA7). The software then uses inverted logic at initialization when interpreting these pins to decide when to erase and dump Flash.

To erase Flash, an operator would:

1. Connect PA6 to GND
2. Reset the ATTiny416
3. UART interface can be monitored for software indication of erase completion.

Similarly, to dump Flash, an operator would:

1. Connect PA7 to GND
2. Reset the ATTiny416
3. Receive the contents of Flash over the UART interface.

The dump Flash command is given priority over the erase Flash command, so if both are ever asserted at the same time, the ATTiny416 will dump the contents of Flash instead of erasing.

These command interfaces are only interrogated by software during initialization. If neither are asserted when the microcontroller is released from reset, the normal Flash recording loop will be entered. Inverted logic is used for these interfaces to protect the ATTiny416 from expected behavior caused by applying voltage to GPIO pins while no power is applied to the microcontroller itself.

The FT230 to ATTiny416 serial receive functionality was verified in bring-up test code that was not included in this repository.

### Software Testing Results

The screen captures that follow are of the ATTiny416's UART outputs while testing software.

The following screenshot shows three separate boot events, each starting with the "Booting..." text. When the software initializes, it determines that neither of the erase or dump flash inputs are asserted, and proceeds to enter the recording loop. To find where the software can begin writing to without overwriting previously recorded data, the software iterates through all of flash until it finds uninitialized memory (0xFF).

The first boot shown occurred right after SPI flash was erased, as the start of memory was detected at the 0x0000 address. The second boot shown occurred after some of flash has been written to, as the start of memory is reported as 0x001010, instead of 0x000000. The program was then allowed to run until flash was completely full, which was when the "Aborting W25X20L write... not enough memory left" message was printed. The third and final boot shown occurred after SPI flash was completely full, which is why the software was unable to find uninitialized flash and the "Could not find start of memory" message was printed.

![Booting](Pictures/Booting.png)

The next two images show the UART output when the dump flash command was asserted at reset. The entire contents of the W25X20L flash area printed to the console in HEX, followed by a two's-compliment checksum.

![Start Flash Dump](Pictures/Start_Flash_Dump.png)

![End Flash Dump](Pictures/End_Flash_Dump.png)

The final image below shows the UART output when the erase flash command was asserted at reset. The software reports when the W25X20L chip has completed erasing.

![Erasing](Pictures/Erase_Flash.png)

# Summary

This project was a lot of fun and gave me a lot more confidence with the PCB layout portions of hardware design. The layout requirements for this design were very simple as we didn't have to contend with high-power, high-speed signals, noise, etc., but it was still a fun and rewarding learning experience for me.

## Additional Work

The following can still be completed on this project:

- Test the external 32kHz crystal oscillator
