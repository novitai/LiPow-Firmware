# Problems to fix

Charging current always reported as 0, leads to overcharging

Seems to be related to regulator, as it reports charge current.

# Useful code

`Get_Charge_Current_ADC_Reading()` returns `regulator.charge_current`, regulator I2C register `ICHG_ADC_ADDR`.

`Set_Charge_Current(current)` sets charge current

`Regulator_Read_ADC()` reads regulator ADCs

`vRegulator` runs as a task and reads regulator status and ADCs, calling `Control_Charger_Output` periodically to control charge parameters

## Files

File|Purpose
-|-
adc_interface.c|Manages ADCs, continuously reads and provides filtered output
bq25703a_regulator.c|Regulator control and charge control code
usbpd.c|USB power delivery control code
CLI-commands.c|Command line interface commands

## Constants

Constant|Defined in|Original|Current|Purpose
-|-|-|-|-
NON_USB_PD_CHARGE_POWER|bq25703a_regulator.h|2500|500|Sets charge current (mA) from non-PD supply
MAX_CHARGE_CURRENT_MA|bq25703a_regulator.h|6000|500|Sets charge current (mA) from PD supply
BATTERY_DISCONNECT_THRESH|bq25703a_regulator.h|4.215|4.15|Per-cell voltage to stop charging at
ADC_FILTER_SUM_COUNT|adc_interface.h|380|380|Number of ADC samples to combine for filtering

## Error flags
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