/*----------------------------------------------------------------------------------------------------
This project describes an Arduino code for a zero export controller. The code controls the 
output power of a grid tie solar inverter based on the power measurements of a 3-phase 
energy meter. The main controller consists of an Arduino Nano with a Max485 RS485-interface 
HW-97 module. The 3-phase energy meter is a SDM630 (Modbus ID:1, baudrate: 9600). The grid 
tie inverter is a SUN GTIL2 1000/2000 with the RS485 interface pcb (Modbus ID:4) of this 
project: https://github.com/trucki-eu/RS485-Interface-for-Sun-GTIL2-1000

Connect Arduino->MAX485, D8(RX)-> RO, D9(TX)->DI , D2(TE)->DE/RE
SDM630 settings: ModbusID=1, baudrate=9600
SUN GTIL2 RS485 interface pcb ModbusID=4

Further documentation to this project can be found at:
https://github.com/trucki-eu/SDM630-zero-export-controller-Arduino-

 */
//------------------------------------------------------------------------------------------- 
#include <AltSoftSerial.h>                            //Use for Atmega328p
AltSoftSerial   mySerial(8, 9); // RX, TX             //Use for Atmega328p SoftwareSerial for Modbus RS485 

#include "ModbusMaster.h"                             //https://github.com/4-20ma/ModbusMaster - by DocWalker
#define MAX485_DE                       2             //DE Pin of Max485 Interface
ModbusMaster                            sdm630_node;
ModbusMaster                            gtil_node;

//SDM630 Modbus values
const uint8_t smd630_id                 = 1;          //Modbus ID of SDM630 smartmeter
float sdm630_p1                         = 0;          //SDM630 power phase 1 [W]
float sdm630_p2                         = 0;          //SDM630 power phase 2 [W]
float sdm630_p3                         = 0;          //SDM630 power phase 3 [W]
float sdm630_pa                         = 0;          //SDM630 power sum all [W]

//SUN GTIL2 Modbus values                     
const uint8_t gtil_id                   = 4;          //Modbus ID of Sun GTIL2 solar inverter
uint16_t gtil_set_ac_power              = 0;          //Sun GTIL2 AC_Setpoint       [W*10]
uint16_t gtil_ac_power                  = 0;          //Sun GTIL2 AC_Display output [W*10]
uint16_t gtil_vgrid                     = 0;          //Sun GTIL2 grid voltage      [V*10]
uint16_t gtil_vbat                      = 0;          //Sun GTIL2 battery voltage   [V*10]
uint16_t gtil_dac                       = 0;          //Sun GTIL2 DAC value
uint16_t gtil_cal_step                  = 0;          //Sun GTIL2 calibration step

//Zero Export variables
const uint8_t grid_offset               = 50;         //Power [W] which you still want to get from the grid
const uint8_t grid_min                  = 25;         //Minimum power [W] from grid
const uint8_t grid_max                  = 75;         //Maximum power [W] from grid
float         avgPower                  = 0;          //Averaged desired power [W](grid+inverter)  
const uint8_t avgcnt                    = 60;         //number of averaged values
uint16_t      t2np                      = 5000;       //give inverter time before sending next value (time2next Power) = 5000ms
    
unsigned long previousMillis_modbus    = 0;           //Counter to next modbus read (every 1300ms)
unsigned long previousMillis_t2np      = 0;           //Counter to next inverter power (every 5000ms)

