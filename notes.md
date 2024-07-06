# Problems to fix

Charging current always reported as 0, leads to overcharging

Seems to be related to regulator, as it reports charge current.

# Useful code

`Get_Charge_Current_ADC_Reading()` returns `regulator.charge_current`, regulator I2C register `ICHG_ADC_ADDR`.

`Set_Charge_Current(current)` sets charge current

`Regulator_Read_ADC()` reads regulator ADCs

`vRegulator` runs as a task and reads regulator status and ADCs, calling `Control_Charger_Output` periodically to control charge parameters

`regulator.charging_status` derived from bit 2 of regulator status word

# Stats
Output of 'stats' command:

Parameter|Source
-|-
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

# Files

File|Purpose
-|-
adc_interface.c|Manages ADCs, continuously reads and provides filtered output
bq25703a_regulator.c|Regulator control and charge control code
usbpd.c|USB power delivery control code
CLI-commands.c|Command line interface commands
battery.c|Manages battery state and balancing

# Constants

Constant|Defined in|Original|Current|Purpose
-|-|-|-|-
ADC_FILTER_SUM_COUNT|adc_interface.h|380|380|Number of ADC samples to combine for filtering
NON_USB_PD_CHARGE_POWER|bq25703a_regulator.h|2500|500|Sets charge current (mA) from non-PD supply
MAX_CHARGE_CURRENT_MA|bq25703a_regulator.h|6000|500|Sets charge current (mA) from PD supply
BATTERY_DISCONNECT_THRESH|bq25703a_regulator.h|4.215|4.215|Per-cell voltage above this threshold interpreted as disconnected charge lead
MIN_CELL_V_FOR_BALANCING|battery.h|3.0|3.0|Balancing not allowed if any cell is under this voltage
CELL_VOLTAGE_TO_ENABLE_CHARGING|battery.h|4.18|4.08|Charging only starts if average cell voltage below this
CELL_OVER_VOLTAGE_ENABLE_DISCHARGE|battery.h|4.205|4.105|Discharge any cells above this voltage
CELL_OVER_VOLTAGE_DISABLE_CHARGING|battery.h|4.22|4.12|Cell overvoltage, stop charging

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

# Regulator notes

PROCHOT is for warning the host system when certain current/voltage thresholds are exceeded


# Test results

## Typical startup output
### Connected to 20V USB-PD supply
STM successfully requests 20V, batteries start overcharging immediately
```
Starting LiPow.
Type Help to view a list of registered commands.
Firmware Version: 1.3

>Number of received Source PDOs: 7
PDO From Source: #0 PDO: 134320428  Voltage: 5000mV  Current: 3000mA  Power: 15000mW
PDO From Source: #1 PDO: 184620  Voltage: 9000mV  Current: 3000mA  Power: 27000mW
PDO From Source: #2 PDO: 246060  Voltage: 12000mV  Current: 3000mA  Power: 36000mW
PDO From Source: #3 PDO: 307500  Voltage: 15000mV  Current: 3000mA  Power: 45000mW
PDO From Source: #4 PDO: 409825  Voltage: 20000mV  Current: 2250mA  Power: 45000mW
PDO From Source: #5 PDO: -1059315366  USBPD_CORE_PDO_TYPE_BATTERY Voltage: 0mV  Current: 0mA  Power: 0mW
PDO From Source: #6 PDO: -1046208211  USBPD_CORE_PDO_TYPE_BATTERY Voltage: 0mV  Current: 0mA  Power: 0mW
OTP Value 4402 at address: 0x1fff7010
OTP Value 3211 at address: 0x1fff700c
OTP Value 2124 at address: 0x1fff7008
OTP Value 1100 at address: 0x1fff7004
OTP Value 4387 at address: 0x1fff7000
Calibration values already present. 32 total calibrations can be performed. Number of calibrations performed: 1
Using these calibration values:
Scalar: 0  Value: 4387
Scalar: 1  Value: 1100
Scalar: 2  Value: 2124
Scalar: 3  Value: 3211
Scalar: 4  Value: 4402
Voltage match found: 20000
Requesting 20V, Result: Success
```

