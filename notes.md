# Useful code

`Get_Charge_Current_ADC_Reading()` returns `regulator.charge_current` (battery charge current), 64mA resolution. Comes from regulator I2C register `ICHG_ADC_ADDR` aka `ADCIBAT`. Can be changed to report discharge current  (256mA resolution) with ChargeOption2 (32h) register.

`Set_Charge_Current(current)` sets charge current

`Regulator_Read_ADC()` reads regulator ADCs

`Read_Charge_Status()` (from regulator.charging_status, from CHARGE_STATUS_ADDR:CHARGING_ENABLED_MASK = ChargerStatus:IN_FCHRG)

`vRegulator` runs as a task and reads regulator status and ADCs, calling `Control_Charger_Output` periodically to control charge parameters

`regulator.charging_status` derived from bit 2 of regulator status word

`regulator.charge_current` ADCIBAT aka 'charge/discharge current' depinding on settings

`regulator.input_current` reads 0, from ADCIINCMPIN aka 'input current' / 'independent comparator' (unused)

Process|Happens inside|Frequency|When
-|-|-|-
ADC sampling|`HAL_ADC_ConvCpltCallback`|12kHz|Every time a reading is taken via DMA
Handling of filtered ADC values|`vRead_ADC` thread|30Hz|Each time a filtered sample is completed
Balancing calculations and actions|`vRead_ADC` via `Balance_Battery`|30Hz|
Regulator control|`vRegulator`|3Hz|In endless loop with delays
Charging control

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

The following constants control the charging strategy:

Constant|Defined in|Original|Current|Purpose
-|-|-|-|-
ADC_FILTER_SUM_COUNT|adc_interface.h|380|380|Number of ADC samples to combine for filtering
NON_USB_PD_CHARGE_POWER|bq25703a_regulator.h|2500|200|Sets charge current (mA) from non-PD supply
MAX_CHARGE_CURRENT_MA|bq25703a_regulator.h|6000|200|Sets charge current (mA) from PD supply
BATTERY_DISCONNECT_THRESH|bq25703a_regulator.h|4.215|4.215|Average cell voltage at charge output above this threshold interpreted as disconnected charge lead (CONNECTION_NO_XT60)
MIN_CELL_V_FOR_BALANCING|battery.h|3.0|3.0|Balancing not allowed if any cell is under this voltage
CELL_VOLTAGE_TO_ENABLE_CHARGING|battery.h|4.18|4.08|Charging only starts if average cell voltage is below this
CELL_OVER_VOLTAGE_ENABLE_DISCHARGE|battery.h|4.205|4.105|Discharge any cells above this voltage
CELL_OVER_VOLTAGE_DISABLE_CHARGING|battery.h|4.22|4.12|Stop charging if any individual cell above this voltage

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
HAL_ADC_ConvCpltCallback| ADC ISR, called every reading
vRead_ADC|Read and filter ADCs
vRegulator|Control regulator and battery charging
prvUARTCommandConsoleTask|Run CLI console

Threads include delays using vTaskDelay(). These are accurate but do not account for time for the task takes to run. Therefore loop times are slower than the delay implies.

## ADC input

ADCs are continuously sampled via DMA. HAL_ADC_ConvCpltCallback is called for each sample. Readings are summed into adc_buffer_filtered for ADC_FILTER_SUM_COUNT (380) cycles, then divided into adc_filtered_output, which is constantly updated and visible in the debugger.

In vRead_ADC thread, each time a new adc_filtered_output set is completed, filtered readings are copied into corresponding variables. Set_Cell_Voltage() is called to calculate cell voltages from ADC readings

Assigned ADC pins aren't referred to by name in the code, but by their positions in the DMA read cycle (see below for setup)

ADC-derived variables such as cell voltages are set directly from elements in adc_filtered_output:

Element|DMA rank|Content
-|-|-
0|1|Battery ground voltage (ADC7, not currently implemented)
1|2|Cell 1 voltage  (ADC2)
2|3|Cell 2 voltage (ADC3)
3|4|Cell 3 voltage (ADC4)
4|5|Cell 4 voltage (ADC5)
5|6|Battery voltage (ADC6)
6|7|VDDa (Vrefint)
7|8|MCU temperature

