# Get stats while adding cells
## 1 cell
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
## 2 cells
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
## 3 cells
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
## 4 cells
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
# Charging cells
Power supply is only supplying 5V
Ticking sound heard from inductor
Discharge resistors for cells 1 & 2 get hot (correctly)