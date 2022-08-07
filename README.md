# SDM630-zero-export-controller-Arduino
Arduino code for a zero export controller
---------------------------------------------------------
![Overview](/assets/images/ZeroExportController_Overview.PNG)
This project describes an Arduino code for a zero export controller. The code controls the output power of a grid tie solar inverter based on the power measurements of a 3-phase engery meter. 
The main controller consits of an Arduino Nano with a Max485 RS485-interface HW-97 module. The 3-phase engery meter is a SDM630 (Modbus ID:1, baudrate: 9600). The grid tie inverter is a SUN GTIL2 1000/2000 with the RS485 interface pcb (Modbus ID:4) of this project: https://github.com/trucki-eu/RS485-Interface-for-Sun-GTIL2-1000

Function of the code:
---------------------
The Arduino Nano fetches the actual power of the SDM630 engery meter AND(!) the SUN GTIL2 every 500ms. A delay of 200ms is needed between the two read commands. It seams to be a peculiarity of the used Modbus Master libary. The new output power for the SUN GTIL2 inverter is calculated by the sum of the SDM630 power and the actual SUN GTIL2 output power. To be sure not to export any power to the grid the code will substract 50W (grid_offset) from the sum.

Especially heating devices like an oven are switching high loads very often. As the inverter doesn't know the future it would export power to the grid every time the oven shuts off. To reduce such export the calculated inverter power is averaged over 60 values (avgcnt). This makes the controller slow, but reduces the export substantially. If the controller reads a grid power from the SDM630 engery meter which is smaller than 25W (grid_min) it will correct the inverter output power immediately.

To give the SUN GTIL2 grid tie inverter time to reach the requested output power new values are transmitted to the inverter only every 5000ms if the received grid power from the SMD630 is higher than 75W (grid_max).

Arduino Serial Plotter:
-----------------------