Variable/array|Purpose
-|-
`bat_voltage`|Voltage at full battery connector (1E-6 V)
`adc_values.cell_voltage[]`|Calculated single-cell voltages (1E-6 V)
`adc_values.two_s_battery_voltage`|2S battery voltage at balance input (1E-6 V)
`adc_values.three_s_battery_voltage`|3S battery voltage at balance input (1E-6 V)
`adc_values.four_s_battery_voltage`|4S battery voltage at balance input (1E-6 V)
`vrefint`|Calculated but only used for temperature
`temperature`|Temperature (deg)

Adding 0S reading would require new ADC measurement, a new value in the adc_values structure, and modification of Set_Cell_Voltage() to use it for calculating cell and battery voltages.

Filtered data is produced at around 30Hz. vRead_ADC() is where filtered ADC channels are copied to variables. It runs as a thread, and waits for a notification from the ADC reading code to indicate that new filtered data has arrived.

### ADC setup

In the .ioc file, analog channels are set up in:

Analog > ADC1 > Configuration > Parameter Settings > ADC_Regular_ConversionMode > Rank 1-8

'Rank' positions 1-5 relate to array positions 0-4 in adc_filtered_output Consider using channel ranks defined automatically in main.c


## Balancing

Cell balancing is controlled via:

- vRead_ADC - reads filtered ADC data
- Battery_Connection_State - Determines the state of connections, safety checks
- Balance_Battery - called if battery not charging, determines if balancing is needed, sets balance bitmask
- Balancing_GPIO_Control() - sets balance GPIOs

vRead_ADC is run as a thread, and contains an infinite loop, which executes every time ADC new filtered ADC data is received, and calls the other functions above. This means balancing runs at 30Hz when it is active.

Balance resistors are 47R, meaning they drain the cells at around 85mA, or 1/3W. This is enough to affect the voltage readings, which appears to cause feedback, manifesting in rapid switching of balance outputs at 15Hz.

When cell voltages are close, balancing oscillates during charge pauses, settling on a random value as charging restarts.

Try: Run balancing algorithm at end of charge-rest cycle, rather than every few ms.

## USB PD

PD is initialised with MX_USBPD_Init() in usbpd.c.

- USBPD_HW_IF_GlobalHwInit() initialises hardware
- USBPD_DPM_InitCore() sets up callbacks
- USBPD_DPM_UserInit() Dynamic Power Management related
- vUSBPD_User() started in user code section, runs continuously
  - Prints number of received Source PDOs
  - Lists available PDOs
  - If no PDOs available, go into infinite loop
  - Otherwise, monitor Get_Balance_Connection_State() and request appropriate voltage

DPM_Ports[USBPD_PORT_0].DPM_NumberOfRcvSRCPDO) is updated and can be polled from inside vUSBPD_User()

The circuit includes 'dead battery functionality' described in ST application note AN5255. This is why the DBCCn pins are connected to corresponding CCn pins.

## LEDs

When balance lead only connected:
LED|Meaning
-|-
Blue|Balancing is active
Green|Battery is balanced

When balance and charging leads connected:
LED|Meaning
-|-
Red|Battery charging
Green|Charged and balanced
Purple|Balancing and charging

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
  - Calculate_Max_Charge_Power
  - Set_Charge_Current - calcs correct
  - Turn on regulator output (Regulator_HI_Z)
- High impedance the other 10%

When powered from USB PD, regulator ticking sound is in time with control loop. On external power, ticking is faster.

# Testing

When testing, remember to connect VBATT and 4S

# PCB notes

Component selection criteria are in section 9 of the bq25703A datasheet.

## Ideas

- Improve heat transfer with vias
- Change balance resistors
- Reduce voltage drops when charging/balancing

## Inductor

Property|New|
-|-
DC res 15|11.5
Saturation current|8-10|8
JLC|C408445|C285617

## MOSFETS

30V or higher

# Current status

Balancing causes voltage readings to fluctuate significantly. Possible solutions:

- Change balancing strategy to avoid inferring wrong cell voltages
- Increase balance resistors

Try:

Measure, then start charge cycle. On error, stop and measure again.
Make list of controlling variables.