void preTransmission() {digitalWrite(MAX485_DE, 1);}
void postTransmission(){digitalWrite(MAX485_DE, 0);}
//-----------------------------------------------------------------------------------------------------------------
float reform_uint16_2_float32(uint16_t u1, uint16_t u2){
  //convert SDM630 2xuint16_t to float
  uint32_t num = ((uint32_t)u1 & 0xFFFF) << 16 | ((uint32_t)u2 & 0xFFFF);
  float numf;
  memcpy(&numf, &num, 4);
  return numf;
}
//-----------------------------------------------------------------------------------------------------------------
int read_sdm630() {
    //Modbus read SDM630 p1-p3
    if (sdm630_node.readInputRegisters(0x000C, 6) == sdm630_node.ku8MBSuccess) {
       sdm630_p1 = reform_uint16_2_float32(sdm630_node.getResponseBuffer(0),sdm630_node.getResponseBuffer(1));
       sdm630_p2 = reform_uint16_2_float32(sdm630_node.getResponseBuffer(2),sdm630_node.getResponseBuffer(3));
       sdm630_p3 = reform_uint16_2_float32(sdm630_node.getResponseBuffer(4),sdm630_node.getResponseBuffer(5)); 
       sdm630_pa = sdm630_p1 + sdm630_p2 + sdm630_p3;
    } else return 0;
  return 1;
}
//-----------------------------------------------------------------------------------------------------------------
int read_gtil() {
    //Modbus read Sun GTIL2 (AC Setpoint and AC output from display) 
    if (gtil_node.readInputRegisters(0, 2) == sdm630_node.ku8MBSuccess) {    
      gtil_set_ac_power = gtil_node.getResponseBuffer(0);
      gtil_ac_power     = gtil_node.getResponseBuffer(1);
      //gtil_vgrid        = node.getResponseBuffer(2);
      //gtil_vbat         = node.getResponseBuffer(3);
      //gtil_dac          = node.getResponseBuffer(4);
      //gtil_cal_step     = node.getResponseBuffer(5);
    } else return 0;     
  return 1;   
}
//-----------------------------------------------------------------------------------------------------------------
int write_gtil(float set_ac_power) {
  //Modbus write ac_setpoint power to Sun GTIL2 
  if (set_ac_power < 0) set_ac_power = 0;
  uint16_t U16_set_ac_power = (uint16_t)set_ac_power*10;
  if (gtil_node.writeSingleRegister(0,U16_set_ac_power) == gtil_node.ku8MBSuccess) return 1;
  return 0;
}  
//-----------------------------------------------------------------------------------------------------------------
void UpdateSerialPlotter() {
   //Serial output to PC
   Serial.print("PA:");     Serial.print(sdm630_pa);                     Serial.print(',');  //AC power sum from SDM630
   Serial.print("GTIL:");   Serial.print((float)(gtil_ac_power/10));     Serial.print(',');  //AC power from GTIL display
   //Serial.print("Set_AC:"); Serial.print((float)(gtil_set_ac_power/10)); Serial.print(',');  //GTIL AC setpoint
   //Serial.print("AVG:");    Serial.print(avgPower);                      Serial.print(',');  //avgPower   
   Serial.println(); 
}
//-----------------------------------------------------------------------------------------------------------------
void setup() {
  Serial.begin(9600);                                 //open serial port for serial debug output
  
  pinMode(MAX485_DE, OUTPUT);                         //Init DE Pin of Max485 Interface as output
  digitalWrite(MAX485_DE, 0);

  mySerial.begin(9600);                               //SoftwareSerial for Modbus RS485
  sdm630_node.begin(smd630_id, mySerial);             //open Modbus node to sdm630
  sdm630_node.preTransmission(preTransmission);
  sdm630_node.postTransmission(postTransmission);

  gtil_node.begin(gtil_id, mySerial);                 //open Modbus node to GTIL2
  gtil_node.preTransmission(preTransmission);
  gtil_node.postTransmission(postTransmission); 
   
  Serial.println("Start");   
}
//-----------------------------------------------------------------------------------------------------------------
void loop() {  
  if ( (millis()-previousMillis_modbus) >= 500){                                                               //enter this IF-Routine every 500ms
    if(read_sdm630()){                                                                                         //Modbus read SDM630
      delay(200);                                                                                              //Modbus Master lib seems to need a delay(200) between two different nodes read    
      if (read_gtil()){                                                                                        //Modbus read SUN GTIL2
        float sumPower = ((float)gtil_ac_power/10) + sdm630_pa - grid_offset;                                  //calculate desired power (GTIL2+grid-grid_offset)
        avgPower = (((avgcnt-1)*avgPower) + sumPower)/avgcnt;                                                  //calculate avgPower from 59*old_values + 1*new_value
        if ( (sdm630_pa < grid_min) || ((sdm630_pa > grid_max) && (millis()-previousMillis_t2np > t2np)) ){    //if (grid<grid_min) OR (grid>grid_max) AND Time2NextPower(5s) ->Update GTIL2 Power
          if (sdm630_pa < grid_min) avgPower=sumPower;                                                         //if power from grid < grid min -> correct inverter power instantly
          if (!write_gtil(avgPower) ) Serial.println("Error writing SUN GTIL2");                               //Modbus write SUN GTIL
          previousMillis_t2np = millis();                                                                      //set next time to set inverter power
        }  
      }else Serial.println("Error reading SUN GTIL2");                                                         //Error Modbus read GTIL2
    }else Serial.println("Error reading SDM630");                                                              //Error Modbus read SDM630 
    UpdateSerialPlotter();                                                                                     //Serial print values for Arduino serial plotter
    previousMillis_modbus = millis();                                                                          //set next time to run this IF-routine again
  }
}
//-----------------------------------------------------------------------------------------------------------------
