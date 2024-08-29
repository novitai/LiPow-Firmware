# Problems to fix

Charging current always reported as 0, leads to overcharging

Seems to be related to regulator, as it reports charge current.

## Next steps

Add regulator status bytes to 'stats' output
Add serial debug statements, find out if any actions coincide with ticking

# Useful code

`Get_Charge_Current_ADC_Reading()` returns `regulator.charge_current`, regulator I2C register `ICHG_ADC_ADDR`.

`Set_Charge_Current(current)` sets charge current

`Regulator_Read_ADC()` reads regulator ADCs

`Read_Charge_Status()` (from regulator.charging_status, from CHARGE_STATUS_ADDR:CHARGING_ENABLED_MASK = ChargerStatus:IN_FCHRG)

`vRegulator` runs as a task and reads regulator status and ADCs, calling `Control_Charger_Output` periodically to control charge parameters

`regulator.charging_status` derived from bit 2 of regulator status word

# Files

File|Purpose
-|-
adc_interface.c|Manages ADCs, continuously reads and provides filtered output
bq25703a_regulator.c|Regulator control and charge control code
usbpd.c|USB power delivery control code
CLI-commands.c|Command line interface commands
battery.c|Manages battery state and balancing

# Stats output
Output of 'stats' command:

Parameter|Source
-|-
USB Voltage (V)             |Voltage requested from USB PD source
Battery Voltage MCU(V)      |ADC VBat
Battery Voltage Reg (V)     |Output voltage measured by regulator
Charging Current (A)        |Output current measured by regulator
Charging Power (W)          |Output power measured by regulator
Cell One Voltage (V)        |Cell 1 ADC measurement, error checked
Cell Two Voltage (V)        |Neighbouring ADC measurements, error checked
Cell Three Voltage (V)      |Neighbouring ADC measurements, error checked
Cell Four Voltage (V)       |Neighbouring ADC measurements, error checked
2 Series Voltage (V)        |2S voltage direct from ADC
3 Series Voltage (V)        |3S voltage direct from ADC
4 Series Voltage (V)        |4S voltage direct from ADC
MCU Temperature (C)         |ADC temp sense channel
VDDa (V)                    |ADC Vrefint channel
XT60 Connected              |Set if battery voltage > VOLTAGE_CONNECTED_THRESHOLD
Balance Connection State    |Set if number_of_cells > 1
Number of Cells             |Number of cells in series connected, from cell ADCs
Battery Requires Charging   |XT60 and balance connected, voltage > CELL_VOLTAGE_TO_ENABLE_CHARGING
Balancing State/Bitmask     |Cell voltage >= CELL_OVER_VOLTAGE_ENABLE_DISCHARGE
Regulator Connection State  |From Query_Regulator_Connection() (read I2C) once at startup
Charging State              |Set from I2C (0x21) IN_FCHRG continuously by Read_Charge_Status()
Max Charge Current          |Estimated available current, based on PD capability
Vbus Voltage (V)            |Input voltage as measured by regulator
Input Current (A)           |Input current as measured by regulator (50mA resolution)
Input Power (W)             |Input power as measured by regulator
Efficiency (OutputW/InputW) |Output Power / Input Power
Error State Flags           |From 'error_state', see below

# Constants

Constant|Defined in|Original|Current|Purpose
-|-|-|-|-
ADC_FILTER_SUM_COUNT|adc_interface.h|380|380|Number of ADC samples to combine for filtering
NON_USB_PD_CHARGE_POWER|bq25703a_regulator.h|2500|200|Sets charge current (mA) from non-PD supply
MAX_CHARGE_CURRENT_MA|bq25703a_regulator.h|6000|200|Sets charge current (mA) from PD supply
BATTERY_DISCONNECT_THRESH|bq25703a_regulator.h|4.215|4.215|Average cell voltage at charge output above this threshold interpreted as disconnected charge lead
MIN_CELL_V_FOR_BALANCING|battery.h|3.0|3.0|Balancing not allowed if any cell is under this voltage
CELL_VOLTAGE_TO_ENABLE_CHARGING|battery.h|4.18|4.08|Charging only starts if average cell voltage is below this
CELL_OVER_VOLTAGE_ENABLE_DISCHARGE|battery.h|4.205|4.105|Discharge any cells above this voltage
CELL_OVER_VOLTAGE_DISABLE_CHARGING|battery.h|4.22|4.12|Stop charging if any cells above this voltage

# Error flags
Error flags stored in `error_state`, accessed via Get_Error_State() and 'stats' output.

Error|Bit|Value|Meaning
-|-|-|-
CELL_CONNECTION_ERROR|0|1|Cell voltages indicate disconnected cell
CELL_VOLTAGE_ERROR|1|2|At least one cell is under voltage
XT60_VOLTAGE_ERROR|2|4|Unused
MCU_OVER_TEMP|3|8|MCU temperature exceeds MAX_MCU_TEMP_C_FOR_OPERATION
REGULATOR_COMMUNICATION_ERROR|4|16|I2C timeout or unexpected response
VOLTAGE_INPUT_ERROR|5|32|Error indicated on regulator CHRG_OK pin

