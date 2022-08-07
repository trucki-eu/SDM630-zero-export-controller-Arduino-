# SDM630-zero-export-controller-Arduino
Arduino code for a zero export controller
---------------------------------------------------------
![Overview](/assets/images/ZeroExportController_Overview.PNG)
This project describes an Arduino code for a zero export controller. The code controls the output power of a grid tie solar inverter based on the power measurements of a 3-phase energy meter. 
The main controller consists of an Arduino Nano with a Max485 RS485-interface HW-97 module. The 3-phase energy meter is a SDM630 (Modbus ID:1, baudrate: 9600). The grid tie inverter is a SUN GTIL2 1000/2000 with the RS485 interface pcb (Modbus ID:4) of this project: https://github.com/trucki-eu/RS485-Interface-for-Sun-GTIL2-1000

Function of the code:
---------------------
The Arduino Nano fetches the actual power of the SDM630 energy meter AND(!) the SUN GTIL2 every 500ms. A delay of 200ms is needed between the two read commands. It seems to be a peculiarity of the used Modbus Master library. The new output power for the SUN GTIL2 inverter is calculated by the sum of the SDM630 power and the actual SUN GTIL2 output power. To be sure not to export any power to the grid, the code will subtract 50W (grid_offset) from the sum.

Especially, heating devices like an oven are switching high loads very often. As the inverter doesn't know the future, it would export power to the grid every time the oven shuts off. To reduce such grid exports, the calculated inverter power is averaged over 60 values (avgcnt). This makes the controller slow, but reduces the grid export substantially. If the controller reads a grid power from the SDM630 energy meter which is smaller than 25W (grid_min) it will correct the inverter output power immediately.

To give the SUN GTIL2 grid tie inverter time to reach the requested output power, new values are transmitted to the inverter only every 5000ms if the received grid power from the SMD630 is higher than 75W (grid_max). Between 25W (grid_min) to 75W (grid_max) the inverter output is not corrected.

Arduino Serial Plotter:
-----------------------
To visualize grid and inverter power, the received measurements of the SDM630 energy meter and the SUN GTIL display output power are transmitted via the serial interace of the Arduino Nano in a CSV format: 'PA: xx, GTIL: xx' .
The Arduino Serial Plotter will visualize the CSV values as two charts:

![Arduino Serial Plotter](/assets/images/SDM630_Sprungantwort_Plotter.PNG) 

The chart above shows the power taken from the grid in blue and the inverter output in brown. Until x:600 the grid power is 25W-75W and the inverter output is slowly rising at about 200W. At x:600 an additional load with 250W is switched on. This increases the inverter output until (x:745) the grid power is back to 25W-75W. From x:745 to x:845 the system is stable. At x:845 the additional load (250W) is switched off and the inverter reduces is output quickly. For 5s there is about 250W exported to the grid.

SDM630 settings:
----------------
To check if your SDM630 is working at ID:1 with 9600baud enter the settings menu by pressing the "E"-Button for 3s. The default password is "1000". With button "P/M" you can scroll through the settings. "Addr" must be "001" and "bAUd" : "9k6". Press the "E"-Button again for 3s to leave the settings menu.

RS485 termination:
------------------
You should use i.e. a 120Ohm resistor on each end of the RS485 bus. As my bus is very short I successfully tested the setup without termination resistors. 
