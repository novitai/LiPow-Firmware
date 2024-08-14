# Introduction
Fork of firmware for LiPow the open Source Lipo Battery Charger with USB C Power Delivery Based on the STM32G0

LiPow uses USB Type C with Power Delivery to charge Lithium Polymer batteries. It supports charging and balancing for 2s-4s packs. The device supports charging up to 100W.

[Original firmware](https://github.com/AlexKlimaj/LiPow-Firmware)
[Original hardware](https://github.com/AlexKlimaj/LiPow-Hardware)

This fork updates the original LiPow Firmware to use STM32CubeIDE and its modern build tools, a change from the original build environment, Atollic TrueStudio.

# **LiPow Features**

- Charges and balances 2s-4s packs (single cell charging possible in future)
- USB type C input
- Supports charging up to 100W (depending on case configuration) from USB PD power supplies or any other USB C port with PD source capability (such as a Thinkpad X1 laptop)
- Supports non USB PD power supplies (limited to 2.5W - 5V, 0.5A)
- Charging is done through an XT60 connector and has JST XH connectors for balancing 2s-4s packs
- User feedback through an RGB LED
- Open source schematic, BOM, and firmware
- SWD and UART breakouts for firmware development
- UART command line interface for debugging and run time information

# **User Guide**

- Plug into any USB C power supply/wall wart/device that supports power output
- Plug in the balance plug from your 2s-4s battery
- If the LED turns blue, the battery needs balancing and balancing is active
- If the LED turns green, the battery is balanced
- Now plug in the XT60 plug from your battery
- If the LED turns red, the battery needs charging and charging is active
- If the LED turns green, the battery is charged and balanced
- Balancing and charging can be active at the same time and both the red and blue LEDs will be on (purple/violet)
- Charging will only start when both the balance and XT60 plugs are connected
- If a damaged pack is attached, charging will stop if any cell rises above 4.21V
- If any cell is below 3.0V it will not balance
- If any cell is below 2.0V it will not charge

Everything runs automatically and will charge up to the max capability of the connected USB PD power supply if the max current output limit exceeds the input power supply. Lower current limits can be programmed as well.

Charging current is decided by the USB PD Source capability. First, it checks the available voltages from the source, then selects the voltage that will result in the highest efficiency for the regulator based on the number of cells. For instance, using a 30W supply with a 20V 1.5A (30W) capability and a 4s Lipo battery at 15.0V. The charging current will be 30W/15.0V=2A. As the battery voltage increases, the max charging current will decrease. 30W/16.0V=1.875A.

# **Tested with these USB PD Supplies**

- Aukey Omnia 100W USB C Charger
- Aukey PA-Y8 27W Turbo Charger
- Lenovo Thinkpad 65W Charger
- Lenovo Thinkpad X1 Carbon USB C Port
- Anker PowerPort Atom PD 1 30W
- Nintendo Switch Power Supply
- Generic 2.5W USB A Wall Wart

# **Developer Guide**
- Program through SWD or UART
- Runs FreeRTOS
- ST USB PD Middleware
- UART Command Line Interface (921600 baud rate, 8N1)
- Build using makefile or in TrueStudio


To load firmware through SWD use a JLINK or STLINK.

To load firmware through UART, use one of these tools:

https://www.st.com/en/development-tools/stm32cubeprog.html

https://sourceforge.net/projects/stm32flash/

To place the STM32G0 into bootloader mode and enable UART firmware loading, jumper BOOT0 to 3.3V before powering on. Use one of the above programs with UART to load the firmware. All necessary pins are located on the debug header shown below.

# **Hardware Specifications**

- STM32G071CBT6 microcontroller with built in USB PD Phy
- BQ25703ARSNR programmable regulator (could be used as programmable power supply in future)
- IFX25001MEV33HTSA1 3.3V regulator supporting up to 45V input
- USB Type C connector power input
- XT60 connector for charging
- JST XH connectors (3, 4, and 5 pin) for balancing and pack cell count detection

# **Debug Header Pinout**

![LiPow CLI](https://i.imgur.com/APBez16.png "LiPow Debug Header")

# **Sample of the CLI**

![LiPow CLI](https://i.imgur.com/6QrrqDk.png "LiPow CLI")

# **Build instructions**

Install STM32CubeIDE

May need GnuWin32:

https://sourceforge.net/projects/gnuwin32/

Add 'make' to system path:

System Properties > Environment Variables > System variables

Add: `C:\ST\STM32CubeIDE_1.15.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.make.win32_2.1.300.202402091052\tools\bin`

Requires:

- STM32Cube FW_G0 V1.2.0
- STM32CubeMX 6.5

Multiple projects can exist in a workspace. Projects can be imported from, and remain located outside of, the workspace folder.
Projects in a workspace are shown as root folders in the Project Explorer. Right click on them to open, close, build, delete them from the workspace.

## Reconfigure and recompile

The project's .ioc file, named after the project, contains the configuration parameters.

Pin definitions and DMA ADC channel setup are the main differences between original and new hardware.

Open the .ioc file, select continue, don't migrate

Ctrl-S to save and generate code

Some old libraries are still in use in this fork of the LiPow code (PD stack).
For this reason, after building, go to VSCode and discard all changes (including deletions), except:
- Lipow.ioc
- mx.scratch
- main.c
- main.h
- stm32g0xx_hal_msp.c
- .cproject
- .mxproject

This will bring back several deleted files that are necessary for the code to compile, including:

- stm32g0xx_ll_usart.h
- \Inc\usbpd*
- \Src\usbpd*
- \Utilities\GUI*
- \Utilities\Tracer*

The compiled binary can be found in: Debug/Lipow.bin
.elf files include the debugger.

## Connect programmer

- Connect programmer as per pinout
- Set programmer type to ST-LINK, refresh serial number. It should be many characters long (legit programmer)

### Versions

.ioc file is different depending on hardware. Notice ADC order at start of file:

- Original hardware: 4321
- Updated hardware: 6234

## Debugging

Run > Debug configurations
Under C/C++ Application, find .elf file from Debug/

- Make sure device is powered
- Click debug button, switch to debug perspective
- First time:
  - Select STM32 (double click)
  - Find .elf file in project files (Debug/)
- Wait for code to download to device. Console will report 'Download verified successfully'
- Press F8 (Resume) to run code, or click resume button.

To view variables live: Live expressions > add new expression

Clicking 'stop' button exits debugger view.