# Workings

## Threads
Threads defined in user code:
Function|Purpose
-|-
StartDefaultTask|Empty default task
vLED_Blinky|Control RGB LED
vRead_ADC|Read and filter ADCs
vRegulator|Control regulator and battery charging
prvUARTCommandConsoleTask|Run CLI console

Threads include delays using vTaskDelay(). These are accurate but do not account for time for the task takes to run. Therefore loop times are slower than the delay implies.

## ADC input
adc_filtered_output is constantly updated and visible in debugger
ADC works by constantly sampling ADCs via DMA, summing into adc_buffer_filtered for ADC_FILTER_SUM_COUNT (380) cycles, then dividing into adc_filtered_output
Assigned ADC pin names aren't used by code, rather their positions in the DMA read cycle
ADC variables are set directly from adc_filtered_output

vRead_ADC() is where ADC channels are converted to variables

In the .ioc file, analog channels are set up in:
Analog > ADC1 > Configuration > Parameter Settings > X

Consider using channel ranks defined automatically in main.c

When testing, remember to connect VBATT and 4S

Overcharging likely happening because voltage/current sensing reads zero. This is being read directly from the regulator IC.

Idea: Read status byte for debugging (only 1 bit currently used)

Use serial debug statements to report charge strategy

## USB PD

The circuit includes 'dead battery functionality' described in ST application note AN5255. This is why the DBCCn pins are connected to corresponding CCn pins.

# Regulator notes

PROCHOT is for warning the host system when certain current/voltage thresholds are exceeded

RENG is a linear regulator which supplies MOSFET gate voltages. Is can be disabled (to save power) via EN_REGN

01h [7] = 1 Low Power Mode Enable

## Regulator control loop

Loop period approximately 330ms (execution + 250ms delay).

Initialisation happens at power-up. If USB power is not connected, charging cycle will not begin.

- Read_Charge_Okay (CHRG_OK pin) - active if USB power is connected
- Check for regulator I2C errors (REGULATOR_COMMUNICATION_ERROR flag) - haven't seen this happen
- Read_Charge_Status (read ChargerStatus:IN_FCHRG via I2C) - currently always 0
- Regulator_Read_ADC (reads regulator ADCs via I2C)
  - Battery Charge Current (ADCIBAT)
  - Input Current (ADCIINCMPIN)
- Control_Charger_Output 90% of the time, approx 30s cycle
  - Control_Charger_Output
  - Set_Charge_Voltage - based on cell count
  - Set_Charge_Current - calcs correct
  - Turn on regulator output (Regulator_HI_Z)
- High impedance the other 10%

When powered from USB PD, regulator ticking sound is in time with control loop. On external power, ticking is faster.

## Charging cells

Conditions for charging:

- Get_XT60_Connection_State() == CONNECTED
- Get_Balance_Connection_State() == CONNECTED
- Get_Error_State() == 0
- Get_Input_Power_Ready() == READY
- Get_Cell_Over_Voltage_State() == 0

Ticking sound analysed with scope.

- USB power starts at 5V, comms with regulator start
- MCU negotites higher voltage, voltage climbs to approx 19.7V, indicating current draw
- Comms with regulator continue
- 820ms later, brief rise in VUSB to 20.7V, indicating drop in load, not coinciding with I2C comms
- 5ms later, all MOSFETs on, VUSB crashes to 3.3V for around 0.5ms
- CHRG_OK pulled low as a result, audible tick from USB connector
- VUSB recovers to 21V then stabilises to 20.2V within 5ms
- CHRG_OK returns high after allotted delay
- VUSB maintains around 20.2V indicating low current draw
- Voltage crash and ticking continues in time with regulator control loop, coinciding with Control_Charger_Output()

Tested these registers while ticking occurs, nothing anomalous found:

- ChargerStatus (20h)
- ChargeOption2 (33h) 

### 2S test

- Battery current flow starts at -3mA, increases to -20mA (flow *from* battery) when 'charging' begins
- Circuit draws approx 20mA when disconnected from battery
- Charge current is set to 200mA in program
- STM requests and usually gets 9V
- Circuit consumes 4.5W from USB, board gets hot around Q1, Q4
- Approv 1.75V dropped over Q1
- Voltage ripple measured at 200kHz, stated switching frequency is 800 kHz but regulator decreases switching frequency to improve efficiency (8.3.4.3)
- Voltage reaching 3.3V regulator approx 0.5V less than Vusb
- Hypothesis: buck converter is not functioning, circuit drains power from both battery and USB when running

# PCB notes

Component selection criteria are in section 9 of the bq25703A datasheet.

## Inductor

Property|New|
-|-
DC res 15|11.5
Saturation current|8-10|8
JLC|C408445|C285617

## MOSFETS

30V or higher