## Get stats while adding cells
### 1 cell
```
Variable                    Value
************************************************
Battery Voltage MCU(V)       3.913
Battery Voltage Reg (V)      3.904
Charging Current (A)         0.000
Charging Power (W)           0.000
Cell One Voltage (V)         4.107
Cell Two Voltage (V)         0.000
Cell Three Voltage (V)       0.000
Cell Four Voltage (V)        1.059
2 Series Voltage (V)         3.462
3 Series Voltage (V)         2.867
4 Series Voltage (V)         3.927
MCU Temperature (C)          28
VDDa (V)                     3.304
XT60 Connected               1
Balance Connection State     0
Number of Cells              0
Battery Requires Charging    0
Balancing State/Bitmask      0
Regulator Connection State   1
Charging State               0
Max Charge Current           0.000
Vbus Voltage (V)             4.992
Input Current (A)            0.000
Input Power (W)              0.000
Efficiency (OutputW/InputW)  nan
Battery Error State          1
```
### 2 cells
```
Variable                    Value
************************************************
Battery Voltage MCU(V)       6.409
Battery Voltage Reg (V)      6.400
Charging Current (A)         0.000
Charging Power (W)           0.000
Cell One Voltage (V)         4.107
Cell Two Voltage (V)         4.108
Cell Three Voltage (V)       0.000
Cell Four Voltage (V)        0.000
2 Series Voltage (V)         8.216
3 Series Voltage (V)         7.279
4 Series Voltage (V)         6.431
MCU Temperature (C)          29
VDDa (V)                     3.302
XT60 Connected               1
Balance Connection State     1
Number of Cells              2
Battery Requires Charging    1
Balancing State/Bitmask      0
Regulator Connection State   1
Charging State               0
Max Charge Current           0.354
Vbus Voltage (V)             5.056
Input Current (A)            0.000
Input Power (W)              0.000
Efficiency (OutputW/InputW)  nan
Battery Error State          32
```
### 3 cells
```
Variable                    Value
************************************************
Battery Voltage MCU(V)       10.691
Battery Voltage Reg (V)      10.688
Charging Current (A)         0.000
Charging Power (W)           0.000
Cell One Voltage (V)         4.095
Cell Two Voltage (V)         4.071
Cell Three Voltage (V)       3.547
Cell Four Voltage (V)        0.000
2 Series Voltage (V)         8.167
3 Series Voltage (V)         11.714
4 Series Voltage (V)         10.732
MCU Temperature (C)          30
VDDa (V)                     3.300
XT60 Connected               1
Balance Connection State     1
Number of Cells              3
Battery Requires Charging    1
Balancing State/Bitmask      11
Regulator Connection State   1
Charging State               0
Max Charge Current           0.212
Vbus Voltage (V)             5.056
Input Current (A)            0.000
Input Power (W)              0.000
Efficiency (OutputW/InputW)  nan
Battery Error State          0
```
### 4 cells
```
Variable                    Value
************************************************
Battery Voltage MCU(V)       15.197
Battery Voltage Reg (V)      15.296
Charging Current (A)         0.000
Charging Power (W)           0.000
Cell One Voltage (V)         4.094
Cell Two Voltage (V)         4.070
Cell Three Voltage (V)       3.549
Cell Four Voltage (V)        3.508
2 Series Voltage (V)         8.165
3 Series Voltage (V)         11.714
4 Series Voltage (V)         15.222
MCU Temperature (C)          30
VDDa (V)                     3.304
XT60 Connected               1
Balance Connection State     1
Number of Cells              4
Battery Requires Charging    1
Balancing State/Bitmask      111
Regulator Connection State   1
Charging State               0
Max Charge Current           0.141
Vbus Voltage (V)             5.056
Input Current (A)            0.000
Input Power (W)              0.000
Efficiency (OutputW/InputW)  nan
Battery Error State          0
```
## Charging cells
Power supply is only supplying 5V
Ticking sound heard from inductor
Discharge resistors for cells 1 & 2 get hot (correctly)