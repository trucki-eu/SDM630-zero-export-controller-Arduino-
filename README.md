# SDM630-zero-export-controller-Arduino
Arduino code for a zero export controller
---------------------------------------------------------
![Overview](/assets/images/ZeroExportController_Overview.PNG)
This project describes an Arduino code for a zero export controller. The code controls the output power of a grid tie solar inverter based on the power measurements of a 3-phase engery meter. 
The main controller consits of an Arduino Nano with a Max485 RS485-interface. The 3-phase engery meter is a SDM630 (Modbus ID:1, baudrate: 9600). The grid tie inverter is a SUN GTIL2 1000/2000 with the RS485 interface pcb (Modbus ID:4) of this project: https://github.com/trucki-eu/RS485-Interface-for-Sun-GTIL2-1000

