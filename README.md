# coldset
Thermostat using the AT89S52 microcontroller

## Description
The 8051 based microcontroller AT89S52 reads data from a DS18B20 digital temperature sensor and activate/deactivate a relay according to the temperature reading and the setpoint entered by the user. The user interface consists of two pushbuttons and a 16x2 HD44780 LCD.

The realy control is based in a setpoint and a tolerance. Whenever the temperature is above *setpoint + tolerance*, the relay is deactivated and when it is below *setpoint - tolerance*, the relay is activated.

## To do list
&rarr;Create a bin2dec() function, like the bin2lcd() that is already implemented, and then stop using the DS18B20 data
as it is provided by the probe and use the value for comparison opertions, etc.
&rarr;Add the hardware documentation to this github
